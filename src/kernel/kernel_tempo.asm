; AtomicOS v2.0 - 100% Tempo Kernel Assembly Wrapper
; This file includes all Tempo-generated assembly modules

[bits 32]
[section .text]

; Kernel entry point - called from bootloader
global _start
_start:
    ; Set up kernel stack
    mov esp, kernel_stack_top
    
    ; Clear direction flag
    cld
    
    ; Call Tempo kernel main
    call kernel_main
    
    ; Should never reach here
    cli
    hlt

; Emergency halt in case kernel_main returns
.halt_loop:
    hlt
    jmp .halt_loop

; Include all Tempo-generated assembly modules
%include "kernel_main.s"
%include "tempo_libc.s"
%include "deterministic_scheduler.s"
%include "virtual_memory.s"
%include "tlb_management.s"
%include "memory_domains.s"
%include "security_primitives.s"
%include "kaslr_complete.s"

; Include additional modules if they exist
%ifdef INCLUDE_CACHE_PARTITIONING
    %include "cache_partitioning.s"
%endif

%ifdef INCLUDE_CHANNELS
    %include "channels.s"
%endif

%ifdef INCLUDE_PARALLEL_TASKS
    %include "parallel_tasks.s"
%endif

%ifdef INCLUDE_TRANSACTIONAL_MEMORY
    %include "transactional_memory.s"
%endif

; Kernel data section
[section .data]
    ; Version information
    kernel_version db "AtomicOS v2.0 - 100% Tempo", 0
    build_info db "Zero C Dependencies - Pure Deterministic Computing", 0
    
    ; System configuration
    max_processes dd 256
    heap_size dd 0x1000000  ; 16MB heap
    stack_size dd 0x10000   ; 64KB stack

; Kernel BSS section
[section .bss]
    ; Kernel stack (64KB)
    align 16
    kernel_stack_bottom:
    resb 0x10000
    kernel_stack_top:
    
    ; Heap area (16MB)
    align 4096
    kernel_heap:
    resb 0x1000000

; Global Descriptor Table
[section .data]
align 8
gdt_start:
    ; Null descriptor
    dd 0x0
    dd 0x0
    
    ; Kernel code segment
    dw 0xFFFF       ; Limit low
    dw 0x0000       ; Base low
    db 0x00         ; Base middle
    db 10011010b    ; Access byte (present, ring 0, code, readable)
    db 11001111b    ; Flags and limit high (4KB granularity, 32-bit)
    db 0x00         ; Base high
    
    ; Kernel data segment
    dw 0xFFFF       ; Limit low
    dw 0x0000       ; Base low
    db 0x00         ; Base middle
    db 10010010b    ; Access byte (present, ring 0, data, writable)
    db 11001111b    ; Flags and limit high
    db 0x00         ; Base high
    
    ; User code segment
    dw 0xFFFF       ; Limit low
    dw 0x0000       ; Base low
    db 0x00         ; Base middle
    db 11111010b    ; Access byte (present, ring 3, code, readable)
    db 11001111b    ; Flags and limit high
    db 0x00         ; Base high
    
    ; User data segment
    dw 0xFFFF       ; Limit low
    dw 0x0000       ; Base low
    db 0x00         ; Base middle
    db 11110010b    ; Access byte (present, ring 3, data, writable)
    db 11001111b    ; Flags and limit high
    db 0x00         ; Base high

gdt_end:

; GDT descriptor
gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Size
    dd gdt_start                ; Offset

; Segment selectors
CODE_SEG equ 0x08
DATA_SEG equ 0x10
USER_CODE_SEG equ 0x18
USER_DATA_SEG equ 0x20

; Interrupt Descriptor Table
[section .bss]
align 8
idt_start:
    resb 256 * 8    ; 256 IDT entries, 8 bytes each

idt_descriptor:
    dw 256 * 8 - 1  ; Size
    dd idt_start    ; Offset

; Page Directory and Tables (for virtual memory)
[section .bss]
align 4096
page_directory:
    resb 4096       ; Page directory (1024 entries * 4 bytes)

page_table_0:
    resb 4096       ; First page table

page_table_kernel:
    resb 4096       ; Kernel page table

; Tempo runtime support functions
[section .text]

; System call interface for Tempo functions
global tempo_syscall
tempo_syscall:
    ; Save registers
    push ebp
    mov ebp, esp
    pushad
    
    ; Get system call number and parameters
    mov eax, [ebp + 8]      ; Syscall number
    mov ebx, [ebp + 12]     ; Param 1
    mov ecx, [ebp + 16]     ; Param 2
    mov edx, [ebp + 20]     ; Param 3
    
    ; Dispatch to appropriate handler
    cmp eax, 1
    je .sys_write
    cmp eax, 2
    je .sys_read
    cmp eax, 3
    je .sys_memory_alloc
    
    ; Unknown syscall
    mov eax, -1
    jmp .return

.sys_write:
    ; Handle write syscall
    call tempo_puts
    mov eax, 0
    jmp .return

.sys_read:
    ; Handle read syscall
    mov eax, 0
    jmp .return

.sys_memory_alloc:
    ; Handle memory allocation
    call tempo_malloc
    jmp .return

.return:
    ; Restore registers and return
    popad
    mov esp, ebp
    pop ebp
    ret

; Hardware abstraction layer for Tempo
global tempo_outb
tempo_outb:
    push ebp
    mov ebp, esp
    
    mov dx, [ebp + 8]   ; Port
    mov al, [ebp + 12]  ; Value
    out dx, al
    
    pop ebp
    ret

global tempo_inb
tempo_inb:
    push ebp
    mov ebp, esp
    
    mov dx, [ebp + 8]   ; Port
    in al, dx
    movzx eax, al       ; Zero-extend to 32 bits
    
    pop ebp
    ret

; Timer functions for Tempo
global tempo_get_ticks
tempo_get_ticks:
    push ebp
    mov ebp, esp
    
    ; Read system timer (simplified)
    mov eax, [timer_ticks]
    
    pop ebp
    ret

; Memory management for Tempo
global tempo_get_free_memory
tempo_get_free_memory:
    push ebp
    mov ebp, esp
    
    ; Return free memory amount (simplified)
    mov eax, 0x1000000  ; 16MB
    
    pop ebp
    ret

; Interrupt handlers
global idt_flush
idt_flush:
    lidt [idt_descriptor]
    ret

global gdt_flush
gdt_flush:
    lgdt [gdt_descriptor]
    
    ; Reload segment registers
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Far jump to reload CS
    jmp CODE_SEG:.reload_cs
.reload_cs:
    ret

; Exception handlers (simplified)
global divide_error_handler
divide_error_handler:
    cli
    push 0      ; Error code
    push 0      ; Exception number
    jmp exception_common

global general_protection_handler
general_protection_handler:
    cli
    push 13     ; Exception number
    jmp exception_common

exception_common:
    ; Save all registers
    pushad
    
    ; Call Tempo exception handler
    call handle_exception
    
    ; Restore registers
    popad
    add esp, 8  ; Remove error code and exception number
    iret

; Variables
[section .bss]
timer_ticks resd 1
current_process resd 1
kernel_heap_ptr resd 1

; Constants
[section .rodata]
tempo_magic dd 0x54454D50  ; "TEMP" magic number
kernel_signature db "AtomicOS-Tempo-Kernel", 0

; End of kernel
kernel_end: