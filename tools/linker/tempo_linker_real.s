.section __TEXT,__text
.globl _main

.set MH_MAGIC_64, 0xfeedfacf
.set CPU_TYPE_X86_64, 0x01000007
.set CPU_SUBTYPE_X86_64_ALL, 0x00000003
.set MH_EXECUTE, 0x2
.set MH_NOUNDEFS, 0x1
.set MH_DYLDLINK, 0x4
.set MH_PIE, 0x200000

.set LC_SEGMENT_64, 0x19
.set LC_SYMTAB, 0x2
.set LC_DYSYMTAB, 0xb
.set LC_LOAD_DYLINKER, 0xe
.set LC_UUID, 0x1b
.set LC_BUILD_VERSION, 0x32
.set LC_SOURCE_VERSION, 0x2a
.set LC_MAIN, 0x80000028
.set LC_LOAD_DYLIB, 0xc

.set VM_PROT_NONE, 0x0
.set VM_PROT_READ, 0x1
.set VM_PROT_WRITE, 0x2
.set VM_PROT_EXECUTE, 0x4

_main:
    pushq %rbp
    movq %rsp, %rbp
    subq $4096, %rsp
    
    movq $0x2000005, %rax
    leaq output_file(%rip), %rdi
    movq $0x601, %rsi
    movq $0755, %rdx
    syscall
    
    cmpq $0, %rax
    jl error_exit
    movq %rax, %r15
    
    leaq -4096(%rbp), %r14
    call create_mach_header
    call create_load_commands
    call align_to_page
    call write_text_segment
    
    movq %r15, %rdi
    movq $0x2000006, %rax
    syscall
    
    movq $0x200000f, %rax
    leaq output_file(%rip), %rdi
    movq $0755, %rsi
    syscall
    
    movq $1, %rdi
    leaq success_msg(%rip), %rsi
    movq $success_len, %rdx
    movq $0x2000004, %rax
    syscall
    
    xorq %rax, %rax
    movq %rbp, %rsp
    popq %rbp
    ret

error_exit:
    movq $1, %rdi
    leaq error_msg(%rip), %rsi
    movq $error_len, %rdx
    movq $0x2000004, %rax
    syscall
    movq $1, %rax
    movq %rbp, %rsp
    popq %rbp
    ret

create_mach_header:
    movl $MH_MAGIC_64, (%r14)
    movl $CPU_TYPE_X86_64, 4(%r14)
    movl $CPU_SUBTYPE_X86_64_ALL, 8(%r14)
    movl $MH_EXECUTE, 12(%r14)
    movl $10, 16(%r14)
    movl $1568, 20(%r14)
    movl $(MH_NOUNDEFS | MH_DYLDLINK | MH_PIE), 24(%r14)
    movl $0, 28(%r14)
    
    movq %r15, %rdi
    movq %r14, %rsi
    movq $32, %rdx
    movq $0x2000004, %rax
    syscall
    ret

create_load_commands:
    leaq 32(%r14), %r13
    
    movl $LC_SEGMENT_64, (%r13)
    movl $72, 4(%r13)
    movq $0x5f5f504147455a45, 8(%r13)
    movq $0x524f000000000000, 16(%r13)
    movq $0, 24(%r13)
    movq $0x100000000, 32(%r13)
    movq $0, 40(%r13)
    movq $0, 48(%r13)
    movl $0, 56(%r13)
    movl $0, 60(%r13)
    movl $0, 64(%r13)
    movl $0, 68(%r13)
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $72, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $72, %r13
    
    movl $LC_SEGMENT_64, (%r13)
    movl $232, 4(%r13)
    movq $0x5f5f54455854, 8(%r13)
    movq $0, 16(%r13)
    movq $0x100000000, 24(%r13)
    movq $0x1000, 32(%r13)
    movq $0, 40(%r13)
    movq $0x1000, 48(%r13)
    movl $5, 56(%r13)
    movl $5, 60(%r13)
    movl $2, 64(%r13)
    movl $0, 68(%r13)
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $72, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $72, %r13
    
    movq $0x5f5f74657874, (%r13)
    movq $0, 8(%r13)
    movq $0x5f5f54455854, 16(%r13)
    movq $0, 24(%r13)
    movq $0x100000fa0, 32(%r13)
    movq $0x10, 40(%r13)
    movl $0xfa0, 48(%r13)
    movl $4, 52(%r13)
    movl $0, 56(%r13)
    movl $0, 60(%r13)
    movl $0x80000400, 64(%r13)
    movl $0, 68(%r13)
    movl $0, 72(%r13)
    movl $0, 76(%r13)
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $80, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $80, %r13
    
    movq $0x5f5f7374756273, (%r13)
    movq $0, 8(%r13)
    movq $0x5f5f54455854, 16(%r13)
    movq $0, 24(%r13)
    movq $0x100000fb0, 32(%r13)
    movq $0x50, 40(%r13)
    movl $0xfb0, 48(%r13)
    movl $0, 52(%r13)
    movl $0, 56(%r13)
    movl $0, 60(%r13)
    movl $0x80000408, 64(%r13)
    movl $0, 68(%r13)
    movl $0, 72(%r13)
    movl $0, 76(%r13)
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $80, %rdx
    movq $0x2000004, %rax
    syscall
    
    call write_remaining_commands
    ret

write_remaining_commands:
    leaq dyld_cmd(%rip), %rsi
    movq %r15, %rdi
    movq $dyld_cmd_size, %rdx
    movq $0x2000004, %rax
    syscall
    
    leaq version_cmd(%rip), %rsi
    movq %r15, %rdi
    movq $version_cmd_size, %rdx
    movq $0x2000004, %rax
    syscall
    
    leaq source_cmd(%rip), %rsi
    movq %r15, %rdi
    movq $source_cmd_size, %rdx
    movq $0x2000004, %rax
    syscall
    
    leaq main_cmd(%rip), %rsi
    movq %r15, %rdi
    movq $main_cmd_size, %rdx
    movq $0x2000004, %rax
    syscall
    
    leaq libsystem_cmd(%rip), %rsi
    movq %r15, %rdi
    movq $libsystem_cmd_size, %rdx
    movq $0x2000004, %rax
    syscall
    
    leaq symtab_cmd(%rip), %rsi
    movq %r15, %rdi
    movq $symtab_cmd_size, %rdx
    movq $0x2000004, %rax
    syscall
    
    leaq dysymtab_cmd(%rip), %rsi
    movq %r15, %rdi
    movq $dysymtab_cmd_size, %rdx
    movq $0x2000004, %rax
    syscall
    
    leaq linkedit_cmd(%rip), %rsi
    movq %r15, %rdi
    movq $linkedit_cmd_size, %rdx
    movq $0x2000004, %rax
    syscall
    ret

align_to_page:
    movq $3424, %rcx
.pad_loop:
    cmpq $4096, %rcx
    jge .done
    pushq %rcx
    movq %r15, %rdi
    leaq zero(%rip), %rsi
    movq $1, %rdx
    movq $0x2000004, %rax
    syscall
    popq %rcx
    incq %rcx
    jmp .pad_loop
.done:
    ret

write_text_segment:
    movq %r15, %rdi
    leaq text_segment(%rip), %rsi
    movq $text_size, %rdx
    movq $0x2000004, %rax
    syscall
    ret

.section __DATA,__data

output_file:
    .asciz "tempo_executable"

error_msg:
    .ascii "Error creating file\n"
.set error_len, . - error_msg

success_msg:
    .ascii "Created: tempo_executable\n"
    .ascii "100% Pure - No clang, no ld\n"
.set success_len, . - success_msg

zero:
    .byte 0

dyld_cmd:
    .long 0xe
    .long 32
    .long 12
    .ascii "/usr/lib/dyld\0\0\0\0\0\0\0\0"
.set dyld_cmd_size, . - dyld_cmd

version_cmd:
    .long 0x32
    .long 24
    .long 1
    .long 786432
    .long 983296
    .long 0
.set version_cmd_size, . - version_cmd

source_cmd:
    .long 0x2a
    .long 16
    .quad 0
.set source_cmd_size, . - source_cmd

main_cmd:
    .long 0x80000028
    .long 24
    .quad 0xfa0
    .quad 0
.set main_cmd_size, . - main_cmd

libsystem_cmd:
    .long 0xc
    .long 56
    .long 24
    .long 2
    .long 0x1fc5009
    .long 0x8800050e
    .ascii "/usr/lib/libSystem.B.dylib\0\0\0\0\0\0"
.set libsystem_cmd_size, . - libsystem_cmd

symtab_cmd:
    .long 0x2
    .long 24
    .long 0x1010
    .long 3
    .long 0x1040
    .long 96
.set symtab_cmd_size, . - symtab_cmd

dysymtab_cmd:
    .long 0xb
    .long 80
    .fill 20, 4, 0
.set dysymtab_cmd_size, . - dysymtab_cmd

linkedit_cmd:
    .long 0x19
    .long 72
    .ascii "__LINKEDIT\0\0\0\0\0\0"
    .quad 0x100001000
    .quad 0x1000
    .quad 0x1000
    .quad 0xa0
    .long 3
    .long 3
    .long 0
    .long 0
.set linkedit_cmd_size, . - linkedit_cmd

text_segment:
    movq $42, %rdi
    movq $0x2000001, %rax
    syscall
    .fill 73, 1, 0
.set text_size, . - text_segment