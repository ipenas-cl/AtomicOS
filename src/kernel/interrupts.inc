; AtomicOS Interrupt System - Include file
; Deterministic interrupt handling integrated with kernel

; IDT structure
align 4096
idt_start:
    times 256 dq 0

idt_ptr:
    dw 256 * 8 - 1
    dd idt_start

; Interrupt statistics
interrupt_count:     times 256 dd 0
interrupt_cycles:    times 256 dd 0
interrupt_max_cycles: times 256 dd 0
nested_interrupt_level: dd 0
interrupt_overruns: dd 0

; Initialize IDT
init_idt:
    push eax
    push ebx
    push ecx
    push edi
    
    ; Set up exception handlers (0-31)
    mov ecx, 0
.setup_exceptions:
    mov eax, ecx
    mov ebx, exception_handler
    call set_idt_entry
    inc ecx
    cmp ecx, 32
    jl .setup_exceptions
    
    ; Set up IRQ handlers (32-47)
    mov ecx, 32
.setup_irqs:
    mov eax, ecx
    mov ebx, irq_handler
    call set_idt_entry
    inc ecx
    cmp ecx, 48
    jl .setup_irqs
    
    ; Set up syscall handler (INT 0x80)
    mov eax, 0x80
    mov ebx, syscall_handler
    call set_idt_entry_user  ; Special entry with DPL=3
    
    ; Remap PIC
    call remap_pic
    
    pop edi
    pop ecx
    pop ebx
    pop eax
    ret

; Set IDT entry
; Input: EAX = vector, EBX = handler address
set_idt_entry:
    push eax
    push edx
    push edi
    
    ; Calculate entry address
    mov edi, eax
    shl edi, 3
    add edi, idt_start
    
    ; Set entry
    mov word [edi], bx          ; Handler low
    mov word [edi + 2], 0x08    ; Code segment
    mov byte [edi + 4], 0       ; Reserved
    mov byte [edi + 5], 0x8E    ; Present, DPL=0, Interrupt gate
    shr ebx, 16
    mov word [edi + 6], bx      ; Handler high
    
    pop edi
    pop edx
    pop eax
    ret

; Set IDT entry with user access (DPL=3)
; Input: EAX = vector, EBX = handler address
set_idt_entry_user:
    push eax
    push edx
    push edi
    
    ; Calculate entry address
    mov edi, eax
    shl edi, 3
    add edi, idt_start
    
    ; Set entry
    mov word [edi], bx          ; Handler low
    mov word [edi + 2], 0x08    ; Code segment
    mov byte [edi + 4], 0       ; Reserved
    mov byte [edi + 5], 0xEE    ; Present, DPL=3, Interrupt gate
    shr ebx, 16
    mov word [edi + 6], bx      ; Handler high
    
    pop edi
    pop edx
    pop eax
    ret

; Remap PIC
remap_pic:
    push eax
    
    ; ICW1
    mov al, 0x11
    out 0x20, al
    out 0xA0, al
    
    ; ICW2 - Remap
    mov al, 0x20    ; IRQ0-7 -> INT 32-39
    out 0x21, al
    mov al, 0x28    ; IRQ8-15 -> INT 40-47
    out 0xA1, al
    
    ; ICW3
    mov al, 0x04
    out 0x21, al
    mov al, 0x02
    out 0xA1, al
    
    ; ICW4
    mov al, 0x01
    out 0x21, al
    out 0xA1, al
    
    ; Mask all interrupts except timer and keyboard
    mov al, 0xFC    ; Enable IRQ0 (timer) and IRQ1 (keyboard)
    out 0x21, al
    mov al, 0xFF
    out 0xA1, al
    
    pop eax
    ret

; Common exception handler
exception_handler:
    pushad
    
    ; Show exception indicator on screen
    mov edi, VGA_BUFFER
    mov esi, exception_msg
    mov ecx, 19  ; Length of message
.print_exc:
    lodsb
    mov [edi], al
    mov byte [edi + 1], 0x04  ; Red on black
    add edi, 2
    loop .print_exc
    
    ; Halt system
    cli
    hlt
    jmp $

; Common IRQ handler
irq_handler:
    pushad
    push ds
    push es
    
    ; Set kernel segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    
    ; Get interrupt number (stored at ESP+44)
    mov eax, [esp + 44]
    
    ; Handle timer (IRQ0 = INT 32)
    cmp eax, 32
    je .timer_handler
    
    ; Handle keyboard (IRQ1 = INT 33)
    cmp eax, 33
    je .keyboard_handler
    
    ; Other IRQs - just acknowledge
    jmp .eoi
    
.timer_handler:
    ; Increment tick counter
    inc dword [system_ticks]
    
    ; Check for scheduling (every 10 ticks for now)
    mov eax, [system_ticks]
    and eax, 0x0F
    jnz .eoi
    
    ; Show tick indicator
    mov edi, VGA_BUFFER + (24 * 80 * 2) + (79 * 2)
    mov al, [tick_char]
    mov [edi], al
    mov byte [edi + 1], 0x0A
    
    ; Rotate tick character
    cmp byte [tick_char], '|'
    je .set_slash
    cmp byte [tick_char], '/'
    je .set_minus
    cmp byte [tick_char], '-'
    je .set_backslash
    mov byte [tick_char], '|'
    jmp .eoi
.set_slash:
    mov byte [tick_char], '/'
    jmp .eoi
.set_minus:
    mov byte [tick_char], '-'
    jmp .eoi
.set_backslash:
    mov byte [tick_char], '\'
    
.eoi:
    ; Send EOI to PIC
    mov al, 0x20
    out 0x20, al
    
    pop es
    pop ds
    popad
    iret
    
.keyboard_handler:
    ; Read scancode
    in al, 0x60
    
    ; Check if key release (bit 7 set)
    test al, 0x80
    jnz .eoi
    
    ; Simple scancode processing
    ; (In real implementation, would add to keyboard buffer)
    
    jmp .eoi

; Load IDT
load_idt:
    lidt [idt_ptr]
    ret

; Enable interrupts
enable_interrupts:
    sti
    ret

; Data
global system_ticks
system_ticks: dd 0
tick_char: db '|'
exception_msg: db 'Exception occurred!', 0


; Helper function to update interrupt statistics
global update_interrupt_cycles
update_interrupt_cycles:
    ; Simple stub - in real implementation would update stats
    ret

; Timer initialization
init_timer:
    push eax
    push edx
    
    ; Set timer frequency (1000 Hz)
    mov al, 0x36
    out 0x43, al
    
    ; Divisor = 1193180 / 1000 = 1193
    mov ax, 1193
    out 0x40, al
    mov al, ah
    out 0x40, al
    
    pop edx
    pop eax
    ret