; AtomicOS IPC System Call Interface
; Assembly interface for inter-process communication

; IPC System call numbers
%define SYS_IPC_CREATE      0x40
%define SYS_IPC_SEND        0x41
%define SYS_IPC_RECEIVE     0x42
%define SYS_IPC_CLOSE       0x43
%define SYS_IPC_STATS       0x44

; IPC Constants from C header
%define IPC_MAX_MESSAGE_SIZE    256
%define IPC_PERM_READ          0x01
%define IPC_PERM_WRITE         0x02
%define IPC_PERM_OWNER         0x04

section .text

; Create IPC channel system call wrapper
; Input: ECX = permissions
; Output: EAX = channel_id (0 on error)
global sys_ipc_create
sys_ipc_create:
    push ebx
    push ecx
    
    mov eax, SYS_IPC_CREATE
    mov ebx, [current_pid]      ; Owner PID
    ; ECX already contains permissions
    int 0x80
    
    pop ecx
    pop ebx
    ret

; Send IPC message system call wrapper
; Input: EBX = channel_id, ECX = data_ptr, EDX = length
; Output: EAX = 0 on success, negative on error
global sys_ipc_send
sys_ipc_send:
    push esi
    push edi
    
    mov eax, SYS_IPC_SEND
    mov esi, [current_pid]      ; Sender PID
    mov edi, ecx                ; Save data pointer
    int 0x80
    
    pop edi
    pop esi
    ret

; Receive IPC message system call wrapper
; Input: EBX = channel_id, ECX = buffer_ptr, EDX = buffer_size
; Output: EAX = 0 on success, negative on error
global sys_ipc_receive
sys_ipc_receive:
    push esi
    push edi
    
    mov eax, SYS_IPC_RECEIVE
    mov esi, [current_pid]      ; Receiver PID
    mov edi, ecx                ; Save buffer pointer
    int 0x80
    
    pop edi
    pop esi
    ret

; Close IPC channel system call wrapper
; Input: EBX = channel_id
; Output: EAX = 0 on success, negative on error
global sys_ipc_close
sys_ipc_close:
    push ecx
    
    mov eax, SYS_IPC_CLOSE
    mov ecx, [current_pid]      ; Owner PID
    int 0x80
    
    pop ecx
    ret

; Get IPC statistics system call wrapper
; Input: EBX = channel_id, ECX = stats_buffer_ptr
; Output: EAX = 0 on success, negative on error
global sys_ipc_stats
sys_ipc_stats:
    mov eax, SYS_IPC_STATS
    int 0x80
    ret

; Helper function to send a simple message
; Input: EBX = channel_id, ECX = message string pointer
; Output: EAX = 0 on success
global ipc_send_string
ipc_send_string:
    push ecx
    push edx
    push esi
    
    ; Calculate string length
    xor edx, edx
    mov esi, ecx
.count_loop:
    lodsb
    test al, al
    jz .count_done
    inc edx
    cmp edx, IPC_MAX_MESSAGE_SIZE
    jb .count_loop
.count_done:
    
    ; Send the message
    call sys_ipc_send
    
    pop esi
    pop edx
    pop ecx
    ret

; Helper function to create a bidirectional channel
; Output: EAX = channel_id
global ipc_create_bidir_channel
ipc_create_bidir_channel:
    mov ecx, IPC_PERM_READ | IPC_PERM_WRITE | IPC_PERM_OWNER
    call sys_ipc_create
    ret

section .data
current_pid: dd 0   ; Set by process initialization