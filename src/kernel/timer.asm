; AtomicOS Timer Driver
; Copyright (c) 2025 Ignacio Peña Sepúlveda
;
; Deterministic timer for real-time scheduling

[BITS 32]

section .data

; Timer constants
PIT_FREQ        equ 1193182     ; PIT frequency in Hz
TIMER_HZ        equ 1000        ; Our timer frequency (1000Hz = 1ms)
TIMER_DIVISOR   equ PIT_FREQ / TIMER_HZ

; Timer ports
PIT_CHANNEL0    equ 0x40
PIT_CHANNEL1    equ 0x41
PIT_CHANNEL2    equ 0x42
PIT_COMMAND     equ 0x43

; APIC Timer registers (simplified for 32-bit)
APIC_BASE       equ 0xFEE00000
APIC_TIMER_LVT  equ 0x320
APIC_TIMER_DIV  equ 0x3E0
APIC_TIMER_INIT equ 0x380
APIC_TIMER_CUR  equ 0x390

; Timer statistics
timer_ticks:    dd 0            ; Total ticks since boot
timer_seconds:  dd 0            ; Seconds since boot
timer_overruns: dd 0            ; Missed deadlines

; Scheduled tasks table
MAX_SCHEDULED_TASKS equ 64
scheduled_tasks:
    times MAX_SCHEDULED_TASKS * 16 db 0  ; 16 bytes per task entry

section .text

global timer_init
global timer_handler
global get_system_ticks
global get_system_time
global schedule_task
global sleep_ticks

; Initialize timer for deterministic scheduling
timer_init:
    push eax
    push edx
    
    ; Initialize PIT for 1000Hz
    mov al, 0x36        ; Channel 0, LSB/MSB, Mode 3
    out PIT_COMMAND, al
    
    ; Set divisor
    mov ax, TIMER_DIVISOR
    out PIT_CHANNEL0, al
    mov al, ah
    out PIT_CHANNEL0, al
    
    ; For now, skip APIC timer (needs more setup in 32-bit mode)
    
    pop edx
    pop eax
    ret

; Timer interrupt handler - DETERMINISTIC!
timer_handler:
    push eax
    push ebx
    push ecx
    push edx
    
    ; Increment tick counter
    inc dword [timer_ticks]
    
    ; Update seconds
    mov eax, [timer_ticks]
    xor edx, edx
    mov ebx, TIMER_HZ
    div ebx
    mov [timer_seconds], eax
    
    ; Check scheduled tasks
    call check_scheduled_tasks
    
    ; Call scheduler tick if available
    ; extern scheduler_tick
    ; call scheduler_tick
    
    ; Send EOI to PIC
    mov al, 0x20
    out 0x20, al
    
    pop edx
    pop ecx
    pop ebx
    pop eax
    iretd

; Check and execute scheduled tasks
check_scheduled_tasks:
    push eax
    push ebx
    push ecx
    push esi
    
    mov esi, scheduled_tasks
    xor ecx, ecx
    
.check_loop:
    cmp ecx, MAX_SCHEDULED_TASKS
    jae .done
    
    ; Check if slot is used
    mov eax, [esi]      ; Task function pointer
    test eax, eax
    jz .next_task
    
    ; Check if it's time to run
    mov ebx, [esi + 4]  ; Next run time
    cmp ebx, [timer_ticks]
    ja .next_task
    
    ; Time to run this task
    push ecx
    push esi
    call eax            ; Call task function
    pop esi
    pop ecx
    
    ; Update next run time
    mov ebx, [esi + 8]  ; Period
    add ebx, [timer_ticks]
    mov [esi + 4], ebx
    
.next_task:
    add esi, 16         ; Next task entry
    inc ecx
    jmp .check_loop
    
.done:
    pop esi
    pop ecx
    pop ebx
    pop eax
    ret

; Get current system ticks
; Returns: EAX = ticks since boot
get_system_ticks:
    mov eax, [timer_ticks]
    ret

; Get current system time in seconds
; Returns: EAX = seconds since boot
get_system_time:
    mov eax, [timer_seconds]
    ret

; Schedule a periodic task
; Input: EDI = task function, ESI = period in ticks
; Returns: EAX = task ID or -1 if full
schedule_task:
    push ebx
    push ecx
    push edx
    
    ; Find free slot
    mov ebx, scheduled_tasks
    xor ecx, ecx
    
.find_slot:
    cmp ecx, MAX_SCHEDULED_TASKS
    jae .no_space
    
    mov eax, [ebx]
    test eax, eax
    jz .found_slot
    
    add ebx, 16
    inc ecx
    jmp .find_slot
    
.found_slot:
    ; Store task info
    mov [ebx], edi          ; Function pointer
    mov eax, [timer_ticks]
    add eax, esi
    mov [ebx + 4], eax      ; Next run time
    mov [ebx + 8], esi      ; Period
    
    mov eax, ecx            ; Return task ID
    jmp .done
    
.no_space:
    mov eax, -1
    
.done:
    pop edx
    pop ecx
    pop ebx
    ret

; Sleep for specified ticks
; Input: EDI = ticks to sleep
sleep_ticks:
    push eax
    push ebx
    
    mov eax, [timer_ticks]
    add eax, edi
    mov ebx, eax
    
.sleep_loop:
    pause               ; CPU hint
    mov eax, [timer_ticks]
    cmp eax, ebx
    jb .sleep_loop
    
    pop ebx
    pop eax
    ret