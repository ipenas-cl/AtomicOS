; Copyright (c) 2025 Ignacio Peña
; AtomicOS - Deterministic Real-Time Security Operating System
; https://github.com/ipenas-cl/AtomicOS
; Licensed under MIT License - see LICENSE file for details

; AtomicOS Interrupt System
; Deterministic interrupt handling with WCET bounds

[BITS 32]

section .data

; IDT (Interrupt Descriptor Table) - 256 entries
align 4096  ; Page-aligned for security
idt_start:
    times 256 dq 0  ; 256 8-byte entries

idt_ptr:
    dw 256 * 8 - 1  ; Limit (size - 1)
    dd idt_start     ; Base address

; Interrupt statistics for real-time analysis
interrupt_count:
    times 256 dd 0

interrupt_cycles:
    times 256 dd 0

interrupt_max_cycles:
    times 256 dd 0

nested_interrupt_level:
    dd 0

interrupt_overruns:
    dd 0

; Maximum allowed cycles for handlers (WCET)
MAX_ISR_CYCLES equ 1000
MAX_IRQ_CYCLES equ 2000
MAX_NESTED_INTERRUPTS equ 3

; Interrupt messages for debugging
exc_messages:
    dd exc_msg_0, exc_msg_1, exc_msg_2, exc_msg_3
    dd exc_msg_4, exc_msg_5, exc_msg_6, exc_msg_7
    dd exc_msg_8, exc_msg_9, exc_msg_10, exc_msg_11
    dd exc_msg_12, exc_msg_13, exc_msg_14, exc_msg_15
    dd exc_msg_16, exc_msg_17, exc_msg_18, exc_msg_19

exc_msg_0  db 'Divide by zero', 0
exc_msg_1  db 'Debug exception', 0
exc_msg_2  db 'Non-maskable interrupt', 0
exc_msg_3  db 'Breakpoint', 0
exc_msg_4  db 'Overflow', 0
exc_msg_5  db 'Bound range exceeded', 0
exc_msg_6  db 'Invalid opcode', 0
exc_msg_7  db 'Device not available', 0
exc_msg_8  db 'Double fault', 0
exc_msg_9  db 'Coprocessor segment overrun', 0
exc_msg_10 db 'Invalid TSS', 0
exc_msg_11 db 'Segment not present', 0
exc_msg_12 db 'Stack segment fault', 0
exc_msg_13 db 'General protection fault', 0
exc_msg_14 db 'Page fault', 0
exc_msg_15 db 'Reserved', 0
exc_msg_16 db 'x87 FPU error', 0
exc_msg_17 db 'Alignment check', 0
exc_msg_18 db 'Machine check', 0
exc_msg_19 db 'SIMD floating-point exception', 0

section .text

global init_idt
global load_idt
global enable_interrupts
global disable_interrupts

; Interrupt Service Routines (ISRs) - Exception handlers
global isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
global isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
global isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
global isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31

; Hardware interrupt handlers (IRQs)
global irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7
global irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15

; External functions
extern handle_interrupt_asm
extern get_tsc_low

; Initialize the IDT with all handlers
init_idt:
    push ebp
    mov ebp, esp
    push edi
    push esi
    
    ; Set up exception handlers (0-31)
    mov edi, 0
    mov esi, isr0
    call set_idt_entry
    
    mov edi, 1
    mov esi, isr1
    call set_idt_entry
    
    ; Continue for all 32 exceptions...
    ; (Using macro to reduce repetition)
    
    %assign i 2
    %rep 30
        mov edi, i
        mov esi, isr%[i]
        call set_idt_entry
        %assign i i+1
    %endrep
    
    ; Set up IRQ handlers (32-47)
    mov edi, 32
    mov esi, irq0
    call set_idt_entry
    
    mov edi, 33
    mov esi, irq1
    call set_idt_entry
    
    ; Continue for all 16 IRQs...
    %assign i 2
    %rep 14
        mov edi, i+32
        mov esi, irq%[i]
        call set_idt_entry
        %assign i i+1
    %endrep
    
    ; Set up system call handler (INT 0x80)
    extern syscall_handler
    mov edi, 0x80           ; INT 0x80
    mov esi, syscall_handler
    call set_idt_entry_user ; Special version for user-callable
    
    ; Remap the PIC
    call remap_pic
    
    pop esi
    pop edi
    pop ebp
    ret

; Set an IDT entry accessible from user space
; Input: EDI = interrupt number, ESI = handler address
set_idt_entry_user:
    push eax
    push ebx
    
    ; Calculate IDT entry address
    mov eax, edi
    shl eax, 3          ; Multiply by 8 (entry size)
    add eax, idt_start
    
    ; Set up the entry
    mov word [eax], si      ; Handler low 16 bits
    mov word [eax + 2], 0x08    ; Kernel code segment
    mov byte [eax + 4], 0       ; Reserved
    mov byte [eax + 5], 0xEE    ; Present, DPL=3 (user), Interrupt gate
    shr esi, 16
    mov word [eax + 6], si      ; Handler high 16 bits
    
    pop ebx
    pop eax
    ret

; Set an IDT entry
; Input: EDI = interrupt number, ESI = handler address
set_idt_entry:
    push eax
    push ebx
    
    ; Calculate IDT entry address
    mov eax, edi
    shl eax, 3          ; Multiply by 8 (entry size)
    add eax, idt_start
    
    ; Set up the entry
    mov word [eax], si      ; Handler low 16 bits
    mov word [eax + 2], 0x08    ; Kernel code segment
    mov byte [eax + 4], 0       ; Reserved
    mov byte [eax + 5], 0x8E    ; Present, DPL=0, Interrupt gate
    shr esi, 16
    mov word [eax + 6], si      ; Handler high 16 bits
    
    pop ebx
    pop eax
    ret

; Load the IDT
load_idt:
    lidt [idt_ptr]
    ret

; Enable interrupts
enable_interrupts:
    sti
    ret

; Disable interrupts
disable_interrupts:
    cli
    ret

; Remap the PIC (Programmable Interrupt Controller)
remap_pic:
    push eax
    
    ; Start initialization sequence
    mov al, 0x11
    out 0x20, al    ; Master PIC command
    out 0xA0, al    ; Slave PIC command
    
    ; Set vector offsets
    mov al, 0x20    ; Master PIC vector offset (32)
    out 0x21, al
    mov al, 0x28    ; Slave PIC vector offset (40)
    out 0xA1, al
    
    ; Set up cascading
    mov al, 0x04    ; Tell master PIC there's a slave at IRQ2
    out 0x21, al
    mov al, 0x02    ; Tell slave PIC its cascade identity
    out 0xA1, al
    
    ; Set 8086 mode
    mov al, 0x01
    out 0x21, al
    out 0xA1, al
    
    ; Mask all interrupts initially
    mov al, 0xFF
    out 0x21, al
    out 0xA1, al
    
    pop eax
    ret

; Common interrupt handler with WCET tracking
common_interrupt_handler:
    ; Save all registers (deterministic)
    pushad
    push ds
    push es
    push fs
    push gs
    
    ; Set up kernel segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Get start time for WCET tracking
    call get_tsc_low
    push eax            ; Save start time
    
    ; Check nested interrupt level
    mov eax, [nested_interrupt_level]
    cmp eax, MAX_NESTED_INTERRUPTS
    jge .overflow
    
    ; Increment nested level
    inc dword [nested_interrupt_level]
    
    ; Call the C handler with interrupt number
    push dword [esp + 52]   ; Interrupt number
    call handle_interrupt_asm
    add esp, 4
    
    ; Get end time and calculate cycles
    call get_tsc_low
    mov ebx, eax
    pop eax             ; Get start time
    sub ebx, eax        ; Calculate cycles used
    
    ; Update statistics
    mov eax, [esp + 48]     ; Get interrupt number
    mov ecx, [interrupt_cycles + eax * 4]
    add ecx, ebx
    mov [interrupt_cycles + eax * 4], ecx
    
    ; Check if this is a new maximum
    mov ecx, [interrupt_max_cycles + eax * 4]
    cmp ebx, ecx
    jle .not_new_max
    mov [interrupt_max_cycles + eax * 4], ebx
    
.not_new_max:
    ; Check WCET violation
    cmp dword [esp + 48], 32
    jl .check_isr_wcet
    cmp dword [esp + 48], 48
    jl .check_irq_wcet
    jmp .wcet_ok
    
.check_isr_wcet:
    cmp ebx, MAX_ISR_CYCLES
    jg .wcet_violation
    jmp .wcet_ok
    
.check_irq_wcet:
    cmp ebx, MAX_IRQ_CYCLES
    jg .wcet_violation
    
.wcet_ok:
    ; Decrement nested level
    dec dword [nested_interrupt_level]
    jmp .done
    
.wcet_violation:
    inc dword [interrupt_overruns]
    dec dword [nested_interrupt_level]
    jmp .done
    
.overflow:
    inc dword [interrupt_overruns]
    
.done:
    ; Restore registers
    pop gs
    pop fs
    pop es
    pop ds
    popad
    add esp, 8      ; Remove error code and interrupt number
    iret

; Macro for ISR without error code
%macro ISR_NO_ERR 1
isr%1:
    cli
    push dword 0    ; Dummy error code
    push dword %1   ; Interrupt number
    jmp common_interrupt_handler
%endmacro

; Macro for ISR with error code
%macro ISR_ERR 1
isr%1:
    cli
    push dword %1   ; Interrupt number
    jmp common_interrupt_handler
%endmacro

; Exception handlers
ISR_NO_ERR 0    ; Divide by zero
ISR_NO_ERR 1    ; Debug
ISR_NO_ERR 2    ; NMI
ISR_NO_ERR 3    ; Breakpoint
ISR_NO_ERR 4    ; Overflow
ISR_NO_ERR 5    ; Bound range exceeded
ISR_NO_ERR 6    ; Invalid opcode
ISR_NO_ERR 7    ; Device not available
ISR_ERR    8    ; Double fault
ISR_NO_ERR 9    ; Coprocessor segment overrun
ISR_ERR    10   ; Invalid TSS
ISR_ERR    11   ; Segment not present
ISR_ERR    12   ; Stack segment fault
ISR_ERR    13   ; General protection fault
ISR_ERR    14   ; Page fault
ISR_NO_ERR 15   ; Reserved
ISR_NO_ERR 16   ; x87 FPU error
ISR_ERR    17   ; Alignment check
ISR_NO_ERR 18   ; Machine check
ISR_NO_ERR 19   ; SIMD floating-point exception
ISR_NO_ERR 20   ; Reserved
ISR_NO_ERR 21   ; Reserved
ISR_NO_ERR 22   ; Reserved
ISR_NO_ERR 23   ; Reserved
ISR_NO_ERR 24   ; Reserved
ISR_NO_ERR 25   ; Reserved
ISR_NO_ERR 26   ; Reserved
ISR_NO_ERR 27   ; Reserved
ISR_NO_ERR 28   ; Reserved
ISR_NO_ERR 29   ; Reserved
ISR_NO_ERR 30   ; Reserved
ISR_NO_ERR 31   ; Reserved

; Macro for IRQ handlers
%macro IRQ 2
irq%1:
    cli
    push dword 0    ; No error code
    push dword %2   ; Interrupt number
    jmp common_interrupt_handler
%endmacro

; IRQ handlers
IRQ 0, 32   ; Timer
IRQ 1, 33   ; Keyboard
IRQ 2, 34   ; Cascade
IRQ 3, 35   ; COM2
IRQ 4, 36   ; COM1
IRQ 5, 37   ; LPT2
IRQ 6, 38   ; Floppy
IRQ 7, 39   ; LPT1
IRQ 8, 40   ; RTC
IRQ 9, 41   ; Free
IRQ 10, 42  ; Free
IRQ 11, 43  ; Free
IRQ 12, 44  ; Mouse
IRQ 13, 45  ; FPU
IRQ 14, 46  ; Primary ATA
IRQ 15, 47  ; Secondary ATA