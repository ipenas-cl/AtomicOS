; Copyright (c) 2025 Ignacio Peña
; Part of AtomicOS Project - https://github.com/ipenas-cl/AtomicOS
; Licensed under MIT License - see LICENSE file for details

; AtomicOS v5.1.1 Working Kernel
[BITS 32]
[ORG 0x10000]

%define VGA_BUFFER 0xB8000

section .text
global _start

_start:
    jmp kernel_main

; Include all Tempo modules (but don't use them yet)
%include "kernel/ssp_functions.inc"
%include "kernel/guard_pages.inc" 
%include "kernel/deterministic_core.inc"
%include "kernel/interrupts.inc"
%include "kernel/logo.inc"
%include "kernel/context_switch.inc"
%include "kernel/process_management.inc"
%include "kernel/realtime_scheduler.inc"
%include "kernel/syscall_handler.inc"
%include "kernel/ipc_stubs.inc"
%include "kernel/fs_stubs.inc"
%include "kernel/paging.inc"

kernel_main:
    ; Setup stack properly
    mov esp, 0x90000
    mov ebp, esp
    
    ; Clear screen
    mov edi, VGA_BUFFER
    mov ecx, 80 * 25
    mov ax, 0x0720
    rep stosw
    
    ; Print welcome message at top
    mov edi, VGA_BUFFER
    mov esi, welcome_msg
    call print_string_direct
    
    ; Print Tempo message on next line
    mov edi, VGA_BUFFER + 160  ; Next line (80 chars * 2 bytes)
    mov esi, tempo_msg
    call print_string_direct
    
    ; Print status on line 5
    mov edi, VGA_BUFFER + 800  ; Line 5
    mov esi, status_msg
    call print_string_direct
    
    ; Initialize shell and scheduler
    call init_shell_system
    
    ; Enable interrupts for scheduler
    sti
    
    ; Enter scheduled mode
.scheduled:
    hlt                     ; Wait for timer interrupt
    jmp .scheduled          ; Scheduler will handle tasks

; Initialize shell subsystem
init_shell_system:
    ; Initialize keyboard driver
    call kb_init
    
    ; Initialize timer
    call timer_init
    
    ; Initialize scheduler (from realtime_scheduler.inc)
    call init_rt_scheduler
    
    ; Schedule shell task to run every 10ms
    mov edi, shell_task_entry
    mov esi, 10         ; 10 ticks = 10ms
    call schedule_task
    
    ret

; Shell task entry point
shell_task_entry:
    call shell_loop
    ret

; Include new assembly modules
%include "kernel/keyboard_driver.asm"
%include "kernel/timer.asm"
%include "kernel/atomic_shell.asm"

; Direct VGA print (no dependencies)
print_string_direct:
    push eax
    push esi
.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0F        ; White on black
    stosw
    jmp .loop
.done:
    pop esi
    pop eax
    ret

section .data
welcome_msg: db 'AtomicOS v5.2.1 - Pure Assembly + Tempo Security', 0
tempo_msg:   db 'No More C Needed! Write your OS in Tempo!', 0
status_msg:  db 'Kernel loaded successfully. All 14 Tempo modules included.', 0

; Pad kernel
times 512*64-($-$$) db 0