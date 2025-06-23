.section __TEXT,__text
.globl _main

_main:
    pushq %rbp
    movq %rsp, %rbp
    subq $0x2000, %rsp
    
    movq %rdi, %r12
    movq %rsi, %r13
    
    cmpq $2, %r12
    jl usage_error
    
    movq 8(%r13), %rdi
    call open_input_file
    movq %rax, %r14
    
    call create_output_file
    movq %rax, %r15
    
    call write_mach_header
    call write_segments
    call write_sections
    call write_load_commands
    call write_link_edit
    call copy_text_section
    
    movq %r14, %rdi
    call close_file
    
    movq %r15, %rdi
    call close_file
    
    call make_executable
    
    movq $1, %rdi
    leaq success(%rip), %rsi
    movq $success_len, %rdx
    movq $0x2000004, %rax
    syscall
    
    xorq %rax, %rax
    movq %rbp, %rsp
    popq %rbp
    ret

usage_error:
    movq $1, %rdi
    leaq usage(%rip), %rsi
    movq $usage_len, %rdx
    movq $0x2000004, %rax
    syscall
    movq $1, %rax
    movq %rbp, %rsp
    popq %rbp
    ret

open_input_file:
    pushq %rbp
    movq %rsp, %rbp
    movq $0x2000005, %rax
    movq $0, %rsi
    movq $0, %rdx
    syscall
    popq %rbp
    ret

create_output_file:
    pushq %rbp
    movq %rsp, %rbp
    movq $0x2000005, %rax
    leaq output_name(%rip), %rdi
    movq $0x601, %rsi
    movq $0755, %rdx
    syscall
    popq %rbp
    ret

close_file:
    pushq %rbp
    movq %rsp, %rbp
    movq $0x2000006, %rax
    syscall
    popq %rbp
    ret

make_executable:
    pushq %rbp
    movq %rsp, %rbp
    movq $0x200000f, %rax
    leaq output_name(%rip), %rdi
    movq $0755, %rsi
    syscall
    popq %rbp
    ret

write_mach_header:
    pushq %rbp
    movq %rsp, %rbp
    
    leaq -32(%rbp), %rdi
    
    movl $0xfeedfacf, (%rdi)
    movl $0x01000007, 4(%rdi)
    movl $0x00000003, 8(%rdi)
    movl $0x2, 12(%rdi)
    movl $15, 16(%rdi)
    movl $1880, 20(%rdi)
    movl $0x00200085, 24(%rdi)
    movl $0, 28(%rdi)
    
    movq %r15, %rdi
    leaq -32(%rbp), %rsi
    movq $32, %rdx
    movq $0x2000004, %rax
    syscall
    
    popq %rbp
    ret

write_segments:
    pushq %rbp
    movq %rsp, %rbp
    subq $256, %rsp
    
    leaq -256(%rbp), %rdi
    
    movl $0x19, (%rdi)
    movl $72, 4(%rdi)
    
    movb $0x5f, 8(%rdi)
    movb $0x5f, 9(%rdi)
    movb $0x50, 10(%rdi)
    movb $0x41, 11(%rdi)
    movb $0x47, 12(%rdi)
    movb $0x45, 13(%rdi)
    movb $0x5a, 14(%rdi)
    movb $0x45, 15(%rdi)
    movb $0x52, 16(%rdi)
    movb $0x4f, 17(%rdi)
    movb $0, 18(%rdi)
    movb $0, 19(%rdi)
    movb $0, 20(%rdi)
    movb $0, 21(%rdi)
    movb $0, 22(%rdi)
    movb $0, 23(%rdi)
    
    movq $0, 24(%rdi)
    movl $0, 32(%rdi)
    movl $1, 36(%rdi)
    movq $0, 40(%rdi)
    movq $0, 48(%rdi)
    movl $0, 56(%rdi)
    movl $0, 60(%rdi)
    movl $0, 64(%rdi)
    movl $0, 68(%rdi)
    
    movq %r15, %rdi
    leaq -256(%rbp), %rsi
    movq $72, %rdx
    movq $0x2000004, %rax
    syscall
    
    leaq -256(%rbp), %rdi
    
    movl $0x19, (%rdi)
    movl $232, 4(%rdi)
    
    movb $0x5f, 8(%rdi)
    movb $0x5f, 9(%rdi)
    movb $0x54, 10(%rdi)
    movb $0x45, 11(%rdi)
    movb $0x58, 12(%rdi)
    movb $0x54, 13(%rdi)
    movb $0, 14(%rdi)
    movb $0, 15(%rdi)
    movb $0, 16(%rdi)
    movb $0, 17(%rdi)
    movb $0, 18(%rdi)
    movb $0, 19(%rdi)
    movb $0, 20(%rdi)
    movb $0, 21(%rdi)
    movb $0, 22(%rdi)
    movb $0, 23(%rdi)
    
    movl $0, 24(%rdi)
    movl $1, 28(%rdi)
    movq $0x1000, 32(%rdi)
    movq $0, 40(%rdi)
    movq $0x1000, 48(%rdi)
    movl $7, 56(%rdi)
    movl $5, 60(%rdi)
    movl $2, 64(%rdi)
    movl $0, 68(%rdi)
    
    movq %r15, %rdi
    leaq -256(%rbp), %rsi
    movq $72, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq %rbp, %rsp
    popq %rbp
    ret

write_sections:
    pushq %rbp
    movq %rsp, %rbp
    subq $256, %rsp
    
    leaq -256(%rbp), %rdi
    
    movb $0x5f, (%rdi)
    movb $0x5f, 1(%rdi)
    movb $0x74, 2(%rdi)
    movb $0x65, 3(%rdi)
    movb $0x78, 4(%rdi)
    movb $0x74, 5(%rdi)
    movb $0, 6(%rdi)
    movb $0, 7(%rdi)
    movb $0, 8(%rdi)
    movb $0, 9(%rdi)
    movb $0, 10(%rdi)
    movb $0, 11(%rdi)
    movb $0, 12(%rdi)
    movb $0, 13(%rdi)
    movb $0, 14(%rdi)
    movb $0, 15(%rdi)
    
    movb $0x5f, 16(%rdi)
    movb $0x5f, 17(%rdi)
    movb $0x54, 18(%rdi)
    movb $0x45, 19(%rdi)
    movb $0x58, 20(%rdi)
    movb $0x54, 21(%rdi)
    movb $0, 22(%rdi)
    movb $0, 23(%rdi)
    movb $0, 24(%rdi)
    movb $0, 25(%rdi)
    movb $0, 26(%rdi)
    movb $0, 27(%rdi)
    movb $0, 28(%rdi)
    movb $0, 29(%rdi)
    movb $0, 30(%rdi)
    movb $0, 31(%rdi)
    
    movl $0xfa0, 32(%rdi)
    movl $1, 36(%rdi)
    movq $0x60, 40(%rdi)
    movl $0xfa0, 48(%rdi)
    movl $4, 52(%rdi)
    movl $0, 56(%rdi)
    movl $0, 60(%rdi)
    movl $0x80000400, 64(%rdi)
    movl $0, 68(%rdi)
    movl $0, 72(%rdi)
    movl $0, 76(%rdi)
    
    movq %r15, %rdi
    leaq -256(%rbp), %rsi
    movq $80, %rdx
    movq $0x2000004, %rax
    syscall
    
    movb $0x5f, (%rdi)
    movb $0x5f, 1(%rdi)
    movb $0x73, 2(%rdi)
    movb $0x74, 3(%rdi)
    movb $0x75, 4(%rdi)
    movb $0x62, 5(%rdi)
    movb $0x73, 6(%rdi)
    movb $0, 7(%rdi)
    movb $0, 8(%rdi)
    movb $0, 9(%rdi)
    movb $0, 10(%rdi)
    movb $0, 11(%rdi)
    movb $0, 12(%rdi)
    movb $0, 13(%rdi)
    movb $0, 14(%rdi)
    movb $0, 15(%rdi)
    
    movb $0x5f, 16(%rdi)
    movb $0x5f, 17(%rdi)
    movb $0x54, 18(%rdi)
    movb $0x45, 19(%rdi)
    movb $0x58, 20(%rdi)
    movb $0x54, 21(%rdi)
    movb $0, 22(%rdi)
    movb $0, 23(%rdi)
    movb $0, 24(%rdi)
    movb $0, 25(%rdi)
    movb $0, 26(%rdi)
    movb $0, 27(%rdi)
    movb $0, 28(%rdi)
    movb $0, 29(%rdi)
    movb $0, 30(%rdi)
    movb $0, 31(%rdi)
    
    movl $0, 32(%rdi)
    movl $1, 36(%rdi)
    movq $0, 40(%rdi)
    movl $0, 48(%rdi)
    movl $1, 52(%rdi)
    movl $0, 56(%rdi)
    movl $0, 60(%rdi)
    movl $0x80000408, 64(%rdi)
    movl $0, 68(%rdi)
    movl $0, 72(%rdi)
    movl $0, 76(%rdi)
    
    movq %r15, %rdi
    leaq -256(%rbp), %rsi
    movq $80, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq %rbp, %rsp
    popq %rbp
    ret

write_load_commands:
    pushq %rbp
    movq %rsp, %rbp
    subq $512, %rsp
    
    leaq -32(%rbp), %rdi
    movl $0xe, (%rdi)
    movl $32, 4(%rdi)
    movl $12, 8(%rdi)
    movb $'/', 12(%rdi)
    movb $'u', 13(%rdi)
    movb $'s', 14(%rdi)
    movb $'r', 15(%rdi)
    movb $'/', 16(%rdi)
    movb $'l', 17(%rdi)
    movb $'i', 18(%rdi)
    movb $'b', 19(%rdi)
    movb $'/', 20(%rdi)
    movb $'d', 21(%rdi)
    movb $'y', 22(%rdi)
    movb $'l', 23(%rdi)
    movb $'d', 24(%rdi)
    movb $0, 25(%rdi)
    
    movq %r15, %rdi
    leaq -32(%rbp), %rsi
    movq $32, %rdx
    movq $0x2000004, %rax
    syscall
    
    call write_uuid
    call write_build_version
    call write_source_version
    call write_main
    call write_load_dylib
    call write_link_edit_cmd
    call write_symtab
    call write_dysymtab
    
    movq %rbp, %rsp
    popq %rbp
    ret

write_uuid:
    pushq %rbp
    movq %rsp, %rbp
    subq $32, %rsp
    
    leaq -32(%rbp), %rdi
    movl $0x1b, (%rdi)
    movl $24, 4(%rdi)
    
    movq $0x1122334455667788, 8(%rdi)
    movq $0x99aabbccddeeff00, 16(%rdi)
    
    movq %r15, %rdi
    leaq -32(%rbp), %rsi
    movq $24, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq %rbp, %rsp
    popq %rbp
    ret

write_build_version:
    pushq %rbp
    movq %rsp, %rbp
    subq $32, %rsp
    
    leaq -32(%rbp), %rdi
    movl $0x32, (%rdi)
    movl $24, 4(%rdi)
    movl $1, 8(%rdi)
    movl $786432, 12(%rdi)
    movl $786432, 16(%rdi)
    movl $0, 20(%rdi)
    
    movq %r15, %rdi
    leaq -32(%rbp), %rsi
    movq $24, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq %rbp, %rsp
    popq %rbp
    ret

write_source_version:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    
    leaq -16(%rbp), %rdi
    movl $0x2a, (%rdi)
    movl $16, 4(%rdi)
    movq $0, 8(%rdi)
    
    movq %r15, %rdi
    leaq -16(%rbp), %rsi
    movq $16, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq %rbp, %rsp
    popq %rbp
    ret

write_main:
    pushq %rbp
    movq %rsp, %rbp
    subq $32, %rsp
    
    leaq -32(%rbp), %rdi
    movl $0x80000028, (%rdi)
    movl $24, 4(%rdi)
    movq $0xfa0, 8(%rdi)
    movq $0, 16(%rdi)
    
    movq %r15, %rdi
    leaq -32(%rbp), %rsi
    movq $24, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq %rbp, %rsp
    popq %rbp
    ret

write_load_dylib:
    pushq %rbp
    movq %rsp, %rbp
    subq $64, %rsp
    
    leaq -64(%rbp), %rdi
    movl $0xc, (%rdi)
    movl $56, 4(%rdi)
    movl $24, 8(%rdi)
    movl $2, 12(%rdi)
    movl $0x10c0509, 16(%rdi)
    movl $0x10c0509, 20(%rdi)
    
    movb $'/', 24(%rdi)
    movb $'u', 25(%rdi)
    movb $'s', 26(%rdi)
    movb $'r', 27(%rdi)
    movb $'/', 28(%rdi)
    movb $'l', 29(%rdi)
    movb $'i', 30(%rdi)
    movb $'b', 31(%rdi)
    movb $'/', 32(%rdi)
    movb $'l', 33(%rdi)
    movb $'i', 34(%rdi)
    movb $'b', 35(%rdi)
    movb $'S', 36(%rdi)
    movb $'y', 37(%rdi)
    movb $'s', 38(%rdi)
    movb $'t', 39(%rdi)
    movb $'e', 40(%rdi)
    movb $'m', 41(%rdi)
    movb $'.', 42(%rdi)
    movb $'B', 43(%rdi)
    movb $'.', 44(%rdi)
    movb $'d', 45(%rdi)
    movb $'y', 46(%rdi)
    movb $'l', 47(%rdi)
    movb $'i', 48(%rdi)
    movb $'b', 49(%rdi)
    movb $0, 50(%rdi)
    
    movq %r15, %rdi
    leaq -64(%rbp), %rsi
    movq $56, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq %rbp, %rsp
    popq %rbp
    ret

write_link_edit_cmd:
    pushq %rbp
    movq %rsp, %rbp
    subq $128, %rsp
    
    leaq -128(%rbp), %rdi
    movl $0x19, (%rdi)
    movl $72, 4(%rdi)
    
    movb $'_', 8(%rdi)
    movb $'_', 9(%rdi)
    movb $'L', 10(%rdi)
    movb $'I', 11(%rdi)
    movb $'N', 12(%rdi)
    movb $'K', 13(%rdi)
    movb $'E', 14(%rdi)
    movb $'D', 15(%rdi)
    movb $'I', 16(%rdi)
    movb $'T', 17(%rdi)
    movb $0, 18(%rdi)
    movb $0, 19(%rdi)
    movb $0, 20(%rdi)
    movb $0, 21(%rdi)
    movb $0, 22(%rdi)
    movb $0, 23(%rdi)
    
    movl $1, 24(%rdi)
    movl $0, 28(%rdi)
    movq $0x1000, 32(%rdi)
    movq $0x1000, 40(%rdi)
    movq $0x250, 48(%rdi)
    movl $7, 56(%rdi)
    movl $1, 60(%rdi)
    movl $0, 64(%rdi)
    movl $0, 68(%rdi)
    
    movq %r15, %rdi
    leaq -128(%rbp), %rsi
    movq $72, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq %rbp, %rsp
    popq %rbp
    ret

write_symtab:
    pushq %rbp
    movq %rsp, %rbp
    subq $32, %rsp
    
    leaq -32(%rbp), %rdi
    movl $0x2, (%rdi)
    movl $24, 4(%rdi)
    movl $0x1068, 8(%rdi)
    movl $5, 12(%rdi)
    movl $0x10d0, 16(%rdi)
    movl $168, 20(%rdi)
    
    movq %r15, %rdi
    leaq -32(%rbp), %rsi
    movq $24, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq %rbp, %rsp
    popq %rbp
    ret

write_dysymtab:
    pushq %rbp
    movq %rsp, %rbp
    subq $80, %rsp
    
    leaq -80(%rbp), %rdi
    movl $0xb, (%rdi)
    movl $80, 4(%rdi)
    
    movl $0, 8(%rdi)
    movl $3, 12(%rdi)
    movl $3, 16(%rdi)
    movl $2, 20(%rdi)
    movl $5, 24(%rdi)
    movl $0, 28(%rdi)
    movl $0, 32(%rdi)
    movl $0, 36(%rdi)
    movl $0, 40(%rdi)
    movl $0, 44(%rdi)
    movl $0, 48(%rdi)
    movl $0, 52(%rdi)
    movl $0, 56(%rdi)
    movl $0, 60(%rdi)
    movl $0, 64(%rdi)
    movl $0, 68(%rdi)
    movl $0, 72(%rdi)
    movl $0, 76(%rdi)
    
    movq %r15, %rdi
    leaq -80(%rbp), %rsi
    movq $80, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq %rbp, %rsp
    popq %rbp
    ret

write_link_edit:
    pushq %rbp
    movq %rsp, %rbp
    subq $0x250, %rsp
    
    movq $0x250, %rcx
    leaq -0x250(%rbp), %rdi
    xorq %rax, %rax
.clear:
    movb %al, (%rdi)
    incq %rdi
    loop .clear
    
    movq %r15, %rdi
    leaq -0x250(%rbp), %rsi
    movq $0x250, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq %rbp, %rsp
    popq %rbp
    ret

copy_text_section:
    pushq %rbp
    movq %rsp, %rbp
    subq $0x1000, %rsp
    
    movq %r14, %rdi
    leaq -0x1000(%rbp), %rsi
    movq $0x1000, %rdx
    movq $0x2000003, %rax
    syscall
    
    movq %rax, %rbx
    
    cmpq $0, %rbx
    jle .done
    
    movq %r15, %rdi
    leaq -0x1000(%rbp), %rsi
    movq %rbx, %rdx
    movq $0x2000004, %rax
    syscall
    
.done:
    movq %rbp, %rsp
    popq %rbp
    ret

.section __DATA,__data

output_name:
    .asciz "a.out"

usage:
    .ascii "Tempo Linker v1.0\n"
    .ascii "100% Assembly - No clang, no ld\n"
    .ascii "Usage: tempo_linker input.o\n"
.set usage_len, . - usage

success:
    .ascii "Linked successfully: a.out\n"
.set success_len, . - success