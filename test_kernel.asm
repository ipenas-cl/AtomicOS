; Simple test kernel for AtomicOS v5.2.1
[BITS 32]
[ORG 0x10000]

%define VGA_BUFFER 0xB8000

section .text
global _start

_start:
    jmp kernel_main

kernel_main:
    ; Setup stack
    mov esp, 0x90000
    mov ebp, esp
    
    ; Clear screen
    mov edi, VGA_BUFFER
    mov ecx, 80 * 25
    mov ax, 0x0720
    rep stosw
    
    ; Print message
    mov edi, VGA_BUFFER
    mov esi, msg1
    call print_string
    
    ; Print second line
    mov edi, VGA_BUFFER + 160
    mov esi, msg2
    call print_string
    
    ; Print shell prompt
    mov edi, VGA_BUFFER + 320
    mov esi, prompt
    call print_string
    
    ; Infinite loop (no interrupts)
.hang:
    hlt
    jmp .hang

print_string:
    push eax
    push esi
.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0F
    stosw
    jmp .loop
.done:
    pop esi
    pop eax
    ret

section .data
msg1:   db 'AtomicOS v5.2.1 - Test Mode', 0
msg2:   db 'Shell ready (no keyboard yet)', 0
prompt: db 'atomicos> ', 0

; Pad to fill sector
times 512*64-($-$$) db 0