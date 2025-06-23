; AtomicOS System Call Handler
; INT 0x80 handler for deterministic syscalls

section .text

global init_syscall_handler

; Forward declarations - these are defined later or in other includes
; IPC functions are in ipc_stubs.inc for now

; System call interrupt handler (INT 0x80)
global syscall_handler
syscall_handler:
    ; Save all registers (syscall convention)
    pushad              ; Save all general purpose registers
    push ds
    push es
    push fs
    push gs
    
    ; Start WCET measurement
    rdtsc
    push edx            ; Save high 32 bits
    push eax            ; Save low 32 bits
    
    ; Set kernel data segments
    mov ax, 0x10        ; Kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Get syscall parameters from saved registers
    ; The pushad instruction saves registers in this order:
    ; EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    ; Plus segment registers: DS, ES, FS, GS
    ; So offsets from ESP are:
    ; ESP+0:  saved EAX (from rdtsc)
    ; ESP+4:  saved EDX (from rdtsc)
    ; ESP+8:  saved GS
    ; ESP+12: saved FS
    ; ESP+16: saved ES
    ; ESP+20: saved DS
    ; ESP+24: saved EDI
    ; ESP+28: saved ESI
    ; ESP+32: saved EBP
    ; ESP+36: saved ESP
    ; ESP+40: saved EBX
    ; ESP+44: saved EDX
    ; ESP+48: saved ECX
    ; ESP+52: saved EAX (syscall number)
    
    mov eax, [esp + 52]  ; Get EAX (syscall number)
    mov ebx, [esp + 40]  ; Get EBX (arg0)
    mov ecx, [esp + 48]  ; Get ECX (arg1)
    mov edx, [esp + 44]  ; Get EDX (arg2)
    mov esi, [esp + 28]  ; Get ESI (arg3)
    mov edi, [esp + 24]  ; Get EDI (arg4)
    
    ; For now, handle syscalls directly in assembly
    ; Check syscall number
    cmp eax, 4          ; SYS_GETPID
    je .handle_getpid
    cmp eax, 10         ; SYS_BRK
    je .handle_brk
    cmp eax, 11         ; SYS_MMAP
    je .handle_mmap
    cmp eax, 12         ; SYS_MUNMAP
    je .handle_munmap
    cmp eax, 32         ; SYS_RT_YIELD  
    je .handle_yield
    cmp eax, 34         ; SYS_RT_GETTIME
    je .handle_gettime
    cmp eax, 0x40       ; SYS_IPC_CREATE
    je .handle_ipc_create
    cmp eax, 0x41       ; SYS_IPC_SEND
    je .handle_ipc_send
    cmp eax, 0x42       ; SYS_IPC_RECEIVE
    je .handle_ipc_receive
    cmp eax, 0x43       ; SYS_IPC_CLOSE
    je .handle_ipc_close
    cmp eax, 0x44       ; SYS_IPC_STATS
    je .handle_ipc_stats
    cmp eax, 0x50       ; SYS_FS_OPEN
    je .handle_fs_open
    cmp eax, 0x51       ; SYS_FS_CLOSE
    je .handle_fs_close
    cmp eax, 0x52       ; SYS_FS_READ
    je .handle_fs_read
    cmp eax, 0x53       ; SYS_FS_WRITE
    je .handle_fs_write
    cmp eax, 0x54       ; SYS_FS_CREATE
    je .handle_fs_create
    cmp eax, 0x55       ; SYS_FS_DELETE
    je .handle_fs_delete
    
    ; Unknown syscall
    mov eax, -10        ; -E_NOSYS
    jmp .syscall_done
    
.handle_getpid:
    call get_current_pid
    jmp .syscall_done
    
.handle_yield:
    call yield_to_scheduler
    xor eax, eax        ; Return 0
    jmp .syscall_done
    
.handle_gettime:
    call get_system_time_us
    ; Result already in EAX
    jmp .syscall_done
    
.handle_brk:
    ; EBX contains new brk address
    ; For now, just return success
    xor eax, eax
    jmp .syscall_done
    
.handle_mmap:
    ; EBX = size, ECX = protection flags
    mov eax, ebx        ; Size
    mov ebx, ecx        ; Flags
    call vmalloc
    ; Result (address or 0) in EAX
    jmp .syscall_done
    
.handle_munmap:
    ; EBX = address, ECX = size
    mov eax, ebx        ; Address
    mov ebx, ecx        ; Size
    call vfree
    xor eax, eax        ; Return success
    jmp .syscall_done
    
.handle_ipc_create:
    ; EBX = owner_pid, ECX = permissions
    push ecx            ; permissions
    push ebx            ; owner_pid
    call ipc_create_channel
    add esp, 8
    ; Result in EAX
    jmp .syscall_done
    
.handle_ipc_send:
    ; EBX = channel_id, ESI = sender_pid, EDI = data_ptr, EDX = length
    push edx            ; length
    push edi            ; data
    push esi            ; sender_pid
    push ebx            ; channel_id
    call ipc_send
    add esp, 16
    ; Result in EAX
    jmp .syscall_done
    
.handle_ipc_receive:
    ; EBX = channel_id, ESI = receiver_pid, EDI = buffer_ptr, EDX = buffer_size
    push 0              ; actual_size placeholder
    mov eax, esp        ; pointer to actual_size
    push eax            ; actual_size pointer
    push edx            ; buffer_size
    push edi            ; buffer
    push esi            ; receiver_pid
    push ebx            ; channel_id
    call ipc_receive
    add esp, 20
    pop edx             ; Get actual_size (could be returned in EDX)
    ; Result in EAX
    jmp .syscall_done
    
.handle_ipc_close:
    ; EBX = channel_id, ECX = owner_pid
    push ecx            ; owner_pid
    push ebx            ; channel_id
    call ipc_close_channel
    add esp, 8
    ; Result in EAX
    jmp .syscall_done
    
.handle_ipc_stats:
    ; EBX = channel_id, ECX = stats_buffer_ptr
    push ecx            ; stats buffer
    push ebx            ; channel_id
    call ipc_get_stats
    add esp, 8
    ; Result in EAX
    jmp .syscall_done
    
.handle_fs_open:
    ; EBX = path_ptr, ECX = flags
    push ecx            ; flags
    push ebx            ; path
    call fs_open
    add esp, 8
    ; Result (fd) in EAX
    jmp .syscall_done
    
.handle_fs_close:
    ; EBX = fd
    push ebx            ; fd
    call fs_close
    add esp, 4
    ; Result in EAX
    jmp .syscall_done
    
.handle_fs_read:
    ; EBX = fd, ECX = buffer_ptr, EDX = size
    push edx            ; size
    push ecx            ; buffer
    push ebx            ; fd
    call fs_read
    add esp, 12
    ; Result (bytes read) in EAX
    jmp .syscall_done
    
.handle_fs_write:
    ; EBX = fd, ECX = buffer_ptr, EDX = size
    push edx            ; size
    push ecx            ; buffer
    push ebx            ; fd
    call fs_write
    add esp, 12
    ; Result (bytes written) in EAX
    jmp .syscall_done
    
.handle_fs_create:
    ; EBX = path_ptr, ECX = type, EDX = permissions
    push edx            ; permissions
    push ecx            ; type
    push ebx            ; path
    call fs_create
    add esp, 12
    ; Result in EAX
    jmp .syscall_done
    
.handle_fs_delete:
    ; EBX = path_ptr
    push ebx            ; path
    call fs_delete
    add esp, 4
    ; Result in EAX
    jmp .syscall_done
    
.syscall_done:
    
    ; Save return value
    mov [esp + 52], eax  ; Store return value in saved EAX
    
    ; End WCET measurement
    rdtsc
    pop ebx             ; Get start time low
    pop ecx             ; Get start time high
    
    ; Calculate cycles (simplified - assumes no overflow)
    sub eax, ebx
    sbb edx, ecx
    
    ; Update interrupt statistics
    push eax            ; Cycle count
    push 0x80           ; INT 0x80
    call update_interrupt_cycles
    add esp, 8
    
    ; Restore registers
    pop gs
    pop fs
    pop es
    pop ds
    popad
    
    ; Return from interrupt
    iretd

; Initialize syscall handler
init_syscall_handler:
    ; Nothing to do - handler is installed by init_idt
    ret

; User-space syscall stub (for testing)
global do_syscall
do_syscall:
    ; Input: EAX = syscall number
    ;        EBX = arg0
    ;        ECX = arg1
    ;        EDX = arg2
    ;        ESI = arg3
    ;        EDI = arg4
    int 0x80
    ; Output: EAX = return value
    ret

; Specific syscall wrappers for user space
global sys_exit
sys_exit:
    mov eax, 0          ; SYS_EXIT
    mov ebx, [esp + 4]  ; Exit code
    int 0x80
    ; Should not return
    jmp $

global sys_getpid
sys_getpid:
    mov eax, 4          ; SYS_GETPID
    int 0x80
    ret

global sys_write
sys_write:
    mov eax, 23         ; SYS_WRITE
    mov ebx, [esp + 4]  ; fd
    mov ecx, [esp + 8]  ; buffer
    mov edx, [esp + 12] ; count
    int 0x80
    ret

global sys_rt_yield
sys_rt_yield:
    mov eax, 32         ; SYS_RT_YIELD
    int 0x80
    ret

global sys_rt_gettime
sys_rt_gettime:
    mov eax, 34         ; SYS_RT_GETTIME
    int 0x80
    ret

global sys_rt_create_task
sys_rt_create_task:
    mov eax, 30         ; SYS_RT_CREATE
    mov ebx, [esp + 4]  ; period_us
    mov ecx, [esp + 8]  ; wcet_us
    mov edx, [esp + 12] ; entry_point
    mov esi, [esp + 16] ; deadline_us
    int 0x80
    ret

global sys_mmap
sys_mmap:
    mov eax, 11         ; SYS_MMAP
    mov ebx, [esp + 4]  ; size
    mov ecx, [esp + 8]  ; flags
    int 0x80
    ret

global sys_munmap
sys_munmap:
    mov eax, 12         ; SYS_MUNMAP
    mov ebx, [esp + 4]  ; address
    mov ecx, [esp + 8]  ; size
    int 0x80
    ret

; Helper functions for kernel use
global get_current_pid
get_current_pid:
    ; Get PID from current process structure
    mov eax, [current_process_ptr]
    test eax, eax
    jz .no_process
    mov eax, [eax]      ; First field is PID
    ret
.no_process:
    mov eax, 1          ; Default PID
    ret

; get_system_time_us is defined in realtime_scheduler.inc

global get_current_security_level
get_current_security_level:
    ; Return user level for now
    mov eax, 1
    ret

global process_exit
process_exit:
    ; Simple halt for now
    cli
    hlt
    jmp $

global yield_to_scheduler
yield_to_scheduler:
    ; Call the scheduler
    call rt_schedule
    ret