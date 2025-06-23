.section __TEXT,__text
.globl _main

_main:
    pushq %rbp
    movq %rsp, %rbp
    subq $256, %rsp
    
    leaq input(%rip), %r12
    xorq %r13, %r13
    
    call emit_header
    call parse_return
    call emit_footer
    
    xorq %rax, %rax
    movq %rbp, %rsp
    popq %rbp
    ret

parse_return:
    pushq %rbp
    movq %rsp, %rbp
    
skip_ws_1:
    movb (%r12), %al
    cmpb $32, %al
    jne check_r
    incq %r12
    jmp skip_ws_1
    
check_r:
    cmpb $114, %al
    jne parse_error
    incq %r12
    incq %r12
    incq %r12
    incq %r12
    incq %r12
    incq %r12
    
skip_ws_2:
    movb (%r12), %al
    cmpb $32, %al
    jne parse_num
    incq %r12
    jmp skip_ws_2
    
parse_num:
    xorq %rax, %rax
digit_loop:
    movb (%r12), %cl
    cmpb $48, %cl
    jb emit_value
    cmpb $57, %cl
    ja emit_value
    
    imulq $10, %rax
    subb $48, %cl
    movzbq %cl, %rcx
    addq %rcx, %rax
    incq %r12
    jmp digit_loop
    
emit_value:
    movq %rax, %r13
    
    movq $1, %rdi
    leaq ret_prefix(%rip), %rsi
    movq $ret_prefix_len, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq %r13, %rdi
    call print_num
    
    movq $1, %rdi
    leaq ret_suffix(%rip), %rsi
    movq $ret_suffix_len, %rdx
    movq $0x2000004, %rax
    syscall
    
    popq %rbp
    ret

parse_error:
    movq $1, %rdi
    movq $0x2000001, %rax
    syscall

print_num:
    pushq %rbp
    movq %rsp, %rbp
    subq $32, %rsp
    
    testq %rdi, %rdi
    jnz .not_zero
    
    movb $48, -1(%rbp)
    movq $1, %rdi
    leaq -1(%rbp), %rsi
    movq $1, %rdx
    movq $0x2000004, %rax
    syscall
    jmp .print_done
    
.not_zero:
    movq %rdi, %rax
    leaq -1(%rbp), %rdi
    movb $0, (%rdi)
    
.convert_loop:
    testq %rax, %rax
    jz .do_print
    
    xorq %rdx, %rdx
    movq $10, %rcx
    divq %rcx
    
    addb $48, %dl
    decq %rdi
    movb %dl, (%rdi)
    jmp .convert_loop
    
.do_print:
    leaq -1(%rbp), %rax
    subq %rdi, %rax
    movq %rax, %rdx
    
    movq %rdi, %rsi
    movq $1, %rdi
    movq $0x2000004, %rax
    syscall
    
.print_done:
    movq %rbp, %rsp
    popq %rbp
    ret

emit_header:
    movq $1, %rdi
    leaq asm_header(%rip), %rsi
    movq $asm_header_len, %rdx
    movq $0x2000004, %rax
    syscall
    ret

emit_footer:
    movq $1, %rdi
    leaq asm_footer(%rip), %rsi
    movq $asm_footer_len, %rdx
    movq $0x2000004, %rax
    syscall
    ret

.section __DATA,__data
input:
    .asciz "return 69;"

asm_header:
    .ascii ".section __TEXT,__text\n"
    .ascii ".globl _main\n"
    .ascii "_main:\n"
    .ascii "    pushq %rbp\n"
    .ascii "    movq %rsp, %rbp\n"
.set asm_header_len, . - asm_header

ret_prefix:
    .ascii "    movq $"
.set ret_prefix_len, . - ret_prefix

ret_suffix:
    .ascii ", %rax\n"
.set ret_suffix_len, . - ret_suffix

asm_footer:
    .ascii "    popq %rbp\n"
    .ascii "    ret\n"
.set asm_footer_len, . - asm_footer