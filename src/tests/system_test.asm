; AtomicOS System Test Suite
; Tests interrupts, process management, real-time scheduling, and syscalls

[BITS 32]

%define VGA_BUFFER 0xB8000
%define TEST_LINE 10        ; Start test output at line 10

section .data

test_title: db 'AtomicOS System Test Suite v0.8.0', 0
test_separator: db '==================================', 0

; Test status messages
test_interrupts_msg: db 'Testing Interrupts...', 0
test_process_msg: db 'Testing Process Management...', 0  
test_scheduler_msg: db 'Testing Real-Time Scheduler...', 0
test_syscalls_msg: db 'Testing System Calls...', 0

pass_msg: db ' [PASS]', 0
fail_msg: db ' [FAIL]', 0

; Test counters
timer_ticks_start: dd 0
timer_ticks_end: dd 0
test_results: dd 0          ; Bit field for test results

section .text

global run_system_tests
extern system_ticks
extern create_process
extern rt_create_task
extern rt_set_scheduler_mode
extern rt_print_stats
extern sys_getpid
extern sys_write
extern sys_rt_yield
extern sys_rt_gettime
extern rt_schedule
extern enable_interrupts
extern disable_interrupts

; Main test runner
run_system_tests:
    push ebp
    mov ebp, esp
    pushad
    
    ; Clear test area
    call clear_test_area
    
    ; Display test title
    mov esi, test_title
    mov edi, VGA_BUFFER + (TEST_LINE * 80 * 2)
    call print_test_string
    
    ; Print separator
    mov esi, test_separator
    mov edi, VGA_BUFFER + ((TEST_LINE + 1) * 80 * 2)
    call print_test_string
    
    ; Run tests
    call test_interrupts
    call test_process_management
    call test_realtime_scheduler
    call test_syscalls
    
    ; Display summary
    call display_test_summary
    
    popad
    pop ebp
    ret

; Test 1: Interrupt System
test_interrupts:
    pushad
    
    ; Display test name
    mov esi, test_interrupts_msg
    mov edi, VGA_BUFFER + ((TEST_LINE + 3) * 80 * 2)
    call print_test_string
    
    ; Test 1.1: Timer interrupt
    ; Save current tick count
    mov eax, [system_ticks]
    mov [timer_ticks_start], eax
    
    ; Enable interrupts and wait
    call enable_interrupts
    
    ; Busy wait for ~100ms (100 ticks)
    mov ecx, 0x100000
.wait_loop:
    nop
    loop .wait_loop
    
    ; Check if ticks increased
    mov eax, [system_ticks]
    mov [timer_ticks_end], eax
    
    ; Verify ticks increased
    mov ebx, [timer_ticks_start]
    cmp eax, ebx
    jle .timer_fail
    
    ; Timer test passed
    or dword [test_results], 0x01
    mov esi, pass_msg
    jmp .timer_done
    
.timer_fail:
    mov esi, fail_msg
    
.timer_done:
    ; Print result
    mov edi, VGA_BUFFER + ((TEST_LINE + 3) * 80 * 2) + (30 * 2)
    call print_test_string
    
    popad
    ret

; Test 2: Process Management
test_process_management:
    pushad
    
    ; Display test name
    mov esi, test_process_msg
    mov edi, VGA_BUFFER + ((TEST_LINE + 4) * 80 * 2)
    call print_test_string
    
    ; Create test processes
    push 1              ; Priority HIGH
    push test_process_1
    push proc1_name
    call create_process
    add esp, 12
    
    test eax, eax
    jz .proc_fail
    
    push 0              ; Priority NORMAL
    push test_process_2
    push proc2_name
    call create_process
    add esp, 12
    
    test eax, eax
    jz .proc_fail
    
    ; Process creation passed
    or dword [test_results], 0x02
    mov esi, pass_msg
    jmp .proc_done
    
.proc_fail:
    mov esi, fail_msg
    
.proc_done:
    mov edi, VGA_BUFFER + ((TEST_LINE + 4) * 80 * 2) + (30 * 2)
    call print_test_string
    
    popad
    ret

; Test 3: Real-Time Scheduler
test_realtime_scheduler:
    pushad
    
    ; Display test name
    mov esi, test_scheduler_msg
    mov edi, VGA_BUFFER + ((TEST_LINE + 5) * 80 * 2)
    call print_test_string
    
    ; Set EDF mode
    mov eax, 1          ; SCHED_MODE_EDF
    call rt_set_scheduler_mode
    
    ; Create RT tasks with different periods
    mov eax, 1000       ; 1ms period
    mov ebx, 100        ; 100us WCET
    mov ecx, rt_task_1
    xor edx, edx        ; Use period as deadline
    call rt_create_task
    
    test eax, eax
    jz .rt_fail
    
    mov eax, 5000       ; 5ms period
    mov ebx, 500        ; 500us WCET
    mov ecx, rt_task_2
    xor edx, edx
    call rt_create_task
    
    test eax, eax
    jz .rt_fail
    
    ; Trigger scheduling
    call rt_schedule
    
    ; RT scheduler test passed
    or dword [test_results], 0x04
    mov esi, pass_msg
    jmp .rt_done
    
.rt_fail:
    mov esi, fail_msg
    
.rt_done:
    mov edi, VGA_BUFFER + ((TEST_LINE + 5) * 80 * 2) + (30 * 2)
    call print_test_string
    
    ; Print RT statistics
    call rt_print_stats
    
    popad
    ret

; Test 4: System Calls
test_syscalls:
    pushad
    
    ; Display test name
    mov esi, test_syscalls_msg
    mov edi, VGA_BUFFER + ((TEST_LINE + 6) * 80 * 2)
    call print_test_string
    
    ; Test getpid
    call sys_getpid
    test eax, eax
    jz .syscall_fail
    
    ; Test rt_gettime
    call sys_rt_gettime
    test eax, eax
    jz .syscall_fail
    
    ; Test write (simple)
    push 5              ; count
    push hello_msg      ; buffer
    push 1              ; stdout
    call sys_write
    add esp, 12
    
    cmp eax, -10        ; Check not E_NOSYS
    je .syscall_fail
    
    ; Syscall test passed
    or dword [test_results], 0x08
    mov esi, pass_msg
    jmp .syscall_done
    
.syscall_fail:
    mov esi, fail_msg
    
.syscall_done:
    mov edi, VGA_BUFFER + ((TEST_LINE + 6) * 80 * 2) + (30 * 2)
    call print_test_string
    
    popad
    ret

; Display test summary
display_test_summary:
    pushad
    
    ; Count passed tests
    mov eax, [test_results]
    xor ecx, ecx
    mov ebx, 4
    
.count_loop:
    test eax, 1
    jz .skip
    inc ecx
.skip:
    shr eax, 1
    dec ebx
    jnz .count_loop
    
    ; Display summary line
    mov esi, summary_msg
    mov edi, VGA_BUFFER + ((TEST_LINE + 8) * 80 * 2)
    call print_test_string
    
    ; Display passed count
    add ecx, '0'
    mov [edi + (20 * 2)], cl
    mov byte [edi + (20 * 2) + 1], 0x0A  ; Green
    
    ; Display total
    mov byte [edi + (22 * 2)], '4'
    mov byte [edi + (22 * 2) + 1], 0x07
    
    popad
    ret

; Helper: Clear test area
clear_test_area:
    pushad
    
    mov edi, VGA_BUFFER + (TEST_LINE * 80 * 2)
    mov ecx, 15 * 80    ; Clear 15 lines
    mov ax, 0x0720      ; Space with default attribute
    
.clear_loop:
    stosw
    loop .clear_loop
    
    popad
    ret

; Helper: Print test string
; Input: ESI = string, EDI = screen position
print_test_string:
    push eax
    push esi
    push edi
    
.print_loop:
    lodsb
    test al, al
    jz .done
    
    mov [edi], al
    mov byte [edi + 1], 0x07  ; Default attribute
    add edi, 2
    jmp .print_loop
    
.done:
    pop edi
    pop esi
    pop eax
    ret

; Test processes
test_process_1:
    ; Simple test process that increments a counter
    mov ebx, proc1_counter
.loop:
    inc dword [ebx]
    
    ; Small delay
    mov ecx, 0x10000
.delay:
    nop
    loop .delay
    
    jmp .loop

test_process_2:
    ; Another test process
    mov ebx, proc2_counter
.loop:
    add dword [ebx], 2
    
    ; Small delay
    mov ecx, 0x10000
.delay:
    nop
    loop .delay
    
    jmp .loop

; RT tasks
rt_task_1:
    ; High frequency RT task
    pushad
    inc dword [rt1_counter]
    
    ; Simulate work
    mov ecx, 50
.work:
    nop
    loop .work
    
    popad
    ret

rt_task_2:
    ; Lower frequency RT task
    pushad
    inc dword [rt2_counter]
    
    ; Simulate work
    mov ecx, 250
.work:
    nop
    loop .work
    
    popad
    ret

; Test data
proc1_name: db 'test_proc1', 0
proc2_name: db 'test_proc2', 0
hello_msg: db 'Hello', 0
summary_msg: db 'Tests Passed: ', 0

; Counters
proc1_counter: dd 0
proc2_counter: dd 0
rt1_counter: dd 0
rt2_counter: dd 0

; Helper to disable interrupts
disable_interrupts:
    cli
    ret