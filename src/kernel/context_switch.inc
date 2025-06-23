; AtomicOS Context Switch (inline version)
; Deterministic task switching

; Simple context switch for demo
; In real implementation, would save/restore all registers
global switch_context_simple
switch_context_simple:
    ; For now, just return
    ret

; Print utilities for process management
global print_hex_value
print_hex_value:
    push ebp
    mov ebp, esp
    push eax
    push ebx
    push ecx
    push edx
    push edi
    
    mov eax, [ebp + 8]  ; Value to print
    mov edi, VGA_BUFFER + (24 * 80 * 2) + (70 * 2)  ; Bottom right
    
    ; Print "0x"
    mov byte [edi], '0'
    mov byte [edi + 1], 0x07
    add edi, 2
    mov byte [edi], 'x'
    mov byte [edi + 1], 0x07
    add edi, 2
    
    ; Convert to hex
    mov ecx, 8  ; 8 hex digits
.loop:
    rol eax, 4
    mov ebx, eax
    and ebx, 0x0F
    
    cmp bl, 10
    jl .digit
    add bl, 'A' - 10
    jmp .store
.digit:
    add bl, '0'
.store:
    mov [edi], bl
    mov byte [edi + 1], 0x07
    add edi, 2
    
    loop .loop
    
    pop edi
    pop edx
    pop ecx
    pop ebx
    pop eax
    pop ebp
    ret