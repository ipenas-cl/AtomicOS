; AtomicOS Process Management
; Deterministic implementation in assembly

section .data

; Process states
PROCESS_STATE_READY     equ 0
PROCESS_STATE_RUNNING   equ 1
PROCESS_STATE_BLOCKED   equ 2
PROCESS_STATE_SUSPENDED equ 3
PROCESS_STATE_ZOMBIE    equ 4

; Priorities
PRIORITY_KERNEL         equ 0
PRIORITY_REALTIME       equ 1
PRIORITY_SYSTEM         equ 2
PRIORITY_NORMAL         equ 3
PRIORITY_LOW            equ 4
PRIORITY_IDLE           equ 5

; Constants
MAX_PROCESSES           equ 32
PCB_SIZE                equ 256
PROCESS_TABLE_BASE      equ 0x600000
DEFAULT_TIME_SLICE      equ 10

; Global variables
current_process_ptr:    dd 0
idle_process_ptr:       dd 0
process_count:          dd 0
next_pid:               dd 1
ready_queue_head:       dd 0
ready_queue_tail:       dd 0

; Messages
proc_init_msg:          db 'Process management initialized', 0
proc_created_msg:       db 'Created process: ', 0
pid_msg:                db ' (PID ', 0
closing_paren:          db ')', 0

section .text

; Initialize process management
global init_process_management
init_process_management:
    push ebp
    mov ebp, esp
    push esi
    push edi
    
    ; Clear process table
    mov edi, PROCESS_TABLE_BASE
    mov ecx, MAX_PROCESSES * PCB_SIZE / 4
    xor eax, eax
    rep stosd
    
    ; Initialize globals
    mov dword [current_process_ptr], 0
    mov dword [process_count], 0
    mov dword [next_pid], 1
    mov dword [ready_queue_head], 0
    mov dword [ready_queue_tail], 0
    
    ; Create idle process
    call create_idle_process
    
    ; Print init message
    push proc_init_msg
    call print_message
    add esp, 4
    
    pop edi
    pop esi
    pop ebp
    ret

; Create idle process
create_idle_process:
    push ebp
    mov ebp, esp
    
    ; Idle process at slot 0
    mov edi, PROCESS_TABLE_BASE
    
    ; Set PID = 0
    mov dword [edi], 0
    
    ; Set state = READY
    mov byte [edi + 4], PROCESS_STATE_READY
    
    ; Set priority = IDLE
    mov byte [edi + 5], PRIORITY_IDLE
    
    ; Set name = "idle"
    mov dword [edi + 8], 'idle'
    mov byte [edi + 12], 0
    
    ; Set as current and idle process
    mov [current_process_ptr], edi
    mov [idle_process_ptr], edi
    
    ; Set entry point to idle loop
    mov eax, idle_loop
    mov [edi + 48], eax  ; EIP field
    
    pop ebp
    ret

; Idle loop
idle_loop:
    sti
.loop:
    hlt
    jmp .loop

; Create new process (simplified)
; Input: ESI = name, EDI = entry point, AL = priority
global create_process
create_process:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    
    ; Find free slot
    xor ebx, ebx  ; slot counter
    
.find_slot:
    cmp ebx, MAX_PROCESSES
    jae .no_slot
    
    ; Check if slot is free
    mov eax, ebx
    imul eax, PCB_SIZE
    add eax, PROCESS_TABLE_BASE
    
    cmp dword [eax], 0  ; Check PID field
    je .found_slot
    
    inc ebx
    jmp .find_slot
    
.no_slot:
    xor eax, eax  ; Return 0 for failure
    jmp .done
    
.found_slot:
    ; EAX = PCB address
    push eax  ; Save PCB address
    
    ; Initialize PCB
    mov edx, [next_pid]
    mov [eax], edx  ; Set PID
    inc dword [next_pid]
    
    mov byte [eax + 4], PROCESS_STATE_READY  ; State
    mov cl, [ebp + 16]  ; Priority parameter
    mov [eax + 5], cl   ; Priority
    
    ; Copy name (max 16 chars)
    push edi
    mov esi, [ebp + 8]   ; Name parameter
    lea edi, [eax + 8]   ; Name field in PCB
    mov ecx, 15
.copy_name:
    lodsb
    stosb
    test al, al
    jz .name_done
    loop .copy_name
.name_done:
    mov byte [edi], 0    ; Null terminate
    pop edi
    
    ; Set entry point
    mov edx, [ebp + 12]  ; Entry point parameter
    mov [eax + 48], edx  ; EIP field
    
    ; Set default time slice
    mov dword [eax + 64], DEFAULT_TIME_SLICE
    
    ; Initialize stack pointers (simplified)
    mov edx, eax
    add edx, 0x1000     ; Stack at PCB + 4KB
    mov [eax + 28], edx ; ESP field
    mov [eax + 24], edx ; EBP field
    
    ; Add to ready queue
    push eax
    call ready_queue_insert
    add esp, 4
    
    ; Print creation message
    push proc_created_msg
    call print_message
    add esp, 4
    
    push dword [ebp + 8]  ; Process name
    call print_message
    add esp, 4
    
    push pid_msg
    call print_message
    add esp, 4
    
    pop eax  ; Restore PCB address
    push dword [eax]  ; Push PID
    call print_hex_value
    add esp, 4
    
    push closing_paren
    call print_message
    add esp, 4
    
    mov eax, [eax]  ; Return PID
    
.done:
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

; Insert process into ready queue
ready_queue_insert:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    
    mov esi, [ebp + 8]  ; Process PCB pointer
    
    ; Clear next/prev pointers
    mov dword [esi + 80], 0  ; next
    mov dword [esi + 84], 0  ; prev
    
    ; If queue is empty
    cmp dword [ready_queue_head], 0
    jne .not_empty
    
    mov [ready_queue_head], esi
    mov [ready_queue_tail], esi
    jmp .done
    
.not_empty:
    ; Insert at tail for now (simple FIFO)
    mov ebx, [ready_queue_tail]
    mov [ebx + 80], esi  ; tail->next = new
    mov [esi + 84], ebx  ; new->prev = tail
    mov [ready_queue_tail], esi
    
.done:
    pop esi
    pop ebx
    pop ebp
    ret

; Simple round-robin scheduler
global schedule
schedule:
    pushad
    
    ; Check if we have processes
    cmp dword [ready_queue_head], 0
    je .use_idle
    
    ; Get current process
    mov esi, [current_process_ptr]
    test esi, esi
    jz .get_next
    
    ; Decrement time slice
    dec dword [esi + 64]
    jnz .no_switch  ; Still has time
    
    ; Time slice expired, move to ready queue
    mov byte [esi + 4], PROCESS_STATE_READY
    push esi
    call ready_queue_insert
    add esp, 4
    
.get_next:
    ; Get next from ready queue
    mov edi, [ready_queue_head]
    test edi, edi
    jz .use_idle
    
    ; Remove from queue
    mov eax, [edi + 80]  ; next
    mov [ready_queue_head], eax
    test eax, eax
    jz .empty_queue
    mov dword [eax + 84], 0  ; next->prev = NULL
    jmp .switch_to
    
.empty_queue:
    mov dword [ready_queue_tail], 0
    jmp .switch_to
    
.use_idle:
    mov edi, [idle_process_ptr]
    
.switch_to:
    ; Set as running
    mov byte [edi + 4], PROCESS_STATE_RUNNING
    mov dword [edi + 64], DEFAULT_TIME_SLICE  ; Reset time slice
    
    ; Perform context switch if different process
    cmp edi, [current_process_ptr]
    je .no_switch
    
    mov [current_process_ptr], edi
    
    ; Simple context switch (registers only for now)
    mov esp, [edi + 28]  ; Load ESP
    mov ebp, [edi + 24]  ; Load EBP
    mov eax, [edi + 48]  ; Load EIP
    jmp eax
    
.no_switch:
    popad
    ret

; Print message helper
print_message:
    push ebp
    mov ebp, esp
    push esi
    push edi
    push eax
    
    mov esi, [ebp + 8]
    
    ; Find current cursor position (simplified - use fixed line)
    mov edi, VGA_BUFFER + (23 * 80 * 2)  ; Line 23
    
.loop:
    lodsb
    test al, al
    jz .done
    
    mov [edi], al
    mov byte [edi + 1], 0x07  ; White on black
    add edi, 2
    
    jmp .loop
    
.done:
    pop eax
    pop edi
    pop esi
    pop ebp
    ret

; print_hex_value is defined in context_switch.inc