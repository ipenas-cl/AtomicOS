; AtomicOS Filesystem System Call Interface
; Assembly interface for filesystem operations

; Filesystem system call numbers
%define SYS_FS_OPEN         0x50
%define SYS_FS_CLOSE        0x51
%define SYS_FS_READ         0x52
%define SYS_FS_WRITE        0x53
%define SYS_FS_CREATE       0x54
%define SYS_FS_DELETE       0x55
%define SYS_FS_STAT         0x56
%define SYS_FS_SEEK         0x57

; File open flags
%define FS_OPEN_READ        0x01
%define FS_OPEN_WRITE       0x02
%define FS_OPEN_CREATE      0x04

; File types
%define FS_TYPE_FILE        0x01
%define FS_TYPE_DIRECTORY   0x02

; Permissions
%define FS_PERM_READ        0x04
%define FS_PERM_WRITE       0x02
%define FS_PERM_EXEC        0x01

section .text

; Open file system call wrapper
; Input: EBX = path_ptr, ECX = flags
; Output: EAX = file descriptor or error
global sys_fs_open
sys_fs_open:
    push ebx
    push ecx
    
    mov eax, SYS_FS_OPEN
    ; EBX already contains path pointer
    ; ECX already contains flags
    int 0x80
    
    pop ecx
    pop ebx
    ret

; Close file system call wrapper
; Input: EBX = file descriptor
; Output: EAX = 0 on success or error
global sys_fs_close
sys_fs_close:
    mov eax, SYS_FS_CLOSE
    ; EBX already contains fd
    int 0x80
    ret

; Read file system call wrapper
; Input: EBX = fd, ECX = buffer_ptr, EDX = size
; Output: EAX = bytes read or error
global sys_fs_read
sys_fs_read:
    push esi
    
    mov eax, SYS_FS_READ
    ; EBX = fd
    ; ECX = buffer
    ; EDX = size
    int 0x80
    
    pop esi
    ret

; Write file system call wrapper
; Input: EBX = fd, ECX = buffer_ptr, EDX = size
; Output: EAX = bytes written or error
global sys_fs_write
sys_fs_write:
    push esi
    
    mov eax, SYS_FS_WRITE
    ; EBX = fd
    ; ECX = buffer
    ; EDX = size
    int 0x80
    
    pop esi
    ret

; Create file system call wrapper
; Input: EBX = path_ptr, ECX = type, EDX = permissions
; Output: EAX = 0 on success or error
global sys_fs_create
sys_fs_create:
    mov eax, SYS_FS_CREATE
    ; EBX = path
    ; ECX = type
    ; EDX = permissions
    int 0x80
    ret

; Delete file system call wrapper
; Input: EBX = path_ptr
; Output: EAX = 0 on success or error
global sys_fs_delete
sys_fs_delete:
    mov eax, SYS_FS_DELETE
    ; EBX = path
    int 0x80
    ret

; Helper function to create and open a file
; Input: EBX = path_ptr
; Output: EAX = file descriptor
global fs_create_and_open
fs_create_and_open:
    push ebx
    push ecx
    push edx
    
    ; Create file with read/write permissions
    mov ecx, FS_TYPE_FILE
    mov edx, FS_PERM_READ | FS_PERM_WRITE
    call sys_fs_create
    
    ; Check if creation succeeded
    test eax, eax
    jnz .error
    
    ; Open the file for read/write
    pop edx
    pop ecx
    pop ebx
    push ebx
    push ecx
    push edx
    
    mov ecx, FS_OPEN_READ | FS_OPEN_WRITE
    call sys_fs_open
    
.error:
    pop edx
    pop ecx
    pop ebx
    ret

; Simple file write helper
; Input: EBX = filename_ptr, ECX = data_ptr, EDX = size
; Output: EAX = 0 on success
global fs_write_file
fs_write_file:
    push ebx
    push ecx
    push edx
    push esi
    
    ; Save data info
    mov esi, ecx        ; Save data pointer
    push edx            ; Save size
    
    ; Create and open file
    call fs_create_and_open
    test eax, eax
    js .error           ; Error if negative
    
    ; Save fd
    mov ebx, eax
    
    ; Write data
    mov ecx, esi        ; Data pointer
    pop edx             ; Size
    push ebx            ; Save fd
    call sys_fs_write
    
    ; Close file
    pop ebx             ; Restore fd
    push eax            ; Save write result
    call sys_fs_close
    
    pop eax             ; Restore write result
    jmp .done
    
.error:
    pop edx             ; Clean stack
    mov eax, -1
    
.done:
    pop esi
    pop edx
    pop ecx
    pop ebx
    ret