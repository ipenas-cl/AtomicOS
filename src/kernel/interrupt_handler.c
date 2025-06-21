// AtomicOS Interrupt Handler
// Deterministic interrupt processing with WCET guarantees

#include "../../include/types.h"
#include "../../include/interrupts.h"

// External assembly functions
extern void outb(uint16_t port, uint8_t value);
extern uint8_t inb(uint16_t port);
extern uint32_t get_tsc_low(void);
extern void print_string(const char* str);
extern void print_hex(uint32_t value);

// Interrupt statistics
extern uint32_t interrupt_count[256];
extern uint32_t interrupt_cycles[256];
extern uint32_t interrupt_max_cycles[256];
extern uint32_t nested_interrupt_level;
extern uint32_t interrupt_overruns;

// Current task (will be properly implemented with task management)
static uint32_t current_task_id = 0;

// System tick counter for scheduling
static volatile uint64_t system_ticks = 0;

// Deadline tracking for real-time tasks
static uint64_t next_deadline_check = 0;
static const uint64_t DEADLINE_CHECK_INTERVAL = 100; // Check every 100 ticks

// Timer frequency for deterministic timing
static const uint32_t TIMER_FREQ = 1000; // 1000 Hz = 1ms tick

// Initialize timer for deterministic scheduling
void init_timer(void) {
    uint32_t divisor = 1193180 / TIMER_FREQ;
    
    // Configure PIT (Programmable Interval Timer)
    outb(0x43, 0x36); // Channel 0, square wave mode
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
    
    // Enable timer IRQ
    uint8_t mask = inb(0x21);
    mask &= ~0x01; // Enable IRQ0
    outb(0x21, mask);
}

// Main interrupt handler called from assembly
void handle_interrupt_asm(uint32_t interrupt_num) {
    // Update interrupt count
    interrupt_count[interrupt_num]++;
    
    // Handle based on interrupt type
    if (interrupt_num < 32) {
        // CPU exception
        handle_exception(interrupt_num);
    } else if (interrupt_num < 48) {
        // Hardware IRQ
        handle_irq(interrupt_num - 32);
    } else {
        // Software interrupt
        handle_software_interrupt(interrupt_num);
    }
}

// Handle CPU exceptions with deterministic recovery
void handle_exception(uint32_t exception_num) {
    const char* exception_messages[] = {
        "Divide by zero",
        "Debug exception",
        "Non-maskable interrupt",
        "Breakpoint",
        "Overflow",
        "Bound range exceeded",
        "Invalid opcode",
        "Device not available",
        "Double fault",
        "Coprocessor segment overrun",
        "Invalid TSS",
        "Segment not present",
        "Stack segment fault",
        "General protection fault",
        "Page fault",
        "Reserved",
        "x87 FPU error",
        "Alignment check",
        "Machine check",
        "SIMD floating-point exception"
    };
    
    // Print exception message
    print_string("\n[EXCEPTION] ");
    if (exception_num < 20) {
        print_string(exception_messages[exception_num]);
    } else {
        print_string("Unknown exception");
    }
    print_string(" (");
    print_hex(exception_num);
    print_string(")\n");
    
    // Handle specific exceptions
    switch (exception_num) {
        case 0: // Divide by zero
            // Terminate current task (when task management is implemented)
            print_string("Task terminated due to divide by zero\n");
            // For now, just halt
            while (1) { asm volatile("hlt"); }
            break;
            
        case 14: // Page fault
            // Get fault address from CR2
            uint32_t fault_addr;
            asm volatile("mov %%cr2, %0" : "=r"(fault_addr));
            print_string("Page fault at address: ");
            print_hex(fault_addr);
            print_string("\n");
            // For now, halt
            while (1) { asm volatile("hlt"); }
            break;
            
        case 13: // General protection fault
            print_string("Security violation - system halted\n");
            while (1) { asm volatile("hlt"); }
            break;
            
        default:
            // Other exceptions - halt system
            print_string("System halted\n");
            while (1) { asm volatile("hlt"); }
    }
}

// Handle hardware IRQs with priority-based processing
void handle_irq(uint32_t irq_num) {
    // Send EOI to PIC
    if (irq_num >= 8) {
        outb(0xA0, 0x20); // Slave PIC
    }
    outb(0x20, 0x20); // Master PIC
    
    // Process specific IRQs
    switch (irq_num) {
        case 0: // Timer
            handle_timer_interrupt();
            break;
            
        case 1: // Keyboard
            handle_keyboard_interrupt();
            break;
            
        case 8: // RTC
            handle_rtc_interrupt();
            break;
            
        case 14: // Primary ATA
        case 15: // Secondary ATA
            handle_ata_interrupt(irq_num);
            break;
            
        default:
            // Spurious or unhandled interrupt
            break;
    }
}

// Timer interrupt handler - critical for real-time scheduling
void handle_timer_interrupt(void) {
    // Increment system tick
    system_ticks++;
    
    // Check if we need to run deadline checks
    if (system_ticks >= next_deadline_check) {
        check_task_deadlines();
        next_deadline_check = system_ticks + DEADLINE_CHECK_INTERVAL;
    }
    
    // Trigger scheduler (when implemented)
    // schedule_next_task();
}

// Keyboard interrupt handler
void handle_keyboard_interrupt(void) {
    uint8_t scancode = inb(0x60);
    
    // Process scancode (simplified for now)
    if (scancode & 0x80) {
        // Key release - ignore for now
    } else {
        // Key press - could add to keyboard buffer
        // For now, just acknowledge
    }
}

// RTC interrupt handler
void handle_rtc_interrupt(void) {
    // Read RTC register C to acknowledge interrupt
    outb(0x70, 0x0C);
    inb(0x71);
    
    // Update real-time clock data
    // (Implementation depends on RTC usage)
}

// ATA interrupt handler
void handle_ata_interrupt(uint32_t irq) {
    // Acknowledge ATA interrupt
    // (Full implementation requires ATA driver)
    
    if (irq == 14) {
        // Primary ATA
        inb(0x1F7); // Read status to clear interrupt
    } else {
        // Secondary ATA
        inb(0x177); // Read status to clear interrupt
    }
}

// Software interrupt handler
void handle_software_interrupt(uint32_t int_num) {
    // Handle system calls and software interrupts
    // (To be implemented with system call interface)
    
    print_string("Software interrupt: ");
    print_hex(int_num);
    print_string("\n");
}

// Check task deadlines for real-time guarantees
void check_task_deadlines(void) {
    // This will be implemented when we have task management
    // For now, just track that we're checking
    static uint32_t deadline_checks = 0;
    deadline_checks++;
}

// Get system tick count
uint64_t get_system_ticks(void) {
    return system_ticks;
}

// Get interrupt statistics
void get_interrupt_statistics(uint32_t int_num, interrupt_stats_t* stats) {
    if (stats && int_num < 256) {
        stats->count = interrupt_count[int_num];
        stats->total_cycles = interrupt_cycles[int_num];
        stats->max_cycles = interrupt_max_cycles[int_num];
    }
}

// Check interrupt system health
bool check_interrupt_health(void) {
    // Check for WCET violations
    if (interrupt_overruns > 0) {
        return false;
    }
    
    // Check for stuck interrupts
    if (nested_interrupt_level > 0) {
        return false;
    }
    
    return true;
}