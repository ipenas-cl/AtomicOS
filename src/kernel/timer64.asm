; AtomicOS Timer Driver 64-bit
; Copyright (c) 2025 Ignacio Peña Sepúlveda
;
; Deterministic timer for real-time scheduling in 64-bit mode

[BITS 64]

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

; Timer statistics
timer_ticks:    dq 0            ; Total ticks since boot
timer_seconds:  dq 0            ; Seconds since boot
timer_overruns: dq 0            ; Missed deadlines

; Scheduled tasks table
MAX_SCHEDULED_TASKS equ 64
scheduled_tasks:
    times MAX_SCHEDULED_TASKS * 32 db 0  ; 32 bytes per task entry (64-bit)

section .text

global timer_init
global timer_handler
global get_system_ticks
global get_system_time
global schedule_task
global sleep_ticks

; Initialize timer for deterministic scheduling
timer_init:
    push rax
    push rdx
    
    ; Initialize PIT for 1000Hz
    mov al, 0x36        ; Channel 0, LSB/MSB, Mode 3
    out PIT_COMMAND, al
    
    ; Set divisor
    mov ax, TIMER_DIVISOR
    out PIT_CHANNEL0, al
    mov al, ah
    out PIT_CHANNEL0, al
    
    pop rdx
    pop rax
    ret

; Timer interrupt handler - DETERMINISTIC!
timer_handler:
    push rax
    push rbx
    push rcx
    push rdx
    
    ; Increment tick counter
    inc qword [timer_ticks]
    
    ; Update seconds
    mov rax, [timer_ticks]
    xor rdx, rdx
    mov rbx, TIMER_HZ
    div rbx
    mov [timer_seconds], rax
    
    ; Check scheduled tasks
    call check_scheduled_tasks
    
    ; Call scheduler tick if available
    ; extern scheduler_tick
    ; call scheduler_tick
    
    ; Send EOI to PIC
    mov al, 0x20
    out 0x20, al
    
    pop rdx
    pop rcx
    pop rbx
    pop rax
    iretq

; Check and execute scheduled tasks
check_scheduled_tasks:
    push rax
    push rbx
    push rcx
    push rsi
    
    mov rsi, scheduled_tasks
    xor rcx, rcx
    
.check_loop:
    cmp rcx, MAX_SCHEDULED_TASKS
    jae .done
    
    ; Check if slot is used
    mov rax, [rsi]      ; Task function pointer
    test rax, rax
    jz .next_task
    
    ; Check if it's time to run
    mov rbx, [rsi + 8]  ; Next run time
    cmp rbx, [timer_ticks]
    ja .next_task
    
    ; Time to run this task
    push rcx
    push rsi
    call rax            ; Call task function
    pop rsi
    pop rcx
    
    ; Update next run time
    mov rbx, [rsi + 16]  ; Period
    add rbx, [timer_ticks]
    mov [rsi + 8], rbx
    
.next_task:
    add rsi, 32         ; Next task entry
    inc rcx
    jmp .check_loop
    
.done:
    pop rsi
    pop rcx
    pop rbx
    pop rax
    ret

; Get current system ticks
; Returns: RAX = ticks since boot
get_system_ticks:
    mov rax, [timer_ticks]
    ret

; Get current system time in seconds
; Returns: RAX = seconds since boot
get_system_time:
    mov rax, [timer_seconds]
    ret

; Schedule a periodic task
; Input: RDI = task function, RSI = period in ticks
; Returns: RAX = task ID or -1 if full
schedule_task:
    push rbx
    push rcx
    push rdx
    
    ; Find free slot
    mov rbx, scheduled_tasks
    xor rcx, rcx
    
.find_slot:
    cmp rcx, MAX_SCHEDULED_TASKS
    jae .no_space
    
    mov rax, [rbx]
    test rax, rax
    jz .found_slot
    
    add rbx, 32
    inc rcx
    jmp .find_slot
    
.found_slot:
    ; Store task info
    mov [rbx], rdi          ; Function pointer
    mov rax, [timer_ticks]
    add rax, rsi
    mov [rbx + 8], rax      ; Next run time
    mov [rbx + 16], rsi     ; Period
    
    mov rax, rcx            ; Return task ID
    jmp .done
    
.no_space:
    mov rax, -1
    
.done:
    pop rdx
    pop rcx
    pop rbx
    ret

; Sleep for specified ticks
; Input: RDI = ticks to sleep
sleep_ticks:
    push rax
    push rbx
    
    mov rax, [timer_ticks]
    add rax, rdi
    mov rbx, rax
    
.sleep_loop:
    pause               ; CPU hint
    mov rax, [timer_ticks]
    cmp rax, rbx
    jb .sleep_loop
    
    pop rbx
    pop rax
    ret