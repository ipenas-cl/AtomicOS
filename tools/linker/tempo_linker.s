; Tempo Linker - 100% Assembly
; Crea ejecutables Mach-O para macOS sin usar NADA externo
; Por Ignacio Peña Sepúlveda

.section __TEXT,__text
.globl _main

; Mach-O File Format Constants
.set MH_MAGIC_64, 0xfeedfacf
.set CPU_TYPE_X86_64, 0x01000007
.set CPU_SUBTYPE_X86_64_ALL, 0x00000003
.set MH_EXECUTE, 0x2
.set MH_NOUNDEFS, 0x1
.set MH_PIE, 0x200000
.set LC_SEGMENT_64, 0x19
.set LC_UNIXTHREAD, 0x5
.set VM_PROT_READ, 0x1
.set VM_PROT_WRITE, 0x2
.set VM_PROT_EXECUTE, 0x4

_main:
    pushq %rbp
    movq %rsp, %rbp
    
    ; Abrir archivo de salida
    movq $0x2000005, %rax      ; open
    leaq output_file(%rip), %rdi
    movq $0x601, %rsi          ; O_CREAT | O_WRONLY | O_TRUNC
    movq $0755, %rdx           ; permisos
    syscall
    
    movq %rax, %r12            ; guardar fd
    
    ; Escribir Mach-O header
    movq %r12, %rdi
    leaq mach_header(%rip), %rsi
    movq $32, %rdx
    movq $0x2000004, %rax      ; write
    syscall
    
    ; Escribir Load Commands
    movq %r12, %rdi
    leaq load_commands(%rip), %rsi
    movq $load_commands_size, %rdx
    movq $0x2000004, %rax
    syscall
    
    ; Padding hasta 0x1000 (donde empieza __TEXT)
    movq %r12, %rdi
    leaq padding(%rip), %rsi
    movq $padding_size, %rdx
    movq $0x2000004, %rax
    syscall
    
    ; Escribir código ejecutable
    movq %r12, %rdi
    leaq executable_code(%rip), %rsi
    movq $code_size, %rdx
    movq $0x2000004, %rax
    syscall
    
    ; Cerrar archivo
    movq %r12, %rdi
    movq $0x2000006, %rax      ; close
    syscall
    
    ; Hacer ejecutable con chmod
    movq $0x200000f, %rax      ; chmod
    leaq output_file(%rip), %rdi
    movq $0755, %rsi
    syscall
    
    ; Imprimir éxito
    movq $1, %rdi
    leaq success_msg(%rip), %rsi
    movq $success_len, %rdx
    movq $0x2000004, %rax
    syscall
    
    xorq %rax, %rax
    popq %rbp
    ret

.section __DATA,__data

output_file:
    .asciz "tempo_program"

success_msg:
    .ascii "Tempo Linker: Created tempo_program\n"
    .ascii "100% Assembly - No clang, No ld\n"
    .ascii "Run: ./tempo_program\n"
.set success_len, . - success_msg

; Mach-O Header (64-bit)
.align 3
mach_header:
    .long MH_MAGIC_64          ; magic
    .long CPU_TYPE_X86_64      ; cputype
    .long CPU_SUBTYPE_X86_64_ALL ; cpusubtype
    .long MH_EXECUTE           ; filetype
    .long 3                    ; ncmds (número de load commands)
    .long load_commands_size   ; sizeofcmds
    .long MH_NOUNDEFS | MH_PIE ; flags
    .long 0                    ; reserved

; Load Commands
load_commands:

; LC_SEGMENT_64 para __PAGEZERO
lc_pagezero:
    .long LC_SEGMENT_64        ; cmd
    .long 72                   ; cmdsize
    .ascii "__PAGEZERO\0\0\0\0\0\0" ; segname (16 bytes)
    .quad 0                    ; vmaddr
    .quad 0x100000000          ; vmsize
    .quad 0                    ; fileoff
    .quad 0                    ; filesize
    .long 0                    ; maxprot
    .long 0                    ; initprot
    .long 0                    ; nsects
    .long 0                    ; flags

; LC_SEGMENT_64 para __TEXT
lc_text:
    .long LC_SEGMENT_64        ; cmd
    .long 152                  ; cmdsize
    .ascii "__TEXT\0\0\0\0\0\0\0\0\0\0" ; segname
    .quad 0x100000000          ; vmaddr
    .quad 0x1000               ; vmsize
    .quad 0x1000               ; fileoff
    .quad code_size            ; filesize
    .long VM_PROT_READ | VM_PROT_EXECUTE ; maxprot
    .long VM_PROT_READ | VM_PROT_EXECUTE ; initprot
    .long 1                    ; nsects
    .long 0                    ; flags

; Section header para __text
    .ascii "__text\0\0\0\0\0\0\0\0\0\0" ; sectname
    .ascii "__TEXT\0\0\0\0\0\0\0\0\0\0" ; segname
    .quad 0x100001000          ; addr
    .quad code_size            ; size
    .long 0x1000               ; offset
    .long 0                    ; align
    .long 0                    ; reloff
    .long 0                    ; nreloc
    .long 0x80000400           ; flags
    .long 0                    ; reserved1
    .long 0                    ; reserved2
    .long 0                    ; reserved3

; LC_UNIXTHREAD
lc_unixthread:
    .long LC_UNIXTHREAD        ; cmd
    .long 184                  ; cmdsize
    .long 4                    ; flavor (x86_THREAD_STATE64)
    .long 42                   ; count (size of thread state / 4)
    
    ; Thread state (registros iniciales)
    .quad 0, 0, 0, 0          ; rax, rbx, rcx, rdx
    .quad 0, 0, 0, 0          ; rdi, rsi, rbp, rsp
    .quad 0, 0, 0, 0          ; r8-r11
    .quad 0, 0, 0, 0          ; r12-r15
    .quad 0x100001000         ; rip (entry point)
    .quad 0, 0, 0, 0          ; rflags, cs, fs, gs

.set load_commands_size, . - load_commands

; Padding hasta 0x1000
.align 12
padding:
    .fill 0x1000 - (. - mach_header), 1, 0
.set padding_size, . - padding

; Código ejecutable que será insertado
executable_code:
    ; Este es el código que nuestro compilador generó
    ; Por ahora, return 42
    movq $42, %rax
    movq $0x2000001, %rdi      ; exit syscall
    movq %rax, %rdi            ; exit code
    syscall
.set code_size, . - executable_code