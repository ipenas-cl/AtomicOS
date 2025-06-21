// AtomicOS Process Management Implementation
// Deterministic process handling with real-time support

#include "../../include/process.h"
#include "../../include/interrupts.h"
#include "../../include/memory.h"

// Process table - fixed size for determinism
process_t* process_table[MAX_PROCESSES] = {NULL};
process_t* current_process = NULL;
process_t* idle_process = NULL;
uint32_t next_pid = 1;  // PID 0 is reserved for idle

// Ready queue head and tail
static process_t* ready_queue_head = NULL;
static process_t* ready_queue_tail = NULL;

// External functions
extern void* static_alloc_block(uint32_t size);
extern void print_string(const char* str);
extern void print_hex(uint32_t value);
extern uint64_t get_system_ticks(void);

// Initialize process management subsystem
void init_process_management(void) {
    // Clear process table
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i] = NULL;
    }
    
    // Create idle process (PID 0)
    idle_process = create_process("idle", idle_task, PRIORITY_IDLE);
    if (!idle_process) {
        print_string("FATAL: Cannot create idle process\n");
        while (1) { asm volatile("hlt"); }
    }
    
    idle_process->pid = 0;
    current_process = idle_process;
    
    print_string("Process management initialized\n");
}

// Idle task - runs when no other process is ready
void idle_task(void) {
    while (1) {
        // Enable interrupts and halt
        asm volatile("sti; hlt");
    }
}

// Create a new process
process_t* create_process(const char* name, void* entry_point, uint8_t priority) {
    // Find free slot in process table
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i] == NULL) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        print_string("Error: Process table full\n");
        return NULL;
    }
    
    // Allocate PCB
    process_t* proc = (process_t*)static_alloc_block(sizeof(process_t));
    if (!proc) {
        print_string("Error: Cannot allocate PCB\n");
        return NULL;
    }
    
    // Initialize PCB
    proc->pid = next_pid++;
    proc->ppid = (current_process) ? current_process->pid : 0;
    
    // Copy process name
    int i;
    for (i = 0; i < MAX_PROCESS_NAME - 1 && name[i]; i++) {
        proc->name[i] = name[i];
    }
    proc->name[i] = '\0';
    
    proc->state = PROCESS_STATE_READY;
    proc->priority = priority;
    proc->dynamic_priority = priority;
    proc->security_level = SECURITY_LEVEL_USER;
    
    // Allocate stacks
    proc->kernel_stack = static_alloc_block(KERNEL_STACK_SIZE);
    proc->user_stack = static_alloc_block(USER_STACK_SIZE);
    
    if (!proc->kernel_stack || !proc->user_stack) {
        print_string("Error: Cannot allocate stacks\n");
        return NULL;
    }
    
    proc->kernel_stack_size = KERNEL_STACK_SIZE;
    proc->user_stack_size = USER_STACK_SIZE;
    
    // Initialize context
    proc->context.eip = (uint32_t)entry_point;
    proc->context.cs = 0x08;  // Kernel code segment
    proc->context.ds = 0x10;  // Kernel data segment
    proc->context.es = 0x10;
    proc->context.fs = 0x10;
    proc->context.gs = 0x10;
    proc->context.eflags = 0x202;  // Enable interrupts
    proc->context.esp = (uint32_t)proc->kernel_stack + KERNEL_STACK_SIZE - 4;
    proc->context.ebp = proc->context.esp;
    
    // Initialize timing
    proc->cpu_time = 0;
    proc->start_time = get_system_ticks();
    proc->last_scheduled = 0;
    proc->time_slice = 10;  // Default 10ms time slice
    
    // Initialize statistics
    proc->exec_count = 0;
    proc->total_cycles = 0;
    proc->max_cycles = 0;
    proc->deadline_misses = 0;
    
    // Not real-time by default
    proc->is_realtime = false;
    
    // Add to process table
    process_table[slot] = proc;
    
    // Add to ready queue
    ready_queue_insert(proc);
    
    print_string("Created process: ");
    print_string(name);
    print_string(" (PID ");
    print_hex(proc->pid);
    print_string(")\n");
    
    return proc;
}

// Create a real-time process
process_t* create_rt_process(const char* name, void* entry_point, 
                            uint32_t period, uint32_t deadline, uint32_t wcet) {
    process_t* proc = create_process(name, entry_point, PRIORITY_REALTIME);
    if (!proc) {
        return NULL;
    }
    
    // Set real-time parameters
    proc->is_realtime = true;
    proc->rt_params.period = period;
    proc->rt_params.deadline = deadline;
    proc->rt_params.wcet = wcet;
    proc->rt_params.next_release = get_system_ticks() + period;
    proc->rt_params.absolute_deadline = get_system_ticks() + deadline;
    
    return proc;
}

// Ready queue management - priority ordered
void ready_queue_insert(process_t* proc) {
    if (!proc || proc->state != PROCESS_STATE_READY) {
        return;
    }
    
    proc->next = NULL;
    proc->prev = NULL;
    
    // Empty queue
    if (!ready_queue_head) {
        ready_queue_head = ready_queue_tail = proc;
        return;
    }
    
    // Insert based on priority (lower number = higher priority)
    process_t* current = ready_queue_head;
    process_t* prev = NULL;
    
    while (current && current->dynamic_priority <= proc->dynamic_priority) {
        // For same priority, real-time tasks go first
        if (current->dynamic_priority == proc->dynamic_priority) {
            if (!current->is_realtime && proc->is_realtime) {
                break;
            }
            // For real-time tasks, EDF ordering
            if (current->is_realtime && proc->is_realtime) {
                if (proc->rt_params.absolute_deadline < current->rt_params.absolute_deadline) {
                    break;
                }
            }
        }
        prev = current;
        current = current->next;
    }
    
    // Insert proc between prev and current
    proc->next = current;
    proc->prev = prev;
    
    if (prev) {
        prev->next = proc;
    } else {
        ready_queue_head = proc;
    }
    
    if (current) {
        current->prev = proc;
    } else {
        ready_queue_tail = proc;
    }
}

// Remove process from ready queue
void ready_queue_remove(process_t* proc) {
    if (!proc) {
        return;
    }
    
    if (proc->prev) {
        proc->prev->next = proc->next;
    } else {
        ready_queue_head = proc->next;
    }
    
    if (proc->next) {
        proc->next->prev = proc->prev;
    } else {
        ready_queue_tail = proc->prev;
    }
    
    proc->next = NULL;
    proc->prev = NULL;
}

// Get next process to run
process_t* ready_queue_get_next(void) {
    process_t* next = ready_queue_head;
    
    // Check for real-time deadline
    if (next && next->is_realtime) {
        uint64_t current_time = get_system_ticks();
        
        // Check if we've passed the next release time
        if (current_time < next->rt_params.next_release) {
            // Skip this task until its next period
            return idle_process;
        }
    }
    
    return next ? next : idle_process;
}

// Main scheduler function - called from timer interrupt
void schedule(void) {
    // Disable interrupts during scheduling
    disable_interrupts();
    
    if (!current_process) {
        current_process = idle_process;
    }
    
    // Update current process statistics
    if (current_process && current_process->state == PROCESS_STATE_RUNNING) {
        current_process->cpu_time++;
        current_process->time_slice--;
        
        // Check if time slice expired
        if (current_process->time_slice == 0 && current_process != idle_process) {
            current_process->state = PROCESS_STATE_READY;
            ready_queue_insert(current_process);
        }
    }
    
    // Get next process to run
    process_t* next = ready_queue_get_next();
    
    // If next is different from current, perform context switch
    if (next != current_process) {
        process_t* prev = current_process;
        
        if (next != idle_process) {
            ready_queue_remove(next);
        }
        
        next->state = PROCESS_STATE_RUNNING;
        next->last_scheduled = get_system_ticks();
        
        // Reset time slice
        if (next->is_realtime) {
            next->time_slice = next->rt_params.wcet;
        } else {
            next->time_slice = 10;  // Default 10ms
        }
        
        current_process = next;
        
        // Perform actual context switch
        switch_context(&prev->context, &next->context);
    }
    
    enable_interrupts();
}

// Yield CPU to next process
void yield(void) {
    disable_interrupts();
    
    if (current_process && current_process != idle_process) {
        current_process->state = PROCESS_STATE_READY;
        current_process->time_slice = 0;  // Force reschedule
        ready_queue_insert(current_process);
    }
    
    schedule();
    enable_interrupts();
}

// Update process statistics
void update_process_stats(process_t* proc, uint32_t cycles) {
    if (!proc) return;
    
    proc->exec_count++;
    proc->total_cycles += cycles;
    
    if (cycles > proc->max_cycles) {
        proc->max_cycles = cycles;
    }
    
    // Check WCET violation for real-time tasks
    if (proc->is_realtime && cycles > proc->rt_params.wcet) {
        print_string("WCET violation in process ");
        print_string(proc->name);
        print_string("\n");
    }
}

// Check for deadline miss
bool check_deadline_miss(process_t* proc) {
    if (!proc || !proc->is_realtime) {
        return false;
    }
    
    uint64_t current_time = get_system_ticks();
    
    if (current_time > proc->rt_params.absolute_deadline) {
        proc->deadline_misses++;
        
        // Update next release and deadline
        proc->rt_params.next_release += proc->rt_params.period;
        proc->rt_params.absolute_deadline = proc->rt_params.next_release + proc->rt_params.deadline;
        
        return true;
    }
    
    return false;
}