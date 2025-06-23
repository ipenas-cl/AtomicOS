; AtomicOS Interactive Shell 64-bit
; Copyright (c) 2025 Ignacio Peña Sepúlveda
;
; Deterministic shell - all commands have fixed execution time

[BITS 64]

%define VGA_BUFFER 0xB8000
%define MAX_PROCESSES 64

section .data

; Shell prompt
prompt_str: db 'atomicos64> ', 0
prompt_len equ $ - prompt_str

; Command table - fixed size, deterministic lookup
align 16
command_table:
    db 'help', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  ; 16 bytes
    dq cmd_help                                    ; 8 bytes handler
    
    db 'ps', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    dq cmd_ps
    
    db 'ver', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    dq cmd_version
    
    db 'clear', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    dq cmd_clear
    
    db 'reboot', 0, 0, 0, 0, 0, 0, 0, 0, 0
    dq cmd_reboot
    
    db 'mem', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    dq cmd_memory
    
    ; End marker
    times 16 db 0xFF
    dq 0

; Messages
help_msg:
    db 'AtomicOS 64-bit Shell Commands:', 10
    db '  help     - Show this help', 10
    db '  ps       - List processes', 10
    db '  ver      - Show version', 10
    db '  mem      - Memory info', 10
    db '  clear    - Clear screen', 10
    db '  reboot   - Reboot system', 10, 0

version_msg:
    db 'AtomicOS v5.2.1 64-bit', 10
    db 'Tempo Language Support (Native macOS)', 10
    db 'Deterministic Real-Time OS', 10, 0

memory_msg:
    db 'Memory Information:', 10
    db '  Kernel: 64-bit mode', 10
    db '  Stack:  0x90000', 10
    db '  VGA:    0xB8000', 10, 0

banner_msg:
    db 'AtomicOS v5.2.1 64-bit - Deterministic Shell', 10
    db 'Type "help" for commands', 10, 10, 0

unknown_cmd_msg:
    db 'Unknown command. Type "help" for list.', 10, 0

ps_header:
    db 'PID  NAME            STATUS', 10
    db '---  --------------- -------', 10, 0

; Input buffer - fixed size for determinism
input_buffer: times 256 db 0
input_pos: dq 0

; Current screen position
screen_row: dq 10
screen_col: dq 0

section .text

global shell_init
global shell_loop

; Initialize shell
shell_init:
    push rbp
    mov rbp, rsp
    
    ; Clear screen
    call clear_screen
    
    ; Print banner
    mov rsi, banner_msg
    call shell_print_string
    
    ; Show prompt
    call show_prompt
    
    pop rbp
    ret

; Main shell loop - deterministic
shell_loop:
    ; Check for keyboard input (non-blocking)
    extern kb_has_char
    call kb_has_char
    test al, al
    jz .no_input
    
    ; Get character
    extern kb_get_char
    call kb_get_char
    
    ; Handle special keys
    cmp al, 13          ; Enter
    je .process_command
    
    cmp al, 8           ; Backspace
    je .backspace
    
    ; Regular character - add to buffer
    mov rbx, [input_pos]
    cmp rbx, 255        ; Buffer full?
    jae .no_input
    
    mov [input_buffer + rbx], al
    inc qword [input_pos]
    
    ; Echo character
    call putchar
    
.no_input:
    ret

.backspace:
    mov rbx, [input_pos]
    test rbx, rbx
    jz .no_input
    
    dec qword [input_pos]
    mov byte [input_buffer + rbx - 1], 0
    
    ; Visual backspace
    mov al, 8
    call putchar
    mov al, ' '
    call putchar
    mov al, 8
    call putchar
    
    jmp .no_input

.process_command:
    ; Null terminate
    mov rbx, [input_pos]
    mov byte [input_buffer + rbx], 0
    
    ; New line
    mov al, 10
    call putchar
    
    ; Parse and execute (deterministic time)
    call execute_command
    
    ; Clear buffer
    xor rax, rax
    mov [input_pos], rax
    mov rcx, 256/8
    mov rdi, input_buffer
    rep stosq
    
    ; Show prompt again
    call show_prompt
    
    ret

; Execute command - O(1) lookup
execute_command:
    push rbp
    mov rbp, rsp
    push rbx
    push rcx
    
    ; Empty command?
    cmp byte [input_buffer], 0
    je .done
    
    ; Linear search through command table
    mov rbx, command_table
    
.search_loop:
    ; Check for end marker
    cmp byte [rbx], 0xFF
    je .not_found
    
    ; Compare command
    mov rsi, input_buffer
    mov rdi, rbx
    call strcmp
    
    test rax, rax
    jz .found
    
    ; Next entry
    add rbx, 24         ; 16 + 8 bytes
    jmp .search_loop

.found:
    ; Call handler
    mov rax, [rbx + 16]  ; Handler address
    call rax
    jmp .done

.not_found:
    mov rsi, unknown_cmd_msg
    call shell_print_string

.done:
    pop rcx
    pop rbx
    pop rbp
    ret

; Command handlers
cmd_help:
    mov rsi, help_msg
    call shell_print_string
    ret

cmd_version:
    mov rsi, version_msg
    call shell_print_string
    ret

cmd_memory:
    mov rsi, memory_msg
    call shell_print_string
    ret

cmd_clear:
    call clear_screen
    mov qword [screen_row], 0
    mov qword [screen_col], 0
    ret

cmd_ps:
    mov rsi, ps_header
    call shell_print_string
    
    ; Show kernel process
    mov rsi, .kernel_proc
    call shell_print_string
    
    ; Show shell process
    mov rsi, .shell_proc
    call shell_print_string
    
    ret
    
.kernel_proc: db '0    kernel64        RUNNING', 10, 0
.shell_proc:  db '1    shell64         RUNNING', 10, 0

cmd_reboot:
    ; Generate invalid IDT reference to cause reboot
    lidt [invalid_idt]
    ret
    
invalid_idt:
    dw 0
    dq 0

; Helper functions
show_prompt:
    mov rsi, prompt_str
    call shell_print_string
    ret

clear_screen:
    push rdi
    push rcx
    push rax
    
    mov rdi, VGA_BUFFER
    mov rcx, 80 * 25
    mov ax, 0x0720      ; Space with white on black
    rep stosw
    
    pop rax
    pop rcx
    pop rdi
    ret

shell_print_string:
    push rax
    push rsi
.loop:
    lodsb
    test al, al
    jz .done
    call putchar
    jmp .loop
.done:
    pop rsi
    pop rax
    ret

putchar:
    push rax
    push rbx
    push rdi
    
    cmp al, 10          ; Newline?
    je .newline
    
    cmp al, 8           ; Backspace?
    je .backspace
    
    ; Calculate VGA position
    mov rbx, [screen_row]
    imul rbx, 80
    add rbx, [screen_col]
    shl rbx, 1          ; * 2 for attribute byte
    
    ; Write character
    mov rdi, VGA_BUFFER
    add rdi, rbx
    mov ah, 0x0F        ; White on black
    stosw
    
    ; Advance column
    inc qword [screen_col]
    cmp qword [screen_col], 80
    jb .done
    
.newline:
    mov qword [screen_col], 0
    inc qword [screen_row]
    cmp qword [screen_row], 25
    jb .done
    
    ; Scroll
    call shell_scroll_screen
    mov qword [screen_row], 24
    jmp .done
    
.backspace:
    cmp qword [screen_col], 0
    je .done
    dec qword [screen_col]
    
.done:
    pop rdi
    pop rbx
    pop rax
    ret

shell_scroll_screen:
    push rsi
    push rdi
    push rcx
    
    ; Copy lines 1-24 to 0-23
    mov rsi, VGA_BUFFER + 160  ; Start of line 1
    mov rdi, VGA_BUFFER         ; Start of line 0
    mov rcx, 80 * 24 * 2 / 8   ; QWORDs to copy
    rep movsq
    
    ; Clear last line
    mov rdi, VGA_BUFFER + (80 * 24 * 2)
    mov rcx, 80
    mov ax, 0x0720
    rep stosw
    
    pop rcx
    pop rdi
    pop rsi
    ret

strcmp:
    push rsi
    push rdi
.loop:
    mov al, [rsi]
    mov dl, [rdi]
    cmp al, dl
    jne .different
    
    test al, al
    jz .equal
    
    inc rsi
    inc rdi
    jmp .loop
    
.equal:
    xor rax, rax
    jmp .done
    
.different:
    mov rax, 1
    
.done:
    pop rdi
    pop rsi
    ret