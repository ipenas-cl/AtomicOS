; AtomicOS System Call Test
; Direct assembly test for syscall functionality

section .data
    test_msg db 'Testing syscalls...', 0
    pid_msg db 'PID: ', 0
    time_msg db 'Time: ', 0
    done_msg db 'Syscall test complete!', 0
    newline db 10, 0

section .text

global test_syscalls
test_syscalls:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    
    ; Print test message
    push test_msg
    call print_string
    add esp, 4
    call print_newline
    
    ; Test 1: getpid()
    push pid_msg
    call print_string
    add esp, 4
    
    mov eax, 4          ; SYS_GETPID
    int 0x80
    
    push eax
    call print_hex
    add esp, 4
    call print_newline
    
    ; Test 2: rt_gettime()
    push time_msg
    call print_string
    add esp, 4
    
    mov eax, 34         ; SYS_RT_GETTIME
    int 0x80
    mov esi, eax        ; Save start time
    
    push eax
    call print_hex
    add esp, 4
    call print_newline
    
    ; Test 3: rt_yield()
    mov eax, 32         ; SYS_RT_YIELD
    int 0x80
    
    ; Test 4: rt_gettime() again
    push time_msg
    call print_string
    add esp, 4
    
    mov eax, 34         ; SYS_RT_GETTIME
    int 0x80
    mov edi, eax        ; Save end time
    
    push eax
    call print_hex
    add esp, 4
    call print_newline
    
    ; Calculate elapsed time
    sub edi, esi
    push elapsed_msg
    call print_string
    add esp, 4
    
    push edi
    call print_hex
    add esp, 4
    push us_msg
    call print_string
    add esp, 4
    call print_newline
    
    ; Done
    push done_msg
    call print_string
    add esp, 4
    call print_newline
    
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

section .data
    elapsed_msg db 'Elapsed: ', 0
    us_msg db ' us', 0

; Helper functions (assuming these exist)
extern print_string
extern print_hex
extern print_newline