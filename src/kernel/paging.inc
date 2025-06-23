; AtomicOS Paging System
; Implements basic memory protection with deterministic page allocation

section .data

; Paging constants
VM_PAGE_SIZE        equ 4096
VM_PAGE_SHIFT       equ 12
PDE_COUNT          equ 1024
PTE_COUNT          equ 1024
PAGE_PRESENT       equ 0x01
PAGE_WRITE         equ 0x02
PAGE_USER          equ 0x04
PAGE_ACCESSED      equ 0x20
PAGE_DIRTY         equ 0x40

; Memory layout
KERNEL_VIRT_BASE   equ 0x00000000   ; Identity mapped for simplicity
KERNEL_PHYS_BASE   equ 0x00000000
USER_VIRT_BASE     equ 0x40000000   ; User space starts at 1GB
USER_HEAP_BASE     equ 0x50000000   ; User heap at 1.25GB
KERNEL_HEAP_BASE   equ 0x20000000   ; Kernel heap at 512MB

; Page directory and table locations
PAGE_DIR_BASE      equ 0x100000     ; Page directory at 1MB
PAGE_TABLE_BASE    equ 0x101000     ; First page table
PAGE_BITMAP_BASE   equ 0x200000     ; Physical page bitmap at 2MB

; Memory size detection
MEMORY_SIZE_MB     equ 16           ; Assume 16MB for now
TOTAL_PAGES        equ (MEMORY_SIZE_MB * 1024 * 1024) / VM_PAGE_SIZE
BITMAP_SIZE        equ TOTAL_PAGES / 8

; Statistics
pages_allocated:    dd 0
pages_freed:       dd 0
page_faults:       dd 0
tlb_flushes:       dd 0

; Messages
paging_init_msg:   db 'Paging initialized: ', 0
mb_msg:           db 'MB available', 0
page_fault_msg:   db 'Page fault at: ', 0

section .text

; Initialize paging system
global init_paging
init_paging:
    push ebp
    mov ebp, esp
    pushad
    
    ; Clear page directory
    mov edi, PAGE_DIR_BASE
    mov ecx, PDE_COUNT
    xor eax, eax
    rep stosd
    
    ; Clear physical page bitmap
    mov edi, PAGE_BITMAP_BASE
    mov ecx, BITMAP_SIZE / 4
    xor eax, eax
    rep stosd
    
    ; Mark kernel pages as used (0-6MB to include deterministic pool)
    mov edi, PAGE_BITMAP_BASE
    mov ecx, 1536 / 8        ; 1536 pages = 6MB, 8 pages per byte
    mov al, 0xFF
    rep stosb
    
    ; Setup identity mapping for kernel (0-6MB)
    call setup_kernel_mapping
    
    ; Setup page fault handler - will be done later when IDT is initialized
    ; mov eax, 14              ; Page fault vector
    ; mov ebx, page_fault_handler
    ; call set_idt_entry
    
    ; Enable paging
    mov eax, PAGE_DIR_BASE
    mov cr3, eax            ; Load page directory base
    
    mov eax, cr0
    or eax, 0x80000000      ; Set PG bit
    mov cr0, eax            ; Enable paging!
    
    ; Print initialization message - commented out to avoid dependency issues
    ; push paging_init_msg
    ; call print_string
    ; add esp, 4
    ; 
    ; push MEMORY_SIZE_MB
    ; call print_decimal
    ; add esp, 4
    ; 
    ; push mb_msg
    ; call print_string
    ; add esp, 4
    ; call print_newline
    
    popad
    pop ebp
    ret

; Setup kernel identity mapping (0-6MB)
setup_kernel_mapping:
    pushad
    
    ; We need 2 page tables for 6MB (each table maps 4MB)
    ; First page table: 0-4MB
    mov edi, PAGE_TABLE_BASE
    xor esi, esi            ; Physical address starts at 0
    mov ecx, 1024           ; 1024 pages = 4MB
    
.map_first_table:
    mov eax, esi
    or eax, PAGE_PRESENT | PAGE_WRITE  ; Kernel pages are writable
    mov [edi], eax
    
    add esi, VM_PAGE_SIZE
    add edi, 4
    loop .map_first_table
    
    ; Add first page table to directory
    mov eax, PAGE_TABLE_BASE
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [PAGE_DIR_BASE], eax
    
    ; Second page table: 4-6MB (only need 512 pages)
    mov edi, PAGE_TABLE_BASE + 0x1000  ; Second page table
    ; esi already points to 4MB
    mov ecx, 512            ; 512 pages = 2MB
    
.map_second_table:
    mov eax, esi
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [edi], eax
    
    add esi, VM_PAGE_SIZE
    add edi, 4
    loop .map_second_table
    
    ; Add second page table to directory
    mov eax, PAGE_TABLE_BASE + 0x1000
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [PAGE_DIR_BASE + 4], eax  ; Second PDE
    
    popad
    ret

; Allocate physical page
; Output: EAX = physical address or 0 on failure
global alloc_physical_page
alloc_physical_page:
    push ebx
    push ecx
    push edx
    push edi
    
    ; Search bitmap for free page
    mov edi, PAGE_BITMAP_BASE
    mov ecx, BITMAP_SIZE
    
.search_byte:
    mov al, [edi]
    cmp al, 0xFF            ; All pages used?
    jne .found_byte
    
    inc edi
    loop .search_byte
    
    ; No free pages
    xor eax, eax
    jmp .done
    
.found_byte:
    ; Find which bit is free
    xor ebx, ebx            ; Bit counter
    
.search_bit:
    bt eax, ebx
    jnc .found_bit
    
    inc ebx
    cmp ebx, 8
    jl .search_bit
    
.found_bit:
    ; Mark page as used
    bts [edi], ebx
    
    ; Calculate page number
    sub edi, PAGE_BITMAP_BASE
    shl edi, 3              ; Multiply by 8
    add edi, ebx            ; Add bit offset
    
    ; Convert to physical address
    mov eax, edi
    shl eax, VM_PAGE_SHIFT
    
    inc dword [pages_allocated]
    
.done:
    pop edi
    pop edx
    pop ecx
    pop ebx
    ret

; Free physical page
; Input: EAX = physical address
global free_physical_page
free_physical_page:
    push eax
    push ebx
    push edx
    
    ; Convert address to page number
    shr eax, VM_PAGE_SHIFT
    
    ; Calculate bitmap byte and bit
    mov ebx, eax
    shr ebx, 3              ; Byte offset
    and eax, 7              ; Bit offset
    
    ; Clear bit in bitmap
    add ebx, PAGE_BITMAP_BASE
    btr [ebx], eax
    
    inc dword [pages_freed]
    
    pop edx
    pop ebx
    pop eax
    ret

; Map virtual page to physical page
; Input: EAX = virtual address, EBX = physical address, ECX = flags
global map_page
map_page:
    pushad
    
    ; Calculate page directory index
    mov edi, eax
    shr edi, 22             ; Top 10 bits
    
    ; Calculate page table index
    mov esi, eax
    shr esi, 12
    and esi, 0x3FF          ; Middle 10 bits
    
    ; Check if page table exists
    mov edx, [PAGE_DIR_BASE + edi * 4]
    test edx, PAGE_PRESENT
    jnz .table_exists
    
    ; Allocate new page table
    call alloc_physical_page
    test eax, eax
    jz .error
    
    ; Clear new page table
    push edi
    mov edi, eax
    mov ecx, PTE_COUNT
    push eax
    xor eax, eax
    rep stosd
    pop eax
    pop edi
    
    ; Add to page directory
    or eax, PAGE_PRESENT | PAGE_WRITE | PAGE_USER
    mov [PAGE_DIR_BASE + edi * 4], eax
    mov edx, eax
    
.table_exists:
    ; Get page table address
    and edx, 0xFFFFF000
    
    ; Add mapping to page table
    mov eax, ebx
    or eax, ecx             ; Add flags
    mov [edx + esi * 4], eax
    
    ; Flush TLB entry
    mov eax, [esp + 32]     ; Original virtual address
    invlpg [eax]
    inc dword [tlb_flushes]
    
.error:
    popad
    ret

; Unmap virtual page
; Input: EAX = virtual address
global unmap_page
unmap_page:
    pushad
    
    ; Calculate page directory index
    mov edi, eax
    shr edi, 22
    
    ; Calculate page table index
    mov esi, eax
    shr esi, 12
    and esi, 0x3FF
    
    ; Check if page table exists
    mov edx, [PAGE_DIR_BASE + edi * 4]
    test edx, PAGE_PRESENT
    jz .done
    
    ; Get page table address
    and edx, 0xFFFFF000
    
    ; Clear mapping
    mov dword [edx + esi * 4], 0
    
    ; Flush TLB
    invlpg [eax]
    inc dword [tlb_flushes]
    
.done:
    popad
    ret

; Page fault handler
page_fault_handler:
    pushad
    
    ; Get fault address from CR2
    mov eax, cr2
    push eax
    
    ; Get error code
    mov ebx, [esp + 36]     ; Error code pushed by CPU
    
    inc dword [page_faults]
    
    ; Print page fault message - commented out to avoid dependency
    ; push page_fault_msg
    ; call print_string
    ; add esp, 4
    ; 
    ; push eax
    ; call print_hex
    ; add esp, 4
    ; call print_newline
    
    ; Check if it's a protection violation
    test ebx, PAGE_PRESENT
    jnz .protection_violation
    
    ; Page not present - try to handle
    call handle_page_not_present
    test eax, eax
    jz .fatal
    
    ; Successfully handled
    pop eax
    popad
    add esp, 4              ; Remove error code
    iretd
    
.protection_violation:
.fatal:
    ; Fatal page fault - halt system
    cli
    hlt
    jmp $

; Handle page not present fault
; Input: Stack contains fault address
; Output: EAX = 1 if handled, 0 if fatal
handle_page_not_present:
    push ebp
    mov ebp, esp
    push ebx
    push ecx
    
    mov eax, [ebp + 8]      ; Fault address
    
    ; Check if it's in valid user space
    cmp eax, USER_VIRT_BASE
    jl .kernel_fault
    cmp eax, 0xC0000000     ; 3GB limit
    jge .invalid
    
    ; Allocate physical page
    call alloc_physical_page
    test eax, eax
    jz .no_memory
    
    ; Map the page
    mov ebx, eax            ; Physical address
    mov eax, [ebp + 8]      ; Virtual address
    and eax, 0xFFFFF000     ; Align to page
    mov ecx, PAGE_PRESENT | PAGE_WRITE | PAGE_USER
    call map_page
    
    mov eax, 1              ; Success
    jmp .done
    
.kernel_fault:
    ; For now, kernel faults are fatal
.invalid:
.no_memory:
    xor eax, eax            ; Failure
    
.done:
    pop ecx
    pop ebx
    pop ebp
    ret 4

; Allocate virtual memory region
; Input: EAX = size in bytes, EBX = flags
; Output: EAX = virtual address or 0
global vmalloc
vmalloc:
    push ebx
    push ecx
    push edx
    push esi
    
    ; Round size up to page boundary
    add eax, VM_PAGE_SIZE - 1
    and eax, ~(VM_PAGE_SIZE - 1)
    mov esi, eax            ; Save size
    
    ; For now, use simple linear allocation
    mov edx, [next_user_vaddr]
    test edx, edx
    jnz .have_addr
    mov edx, USER_HEAP_BASE
    
.have_addr:
    mov ecx, esi
    shr ecx, VM_PAGE_SHIFT     ; Number of pages
    
    ; Check if we have room
    mov eax, edx
    add eax, esi
    cmp eax, 0xC0000000     ; 3GB limit
    jae .no_space
    
    ; Allocate and map pages
    push edx                ; Save start address
    
.alloc_loop:
    push ecx
    
    call alloc_physical_page
    test eax, eax
    jz .alloc_failed
    
    mov ebx, eax            ; Physical address
    mov eax, edx            ; Virtual address
    mov ecx, PAGE_PRESENT | PAGE_WRITE | PAGE_USER
    call map_page
    
    add edx, VM_PAGE_SIZE
    pop ecx
    loop .alloc_loop
    
    ; Update next address
    mov [next_user_vaddr], edx
    
    pop eax                 ; Return start address
    jmp .done
    
.alloc_failed:
    pop ecx
    pop edx
    ; TODO: Free already allocated pages
    
.no_space:
    xor eax, eax
    
.done:
    pop esi
    pop edx
    pop ecx
    pop ebx
    ret

; Free virtual memory region
; Input: EAX = virtual address, EBX = size
global vfree
vfree:
    pushad
    
    ; Round size up to page boundary
    add ebx, VM_PAGE_SIZE - 1
    and ebx, ~(VM_PAGE_SIZE - 1)
    
    shr ebx, VM_PAGE_SHIFT     ; Number of pages
    
.free_loop:
    push eax
    push ebx
    
    ; Get physical address before unmapping
    call get_physical_addr
    mov ebx, eax
    
    pop ebx
    pop eax
    
    push ebx
    test ebx, ebx
    jz .skip_free
    
    ; Free physical page
    push eax
    mov eax, ebx
    call free_physical_page
    pop eax
    
.skip_free:
    ; Unmap the page
    push eax
    call unmap_page
    pop eax
    
    add eax, VM_PAGE_SIZE
    pop ebx
    dec ebx
    jnz .free_loop
    
    popad
    ret

; Get physical address for virtual address
; Input: EAX = virtual address
; Output: EAX = physical address or 0
get_physical_addr:
    push ebx
    push ecx
    push edx
    
    mov ebx, eax
    
    ; Get page directory index
    shr eax, 22
    
    ; Get page directory entry
    mov edx, [PAGE_DIR_BASE + eax * 4]
    test edx, PAGE_PRESENT
    jz .not_mapped
    
    ; Get page table address
    and edx, 0xFFFFF000
    
    ; Get page table index
    mov eax, ebx
    shr eax, 12
    and eax, 0x3FF
    
    ; Get page table entry
    mov eax, [edx + eax * 4]
    test eax, PAGE_PRESENT
    jz .not_mapped
    
    ; Get physical page address
    and eax, 0xFFFFF000
    
    ; Add page offset
    mov ecx, ebx
    and ecx, 0xFFF
    or eax, ecx
    
    jmp .done
    
.not_mapped:
    xor eax, eax
    
.done:
    pop edx
    pop ecx
    pop ebx
    ret

; Data
next_user_vaddr: dd 0

; External functions
; These are already defined in other modules