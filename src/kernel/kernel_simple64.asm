; Copyright (c) 2025 Ignacio Peña
; Part of AtomicOS Project - https://github.com/ipenas-cl/AtomicOS
; Licensed under MIT License - see LICENSE file for details

; AtomicOS v5.2.1 64-bit Kernel (Simplified)
[BITS 64]
[ORG 0x10000]

%define VGA_BUFFER 0xB8000

section .text
global _start

_start:
    jmp kernel_main

kernel_main:
    ; Setup 64-bit stack properly
    mov rsp, 0x90000
    mov rbp, rsp
    
    ; Clear screen
    mov rdi, VGA_BUFFER
    mov rcx, 80 * 25
    mov ax, 0x0720
    rep stosw
    
    ; Print welcome message at top
    mov rdi, VGA_BUFFER
    mov rsi, welcome_msg
    call print_string_direct
    
    ; Print Tempo message on next line
    mov rdi, VGA_BUFFER + 160  ; Next line (80 chars * 2 bytes)
    mov rsi, tempo_msg
    call print_string_direct
    
    ; Print status on line 5
    mov rdi, VGA_BUFFER + 800  ; Line 5
    mov rsi, status_msg
    call print_string_direct
    
    ; Print shell prompt on line 7
    mov rdi, VGA_BUFFER + 1120  ; Line 7
    mov rsi, prompt_msg
    call print_string_direct
    
    ; Simple infinite loop (no interrupts for now)
.hang:
    hlt
    jmp .hang

; Direct VGA print (no dependencies)
print_string_direct:
    push rax
    push rsi
.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0F        ; White on black
    stosw
    jmp .loop
.done:
    pop rsi
    pop rax
    ret

section .data
welcome_msg: db 'AtomicOS v5.2.1 - 64-bit Native Mode', 0
tempo_msg:   db 'Compatible with Tempo v5.2.1 macOS Native Compiler', 0
status_msg:  db 'Kernel running in pure 64-bit mode - No legacy 32-bit code', 0
prompt_msg:  db 'atomicos64> Ready for input (keyboard driver loading...)', 0

; Pad kernel
times 512*64-($-$$) db 0