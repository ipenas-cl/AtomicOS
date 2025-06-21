// AtomicOS WCET Hardware Validation
// Measures actual execution time using CPU performance counters

#include "../include/types.h"
#include "wcet_model.h"

// x86 Time Stamp Counter (TSC) access
static inline uint64_t read_tsc(void) {
    uint32_t low, high;
    asm volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

// CPU ID to serialize instruction stream
static inline void cpuid_serialize(void) {
    asm volatile("cpuid" : : : "eax", "ebx", "ecx", "edx");
}

// Memory fence to ensure all memory operations complete
static inline void memory_fence(void) {
    asm volatile("mfence" : : : "memory");
}

// Disable interrupts and return previous state
static inline uint32_t disable_interrupts(void) {
    uint32_t flags;
    asm volatile(
        "pushfl\n"
        "popl %0\n"
        "cli"
        : "=r"(flags)
    );
    return flags;
}

// Restore interrupt state
static inline void restore_interrupts(uint32_t flags) {
    if (flags & 0x200) {  // IF flag
        asm volatile("sti");
    }
}

// Flush cache line
static inline void clflush(void* addr) {
    asm volatile("clflush (%0)" : : "r"(addr) : "memory");
}

// WCET measurement result
typedef struct {
    uint64_t min_cycles;
    uint64_t max_cycles;
    uint64_t avg_cycles;
    uint64_t total_cycles;
    uint32_t iterations;
    uint32_t cache_misses;    // If PMC available
    uint32_t branch_misses;   // If PMC available
    bool interrupted;         // Measurement was interrupted
} wcet_measurement_t;

// Function signature for code to measure
typedef void (*wcet_test_func_t)(void* arg);

// Measure WCET of a function with cold cache
wcet_measurement_t measure_wcet_cold_cache(wcet_test_func_t func, void* arg, 
                                          uint32_t iterations) {
    wcet_measurement_t result = {
        .min_cycles = UINT64_MAX,
        .max_cycles = 0,
        .total_cycles = 0,
        .iterations = iterations,
        .interrupted = false
    };
    
    // Disable interrupts for accurate measurement
    uint32_t interrupt_state = disable_interrupts();
    
    for (uint32_t i = 0; i < iterations; i++) {
        // Flush caches (simulate cold cache)
        // Note: This is simplified - real implementation would flush all
        // relevant cache lines
        clflush((void*)func);
        memory_fence();
        
        // Serialize instruction stream
        cpuid_serialize();
        
        // Measure execution time
        uint64_t start = read_tsc();
        memory_fence();
        
        func(arg);
        
        memory_fence();
        uint64_t end = read_tsc();
        cpuid_serialize();
        
        // Calculate cycles
        uint64_t cycles = end - start;
        
        // Update statistics
        if (cycles < result.min_cycles) {
            result.min_cycles = cycles;
        }
        if (cycles > result.max_cycles) {
            result.max_cycles = cycles;
        }
        result.total_cycles += cycles;
        
        // Check if we were interrupted (cycles too high)
        if (cycles > result.max_cycles * 10) {
            result.interrupted = true;
        }
    }
    
    // Restore interrupts
    restore_interrupts(interrupt_state);
    
    // Calculate average
    result.avg_cycles = result.total_cycles / iterations;
    
    return result;
}

// Measure WCET with warm cache
wcet_measurement_t measure_wcet_warm_cache(wcet_test_func_t func, void* arg,
                                          uint32_t iterations) {
    wcet_measurement_t result = {
        .min_cycles = UINT64_MAX,
        .max_cycles = 0,
        .total_cycles = 0,
        .iterations = iterations,
        .interrupted = false
    };
    
    // Warm up cache
    for (int i = 0; i < 10; i++) {
        func(arg);
    }
    
    uint32_t interrupt_state = disable_interrupts();
    
    for (uint32_t i = 0; i < iterations; i++) {
        cpuid_serialize();
        uint64_t start = read_tsc();
        memory_fence();
        
        func(arg);
        
        memory_fence();
        uint64_t end = read_tsc();
        cpuid_serialize();
        
        uint64_t cycles = end - start;
        
        if (cycles < result.min_cycles) {
            result.min_cycles = cycles;
        }
        if (cycles > result.max_cycles) {
            result.max_cycles = cycles;
        }
        result.total_cycles += cycles;
    }
    
    restore_interrupts(interrupt_state);
    
    result.avg_cycles = result.total_cycles / iterations;
    
    return result;
}

// Validate WCET model against hardware measurements
typedef struct {
    uint32_t model_wcet;      // Predicted by model
    uint32_t measured_wcet;   // Measured on hardware
    uint32_t difference;      // Absolute difference
    float error_percent;      // Percentage error
    bool model_conservative;  // True if model >= measured
} wcet_validation_t;

wcet_validation_t validate_wcet_model(wcet_context_t* model, 
                                    wcet_test_func_t func, void* arg) {
    wcet_validation_t validation;
    
    // Get model prediction
    validation.model_wcet = wcet_calculate_total(model);
    
    // Measure on hardware (cold cache to match model assumptions)
    wcet_measurement_t measurement = measure_wcet_cold_cache(func, arg, 1000);
    validation.measured_wcet = (uint32_t)measurement.max_cycles;
    
    // Calculate error
    if (validation.measured_wcet > validation.model_wcet) {
        validation.difference = validation.measured_wcet - validation.model_wcet;
        validation.model_conservative = false;
    } else {
        validation.difference = validation.model_wcet - validation.measured_wcet;
        validation.model_conservative = true;
    }
    
    validation.error_percent = (float)validation.difference / 
                              validation.measured_wcet * 100.0f;
    
    return validation;
}

// Performance monitoring counters (PMC) support
// Note: Simplified - real implementation would use MSRs
typedef struct {
    uint32_t l1_cache_misses;
    uint32_t l2_cache_misses;
    uint32_t branch_mispredicts;
    uint32_t instructions_retired;
} pmc_counters_t;

// Read performance counters (requires kernel mode)
bool read_pmc_counters(pmc_counters_t* counters) {
    // This would read actual MSRs on real hardware
    // For now, return false (not implemented)
    return false;
}

// Self-test for WCET measurement accuracy
bool wcet_validator_self_test(void) {
    // Test 1: NOP instruction timing
    void nop_test(void* arg) {
        asm volatile("nop");
    }
    
    wcet_measurement_t nop_result = measure_wcet_warm_cache(nop_test, NULL, 1000);
    
    // NOP should be ~1 cycle, but measurement overhead adds ~40-100 cycles
    if (nop_result.min_cycles > 200) {
        return false;  // Measurement overhead too high
    }
    
    // Test 2: Serialization works
    uint64_t tsc1 = read_tsc();
    cpuid_serialize();
    uint64_t tsc2 = read_tsc();
    
    if (tsc2 <= tsc1) {
        return false;  // TSC not incrementing
    }
    
    return true;
}

// Export for use in Tempo compiler
void export_validation_results(const char* function_name,
                             const wcet_validation_t* validation) {
    // This would write to a file or debug section
    // Format: function_name,model_wcet,measured_wcet,error_percent
}