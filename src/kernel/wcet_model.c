// AtomicOS WCET Model Implementation
// Provides realistic worst-case execution time analysis

#include "wcet_model.h"

// Initialize WCET context with default values
void wcet_init_context(wcet_context_t* ctx) {
    ctx->base_cycles = 0;
    ctx->cache_misses = 0;
    ctx->branch_mispredicts = 0;
    ctx->memory_accesses = 0;
    ctx->total_wcet = 0;
    
    // Copy default penalties
    ctx->cache_config = default_cache_penalties;
    ctx->pipeline_config = default_pipeline_penalties;
    
    // Conservative defaults for real-time systems
    ctx->assume_cold_cache = true;       // Worst case
    ctx->assume_worst_branches = true;   // Worst case
    ctx->include_interrupt_delay = true; // Account for interrupts
}

// Add an instruction to the WCET calculation
void wcet_add_instruction(wcet_context_t* ctx, instruction_type_t type) {
    ctx->base_cycles += instruction_cycles[type];
    
    // Account for instruction cache miss on cold cache
    if (ctx->assume_cold_cache && ctx->base_cycles == instruction_cycles[type]) {
        // First instruction in sequence - likely I-cache miss
        ctx->cache_misses++;
    }
}

// Add a memory access
void wcet_add_memory_access(wcet_context_t* ctx, bool is_write) {
    ctx->memory_accesses++;
    
    if (ctx->assume_cold_cache) {
        // Assume data cache miss
        ctx->cache_misses++;
        
        if (is_write) {
            // Write-through cache adds extra latency
            ctx->base_cycles += 1;
        }
    }
}

// Add a branch instruction
void wcet_add_branch(wcet_context_t* ctx, bool is_loop) {
    if (ctx->assume_worst_branches) {
        ctx->branch_mispredicts++;
        
        if (is_loop) {
            // Loop branches are often predicted correctly after first iteration
            // But we assume worst case - misprediction on exit
            ctx->base_cycles += instruction_cycles[INST_BRANCH_TAKEN];
        } else {
            // Conditional branches - assume misprediction
            ctx->base_cycles += instruction_cycles[INST_BRANCH_TAKEN];
        }
    } else {
        // Best case - correct prediction
        ctx->base_cycles += instruction_cycles[INST_BRANCH_NOT];
    }
}

// Calculate total WCET including all penalties
uint32_t wcet_calculate_total(wcet_context_t* ctx) {
    uint32_t total = ctx->base_cycles;
    
    // Add cache miss penalties
    // Assume worst case: all misses go to L3
    if (ctx->assume_cold_cache) {
        total += ctx->cache_misses * ctx->cache_config.l3_miss;
    } else {
        // More realistic: most hits in L1, some in L2
        uint32_t l1_misses = ctx->cache_misses / 10;  // 90% L1 hit rate
        uint32_t l2_misses = l1_misses / 5;           // 80% L2 hit rate
        
        total += l1_misses * ctx->cache_config.l1_miss;
        total += l2_misses * ctx->cache_config.l2_miss;
    }
    
    // Add branch misprediction penalties
    total += ctx->branch_mispredicts * ctx->pipeline_config.branch_mispredict;
    
    // Add interrupt latency if enabled
    if (ctx->include_interrupt_delay) {
        // Assume worst case: interrupt at most critical point
        // Typical interrupt latency: save context + ISR + restore
        total += 200; // ~200 cycles for interrupt handling
    }
    
    // Memory fence/serialization for every N memory accesses
    if (ctx->memory_accesses > 10) {
        uint32_t fences = ctx->memory_accesses / 10;
        total += fences * ctx->pipeline_config.memory_fence;
    }
    
    ctx->total_wcet = total;
    return total;
}

// Generate analysis report
void wcet_generate_report(wcet_context_t* ctx, wcet_report_t* report) {
    report->wcet_cycles = wcet_calculate_total(ctx);
    report->memory_accesses = ctx->memory_accesses;
    report->branches = ctx->branch_mispredicts;
    report->validated = false; // Set to true after hardware validation
}

// Print WCET analysis report (simplified - would need actual print functions)
void wcet_print_report(const wcet_report_t* report) {
    // This would use the kernel's print functions
    // For now, just a placeholder
    
    // Example output:
    // Function: interrupt_handler
    // WCET: 1,234 cycles (41.13 µs @ 30 MHz)
    // Memory accesses: 15
    // Branch mispredicts: 3
    // Validated: No
}