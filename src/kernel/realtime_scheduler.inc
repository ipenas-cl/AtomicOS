; AtomicOS Real-Time Scheduler
; Implements EDF (Earliest Deadline First) and RMS (Rate Monotonic Scheduling)
; with deterministic behavior and WCET guarantees

section .data

; Scheduler modes
SCHED_MODE_RR           equ 0   ; Round Robin (legacy)
SCHED_MODE_EDF          equ 1   ; Earliest Deadline First
SCHED_MODE_RMS          equ 2   ; Rate Monotonic Scheduling
SCHED_MODE_DMS          equ 3   ; Deadline Monotonic Scheduling

; Current scheduler mode
current_sched_mode:     dd SCHED_MODE_EDF

; Real-time task states
RT_TASK_DORMANT         equ 0
RT_TASK_READY           equ 1
RT_TASK_RUNNING         equ 2
RT_TASK_SUSPENDED       equ 3

; Real-time constants
MAX_RT_TASKS            equ 64
RT_TASK_STRUCT_SIZE     equ 128
RT_TASK_TABLE_BASE      equ 0x700000  ; After process table

; Timing constants (in microseconds)
TICK_PERIOD_US          equ 1000      ; 1ms tick
MAX_DEADLINE_US         equ 1000000   ; 1 second max deadline

; RMS priority mapping (shorter period = higher priority)
RMS_PRIORITY_CRITICAL   equ 250
RMS_PRIORITY_HIGH       equ 200
RMS_PRIORITY_MEDIUM     equ 150
RMS_PRIORITY_NORMAL     equ 100
RMS_PRIORITY_LOW        equ 50

; RT Task Structure Offsets
RT_TASK_ID              equ 0    ; 4 bytes
RT_TASK_STATE           equ 4    ; 4 bytes
RT_TASK_PRIORITY        equ 8    ; 4 bytes
RT_TASK_PERIOD_US       equ 12   ; 4 bytes
RT_TASK_WCET_US         equ 16   ; 4 bytes
RT_TASK_DEADLINE_US     equ 20   ; 4 bytes
RT_TASK_NEXT_RELEASE    equ 24   ; 4 bytes
RT_TASK_EXEC_TIME       equ 28   ; 4 bytes
RT_TASK_ENTRY_POINT     equ 32   ; 4 bytes
RT_TASK_STACK_PTR       equ 36   ; 4 bytes
RT_TASK_CONTEXT         equ 40   ; Context save area (88 bytes)

; Statistics
rt_task_count:          dd 0
rt_deadline_misses:     dd 0
rt_context_switches:    dd 0
rt_total_utilization:   dd 0      ; In percentage * 100

; Current running RT task
current_rt_task:        dd 0

; Messages
rt_init_msg:            db 'Real-time scheduler initialized (Mode: ', 0
rt_mode_edf:            db 'EDF)', 0
rt_mode_rms:            db 'RMS)', 0
rt_deadline_miss_msg:   db 'DEADLINE MISS: Task ', 0
rt_util_msg:            db 'Total utilization: ', 0
rt_percent_msg:         db '%', 0

section .text

; Initialize real-time scheduler
global init_rt_scheduler
init_rt_scheduler:
    push ebp
    mov ebp, esp
    push esi
    push edi
    
    ; Clear RT task table
    mov edi, RT_TASK_TABLE_BASE
    mov ecx, MAX_RT_TASKS * RT_TASK_STRUCT_SIZE / 4
    xor eax, eax
    rep stosd
    
    ; Initialize statistics
    mov dword [rt_task_count], 0
    mov dword [rt_deadline_misses], 0
    mov dword [rt_context_switches], 0
    mov dword [rt_total_utilization], 0
    mov dword [current_rt_task], 0
    
    ; Print initialization message
    push rt_init_msg
    call print_string
    add esp, 4
    
    ; Print scheduler mode
    mov eax, [current_sched_mode]
    cmp eax, SCHED_MODE_EDF
    jne .check_rms
    push rt_mode_edf
    jmp .print_mode
    
.check_rms:
    push rt_mode_rms
    
.print_mode:
    call print_string
    add esp, 4
    call print_newline
    
    pop edi
    pop esi
    pop ebp
    ret

; Create real-time task
; Input: EAX = period (microseconds), EBX = WCET (microseconds), 
;        ECX = entry point, EDX = deadline (0 = period)
; Output: EAX = task ID or 0 on failure
global rt_create_task
rt_create_task:
    push ebp
    mov ebp, esp
    push esi
    push edi
    push ebx
    
    ; Validate parameters
    cmp eax, 0
    jle .error
    cmp eax, MAX_DEADLINE_US
    jg .error
    
    cmp ebx, 0
    jle .error
    cmp ebx, eax              ; WCET must be <= period
    jg .error
    
    ; Find free task slot
    mov esi, RT_TASK_TABLE_BASE
    xor edi, edi              ; Task counter
    
.find_slot:
    cmp edi, MAX_RT_TASKS
    jae .error
    
    cmp dword [esi + RT_TASK_STATE], RT_TASK_DORMANT
    je .found_slot
    
    add esi, RT_TASK_STRUCT_SIZE
    inc edi
    jmp .find_slot
    
.found_slot:
    ; Initialize task structure
    inc edi                   ; Task ID starts at 1
    mov [esi + RT_TASK_ID], edi
    mov dword [esi + RT_TASK_STATE], RT_TASK_READY
    mov [esi + RT_TASK_PERIOD_US], eax
    mov [esi + RT_TASK_WCET_US], ebx
    
    ; Set deadline (use period if not specified)
    test edx, edx
    jnz .use_deadline
    mov edx, eax              ; deadline = period
.use_deadline:
    mov [esi + RT_TASK_DEADLINE_US], edx
    
    ; Set entry point
    mov [esi + RT_TASK_ENTRY_POINT], ecx
    
    ; Calculate priority based on scheduler mode
    cmp dword [current_sched_mode], SCHED_MODE_RMS
    jne .edf_priority
    
    ; RMS: priority based on period (shorter = higher)
    call calculate_rms_priority
    mov [esi + RT_TASK_PRIORITY], eax
    jmp .priority_done
    
.edf_priority:
    ; EDF: dynamic priority based on deadline
    mov dword [esi + RT_TASK_PRIORITY], 0  ; Not used in EDF
    
.priority_done:
    ; Calculate and update total utilization
    call update_utilization
    
    ; Initialize timing
    call get_system_time_us
    mov [esi + RT_TASK_NEXT_RELEASE], eax
    add eax, edx
    mov [esi + RT_TASK_DEADLINE_US], eax
    
    ; Allocate stack for task (simplified)
    mov eax, edi
    shl eax, 12               ; 4KB per task
    add eax, 0x800000         ; Stack base at 8MB
    mov [esi + RT_TASK_STACK_PTR], eax
    
    inc dword [rt_task_count]
    
    mov eax, edi              ; Return task ID
    jmp .done
    
.error:
    xor eax, eax
    
.done:
    pop ebx
    pop edi
    pop esi
    pop ebp
    ret

; Calculate RMS priority from period
; Input: EAX = period in microseconds
; Output: EAX = priority value
calculate_rms_priority:
    cmp eax, 1000             ; < 1ms
    jle .critical
    cmp eax, 5000             ; < 5ms
    jle .high
    cmp eax, 10000            ; < 10ms
    jle .medium
    cmp eax, 50000            ; < 50ms
    jle .normal
    
    mov eax, RMS_PRIORITY_LOW
    ret
    
.critical:
    mov eax, RMS_PRIORITY_CRITICAL
    ret
.high:
    mov eax, RMS_PRIORITY_HIGH
    ret
.medium:
    mov eax, RMS_PRIORITY_MEDIUM
    ret
.normal:
    mov eax, RMS_PRIORITY_NORMAL
    ret

; Update system utilization
; Input: ESI = task structure pointer
update_utilization:
    push eax
    push ebx
    push edx
    
    ; Calculate this task's utilization: (WCET * 100) / period
    mov eax, [esi + RT_TASK_WCET_US]
    mov ebx, 100
    mul ebx
    mov ebx, [esi + RT_TASK_PERIOD_US]
    xor edx, edx
    div ebx
    
    ; Add to total utilization
    add [rt_total_utilization], eax
    
    pop edx
    pop ebx
    pop eax
    ret

; Main RT scheduler entry point
global rt_schedule
rt_schedule:
    pushad
    
    ; Update current time
    call get_system_time_us
    mov ebx, eax              ; EBX = current time
    
    ; Check for deadline misses
    call check_deadline_misses
    
    ; Select scheduler algorithm
    mov eax, [current_sched_mode]
    cmp eax, SCHED_MODE_EDF
    je .edf_schedule
    cmp eax, SCHED_MODE_RMS
    je .rms_schedule
    
    ; Default to round-robin
    jmp .done
    
.edf_schedule:
    call edf_scheduler
    jmp .switch_task
    
.rms_schedule:
    call rms_scheduler
    
.switch_task:
    ; EAX contains selected task ID (0 = idle)
    test eax, eax
    jz .done
    
    ; Find task structure
    call find_rt_task_by_id
    test esi, esi
    jz .done
    
    ; Check if different from current
    cmp esi, [current_rt_task]
    je .done
    
    ; Save current task context if exists
    mov edi, [current_rt_task]
    test edi, edi
    jz .no_save
    
    call save_rt_context
    
.no_save:
    ; Switch to new task
    mov [current_rt_task], esi
    mov dword [esi + RT_TASK_STATE], RT_TASK_RUNNING
    inc dword [rt_context_switches]
    
    ; Restore new task context
    call restore_rt_context
    
.done:
    popad
    ret

; EDF Scheduler - Select task with earliest deadline
edf_scheduler:
    push esi
    push edi
    push ebx
    push ecx
    
    mov esi, RT_TASK_TABLE_BASE
    xor edi, edi              ; Best task
    mov ebx, 0xFFFFFFFF       ; Earliest deadline
    xor ecx, ecx              ; Task counter
    
.check_task:
    cmp ecx, MAX_RT_TASKS
    jae .done
    
    ; Check if task is ready
    cmp dword [esi + RT_TASK_STATE], RT_TASK_READY
    jne .next_task
    
    ; Check if released
    call get_system_time_us
    cmp eax, [esi + RT_TASK_NEXT_RELEASE]
    jl .next_task
    
    ; Compare deadline
    mov eax, [esi + RT_TASK_DEADLINE_US]
    cmp eax, ebx
    jae .next_task
    
    ; New earliest deadline
    mov ebx, eax
    mov edi, esi
    
.next_task:
    add esi, RT_TASK_STRUCT_SIZE
    inc ecx
    jmp .check_task
    
.done:
    ; Return task ID or 0
    xor eax, eax
    test edi, edi
    jz .no_task
    mov eax, [edi + RT_TASK_ID]
    
.no_task:
    pop ecx
    pop ebx
    pop edi
    pop esi
    ret

; RMS Scheduler - Select highest priority ready task
rms_scheduler:
    push esi
    push edi
    push ebx
    push ecx
    
    mov esi, RT_TASK_TABLE_BASE
    xor edi, edi              ; Best task
    xor ebx, ebx              ; Highest priority
    xor ecx, ecx              ; Task counter
    
.check_task:
    cmp ecx, MAX_RT_TASKS
    jae .done
    
    ; Check if task is ready
    cmp dword [esi + RT_TASK_STATE], RT_TASK_READY
    jne .next_task
    
    ; Check if released
    call get_system_time_us
    cmp eax, [esi + RT_TASK_NEXT_RELEASE]
    jl .next_task
    
    ; Compare priority
    mov eax, [esi + RT_TASK_PRIORITY]
    cmp eax, ebx
    jbe .next_task
    
    ; New highest priority
    mov ebx, eax
    mov edi, esi
    
.next_task:
    add esi, RT_TASK_STRUCT_SIZE
    inc ecx
    jmp .check_task
    
.done:
    ; Return task ID or 0
    xor eax, eax
    test edi, edi
    jz .no_task
    mov eax, [edi + RT_TASK_ID]
    
.no_task:
    pop ecx
    pop ebx
    pop edi
    pop esi
    ret

; Check for deadline misses
check_deadline_misses:
    push esi
    push ecx
    push eax
    
    mov esi, RT_TASK_TABLE_BASE
    xor ecx, ecx
    
    call get_system_time_us
    mov ebx, eax              ; Current time
    
.check_task:
    cmp ecx, MAX_RT_TASKS
    jae .done
    
    ; Check if task is active
    cmp dword [esi + RT_TASK_STATE], RT_TASK_DORMANT
    je .next_task
    
    ; Check deadline
    mov eax, [esi + RT_TASK_DEADLINE_US]
    cmp ebx, eax
    jle .next_task
    
    ; Deadline missed!
    inc dword [rt_deadline_misses]
    
    ; Print warning
    push rt_deadline_miss_msg
    call print_string
    add esp, 4
    
    push dword [esi + RT_TASK_ID]
    call print_hex
    add esp, 4
    call print_newline
    
    ; Reset task timing
    mov [esi + RT_TASK_NEXT_RELEASE], ebx
    add ebx, [esi + RT_TASK_PERIOD_US]
    mov [esi + RT_TASK_DEADLINE_US], ebx
    
.next_task:
    add esi, RT_TASK_STRUCT_SIZE
    inc ecx
    jmp .check_task
    
.done:
    pop eax
    pop ecx
    pop esi
    ret

; Find RT task by ID
; Input: EAX = task ID
; Output: ESI = task structure pointer or 0
find_rt_task_by_id:
    push ecx
    
    mov esi, RT_TASK_TABLE_BASE
    xor ecx, ecx
    
.search:
    cmp ecx, MAX_RT_TASKS
    jae .not_found
    
    cmp [esi + RT_TASK_ID], eax
    je .found
    
    add esi, RT_TASK_STRUCT_SIZE
    inc ecx
    jmp .search
    
.not_found:
    xor esi, esi
    
.found:
    pop ecx
    ret

; Save RT task context
; Input: EDI = task structure pointer
save_rt_context:
    lea edi, [edi + RT_TASK_CONTEXT]
    mov [edi], eax
    mov [edi + 4], ebx
    mov [edi + 8], ecx
    mov [edi + 12], edx
    mov [edi + 16], esi
    mov [edi + 20], edi
    mov [edi + 24], ebp
    mov [edi + 28], esp
    ; Save flags
    pushfd
    pop dword [edi + 32]
    ret

; Restore RT task context
; Input: ESI = task structure pointer
restore_rt_context:
    lea esi, [esi + RT_TASK_CONTEXT]
    mov eax, [esi]
    mov ebx, [esi + 4]
    mov ecx, [esi + 8]
    mov edx, [esi + 12]
    ; Skip ESI/EDI for now
    mov ebp, [esi + 24]
    mov esp, [esi + 28]
    ; Restore flags
    push dword [esi + 32]
    popfd
    ret

; External system tick counter
extern system_ticks

; Get system time in microseconds (simplified)
global get_system_time_us
get_system_time_us:
    ; Read from PIT or TSC (simplified implementation)
    ; For now, use tick counter
    mov eax, [system_ticks]
    mov ebx, TICK_PERIOD_US
    mul ebx
    ret

; RMS schedulability test
; Input: EAX = number of tasks
; Output: EAX = 1 if schedulable, 0 if not
global rms_schedulability_test
rms_schedulability_test:
    push ebx
    push ecx
    push edx
    
    ; Get utilization bound for N tasks
    ; U <= n * (2^(1/n) - 1)
    ; Approximation for small N:
    cmp eax, 1
    jle .one_task
    cmp eax, 2
    jle .two_tasks
    cmp eax, 3
    jle .three_tasks
    cmp eax, 4
    jle .four_tasks
    
    ; For many tasks, use ln(2) ≈ 69%
    mov ebx, 69
    jmp .check_util
    
.one_task:
    mov ebx, 100              ; 100%
    jmp .check_util
.two_tasks:
    mov ebx, 82               ; ~82.8%
    jmp .check_util
.three_tasks:
    mov ebx, 77               ; ~77.9%
    jmp .check_util
.four_tasks:
    mov ebx, 75               ; ~75.7%
    
.check_util:
    ; Compare with actual utilization
    mov eax, [rt_total_utilization]
    cmp eax, ebx
    jle .schedulable
    
    xor eax, eax              ; Not schedulable
    jmp .done
    
.schedulable:
    mov eax, 1
    
.done:
    pop edx
    pop ecx
    pop ebx
    ret

; Set scheduler mode
; Input: EAX = mode (SCHED_MODE_EDF or SCHED_MODE_RMS)
global rt_set_scheduler_mode
rt_set_scheduler_mode:
    cmp eax, SCHED_MODE_RMS
    ja .error
    
    mov [current_sched_mode], eax
    mov eax, 1                ; Success
    ret
    
.error:
    xor eax, eax
    ret

; Print RT scheduler statistics
global rt_print_stats
rt_print_stats:
    pushad
    
    ; Print utilization
    push rt_util_msg
    call print_string
    add esp, 4
    
    mov eax, [rt_total_utilization]
    push eax
    call print_decimal
    add esp, 4
    
    push rt_percent_msg
    call print_string
    add esp, 4
    call print_newline
    
    ; Print deadline misses
    push deadline_miss_count_msg
    call print_string
    add esp, 4
    
    push dword [rt_deadline_misses]
    call print_decimal
    add esp, 4
    call print_newline
    
    ; Print context switches
    push context_switch_msg
    call print_string
    add esp, 4
    
    push dword [rt_context_switches]
    call print_decimal
    add esp, 4
    call print_newline
    
    popad
    ret

; Messages for statistics
deadline_miss_count_msg: db 'Deadline misses: ', 0
context_switch_msg:      db 'Context switches: ', 0

; Print string helper
print_string:
    push ebp
    mov ebp, esp
    push esi
    push edi
    
    mov esi, [ebp + 8]
    mov edi, [cursor_pos]
    
.loop:
    lodsb
    test al, al
    jz .done
    
    mov [edi], al
    mov byte [edi + 1], 0x07
    add edi, 2
    
    ; Check for end of screen
    cmp edi, VGA_BUFFER + (80 * 25 * 2)
    jb .loop
    
    ; Scroll if needed
    call scroll_screen
    mov edi, VGA_BUFFER + (24 * 80 * 2)
    jmp .loop
    
.done:
    mov [cursor_pos], edi
    pop edi
    pop esi
    pop ebp
    ret

; Print newline
print_newline:
    push eax
    push edi
    
    mov edi, [cursor_pos]
    ; Calculate start of next line
    sub edi, VGA_BUFFER
    mov eax, edi
    xor edx, edx
    mov ebx, 160          ; 80 columns * 2 bytes
    div ebx
    inc eax
    mul ebx
    add eax, VGA_BUFFER
    mov [cursor_pos], eax
    
    ; Check if we need to scroll
    cmp eax, VGA_BUFFER + (80 * 25 * 2)
    jb .no_scroll
    call scroll_screen
    mov dword [cursor_pos], VGA_BUFFER + (24 * 80 * 2)
    
.no_scroll:
    pop edi
    pop eax
    ret

; Print hex value
print_hex:
    push ebp
    mov ebp, esp
    push eax
    push ebx
    push ecx
    push edx
    push edi
    
    mov eax, [ebp + 8]    ; Value to print
    mov edi, [cursor_pos]
    
    ; Print "0x" prefix
    mov word [edi], 0x0730  ; '0' with attribute
    mov word [edi + 2], 0x0778  ; 'x' with attribute
    add edi, 4
    
    ; Convert to hex
    mov ecx, 8            ; 8 hex digits
.hex_loop:
    rol eax, 4
    mov edx, eax
    and edx, 0x0F
    cmp edx, 10
    jb .digit
    add edx, 'A' - 10
    jmp .store
.digit:
    add edx, '0'
.store:
    mov [edi], dl
    mov byte [edi + 1], 0x07
    add edi, 2
    loop .hex_loop
    
    mov [cursor_pos], edi
    
    pop edi
    pop edx
    pop ecx
    pop ebx
    pop eax
    pop ebp
    ret

; Print decimal value
print_decimal:
    push ebp
    mov ebp, esp
    push eax
    push ebx
    push ecx
    push edx
    push edi
    push esi
    
    mov eax, [ebp + 8]    ; Value to print
    mov edi, decimal_buffer + 10  ; End of buffer
    mov byte [edi], 0     ; Null terminator
    dec edi
    
    test eax, eax
    jnz .convert
    mov byte [edi], '0'
    jmp .print
    
.convert:
    mov ebx, 10
.div_loop:
    xor edx, edx
    div ebx
    add dl, '0'
    mov [edi], dl
    dec edi
    test eax, eax
    jnz .div_loop
    inc edi
    
.print:
    push edi
    call print_string
    add esp, 4
    
    pop esi
    pop edi
    pop edx
    pop ecx
    pop ebx
    pop eax
    pop ebp
    ret

; Scroll screen
scroll_screen:
    push esi
    push edi
    push ecx
    
    ; Copy lines 1-24 to lines 0-23
    mov esi, VGA_BUFFER + (80 * 2)
    mov edi, VGA_BUFFER
    mov ecx, 80 * 24
    rep movsw
    
    ; Clear last line
    mov edi, VGA_BUFFER + (24 * 80 * 2)
    mov ecx, 80
    mov ax, 0x0720        ; Space with default attribute
    rep stosw
    
    pop ecx
    pop edi
    pop esi
    ret

; Data
cursor_pos: dd VGA_BUFFER
decimal_buffer: times 11 db 0