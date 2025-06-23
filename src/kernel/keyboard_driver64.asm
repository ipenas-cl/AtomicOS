; AtomicOS Keyboard Driver 64-bit
; Copyright (c) 2025 Ignacio Peña Sepúlveda
; 
; Deterministic PS/2 keyboard driver for 64-bit mode

[BITS 64]

section .data

; Keyboard I/O ports
KB_DATA_PORT    equ 0x60
KB_STATUS_PORT  equ 0x64
KB_CMD_PORT     equ 0x64

; Keyboard buffer - circular buffer with fixed size
KB_BUFFER_SIZE  equ 256
kb_buffer:      times KB_BUFFER_SIZE db 0
kb_read_ptr:    dq 0
kb_write_ptr:   dq 0
kb_count:       dq 0

; Scancode to ASCII table (US layout)
scancode_table:
    db 0,  27, '1', '2', '3', '4', '5', '6', '7', '8'    ; 0-9
    db '9', '0', '-', '=', 8, 9                           ; 10-15 (8=backspace, 9=tab)
    db 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'  ; 16-25
    db '[', ']', 13, 0, 'a', 's', 'd', 'f', 'g', 'h'     ; 26-35 (13=enter)
    db 'j', 'k', 'l', ';', "'", '`', 0, '\', 'z', 'x'    ; 36-45
    db 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*'    ; 46-55
    db 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0                    ; 56-65
    times 128-66 db 0                                      ; Rest unmapped

; Shift scancode table
shift_scancode_table:
    db 0,  27, '!', '@', '#', '$', '%', '^', '&', '*'
    db '(', ')', '_', '+', 8, 9
    db 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'
    db '{', '}', 13, 0, 'A', 'S', 'D', 'F', 'G', 'H'
    db 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X'
    db 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*'
    db 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0
    times 128-66 db 0

; Keyboard state
kb_shift_pressed: db 0
kb_ctrl_pressed:  db 0
kb_alt_pressed:   db 0

section .text

global kb_init
global kb_handler
global kb_has_char
global kb_get_char
global kb_wait_char

; Initialize keyboard driver
kb_init:
    push rax
    push rbx
    
    ; Clear keyboard buffer
    mov al, 0xAD        ; Disable keyboard
    out KB_CMD_PORT, al
    
    ; Flush output buffer
.flush_loop:
    in al, KB_STATUS_PORT
    test al, 0x01       ; Output buffer full?
    jz .flush_done
    in al, KB_DATA_PORT ; Read and discard
    jmp .flush_loop
    
.flush_done:
    ; Enable keyboard
    mov al, 0xAE
    out KB_CMD_PORT, al
    
    ; Set scan code set 1
    mov al, 0xF0
    out KB_DATA_PORT, al
    call kb_wait_write
    mov al, 0x01
    out KB_DATA_PORT, al
    
    ; Enable keyboard scanning
    mov al, 0xF4
    out KB_DATA_PORT, al
    
    pop rbx
    pop rax
    ret

; Keyboard interrupt handler (IRQ1)
kb_handler:
    push rax
    push rbx
    push rcx
    
    ; Read scan code
    in al, KB_DATA_PORT
    
    ; Handle special keys
    cmp al, 0x2A        ; Left shift pressed
    je .shift_pressed
    cmp al, 0xAA        ; Left shift released
    je .shift_released
    cmp al, 0x36        ; Right shift pressed
    je .shift_pressed
    cmp al, 0xB6        ; Right shift released
    je .shift_released
    cmp al, 0x1D        ; Ctrl pressed
    je .ctrl_pressed
    cmp al, 0x9D        ; Ctrl released
    je .ctrl_released
    
    ; Check if key release (bit 7 set)
    test al, 0x80
    jnz .done           ; Ignore key releases
    
    ; Convert scancode to ASCII
    movzx rbx, al
    cmp rbx, 128
    jae .done           ; Out of range
    
    ; Choose table based on shift state
    cmp byte [kb_shift_pressed], 0
    je .use_normal_table
    
    ; Use shift table
    mov al, [shift_scancode_table + rbx]
    jmp .store_char
    
.use_normal_table:
    mov al, [scancode_table + rbx]
    
.store_char:
    test al, al         ; Valid character?
    jz .done
    
    ; Store in circular buffer
    mov rbx, [kb_write_ptr]
    mov rcx, [kb_count]
    cmp rcx, KB_BUFFER_SIZE
    jae .done           ; Buffer full
    
    mov [kb_buffer + rbx], al
    
    ; Update write pointer
    inc rbx
    and rbx, KB_BUFFER_SIZE - 1
    mov [kb_write_ptr], rbx
    
    ; Update count
    inc qword [kb_count]
    jmp .done
    
.shift_pressed:
    mov byte [kb_shift_pressed], 1
    jmp .done
    
.shift_released:
    mov byte [kb_shift_pressed], 0
    jmp .done
    
.ctrl_pressed:
    mov byte [kb_ctrl_pressed], 1
    jmp .done
    
.ctrl_released:
    mov byte [kb_ctrl_pressed], 0
    
.done:
    ; Send EOI to PIC
    mov al, 0x20
    out 0x20, al
    
    pop rcx
    pop rbx
    pop rax
    iretq

; Check if character available (non-blocking)
; Returns: AL = 1 if char available, 0 if not
kb_has_char:
    xor rax, rax
    cmp qword [kb_count], 0
    setnz al
    ret

; Get character from buffer (non-blocking)
; Returns: AL = character, or 0 if none
kb_get_char:
    push rbx
    push rcx
    
    ; Check if char available
    mov rcx, [kb_count]
    test rcx, rcx
    jz .no_char
    
    ; Get char from buffer
    mov rbx, [kb_read_ptr]
    movzx rax, byte [kb_buffer + rbx]
    
    ; Update read pointer
    inc rbx
    and rbx, KB_BUFFER_SIZE - 1
    mov [kb_read_ptr], rbx
    
    ; Update count
    dec qword [kb_count]
    
    pop rcx
    pop rbx
    ret
    
.no_char:
    xor rax, rax
    pop rcx
    pop rbx
    ret

; Wait for character (blocking)
; Returns: AL = character
kb_wait_char:
.wait_loop:
    call kb_has_char
    test al, al
    jz .wait_loop
    
    call kb_get_char
    ret

; Wait for keyboard ready to write
kb_wait_write:
    push rax
.wait:
    in al, KB_STATUS_PORT
    test al, 0x02       ; Input buffer full?
    jnz .wait
    pop rax
    ret