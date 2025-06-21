// AtomicOS Interrupt System Header
// Deterministic interrupt handling definitions

#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include "types.h"

// Interrupt vector numbers
#define INT_DIVIDE_ERROR        0
#define INT_DEBUG              1
#define INT_NMI                2
#define INT_BREAKPOINT         3
#define INT_OVERFLOW           4
#define INT_BOUND_RANGE        5
#define INT_INVALID_OPCODE     6
#define INT_DEVICE_NOT_AVAIL   7
#define INT_DOUBLE_FAULT       8
#define INT_INVALID_TSS        10
#define INT_SEGMENT_NOT_PRESENT 11
#define INT_STACK_SEGMENT      12
#define INT_GENERAL_PROTECTION 13
#define INT_PAGE_FAULT         14
#define INT_FPU_ERROR          16
#define INT_ALIGNMENT_CHECK    17
#define INT_MACHINE_CHECK      18
#define INT_SIMD_ERROR         19

// IRQ numbers (remapped to 32-47)
#define IRQ0_TIMER             32
#define IRQ1_KEYBOARD          33
#define IRQ2_CASCADE           34
#define IRQ3_COM2              35
#define IRQ4_COM1              36
#define IRQ5_LPT2              37
#define IRQ6_FLOPPY            38
#define IRQ7_LPT1              39
#define IRQ8_RTC               40
#define IRQ9_FREE              41
#define IRQ10_FREE             42
#define IRQ11_FREE             43
#define IRQ12_MOUSE            44
#define IRQ13_FPU              45
#define IRQ14_ATA_PRIMARY      46
#define IRQ15_ATA_SECONDARY    47

// Interrupt priorities for real-time scheduling
#define PRIORITY_NMI           0    // Highest priority
#define PRIORITY_TIMER         1    // Timer for scheduling
#define PRIORITY_CRITICAL      2    // Critical system interrupts
#define PRIORITY_HIGH          3    // High priority devices
#define PRIORITY_NORMAL        4    // Normal priority
#define PRIORITY_LOW           5    // Low priority devices

// IDT constants
#define IDT_ENTRIES            256
#define IDT_ENTRY_SIZE         8

// IDT entry flags
#define IDT_PRESENT            0x80
#define IDT_DPL_0              0x00
#define IDT_DPL_3              0x60
#define IDT_INTERRUPT_GATE     0x0E
#define IDT_TRAP_GATE          0x0F
#define IDT_TASK_GATE          0x05

// WCET bounds for interrupt handlers (in CPU cycles)
#define MAX_ISR_CYCLES         1000
#define MAX_IRQ_CYCLES         2000
#define MAX_NESTED_INTERRUPTS  3

// PIC (8259A) ports
#define PIC1_COMMAND           0x20
#define PIC1_DATA              0x21
#define PIC2_COMMAND           0xA0
#define PIC2_DATA              0xA1

// PIC commands
#define PIC_EOI                0x20
#define PIC_INIT               0x11
#define PIC_8086_MODE          0x01

// IDT entry structure
typedef struct {
    uint16_t offset_low;    // Offset bits 0-15
    uint16_t selector;      // Code segment selector
    uint8_t  reserved;      // Reserved, should be 0
    uint8_t  flags;         // Type and attributes
    uint16_t offset_high;   // Offset bits 16-31
} __attribute__((packed)) idt_entry_t;

// IDT pointer structure
typedef struct {
    uint16_t limit;         // Size of IDT - 1
    uint32_t base;          // Base address of IDT
} __attribute__((packed)) idt_ptr_t;

// Interrupt frame pushed by CPU
typedef struct {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp;
    uint32_t ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags;
    uint32_t useresp, ss;   // Only when crossing privilege levels
} interrupt_frame_t;

// Interrupt statistics structure
typedef struct {
    uint32_t count;         // Number of times interrupt occurred
    uint64_t total_cycles;  // Total cycles spent in handler
    uint32_t max_cycles;    // Maximum cycles for single invocation
} interrupt_stats_t;

// Function prototypes - Assembly
extern void init_idt(void);
extern void load_idt(void);
extern void enable_interrupts(void);
extern void disable_interrupts(void);

// Function prototypes - C handlers
void handle_interrupt_asm(uint32_t interrupt_num);
void handle_exception(uint32_t exception_num);
void handle_irq(uint32_t irq_num);
void handle_software_interrupt(uint32_t int_num);

// Specific interrupt handlers
void handle_timer_interrupt(void);
void handle_keyboard_interrupt(void);
void handle_rtc_interrupt(void);
void handle_ata_interrupt(uint32_t irq);

// Timer functions
void init_timer(void);
uint64_t get_system_ticks(void);

// Real-time functions
void check_task_deadlines(void);

// Statistics and debugging
void get_interrupt_statistics(uint32_t int_num, interrupt_stats_t* stats);
bool check_interrupt_health(void);

// IRQ control
void enable_irq(uint8_t irq);
void disable_irq(uint8_t irq);
void set_irq_priority(uint8_t irq, uint8_t priority);

// Inline assembly helpers
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline uint32_t get_tsc_low(void) {
    uint32_t low;
    asm volatile("rdtsc" : "=a"(low) : : "edx");
    return low;
}

#endif // _INTERRUPTS_H