; AtomicOS 64-bit kernel entry. The bootloader (boot64.asm) loads this image
; at 0x10000 and jumps here in long mode with identity-mapped low memory.
; Everything after kmain is Copihue code compiled with `copihuec --freestanding`.
[BITS 64]
[ORG 0x10000]

section .text
kernel_entry:
    mov rsp, 0x90000
    xor rbp, rbp
    ; zero .bss (Copihue globals declared without an initializer)
    mov rdi, kernel_bss_start
    mov rcx, kernel_bss_end
    sub rcx, rdi
    xor eax, eax
    rep stosb
    call kmain
.halt:
    cli
    hlt
    jmp .halt

section .bss
kernel_bss_start:

%include "kernel_copihue.asm"

section .bss
kernel_bss_end:
