; AtomicOS IPC Assembly Stubs
; Temporary stubs until proper linking is implemented

section .text

; IPC function stubs that return mock values
global ipc_create_channel
ipc_create_channel:
    mov eax, 1          ; Return channel ID 1
    ret

global ipc_send
ipc_send:
    xor eax, eax        ; Return success (0)
    ret

global ipc_receive
ipc_receive:
    mov eax, -3         ; Return "would block" error
    ret

global ipc_close_channel
ipc_close_channel:
    xor eax, eax        ; Return success (0)
    ret

global ipc_get_stats
ipc_get_stats:
    xor eax, eax        ; Return success (0)
    ret