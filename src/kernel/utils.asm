; AtomicOS Kernel Utilities
; Common functions used by interrupt system

[BITS 32]

section .text

global print_string
global print_hex
global outb
global inb
global get_tsc_low

%define VGA_BUFFER 0xB8000

; Current cursor position
cursor_x: dd 0
cursor_y: dd 2  ; Start at line 3 (after prompt)

; Print a null-terminated string
; Input: pointer to string on stack
print_string:
    push ebp
    mov ebp, esp
    push esi
    push edi
    push eax
    push ebx
    
    mov esi, [ebp + 8]      ; String pointer
    mov edi, [cursor_y]
    imul edi, 80
    add edi, [cursor_x]
    shl edi, 1              ; Multiply by 2 (char + attribute)
    add edi, VGA_BUFFER
    
.loop:
    lodsb                   ; Load byte from string
    test al, al             ; Check for null terminator
    jz .done
    
    cmp al, 0x0A            ; Check for newline
    je .newline
    
    mov byte [edi], al      ; Write character
    mov byte [edi + 1], 0x07 ; White on black
    add edi, 2
    
    inc dword [cursor_x]
    cmp dword [cursor_x], 80
    jl .loop
    
.newline:
    mov dword [cursor_x], 0
    inc dword [cursor_y]
    cmp dword [cursor_y], 25
    jl .update_pos
    
    ; Scroll if needed
    call scroll_screen
    mov dword [cursor_y], 24
    
.update_pos:
    mov edi, [cursor_y]
    imul edi, 80
    add edi, [cursor_x]
    shl edi, 1
    add edi, VGA_BUFFER
    jmp .loop
    
.done:
    pop ebx
    pop eax
    pop edi
    pop esi
    pop ebp
    ret

; Print a 32-bit hex value
; Input: value on stack
print_hex:
    push ebp
    mov ebp, esp
    push eax
    push ebx
    push ecx
    push edx
    
    mov eax, [ebp + 8]      ; Value to print
    mov ecx, 8              ; 8 hex digits
    
    ; Print "0x" prefix
    push dword hex_prefix
    call print_string
    add esp, 4
    
.loop:
    rol eax, 4              ; Rotate left by 4 bits
    mov edx, eax
    and edx, 0x0F           ; Get lower 4 bits
    
    cmp edx, 10
    jl .digit
    add dl, 'A' - 10
    jmp .print_char
    
.digit:
    add dl, '0'
    
.print_char:
    push eax
    push ecx
    
    ; Print single character
    mov edi, [cursor_y]
    imul edi, 80
    add edi, [cursor_x]
    shl edi, 1
    add edi, VGA_BUFFER
    
    mov byte [edi], dl
    mov byte [edi + 1], 0x07
    inc dword [cursor_x]
    
    pop ecx
    pop eax
    
    dec ecx
    jnz .loop
    
    pop edx
    pop ecx
    pop ebx
    pop eax
    pop ebp
    ret

; Scroll screen up by one line
scroll_screen:
    push esi
    push edi
    push ecx
    
    ; Copy lines 1-24 to lines 0-23
    mov esi, VGA_BUFFER + 160   ; Start of line 1
    mov edi, VGA_BUFFER         ; Start of line 0
    mov ecx, 80 * 24            ; 24 lines
    rep movsw                   ; Copy word by word
    
    ; Clear last line
    mov edi, VGA_BUFFER + (80 * 24 * 2)
    mov ecx, 80
    mov ax, 0x0720              ; Space with white on black
    rep stosw
    
    pop ecx
    pop edi
    pop esi
    ret

; Output byte to port
; void outb(uint16_t port, uint8_t value)
outb:
    push ebp
    mov ebp, esp
    
    mov dx, [ebp + 8]   ; port
    mov al, [ebp + 12]  ; value
    out dx, al
    
    pop ebp
    ret

; Input byte from port
; uint8_t inb(uint16_t port)
inb:
    push ebp
    mov ebp, esp
    
    mov dx, [ebp + 8]   ; port
    xor eax, eax
    in al, dx
    
    pop ebp
    ret

; Get low 32 bits of TSC
; uint32_t get_tsc_low(void)
get_tsc_low:
    rdtsc               ; Read time stamp counter
    ret                 ; Return low 32 bits in EAX

section .data
hex_prefix: db '0x', 0