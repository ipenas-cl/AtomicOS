// AtomicOS Process Management
// Deterministic task structures for real-time systems

#ifndef _PROCESS_H
#define _PROCESS_H

#include "types.h"

// Process states
#define PROCESS_STATE_READY     0
#define PROCESS_STATE_RUNNING   1
#define PROCESS_STATE_BLOCKED   2
#define PROCESS_STATE_SUSPENDED 3
#define PROCESS_STATE_ZOMBIE    4

// Process priorities (0 = highest)
#define PRIORITY_KERNEL         0
#define PRIORITY_REALTIME       1
#define PRIORITY_SYSTEM         2
#define PRIORITY_NORMAL         3
#define PRIORITY_LOW            4
#define PRIORITY_IDLE           5

// Maximum values
#define MAX_PROCESSES           32      // Fixed for determinism
#define MAX_PROCESS_NAME        16
#define KERNEL_STACK_SIZE       4096    // 4KB per process
#define USER_STACK_SIZE         8192    // 8KB per process

// Real-time task parameters
typedef struct {
    uint32_t period;            // Task period in ticks
    uint32_t deadline;          // Relative deadline
    uint32_t wcet;              // Worst-case execution time
    uint32_t next_release;      // Next release time
    uint32_t absolute_deadline; // Current absolute deadline
} rt_params_t;

// CPU context structure - saved during context switch
typedef struct {
    // General purpose registers
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    
    // Segment registers
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t gs;
    
    // Control registers
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t cr3;       // Page directory (when paging enabled)
} cpu_context_t;

// Process Control Block (PCB)
typedef struct process {
    // Process identification
    uint32_t pid;                           // Process ID
    uint32_t ppid;                          // Parent process ID
    char name[MAX_PROCESS_NAME];            // Process name
    
    // Process state
    uint8_t state;                          // Current state
    uint8_t priority;                       // Base priority
    uint8_t dynamic_priority;               // Dynamic priority for scheduling
    uint8_t security_level;                 // Security access level
    
    // CPU context
    cpu_context_t context;                  // Saved CPU state
    
    // Memory management
    void* kernel_stack;                     // Kernel stack pointer
    void* user_stack;                       // User stack pointer
    uint32_t kernel_stack_size;             // Kernel stack size
    uint32_t user_stack_size;               // User stack size
    
    // Real-time parameters
    rt_params_t rt_params;                  // Real-time task parameters
    bool is_realtime;                       // Is this a real-time task?
    
    // Scheduling information
    uint64_t cpu_time;                      // Total CPU time used
    uint64_t start_time;                    // Process start time
    uint64_t last_scheduled;                // Last time scheduled
    uint32_t time_slice;                    // Remaining time slice
    
    // Statistics for WCET tracking
    uint32_t exec_count;                    // Number of executions
    uint32_t total_cycles;                  // Total cycles consumed
    uint32_t max_cycles;                    // Maximum cycles in one execution
    uint32_t deadline_misses;               // Number of deadline misses
    
    // Process relationships
    struct process* next;                   // Next process in queue
    struct process* prev;                   // Previous process in queue
    
    // Synchronization
    uint32_t waiting_on;                    // Resource/event waiting on
    uint32_t wake_time;                     // Time to wake up (for sleep)
} process_t;

// Process table
extern process_t* process_table[MAX_PROCESSES];
extern process_t* current_process;
extern process_t* idle_process;
extern uint32_t next_pid;

// Function prototypes
void init_process_management(void);
process_t* create_process(const char* name, void* entry_point, uint8_t priority);
process_t* create_rt_process(const char* name, void* entry_point, 
                            uint32_t period, uint32_t deadline, uint32_t wcet);
void destroy_process(process_t* proc);
void schedule(void);
void context_switch(process_t* next);
void yield(void);
void sleep(uint32_t ticks);
void block_process(process_t* proc, uint32_t reason);
void unblock_process(process_t* proc);
process_t* get_process_by_pid(uint32_t pid);
void update_process_stats(process_t* proc, uint32_t cycles);
bool check_deadline_miss(process_t* proc);

// Assembly functions
extern void switch_context(cpu_context_t* old, cpu_context_t* new);
extern void enter_usermode(void* entry_point, void* user_stack);

// Ready queue management
void ready_queue_insert(process_t* proc);
void ready_queue_remove(process_t* proc);
process_t* ready_queue_get_next(void);

#endif // _PROCESS_H