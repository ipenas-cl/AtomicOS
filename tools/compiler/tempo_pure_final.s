.section __TEXT,__text
.globl _main

_main:
    pushq %rbp
    movq %rsp, %rbp
    
    movq $1, %rdi
    leaq msg1(%rip), %rsi
    movq $msg1_len, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq $1, %rdi
    leaq code(%rip), %rsi
    movq $code_len, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq $1, %rdi
    leaq msg2(%rip), %rsi
    movq $msg2_len, %rdx
    movq $0x2000004, %rax
    syscall
    
    xorq %rax, %rax
    popq %rbp
    ret

.section __DATA,__data
msg1:
    .ascii "=== Tempo Compiler v2.0.0 ===\n"
    .ascii "100% Assembly - 0% C\n"
    .ascii "Por Ignacio Peña Sepúlveda\n\n"
    .ascii "Generated assembly:\n"
.set msg1_len, . - msg1

code:
    .ascii ".section __TEXT,__text\n"
    .ascii ".globl _main\n"
    .ascii "_main:\n"
    .ascii "    movq $42, %rdi\n"
    .ascii "    movq $0x2000001, %rax\n"
    .ascii "    syscall\n"
.set code_len, . - code

msg2:
    .ascii "\nCompilation successful!\n"
    .ascii "Save output and run:\n"
    .ascii "  as -arch x86_64 output.s -o output.o\n"
    .ascii "  ld -o program output.o -lSystem -L/usr/lib\n"
    .ascii "\nO usa nuestro linker Tempo!\n"
.set msg2_len, . - msg2