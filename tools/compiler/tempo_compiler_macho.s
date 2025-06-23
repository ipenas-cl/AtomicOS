; Tempo Compiler + Linker Todo-en-Uno
; Genera ejecutables Mach-O directamente
; 100% Assembly - Ignacio Peña Sepúlveda

.section __TEXT,__text
.globl _main

; Constantes Mach-O
.set MH_MAGIC_64, 0xfeedfacf
.set CPU_TYPE_X86_64, 0x01000007
.set CPU_SUBTYPE_X86_64_ALL, 0x3
.set MH_EXECUTE, 0x2
.set LC_SEGMENT_64, 0x19
.set LC_UNIXTHREAD, 0x5

_main:
    pushq %rbp
    movq %rsp, %rbp
    subq $512, %rsp
    
    ; Parsear "return N;"
    leaq input(%rip), %r12
    call parse_return_value
    movq %rax, %r13            ; r13 = valor a retornar
    
    ; Crear archivo ejecutable
    movq $0x2000005, %rax      ; open
    leaq output_name(%rip), %rdi
    movq $0x601, %rsi          ; O_CREAT|O_WRONLY|O_TRUNC  
    movq $0755, %rdx
    syscall
    movq %rax, %r14            ; r14 = fd
    
    ; Generar y escribir ejecutable Mach-O
    call write_macho_header
    call write_load_commands
    call write_padding
    call write_code_section
    
    ; Cerrar archivo
    movq %r14, %rdi
    movq $0x2000006, %rax      ; close
    syscall
    
    ; Hacer ejecutable
    movq $0x200000f, %rax      ; chmod
    leaq output_name(%rip), %rdi
    movq $0755, %rsi
    syscall
    
    ; Mensaje de éxito
    movq $1, %rdi
    leaq success(%rip), %rsi
    movq $success_len, %rdx
    movq $0x2000004, %rax
    syscall
    
    xorq %rax, %rax
    movq %rbp, %rsp
    popq %rbp
    ret

; Parser mínimo para "return N;"
parse_return_value:
    pushq %rbp
    movq %rsp, %rbp
    
    ; Buscar número después de "return"
    addq $7, %r12              ; skip "return "
    xorq %rax, %rax
.parse_digit:
    movb (%r12), %cl
    cmpb $48, %cl              ; '0'
    jb .done
    cmpb $57, %cl              ; '9'
    ja .done
    imulq $10, %rax
    subb $48, %cl
    movzbq %cl, %rcx
    addq %rcx, %rax
    incq %r12
    jmp .parse_digit
.done:
    popq %rbp
    ret

; Escribir Mach-O header
write_macho_header:
    pushq %rbp
    movq %rsp, %rbp
    subq $32, %rsp
    
    ; Llenar header
    movl $MH_MAGIC_64, -32(%rbp)
    movl $CPU_TYPE_X86_64, -28(%rbp)
    movl $CPU_SUBTYPE_X86_64_ALL, -24(%rbp)
    movl $MH_EXECUTE, -20(%rbp)
    movl $2, -16(%rbp)         ; ncmds
    movl $232, -12(%rbp)       ; sizeofcmds
    movl $0x200001, -8(%rbp)   ; flags
    movl $0, -4(%rbp)          ; reserved
    
    ; Escribir
    movq %r14, %rdi
    leaq -32(%rbp), %rsi
    movq $32, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq %rbp, %rsp
    popq %rbp
    ret

; Escribir load commands
write_load_commands:
    pushq %rbp
    movq %rsp, %rbp
    
    ; LC_SEGMENT_64 __TEXT
    movq %r14, %rdi
    leaq lc_text(%rip), %rsi
    movq $152, %rdx
    movq $0x2000004, %rax
    syscall
    
    ; LC_UNIXTHREAD
    movq %r14, %rdi
    leaq lc_thread(%rip), %rsi
    movq $80, %rdx
    movq $0x2000004, %rax
    syscall
    
    popq %rbp
    ret

; Padding hasta 0x1000
write_padding:
    pushq %rbp
    movq %rsp, %rbp
    subq $768, %rsp            ; buffer para padding
    
    ; Llenar con ceros
    movq $768, %rcx
    leaq -768(%rbp), %rdi
    xorq %rax, %rax
.fill:
    movb %al, (%rdi)
    incq %rdi
    loop .fill
    
    ; Escribir padding
    movq %r14, %rdi
    leaq -768(%rbp), %rsi
    movq $768, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq %rbp, %rsp
    popq %rbp
    ret

; Escribir sección de código
write_code_section:
    pushq %rbp
    movq %rsp, %rbp
    subq $32, %rsp
    
    ; Generar código para "return N"
    ; movq $N, %rdi
    movb $0x48, -32(%rbp)      ; REX.W
    movb $0xc7, -31(%rbp)      ; MOV
    movb $0xc7, -30(%rbp)      ; ModRM
    movl %r13d, -29(%rbp)      ; valor inmediato
    
    ; movq $0x2000001, %rax (exit)
    movb $0x48, -25(%rbp)
    movb $0xc7, -24(%rbp)
    movb $0xc0, -23(%rbp)
    movl $0x2000001, -22(%rbp)
    
    ; syscall
    movb $0x0f, -18(%rbp)
    movb $0x05, -17(%rbp)
    
    ; Escribir código
    movq %r14, %rdi
    leaq -32(%rbp), %rsi
    movq $16, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq %rbp, %rsp
    popq %rbp
    ret

.section __DATA,__data

input:
    .asciz "return 69;"

output_name:
    .asciz "tempo_output"

success:
    .ascii "=== Tempo Compiler + Linker ===\n"
    .ascii "Creado: tempo_output\n"
    .ascii "100% Assembly - Sin clang, sin ld\n"
    .ascii "Ejecuta: ./tempo_output\n"
.set success_len, . - success

; Load Command templates
.align 3
lc_text:
    .long 0x19                 ; LC_SEGMENT_64
    .long 152                  ; cmdsize
    .ascii "__TEXT\0\0\0\0\0\0\0\0\0\0"
    .quad 0x100000000          ; vmaddr
    .quad 0x1000               ; vmsize  
    .quad 0x1000               ; fileoff
    .quad 16                   ; filesize
    .long 7                    ; maxprot (rwx)
    .long 5                    ; initprot (r-x)
    .long 1                    ; nsects
    .long 0                    ; flags
    ; Section
    .ascii "__text\0\0\0\0\0\0\0\0\0\0"
    .ascii "__TEXT\0\0\0\0\0\0\0\0\0\0"
    .quad 0x100001000          ; addr
    .quad 16                   ; size
    .long 0x1000               ; offset
    .long 0                    ; align
    .long 0                    ; reloff
    .long 0                    ; nreloc
    .long 0x80000400           ; flags
    .long 0, 0, 0              ; reserved

lc_thread:
    .long 0x5                  ; LC_UNIXTHREAD
    .long 80                   ; cmdsize
    .long 4                    ; flavor
    .long 16                   ; count
    .quad 0, 0, 0, 0          ; registros vacíos
    .quad 0x100001000         ; rip (entry point)