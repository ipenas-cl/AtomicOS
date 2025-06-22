; Copyright (c) 2024 Ignacio Peña
; Part of AtomicOS Project - https://github.com/ipenas-cl/AtomicOS
; Licensed under MIT License - see LICENSE file for details

; Working bootloader for AtomicOS v1.0.0
[BITS 16]
[ORG 0x7C00]

start:
    ; Setup segments
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    mov [boot_drive], dl
    sti

    ; Set video mode
    mov ax, 0x0003
    int 0x10

    ; Print message
    mov si, boot_msg
    call print

    ; Load kernel
    mov ah, 0x02       ; Read sectors
    mov al, 64         ; 64 sectors
    mov ch, 0          ; Cylinder 0
    mov cl, 2          ; Start sector 2
    mov dh, 0          ; Head 0
    mov dl, [boot_drive]
    mov bx, 0x1000     ; Load at 0x1000:0000
    mov es, bx
    xor bx, bx
    int 0x13
    
    jc error           ; If error, jump
    
    ; Enter protected mode
    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pmode

error:
    mov si, err_msg
    call print
hang:
    hlt
    jmp hang

print:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print
.done:
    ret

[BITS 32]
pmode:
    ; Setup segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    
    ; Jump to kernel
    jmp 0x10000

; Data
boot_drive: db 0
boot_msg:   db 'AtomicOS v1.0.0', 13, 10, 0
err_msg:    db 'Error!', 13, 10, 0

; GDT
align 8
gdt:
    ; Null
    dq 0
    
    ; Code segment
    dw 0xFFFF          ; Limit
    dw 0               ; Base low
    db 0               ; Base mid
    db 10011010b       ; Access
    db 11001111b       ; Flags + limit high
    db 0               ; Base high
    
    ; Data segment  
    dw 0xFFFF          ; Limit
    dw 0               ; Base low
    db 0               ; Base mid
    db 10010010b       ; Access
    db 11001111b       ; Flags + limit high
    db 0               ; Base high
gdt_end:

gdt_desc:
    dw gdt_end - gdt - 1
    dd gdt

; Padding
times 510-($-$$) db 0
dw 0xAA55