; Copyright (c) 2024 Ignacio Peña
; Part of AtomicOS Project - https://github.com/ipenas-cl/AtomicOS
; Licensed under MIT License - see LICENSE file for details

; AtomicOS Fixed Bootloader - Forces proper VGA text mode
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
    mov [drive], dl
    sti

    ; Force VGA text mode 80x25
    mov ax, 0x0003
    int 0x10
    
    ; Verify we're in text mode
    mov ah, 0x0F
    int 0x10
    cmp al, 0x03
    jne .mode_error
    
    ; Clear screen with known good method
    mov ax, 0x0600
    mov bh, 0x07
    mov cx, 0x0000
    mov dx, 0x184F
    int 0x10
    
    ; Set cursor to 0,0
    mov ah, 0x02
    mov bh, 0x00
    mov dx, 0x0000
    int 0x10
    
    ; Test VGA by writing directly
    mov ax, 0xB800
    mov es, ax
    mov di, 0
    mov ax, 0x0F41  ; 'A' with bright white
    mov [es:di], ax
    mov ax, 0x0F54  ; 'T' 
    mov [es:di+2], ax
    mov ax, 0x0F4F  ; 'O'
    mov [es:di+4], ax
    mov ax, 0x0F4D  ; 'M'
    mov [es:di+6], ax
    
    ; Reset ES
    xor ax, ax
    mov es, ax
    
    ; Load kernel
    call load_kernel
    
    ; Switch to protected mode
    cli
    lgdt [gdtr]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:pmode

.mode_error:
    ; If we can't set text mode, halt
    mov si, error_msg
    call print_string
    cli
    hlt
    jmp $

load_kernel:
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    
    mov ah, 0x02
    mov al, 64          ; Read more sectors
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [drive]
    int 0x13
    
    jc .load_error
    ret

.load_error:
    mov si, load_error_msg
    call print_string
    cli
    hlt
    jmp $

print_string:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp print_string
.done:
    ret

[BITS 32]
pmode:
    ; Setup segments for protected mode
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000
    
    ; Test VGA in protected mode
    mov edi, 0xB8000
    mov eax, 0x0F420F41  ; "AB" with attributes
    mov [edi+8], eax     ; Write after "ATOM"
    
    ; Jump to kernel
    jmp 0x10000

; Data
drive           db 0
error_msg       db 'VGA mode error!', 13, 10, 0
load_error_msg  db 'Disk read error!', 13, 10, 0

; GDT
gdt:
    dq 0                                ; Null descriptor
    dq 0x00CF9A000000FFFF              ; Code segment
    dq 0x00CF92000000FFFF              ; Data segment

gdtr:
    dw $ - gdt - 1
    dd gdt

times 510-($-$$) db 0
dw 0xAA55