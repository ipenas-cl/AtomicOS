; AtomicOS Filesystem Assembly Stubs
; Temporary stubs until proper linking is implemented

section .text

; Filesystem function stubs
global fs_open
fs_open:
    mov eax, -1         ; Return error (not implemented)
    ret

global fs_close
fs_close:
    xor eax, eax        ; Return success
    ret

global fs_read
fs_read:
    xor eax, eax        ; Return 0 bytes read
    ret

global fs_write
fs_write:
    mov eax, [esp+12]   ; Return size parameter (pretend we wrote it all)
    ret

global fs_create
fs_create:
    xor eax, eax        ; Return success
    ret

global fs_delete
fs_delete:
    xor eax, eax        ; Return success
    ret

global fs_stat
fs_stat:
    mov eax, -1         ; Return error
    ret