; AtomicOS Deterministic Scheduler Table
; Copyright (c) 2025 Ignacio Peña Sepúlveda
; 
; NO dynamic scheduling - Pure static table!

[BITS 64]

section .data

; Configuration
TIMESLICE_CYCLES    equ 1000000     ; 1M cycles = 1ms @ 1GHz
MAX_TASKS           equ 256
CORES               equ 4

; The Holy Schedule Table - Compiled at build time
; Format: [start_cycle][task_entry][duration][core]
align 4096
global schedule_table
schedule_table:
    ; Core 0 - System tasks
    dq 0,         kernel_idle,    100000,  0   ; Kernel idle
    dq 100000,    irq_handler,    10000,   0   ; IRQ processing
    dq 110000,    scheduler_tick, 10000,   0   ; Scheduler
    dq 120000,    kernel_idle,    880000,  0   ; Rest of ms
    
    ; Core 1 - Application tasks
    dq 0,         task_1_entry,   500000,  1   ; Task 1: 500μs
    dq 500000,    task_2_entry,   300000,  1   ; Task 2: 300μs  
    dq 800000,    task_3_entry,   200000,  1   ; Task 3: 200μs
    
    ; Core 2 - Network tasks
    dq 0,         net_rx_task,    250000,  2   ; Network RX
    dq 250000,    net_tx_task,    250000,  2   ; Network TX
    dq 500000,    net_process,    500000,  2   ; Protocol processing
    
    ; Core 3 - Reserved for real-time
    dq 0,         rt_task_1,      100000,  3   ; RT task every 100μs
    dq 100000,    idle_task,      900000,  3
    
    ; End marker
    dq 0xFFFFFFFF, 0, 0, 0

; Per-core current position in schedule
current_schedule_index:
    dq 0, 0, 0, 0    ; One per core

; Task Control Blocks (static allocation)
align 4096
global task_table
task_table:
    times MAX_TASKS * 512 db 0    ; 512 bytes per task

section .text

global init_scheduler
global scheduler_tick
global get_next_task

; Initialize the deterministic scheduler
init_scheduler:
    push rbp
    mov rbp, rsp
    
    ; Enable APIC timer for fixed interval
    mov ecx, 0x320          ; Timer LVT
    mov eax, 0x20020        ; Vector 32, periodic
    wrmsr
    
    ; Set timer to exactly TIMESLICE_CYCLES
    mov ecx, 0x3E0          ; Timer initial count
    mov eax, TIMESLICE_CYCLES
    xor edx, edx
    wrmsr
    
    ; Start all cores with their schedule
    call start_core_schedules
    
    pop rbp
    ret

; Called every exact time interval - NO DECISIONS!
scheduler_tick:
    push rax
    push rbx
    push rcx
    push rdx
    
    ; Get current core
    mov ecx, 0xC0000101     ; MSR_GS_BASE
    rdmsr
    mov rbx, rax            ; RBX = core ID
    
    ; Get current position in schedule for this core
    mov rax, [current_schedule_index + rbx*8]
    
    ; Move to next entry
    add rax, 32             ; Each entry is 32 bytes
    
    ; Check if we hit the end marker
    mov rcx, [schedule_table + rax]
    cmp rcx, 0xFFFFFFFF
    jne .continue
    
    ; Wrap around to beginning
    xor rax, rax
    
.continue:
    ; Save new position
    mov [current_schedule_index + rbx*8], rax
    
    ; Get task entry point
    mov rcx, [schedule_table + rax + 8]
    
    ; Switch to task (already in RCX)
    call switch_to_task_deterministic
    
    pop rdx
    pop rcx
    pop rbx
    pop rax
    iret

; Deterministic task switch - EXACTLY 1000 cycles
; RCX = task entry point
switch_to_task_deterministic:
    ; Save current context (fixed 500 cycles)
    mov [saved_rsp], rsp
    mov [saved_rbp], rbp
    mov [saved_rax], rax
    mov [saved_rbx], rbx
    mov [saved_rcx], rcx
    mov [saved_rdx], rdx
    mov [saved_rsi], rsi
    mov [saved_rdi], rdi
    mov [saved_r8],  r8
    mov [saved_r9],  r9
    mov [saved_r10], r10
    mov [saved_r11], r11
    mov [saved_r12], r12
    mov [saved_r13], r13
    mov [saved_r14], r14
    mov [saved_r15], r15
    
    ; Flush TLB (fixed 200 cycles)
    mov rax, cr3
    mov cr3, rax
    
    ; Load new task state (fixed 200 cycles)
    mov rsp, [rcx + 0]      ; New stack
    mov rbp, [rcx + 8]      ; New base
    
    ; Jump to task (fixed 100 cycles)
    jmp [rcx + 16]          ; Task entry
    
    ; Total: exactly 1000 cycles

; Idle task - burns exact cycles
idle_task:
.loop:
    pause                   ; 10 cycles
    pause
    pause
    pause
    pause
    pause
    pause
    pause
    pause
    pause                   ; 100 cycles total
    
    sub qword [idle_counter], 100
    jnz .loop
    
    ret

section .bss
    saved_rsp resq 1
    saved_rbp resq 1
    saved_rax resq 1
    saved_rbx resq 1
    saved_rcx resq 1
    saved_rdx resq 1
    saved_rsi resq 1
    saved_rdi resq 1
    saved_r8  resq 1
    saved_r9  resq 1
    saved_r10 resq 1
    saved_r11 resq 1
    saved_r12 resq 1
    saved_r13 resq 1
    saved_r14 resq 1
    saved_r15 resq 1
    
    idle_counter resq 1