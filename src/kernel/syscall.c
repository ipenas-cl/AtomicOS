/*
 * Copyright (c) 2024 Ignacio Peña
 * AtomicOS - Deterministic Real-Time Security Operating System
 * https://github.com/ipenas-cl/AtomicOS
 * Licensed under MIT License - see LICENSE file for details
 */

// AtomicOS System Call Implementation
// Deterministic syscall handling with security and WCET enforcement

#include "../../include/syscall.h"
#include "../../include/types.h"

// External functions (assembly implementations)
uint32_t get_current_pid(void);
uint32_t get_system_time_us(void);
void process_exit(int32_t code);
void yield_to_scheduler(void);
uint8_t get_current_security_level(void);

// Simple print functions for kernel
static void print_string(const char* str) {
    // Simplified VGA output
    volatile char* vga = (volatile char*)0xB8000;
    static int pos = 0;
    
    while (*str) {
        vga[pos * 2] = *str++;
        vga[pos * 2 + 1] = 0x07; // White on black
        pos++;
        if (pos >= 80 * 25) pos = 0;
    }
}

static void print_hex(uint32_t value) {
    char buf[9];
    const char* hex = "0123456789ABCDEF";
    
    for (int i = 7; i >= 0; i--) {
        buf[i] = hex[value & 0xF];
        value >>= 4;
    }
    buf[8] = 0;
    
    print_string("0x");
    print_string(buf);
}

// TSC for WCET measurement
static inline uint64_t read_tsc(void) {
    uint32_t low, high;
    __asm__ __volatile__("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

// System call statistics
static struct {
    uint32_t call_count[SYS_MAX + 1];
    uint32_t wcet_violations[SYS_MAX + 1];
    uint32_t security_violations[SYS_MAX + 1];
    uint64_t total_cycles[SYS_MAX + 1];
} syscall_stats;

// Forward declarations for handlers
static syscall_result_t sys_exit_handler(syscall_params_t* params);
static syscall_result_t sys_getpid_handler(syscall_params_t* params);
static syscall_result_t sys_rt_yield_handler(syscall_params_t* params);
static syscall_result_t sys_rt_gettime_handler(syscall_params_t* params);
static syscall_result_t sys_write_handler(syscall_params_t* params);
static syscall_result_t sys_rt_create_handler(syscall_params_t* params);
static syscall_result_t sys_not_implemented(syscall_params_t* params);

// System call table
static syscall_entry_t syscall_table[SYS_MAX + 1] = {
    // Process management
    [SYS_EXIT] = {
        .handler = sys_exit_handler,
        .wcet_bound = 1000,
        .min_security_level = 0,
        .param_count = 1,
        .interruptible = false,
        .realtime_safe = true
    },
    [SYS_GETPID] = {
        .handler = sys_getpid_handler,
        .wcet_bound = 100,
        .min_security_level = 0,
        .param_count = 0,
        .interruptible = true,
        .realtime_safe = true
    },
    
    // Real-time operations
    [SYS_RT_YIELD] = {
        .handler = sys_rt_yield_handler,
        .wcet_bound = 300,
        .min_security_level = 0,
        .param_count = 0,
        .interruptible = false,
        .realtime_safe = true
    },
    [SYS_RT_GETTIME] = {
        .handler = sys_rt_gettime_handler,
        .wcet_bound = 100,
        .min_security_level = 0,
        .param_count = 0,
        .interruptible = true,
        .realtime_safe = true
    },
    [SYS_RT_CREATE] = {
        .handler = sys_rt_create_handler,
        .wcet_bound = 2000,
        .min_security_level = 1,  // System level required
        .param_count = 4,
        .interruptible = false,
        .realtime_safe = false    // Can't create tasks from RT context
    },
    
    // File operations (simplified)
    [SYS_WRITE] = {
        .handler = sys_write_handler,
        .wcet_bound = 2000,
        .min_security_level = 0,
        .param_count = 3,
        .interruptible = true,
        .realtime_safe = false    // I/O not RT safe
    },
    
    // Default for unimplemented
    [SYS_FORK] = {.handler = sys_not_implemented, .wcet_bound = 0},
    [SYS_EXEC] = {.handler = sys_not_implemented, .wcet_bound = 0},
    // ... others default to not implemented
};

// Initialize syscall subsystem
void syscall_init(void) {
    // Clear statistics
    for (int i = 0; i <= SYS_MAX; i++) {
        syscall_stats.call_count[i] = 0;
        syscall_stats.wcet_violations[i] = 0;
        syscall_stats.security_violations[i] = 0;
        syscall_stats.total_cycles[i] = 0;
        
        // Set default handler for uninitialized entries
        if (!syscall_table[i].handler) {
            syscall_table[i].handler = sys_not_implemented;
        }
    }
    
    print_string("System call interface initialized\n");
}

// Main syscall dispatcher
syscall_result_t syscall_dispatch(uint32_t number, syscall_params_t* params) {
    syscall_result_t result = {.value = -E_NOSYS, .wcet = 0};
    
    // Validate syscall number
    if (number > SYS_MAX) {
        result.value = -E_NOSYS;
        return result;
    }
    
    // Get entry
    syscall_entry_t* entry = &syscall_table[number];
    
    // Check if implemented
    if (!entry->handler || entry->handler == sys_not_implemented) {
        result.value = -E_NOSYS;
        return result;
    }
    
    // Security check
    uint8_t current_level = get_current_security_level();
    if (current_level < entry->min_security_level) {
        syscall_stats.security_violations[number]++;
        result.value = -E_PERM;
        return result;
    }
    
    // WCET measurement start
    uint64_t start_tsc = read_tsc();
    
    // Update statistics
    syscall_stats.call_count[number]++;
    
    // Call handler
    result = entry->handler(params);
    
    // WCET measurement end
    uint64_t end_tsc = read_tsc();
    uint32_t cycles = (uint32_t)(end_tsc - start_tsc);
    result.wcet = cycles;
    
    // Update total cycles
    syscall_stats.total_cycles[number] += cycles;
    
    // Check WCET violation
    if (cycles > entry->wcet_bound) {
        syscall_stats.wcet_violations[number]++;
        // In a real-time system, this could trigger error handling
        print_string("WCET violation in syscall ");
        print_hex(number);
        print_string(": ");
        print_hex(cycles);
        print_string(" > ");
        print_hex(entry->wcet_bound);
        print_string("\n");
    }
    
    return result;
}

// Validate syscall parameters
bool syscall_validate_params(uint32_t number, syscall_params_t* params) {
    if (number > SYS_MAX || !params) {
        return false;
    }
    
    // Check parameter count (simplified - real implementation would validate each param)
    // For now, just ensure params is not NULL
    return true;
}

// Get WCET bound for planning
uint32_t syscall_get_wcet_bound(uint32_t number) {
    if (number > SYS_MAX) {
        return 0;
    }
    return syscall_table[number].wcet_bound;
}

// System call handlers implementation

static syscall_result_t sys_exit_handler(syscall_params_t* params) {
    syscall_result_t result = {.value = 0, .wcet = 0};
    
    int32_t exit_code = (int32_t)params->arg0;
    
    // Log exit
    print_string("Process exiting with code: ");
    print_hex(exit_code);
    print_string("\n");
    
    // Call kernel exit function
    process_exit(exit_code);
    
    // Should not return
    result.value = 0;
    return result;
}

static syscall_result_t sys_getpid_handler(syscall_params_t* params) {
    (void)params;  // Unused
    syscall_result_t result = {.value = 0, .wcet = 0};
    
    // Simple and fast
    result.value = get_current_pid();
    
    return result;
}

static syscall_result_t sys_rt_yield_handler(syscall_params_t* params) {
    (void)params;  // Unused
    syscall_result_t result = {.value = 0, .wcet = 0};
    
    // Yield CPU to scheduler
    yield_to_scheduler();
    
    result.value = 0;
    return result;
}

static syscall_result_t sys_rt_gettime_handler(syscall_params_t* params) {
    (void)params;  // Unused
    syscall_result_t result = {.value = 0, .wcet = 0};
    
    // Return system time in microseconds
    result.value = get_system_time_us();
    
    return result;
}

static syscall_result_t sys_write_handler(syscall_params_t* params) {
    syscall_result_t result = {.value = 0, .wcet = 0};
    
    uint32_t fd = params->arg0;
    const char* buffer = (const char*)params->arg1;
    uint32_t count = params->arg2;
    
    // Validate parameters
    if (!buffer || count == 0) {
        result.value = -E_INVAL;
        return result;
    }
    
    // For now, only support stdout (fd = 1)
    if (fd != 1) {
        result.value = -E_INVAL;  // EBADF not defined
        return result;
    }
    
    // Simple write to console
    for (uint32_t i = 0; i < count && i < 256; i++) {  // Limit for WCET
        if (buffer[i] == '\0') break;
        // Would call VGA output here
        // For now, just count bytes
    }
    
    result.value = count;
    return result;
}

static syscall_result_t sys_rt_create_handler(syscall_params_t* params) {
    syscall_result_t result = {.value = 0, .wcet = 0};
    
    uint32_t period_us = params->arg0;
    uint32_t wcet_us = params->arg1;
    void* entry_point = (void*)params->arg2;
    // uint32_t deadline_us = params->arg3;  // TODO: Use when implementing
    
    // Validate parameters
    if (period_us == 0 || wcet_us > period_us || !entry_point) {
        result.value = -E_INVAL;
        return result;
    }
    
    // Call RT task creation
    // For now, return a dummy task ID
    int32_t task_id = 1; // TODO: Call actual rt_create_task when linked
    
    if (task_id <= 0) {
        result.value = -E_NOMEM;  // Or appropriate error
    } else {
        result.value = task_id;
    }
    
    return result;
}

static syscall_result_t sys_not_implemented(syscall_params_t* params) {
    (void)params;  // Unused
    syscall_result_t result = {.value = -E_NOSYS, .wcet = 10};
    return result;
}

// System call entry point from assembly (called by INT 0x80)
void syscall_entry_asm(uint32_t syscall_num, uint32_t ebx, uint32_t ecx, 
                      uint32_t edx, uint32_t esi, uint32_t edi) {
    // Build parameter structure
    syscall_params_t params = {
        .arg0 = ebx,
        .arg1 = ecx,
        .arg2 = edx,
        .arg3 = esi,
        .arg4 = edi,
        .arg5 = 0  // Not used in x86-32 convention
    };
    
    // Dispatch syscall
    syscall_result_t result = syscall_dispatch(syscall_num, &params);
    
    // Return value is passed back in EAX by assembly wrapper
    __asm__ __volatile__("" : : "a"(result.value));
}

// Print syscall statistics
void syscall_print_stats(void) {
    print_string("\nSystem Call Statistics:\n");
    print_string("========================\n");
    
    for (uint32_t i = 0; i <= SYS_MAX; i++) {
        if (syscall_stats.call_count[i] > 0) {
            print_string("Syscall ");
            print_hex(i);
            print_string(": calls=");
            print_hex(syscall_stats.call_count[i]);
            
            if (syscall_stats.wcet_violations[i] > 0) {
                print_string(" WCET_violations=");
                print_hex(syscall_stats.wcet_violations[i]);
            }
            
            if (syscall_stats.security_violations[i] > 0) {
                print_string(" security_violations=");
                print_hex(syscall_stats.security_violations[i]);
            }
            
            uint32_t avg_cycles = syscall_stats.total_cycles[i] / syscall_stats.call_count[i];
            print_string(" avg_cycles=");
            print_hex(avg_cycles);
            
            print_string("\n");
        }
    }
}