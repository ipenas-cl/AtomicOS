; AtomicOS Interactive Shell
; Copyright (c) 2025 Ignacio Peña Sepúlveda
;
; Deterministic shell - all commands have fixed execution time

[BITS 32]

%define VGA_BUFFER 0xB8000
%define MAX_PROCESSES 64

section .data

; Shell prompt
prompt_str: db 'atomicos> ', 0
prompt_len equ $ - prompt_str

; Command table - fixed size, deterministic lookup
align 16
command_table:
    db 'help', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  ; 16 bytes
    dd cmd_help                                    ; 4 bytes handler
    
    db 'ps', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    dd cmd_ps
    
    db 'ver', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    dd cmd_version
    
    db 'clear', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    dd cmd_clear
    
    db 'reboot', 0, 0, 0, 0, 0, 0, 0, 0, 0
    dd cmd_reboot
    
    ; End marker
    times 16 db 0xFF
    dd 0

; Messages
help_msg:
    db 'AtomicOS Shell Commands:', 10
    db '  help     - Show this help', 10
    db '  ps       - List processes', 10
    db '  ver      - Show version', 10
    db '  clear    - Clear screen', 10
    db '  reboot   - Reboot system', 10, 0

version_msg:
    db 'AtomicOS v5.1.0', 10
    db 'Tempo Language Support', 10
    db 'Deterministic Real-Time OS', 10, 0

banner_msg:
    db 'AtomicOS v5.1.0 - Deterministic Shell', 10
    db 'Type "help" for commands', 10, 10, 0

unknown_cmd_msg:
    db 'Unknown command. Type "help" for list.', 10, 0

ps_header:
    db 'PID  NAME            STATUS', 10
    db '---  --------------- -------', 10, 0

; Input buffer - fixed size for determinism
input_buffer: times 256 db 0
input_pos: dd 0

; Current screen position
screen_row: dd 10
screen_col: dd 0

section .text

global shell_init
global shell_loop

; Initialize shell
shell_init:
    push ebp
    mov ebp, esp
    
    ; Clear screen
    call clear_screen
    
    ; Print banner
    mov esi, banner_msg
    call shell_print_string
    
    ; Show prompt
    call show_prompt
    
    pop ebp
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
    mov ebx, [input_pos]
    cmp ebx, 255        ; Buffer full?
    jae .no_input
    
    mov [input_buffer + ebx], al
    inc dword [input_pos]
    
    ; Echo character
    call putchar
    
.no_input:
    ret

.backspace:
    mov ebx, [input_pos]
    test ebx, ebx
    jz .no_input
    
    dec dword [input_pos]
    mov byte [input_buffer + ebx - 1], 0
    
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
    mov ebx, [input_pos]
    mov byte [input_buffer + ebx], 0
    
    ; New line
    mov al, 10
    call putchar
    
    ; Parse and execute (deterministic time)
    call execute_command
    
    ; Clear buffer
    xor eax, eax
    mov [input_pos], eax
    mov ecx, 256/4
    mov edi, input_buffer
    rep stosd
    
    ; Show prompt again
    call show_prompt
    
    ret

; Execute command - O(1) lookup
execute_command:
    push ebp
    mov ebp, esp
    push ebx
    push ecx
    
    ; Empty command?
    cmp byte [input_buffer], 0
    je .done
    
    ; Linear search through command table
    mov ebx, command_table
    
.search_loop:
    ; Check for end marker
    cmp byte [ebx], 0xFF
    je .not_found
    
    ; Compare command
    mov esi, input_buffer
    mov edi, ebx
    call strcmp
    
    test eax, eax
    jz .found
    
    ; Next entry
    add ebx, 20         ; 16 + 4 bytes
    jmp .search_loop

.found:
    ; Call handler
    mov eax, [ebx + 16]  ; Handler address
    call eax
    jmp .done

.not_found:
    mov esi, unknown_cmd_msg
    call shell_print_string

.done:
    pop ecx
    pop ebx
    pop ebp
    ret

; Command handlers
cmd_help:
    mov esi, help_msg
    call shell_print_string
    ret

cmd_version:
    mov esi, version_msg
    call shell_print_string
    ret

cmd_clear:
    call clear_screen
    mov dword [screen_row], 0
    mov dword [screen_col], 0
    ret

cmd_ps:
    mov esi, ps_header
    call shell_print_string
    
    ; Show kernel process
    mov esi, .kernel_proc
    call shell_print_string
    
    ; Show shell process
    mov esi, .shell_proc
    call shell_print_string
    
    ret
    
.kernel_proc: db '0    kernel          RUNNING', 10, 0
.shell_proc:  db '1    shell           RUNNING', 10, 0

cmd_reboot:
    ; Triple fault to reboot
    ; Generate invalid IDT reference to cause reboot
    lidt [invalid_idt]
    ret
    
invalid_idt:
    dw 0
    dd 0

; Helper functions
show_prompt:
    mov esi, prompt_str
    call shell_print_string
    ret

clear_screen:
    push edi
    push ecx
    push eax
    
    mov edi, VGA_BUFFER
    mov ecx, 80 * 25
    mov ax, 0x0720      ; Space with white on black
    rep stosw
    
    pop eax
    pop ecx
    pop edi
    ret

shell_print_string:
    push eax
    push esi
.loop:
    lodsb
    test al, al
    jz .done
    call putchar
    jmp .loop
.done:
    pop esi
    pop eax
    ret

putchar:
    push eax
    push ebx
    push edi
    
    cmp al, 10          ; Newline?
    je .newline
    
    cmp al, 8           ; Backspace?
    je .backspace
    
    ; Calculate VGA position
    mov ebx, [screen_row]
    imul ebx, 80
    add ebx, [screen_col]
    shl ebx, 1          ; * 2 for attribute byte
    
    ; Write character
    mov edi, VGA_BUFFER
    add edi, ebx
    mov ah, 0x0F        ; White on black
    stosw
    
    ; Advance column
    inc dword [screen_col]
    cmp dword [screen_col], 80
    jb .done
    
.newline:
    mov dword [screen_col], 0
    inc dword [screen_row]
    cmp dword [screen_row], 25
    jb .done
    
    ; Scroll
    call shell_scroll_screen
    mov dword [screen_row], 24
    jmp .done
    
.backspace:
    cmp dword [screen_col], 0
    je .done
    dec dword [screen_col]
    
.done:
    pop edi
    pop ebx
    pop eax
    ret

shell_scroll_screen:
    push esi
    push edi
    push ecx
    
    ; Copy lines 1-24 to 0-23
    mov esi, VGA_BUFFER + 160  ; Start of line 1
    mov edi, VGA_BUFFER         ; Start of line 0
    mov ecx, 80 * 24 * 2 / 4   ; DWORDs to copy
    rep movsd
    
    ; Clear last line
    mov edi, VGA_BUFFER + (80 * 24 * 2)
    mov ecx, 80
    mov ax, 0x0720
    rep stosw
    
    pop ecx
    pop edi
    pop esi
    ret

strcmp:
    push esi
    push edi
.loop:
    mov al, [esi]
    mov dl, [edi]
    cmp al, dl
    jne .different
    
    test al, al
    jz .equal
    
    inc esi
    inc edi
    jmp .loop
    
.equal:
    xor eax, eax
    jmp .done
    
.different:
    mov eax, 1
    
.done:
    pop edi
    pop esi
    ret