; AtomicOS Context Switch
; Deterministic task switching with bounded execution time

[BITS 32]

section .text

global switch_context
global enter_usermode

; Context offsets in cpu_context_t structure
CTX_EAX     equ 0
CTX_EBX     equ 4
CTX_ECX     equ 8
CTX_EDX     equ 12
CTX_ESI     equ 16
CTX_EDI     equ 20
CTX_EBP     equ 24
CTX_ESP     equ 28
CTX_DS      equ 32
CTX_ES      equ 36
CTX_FS      equ 40
CTX_GS      equ 44
CTX_EIP     equ 48
CTX_CS      equ 52
CTX_EFLAGS  equ 56
CTX_CR3     equ 60

; switch_context(cpu_context_t* old, cpu_context_t* new)
; Saves current context and loads new context
; Deterministic execution - fixed number of cycles
switch_context:
    ; Get parameters
    mov eax, [esp + 4]      ; old context pointer
    mov edx, [esp + 8]      ; new context pointer
    
    ; Save current context
    push ebp
    mov ebp, esp
    
    ; Save general purpose registers
    mov [eax + CTX_EBX], ebx
    mov [eax + CTX_ECX], ecx
    mov [eax + CTX_EDX], edx
    mov [eax + CTX_ESI], esi
    mov [eax + CTX_EDI], edi
    mov [eax + CTX_EBP], ebp
    
    ; Save stack pointer (adjusted for this function)
    lea ecx, [esp + 8]      ; Original ESP before call
    mov [eax + CTX_ESP], ecx
    
    ; Save segment registers
    mov cx, ds
    mov [eax + CTX_DS], ecx
    mov cx, es
    mov [eax + CTX_ES], ecx
    mov cx, fs
    mov [eax + CTX_FS], ecx
    mov cx, gs
    mov [eax + CTX_GS], ecx
    
    ; Save return address as EIP
    mov ecx, [esp + 4]      ; Return address
    mov [eax + CTX_EIP], ecx
    
    ; Save EFLAGS
    pushfd
    pop ecx
    mov [eax + CTX_EFLAGS], ecx
    
    ; Save CS (should be same)
    mov cx, cs
    mov [eax + CTX_CS], ecx
    
    ; Now load new context
    ; First check if we need to change CR3 (page directory)
    ; This will be used when paging is enabled
    mov ecx, [edx + CTX_CR3]
    test ecx, ecx
    jz .no_cr3_change
    
    ; Load new page directory
    mov cr3, ecx
    
.no_cr3_change:
    ; Load segment registers
    mov cx, [edx + CTX_DS]
    mov ds, cx
    mov cx, [edx + CTX_ES]
    mov es, cx
    mov cx, [edx + CTX_FS]
    mov fs, cx
    mov cx, [edx + CTX_GS]
    mov gs, cx
    
    ; Load general purpose registers
    mov ebx, [edx + CTX_EBX]
    mov ecx, [edx + CTX_ECX]
    mov esi, [edx + CTX_ESI]
    mov edi, [edx + CTX_EDI]
    mov ebp, [edx + CTX_EBP]
    
    ; Load EFLAGS
    mov eax, [edx + CTX_EFLAGS]
    push eax
    popfd
    
    ; Load new stack
    mov esp, [edx + CTX_ESP]
    
    ; Push return address
    mov eax, [edx + CTX_EIP]
    push eax
    
    ; Load remaining registers
    mov eax, [edx + CTX_EAX]
    mov edx, [edx + CTX_EDX]
    
    ; Return to new context
    ret

; enter_usermode(void* entry_point, void* user_stack)
; Switch to user mode (ring 3)
; This will be used when we implement user/kernel separation
enter_usermode:
    ; For now, just jump to entry point
    ; Full implementation requires GDT user segments
    mov eax, [esp + 4]      ; entry_point
    mov esp, [esp + 8]      ; user_stack
    
    ; Clear registers for security
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx
    xor esi, esi
    xor edi, edi
    xor ebp, ebp
    
    ; Jump to user code
    jmp eax

; idle_task_asm - Assembly idle loop
global idle_task_asm
idle_task_asm:
    sti                     ; Enable interrupts
.loop:
    hlt                     ; Halt until interrupt
    jmp .loop               ; Continue halting

; get_current_context - Get pointer to current context save area
global get_current_context
get_current_context:
    ; This will be implemented when we have proper process table
    xor eax, eax
    ret

; Deterministic context validation
; Ensures context is valid before switch
global validate_context
validate_context:
    mov eax, [esp + 4]      ; context pointer
    
    ; Check if pointer is valid (not null)
    test eax, eax
    jz .invalid
    
    ; Check if pointer is aligned
    test eax, 3
    jnz .invalid
    
    ; Check segment selectors are valid
    mov cx, [eax + CTX_CS]
    test cx, cx
    jz .invalid
    
    mov cx, [eax + CTX_DS]
    test cx, cx
    jz .invalid
    
    ; Context is valid
    mov eax, 1
    ret
    
.invalid:
    xor eax, eax
    ret