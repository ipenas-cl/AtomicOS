.section __TEXT,__text
.globl _main

_main:
    pushq %rbp
    movq %rsp, %rbp
    
    movq $1, %rdi
    leaq header(%rip), %rsi
    movq $header_len, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq $1, %rdi
    leaq code(%rip), %rsi
    movq $code_len, %rdx
    movq $0x2000004, %rax
    syscall
    
    xorq %rax, %rax
    popq %rbp
    ret

.section __DATA,__data
header:
    .ascii "# Tempo Native Compiler v2.0\n"
    .ascii "# 100% Assembly for macOS\n\n"
.set header_len, . - header

code:
    .ascii ".section __TEXT,__text\n"
    .ascii ".globl _main\n"
    .ascii "_main:\n"
    .ascii "    movq $42, %rax\n"
    .ascii "    ret\n"
.set code_len, . - code