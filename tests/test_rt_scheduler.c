// Test program for AtomicOS Real-Time Scheduler
// Tests EDF and RMS scheduling algorithms

#include "../include/types.h"

// Real-time task structure for testing
typedef struct {
    uint32_t id;
    uint32_t period_us;
    uint32_t wcet_us;
    uint32_t deadline_us;
    uint32_t priority;
    uint32_t executions;
} rt_task_test_t;

// Test tasks with different characteristics
rt_task_test_t test_tasks[] = {
    // ID, Period, WCET, Deadline, Priority, Executions
    {1, 1000, 100, 1000, 0, 0},      // 1ms period, 100us WCET (10% util)
    {2, 5000, 200, 5000, 0, 0},      // 5ms period, 200us WCET (4% util)
    {3, 10000, 500, 10000, 0, 0},    // 10ms period, 500us WCET (5% util)
    {4, 20000, 1000, 20000, 0, 0},   // 20ms period, 1ms WCET (5% util)
    {5, 50000, 2000, 50000, 0, 0}    // 50ms period, 2ms WCET (4% util)
};

// Total utilization: 28% (well below RMS bound)

// Function prototypes
extern uint32_t rt_create_task(uint32_t period, uint32_t wcet, void* entry, uint32_t deadline);
extern uint32_t rms_schedulability_test(uint32_t num_tasks);
extern void rt_set_scheduler_mode(uint32_t mode);
extern void rt_print_stats(void);

// Simulated task entry points
void task_1_entry(void) {
    test_tasks[0].executions++;
    // Simulate work for 100us
    for (volatile int i = 0; i < 100; i++) {}
}

void task_2_entry(void) {
    test_tasks[1].executions++;
    // Simulate work for 200us
    for (volatile int i = 0; i < 200; i++) {}
}

void task_3_entry(void) {
    test_tasks[2].executions++;
    // Simulate work for 500us
    for (volatile int i = 0; i < 500; i++) {}
}

void task_4_entry(void) {
    test_tasks[3].executions++;
    // Simulate work for 1000us
    for (volatile int i = 0; i < 1000; i++) {}
}

void task_5_entry(void) {
    test_tasks[4].executions++;
    // Simulate work for 2000us
    for (volatile int i = 0; i < 2000; i++) {}
}

// Task entry point array
void (*task_entries[])(void) = {
    task_1_entry,
    task_2_entry,
    task_3_entry,
    task_4_entry,
    task_5_entry
};

// Test RMS schedulability
bool test_rms_schedulability(void) {
    // Calculate total utilization
    uint32_t total_util = 0;
    for (int i = 0; i < 5; i++) {
        uint32_t util = (test_tasks[i].wcet_us * 100) / test_tasks[i].period_us;
        total_util += util;
    }
    
    // Check RMS bound for 5 tasks (~69%)
    if (total_util > 69) {
        return false;
    }
    
    // Use built-in schedulability test
    return rms_schedulability_test(5) == 1;
}

// Main test function
void test_rt_scheduler(void) {
    // Test 1: RMS Schedulability
    if (test_rms_schedulability()) {
        print_string("RMS Schedulability Test: PASSED\n");
    } else {
        print_string("RMS Schedulability Test: FAILED\n");
    }
    
    // Test 2: Create RT tasks in EDF mode
    rt_set_scheduler_mode(1); // EDF mode
    print_string("Creating tasks in EDF mode...\n");
    
    for (int i = 0; i < 5; i++) {
        uint32_t task_id = rt_create_task(
            test_tasks[i].period_us,
            test_tasks[i].wcet_us,
            task_entries[i],
            test_tasks[i].deadline_us
        );
        
        if (task_id > 0) {
            print_string("Created task ");
            print_hex(task_id);
            print_string(" (period=");
            print_decimal(test_tasks[i].period_us);
            print_string("us)\n");
        } else {
            print_string("Failed to create task!\n");
        }
    }
    
    // Test 3: Let tasks run for a while
    print_string("Running tasks for 1 second...\n");
    
    // Wait for approximately 1 second (1000 timer ticks)
    extern volatile uint32_t system_ticks;
    uint32_t start_ticks = system_ticks;
    while (system_ticks - start_ticks < 1000) {
        // Tasks are running in background via timer interrupts
        asm volatile("hlt");
    }
    
    // Test 4: Check execution counts
    print_string("\nTask execution counts:\n");
    for (int i = 0; i < 5; i++) {
        print_string("Task ");
        print_decimal(i + 1);
        print_string(": ");
        print_decimal(test_tasks[i].executions);
        print_string(" executions (expected ~");
        print_decimal(1000000 / test_tasks[i].period_us);
        print_string(")\n");
    }
    
    // Test 5: Switch to RMS mode
    print_string("\nSwitching to RMS mode...\n");
    rt_set_scheduler_mode(2); // RMS mode
    
    // Reset execution counts
    for (int i = 0; i < 5; i++) {
        test_tasks[i].executions = 0;
    }
    
    // Run again for 1 second
    start_ticks = system_ticks;
    while (system_ticks - start_ticks < 1000) {
        asm volatile("hlt");
    }
    
    // Check execution counts again
    print_string("\nRMS execution counts:\n");
    for (int i = 0; i < 5; i++) {
        print_string("Task ");
        print_decimal(i + 1);
        print_string(": ");
        print_decimal(test_tasks[i].executions);
        print_string(" executions\n");
    }
    
    // Print scheduler statistics
    print_string("\nScheduler statistics:\n");
    rt_print_stats();
}

// Helper functions (assuming these exist)
extern void print_string(const char* str);
extern void print_hex(uint32_t value);
extern void print_decimal(uint32_t value);