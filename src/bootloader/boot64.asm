; AtomicOS 64-bit Bootloader
; Copyright (c) 2025 Ignacio Peña Sepúlveda
;
; Boot process: Real mode -> Protected mode -> Long mode (64-bit)

[BITS 16]
[ORG 0x7C00]

; Boot sector entry
start:
    ; Setup segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    
    ; Clear screen
    mov ax, 0x0003
    int 0x10
    
    ; Print boot message
    mov si, boot_msg
    call print_string_16
    
    ; Enable A20 line
    call enable_a20
    
    ; Load kernel from disk
    mov ah, 0x02        ; Read sectors
    mov al, 64          ; 64 sectors
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Sector 2
    mov dh, 0           ; Head 0
    mov bx, 0x1000      ; Load at 0x10000
    mov es, bx
    xor bx, bx
    int 0x13
    jc disk_error
    
    ; Check for long mode support
    call check_long_mode
    jc no_long_mode
    
    ; Setup page tables for long mode
    call setup_page_tables
    
    ; Enter protected mode first
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SEG:protected_mode

; Print string in real mode
print_string_16:
    push ax
    push si
.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp .loop
.done:
    pop si
    pop ax
    ret

; Enable A20 line
enable_a20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

; Check for long mode support
check_long_mode:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 0x200000
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode
    
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29    ; Check LM bit
    jz .no_long_mode
    
    clc
    ret
.no_cpuid:
.no_long_mode:
    stc
    ret

; Setup page tables for long mode
setup_page_tables:
    ; Clear page tables
    mov edi, 0x1000
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    rep stosd
    
    ; Setup page tables (identity map first 2MB)
    mov edi, 0x1000
    mov dword [edi], 0x2003     ; PML4[0] -> PDPT
    add edi, 0x1000
    mov dword [edi], 0x3003     ; PDPT[0] -> PD  
    add edi, 0x1000
    mov dword [edi], 0x83       ; PD[0] -> 0-2MB (2MB page)
    ret

disk_error:
    mov si, disk_err_msg
    call print_string_16
    jmp $

no_long_mode:
    mov si, no_lm_msg
    call print_string_16
    jmp $

[BITS 32]
protected_mode:
    ; Setup segments
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    
    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    
    ; Enable long mode in EFER
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    
    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    
    ; Jump to long mode
    jmp CODE_SEG_64:long_mode

[BITS 64]
long_mode:
    ; Setup 64-bit segments
    mov ax, DATA_SEG_64
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, 0x90000
    
    ; Jump to kernel
    jmp 0x10000

; Data
boot_msg:       db 'AtomicOS v5.2.1 64-bit', 13, 10, 0
disk_err_msg:   db 'Disk error!', 13, 10, 0
no_lm_msg:      db 'CPU does not support 64-bit mode!', 13, 10, 0

; GDT
gdt_start:
    dq 0                    ; Null descriptor
gdt_code_32:
    dw 0xFFFF               ; Limit
    dw 0                    ; Base (low)
    db 0                    ; Base (mid)
    db 10011010b            ; Access
    db 11001111b            ; Flags + limit
    db 0                    ; Base (high)
gdt_data_32:
    dw 0xFFFF
    dw 0
    db 0
    db 10010010b
    db 11001111b
    db 0
gdt_code_64:
    dw 0                    ; Limit ignored
    dw 0                    ; Base (low)
    db 0                    ; Base (mid)
    db 10011010b            ; Access
    db 10101111b            ; Flags (64-bit)
    db 0                    ; Base (high)
gdt_data_64:
    dw 0
    dw 0
    db 0
    db 10010010b
    db 00000000b
    db 0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code_32 - gdt_start
DATA_SEG equ gdt_data_32 - gdt_start
CODE_SEG_64 equ gdt_code_64 - gdt_start
DATA_SEG_64 equ gdt_data_64 - gdt_start

; Boot signature
times 510-($-$$) db 0
dw 0xAA55