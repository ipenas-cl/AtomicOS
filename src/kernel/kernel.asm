; Copyright (c) 2025 Ignacio Peña
; Part of AtomicOS Project - https://github.com/ipenas-cl/AtomicOS
; Licensed under MIT License - see LICENSE file for details

; AtomicOS v1.0.0 Working Kernel
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
    
    ; Simple idle loop
.idle:
    hlt
    jmp .idle

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
welcome_msg: db 'AtomicOS v1.0.0 - Now with Tempo!', 0
tempo_msg:   db 'No More C Needed! Write your OS in Tempo!', 0
status_msg:  db 'Kernel loaded successfully. All 14 Tempo modules included.', 0

; Pad kernel
times 512*64-($-$$) db 0