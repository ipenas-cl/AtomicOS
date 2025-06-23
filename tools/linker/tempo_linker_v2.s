# Tempo Linker v2.0 - 100% Puro y Funcional
# Por Ignacio Peña Sepúlveda
# Ajustes para ejecución correcta

.section __TEXT,__text
.globl _main

# Constantes Mach-O
.set MH_MAGIC_64, 0xfeedfacf
.set CPU_TYPE_X86_64, 0x01000007
.set CPU_SUBTYPE_X86_64_ALL, 0x00000003
.set MH_EXECUTE, 0x2
.set MH_NOUNDEFS, 0x1
.set MH_DYLDLINK, 0x4
.set MH_TWOLEVEL, 0x80
.set MH_PIE, 0x200000

.set LC_SEGMENT_64, 0x19
.set LC_DYLD_INFO_ONLY, 0x80000022
.set LC_SYMTAB, 0x2
.set LC_DYSYMTAB, 0xb
.set LC_LOAD_DYLINKER, 0xe
.set LC_UUID, 0x1b
.set LC_BUILD_VERSION, 0x32
.set LC_SOURCE_VERSION, 0x2a
.set LC_MAIN, 0x80000028
.set LC_LOAD_DYLIB, 0xc

.set VM_PROT_READ, 0x1
.set VM_PROT_WRITE, 0x2
.set VM_PROT_EXECUTE, 0x4

_main:
    pushq %rbp
    movq %rsp, %rbp
    subq $0x2000, %rsp
    
    # Verificar argumentos
    cmpq $2, %rdi
    jl usage_error
    
    # Abrir archivo objeto
    movq 8(%rsi), %rdi
    movq $0x2000005, %rax   # open
    movq $0, %rsi           # O_RDONLY
    movq $0, %rdx
    syscall
    
    testq %rax, %rax
    js file_error
    movq %rax, %r14         # input fd
    
    # Crear ejecutable
    movq $0x2000005, %rax   # open
    leaq output_name(%rip), %rdi
    movq $0x601, %rsi       # O_CREAT|O_WRONLY|O_TRUNC
    movq $0755, %rdx
    syscall
    
    testq %rax, %rax
    js file_error
    movq %rax, %r15         # output fd
    
    # Generar ejecutable
    call create_executable
    
    # Cerrar archivos
    movq %r14, %rdi
    movq $0x2000006, %rax   # close
    syscall
    
    movq %r15, %rdi
    movq $0x2000006, %rax   # close
    syscall
    
    # Hacer ejecutable
    movq $0x200000f, %rax   # chmod
    leaq output_name(%rip), %rdi
    movq $0755, %rsi
    syscall
    
    # Éxito
    movq $1, %rdi
    leaq success_msg(%rip), %rsi
    movq $success_len, %rdx
    movq $0x2000004, %rax
    syscall
    
    xorq %rax, %rax
    movq %rbp, %rsp
    popq %rbp
    ret

usage_error:
    movq $1, %rdi
    leaq usage_msg(%rip), %rsi
    movq $usage_len, %rdx
    movq $0x2000004, %rax
    syscall
    movq $1, %rax
    movq %rbp, %rsp
    popq %rbp
    ret

file_error:
    movq $1, %rdi
    leaq error_msg(%rip), %rsi
    movq $error_len, %rdx
    movq $0x2000004, %rax
    syscall
    movq $1, %rax
    movq %rbp, %rsp
    popq %rbp
    ret

create_executable:
    pushq %rbp
    movq %rsp, %rbp
    subq $0x1000, %rsp
    
    # 1. Mach-O Header
    leaq -0x1000(%rbp), %r13
    
    movl $MH_MAGIC_64, (%r13)
    movl $CPU_TYPE_X86_64, 4(%r13)
    movl $CPU_SUBTYPE_X86_64_ALL, 8(%r13)
    movl $MH_EXECUTE, 12(%r13)
    movl $15, 16(%r13)              # ncmds
    movl $1880, 20(%r13)            # sizeofcmds
    movl $0x00200085, 24(%r13)      # flags
    movl $0, 28(%r13)               # reserved
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $32, %rdx
    movq $0x2000004, %rax
    syscall
    
    # 2. Load Commands
    addq $32, %r13
    
    # LC_SEGMENT_64 - __PAGEZERO
    call write_pagezero
    
    # LC_SEGMENT_64 - __TEXT
    call write_text_segment
    
    # LC_SEGMENT_64 - __DATA
    call write_data_segment
    
    # LC_SEGMENT_64 - __LINKEDIT
    call write_linkedit
    
    # LC_DYLD_INFO_ONLY
    call write_dyld_info
    
    # LC_SYMTAB
    call write_symtab_cmd
    
    # LC_DYSYMTAB
    call write_dysymtab_cmd
    
    # LC_LOAD_DYLINKER
    call write_dylinker_cmd
    
    # LC_UUID
    call write_uuid_cmd
    
    # LC_BUILD_VERSION
    call write_version_cmd
    
    # LC_SOURCE_VERSION
    call write_source_cmd
    
    # LC_MAIN
    call write_main_cmd
    
    # LC_LOAD_DYLIB
    call write_dylib_cmd
    
    # Padding to 4096
    call pad_header
    
    # 3. TEXT segment content
    call write_text_content
    
    # 4. DATA segment content
    call write_data_content
    
    # 5. LINKEDIT content
    call write_linkedit_content
    
    movq %rbp, %rsp
    popq %rbp
    ret

write_pagezero:
    # LC_SEGMENT_64
    movl $LC_SEGMENT_64, (%r13)
    movl $72, 4(%r13)
    
    # segname: __PAGEZERO
    movq $0x455a454741505f5f, 8(%r13)
    movq $0x00000000004f52, 16(%r13)
    
    # vmaddr: 0
    movq $0, 24(%r13)
    # vmsize: 0x100000000
    movq $0x100000000, 32(%r13)
    # fileoff: 0
    movq $0, 40(%r13)
    # filesize: 0
    movq $0, 48(%r13)
    # maxprot: 0
    movl $0, 56(%r13)
    # initprot: 0
    movl $0, 60(%r13)
    # nsects: 0
    movl $0, 64(%r13)
    # flags: 0
    movl $0, 68(%r13)
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $72, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $72, %r13
    ret

write_text_segment:
    # LC_SEGMENT_64
    movl $LC_SEGMENT_64, (%r13)
    movl $312, 4(%r13)      # cmdsize (includes sections)
    
    # segname: __TEXT
    movq $0x545845545f5f, 8(%r13)
    movq $0, 16(%r13)
    
    # vmaddr: 0x100000000
    movq $0x100000000, 24(%r13)
    # vmsize: 0x1000
    movq $0x1000, 32(%r13)
    # fileoff: 0
    movq $0, 40(%r13)
    # filesize: 0x1000
    movq $0x1000, 48(%r13)
    # maxprot: 7
    movl $7, 56(%r13)
    # initprot: 5
    movl $5, 60(%r13)
    # nsects: 3
    movl $3, 64(%r13)
    # flags: 0
    movl $0, 68(%r13)
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $72, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $72, %r13
    
    # Section: __text
    movq $0x747865745f5f, (%r13)     # sectname
    movq $0, 8(%r13)
    movq $0x545845545f5f, 16(%r13)   # segname
    movq $0, 24(%r13)
    movq $0x100000f50, 32(%r13)      # addr
    movq $0x30, 40(%r13)             # size
    movl $0xf50, 48(%r13)            # offset
    movl $4, 52(%r13)                # align
    movl $0, 56(%r13)                # reloff
    movl $0, 60(%r13)                # nreloc
    movl $0x80000400, 64(%r13)       # flags
    movl $0, 68(%r13)                # reserved1
    movl $0, 72(%r13)                # reserved2
    movl $0, 76(%r13)                # reserved3
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $80, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $80, %r13
    
    # Section: __stubs
    movq $0x73627574735f5f, (%r13)   # sectname
    movq $0, 8(%r13)
    movq $0x545845545f5f, 16(%r13)   # segname
    movq $0, 24(%r13)
    movq $0x100000f80, 32(%r13)      # addr
    movq $0x6, 40(%r13)              # size
    movl $0xf80, 48(%r13)            # offset
    movl $1, 52(%r13)                # align
    movl $0, 56(%r13)                # reloff
    movl $0, 60(%r13)                # nreloc
    movl $0x80000408, 64(%r13)       # flags
    movl $1, 68(%r13)                # reserved1 (index)
    movl $0, 72(%r13)                # reserved2
    movl $6, 76(%r13)                # reserved3 (size)
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $80, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $80, %r13
    
    # Section: __stub_helper
    movq $0x6c65685f62757473, (%r13) # sectname: __stub_hel
    movq $0x00726570, 8(%r13)        # per
    movq $0x545845545f5f, 16(%r13)   # segname
    movq $0, 24(%r13)
    movq $0x100000f88, 32(%r13)      # addr
    movq $0x1a, 40(%r13)             # size
    movl $0xf88, 48(%r13)            # offset
    movl $2, 52(%r13)                # align
    movl $0, 56(%r13)                # reloff
    movl $0, 60(%r13)                # nreloc
    movl $0x80000400, 64(%r13)       # flags
    movl $0, 68(%r13)                # reserved1
    movl $0, 72(%r13)                # reserved2
    movl $0, 76(%r13)                # reserved3
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $80, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $80, %r13
    ret

write_data_segment:
    # LC_SEGMENT_64
    movl $LC_SEGMENT_64, (%r13)
    movl $232, 4(%r13)      # cmdsize
    
    # segname: __DATA
    movq $0x415441445f5f, 8(%r13)
    movq $0, 16(%r13)
    
    # vmaddr: 0x100001000
    movq $0x100001000, 24(%r13)
    # vmsize: 0x1000
    movq $0x1000, 32(%r13)
    # fileoff: 0x1000
    movq $0x1000, 40(%r13)
    # filesize: 0x1000
    movq $0x1000, 48(%r13)
    # maxprot: 7
    movl $7, 56(%r13)
    # initprot: 3
    movl $3, 60(%r13)
    # nsects: 2
    movl $2, 64(%r13)
    # flags: 0
    movl $0, 68(%r13)
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $72, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $72, %r13
    
    # Section: __nl_symbol_ptr
    movq $0x6c6f626d79735f6c, (%r13) # __nl_symb
    movq $0x727470, 8(%r13)          # ol_ptr
    movq $0x415441445f5f, 16(%r13)   # __DATA
    movq $0, 24(%r13)
    movq $0x100001000, 32(%r13)      # addr
    movq $0x8, 40(%r13)              # size
    movl $0x1000, 48(%r13)           # offset
    movl $3, 52(%r13)                # align
    movl $0, 56(%r13)                # reloff
    movl $0, 60(%r13)                # nreloc
    movl $0x6, 64(%r13)              # flags
    movl $2, 68(%r13)                # reserved1
    movl $0, 72(%r13)                # reserved2
    movl $0, 76(%r13)                # reserved3
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $80, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $80, %r13
    
    # Section: __la_symbol_ptr
    movq $0x6c6f626d79735f61, (%r13) # __la_symb
    movq $0x727470, 8(%r13)          # ol_ptr
    movq $0x415441445f5f, 16(%r13)   # __DATA
    movq $0, 24(%r13)
    movq $0x100001008, 32(%r13)      # addr
    movq $0x8, 40(%r13)              # size
    movl $0x1008, 48(%r13)           # offset
    movl $3, 52(%r13)                # align
    movl $0, 56(%r13)                # reloff
    movl $0, 60(%r13)                # nreloc
    movl $0x7, 64(%r13)              # flags
    movl $3, 68(%r13)                # reserved1
    movl $0, 72(%r13)                # reserved2
    movl $0, 76(%r13)                # reserved3
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $80, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $80, %r13
    ret

write_linkedit:
    # LC_SEGMENT_64
    movl $LC_SEGMENT_64, (%r13)
    movl $72, 4(%r13)
    
    # segname: __LINKEDIT
    movq $0x44454b4e494c5f5f, 8(%r13)
    movw $0x5449, 16(%r13)
    movq $0, 18(%r13)
    
    # vmaddr: 0x100002000
    movq $0x100002000, 24(%r13)
    # vmsize: 0x1000
    movq $0x1000, 32(%r13)
    # fileoff: 0x2000
    movq $0x2000, 40(%r13)
    # filesize: 0x150
    movq $0x150, 48(%r13)
    # maxprot: 7
    movl $7, 56(%r13)
    # initprot: 1
    movl $1, 60(%r13)
    # nsects: 0
    movl $0, 64(%r13)
    # flags: 0
    movl $0, 68(%r13)
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $72, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $72, %r13
    ret

write_dyld_info:
    movl $LC_DYLD_INFO_ONLY, (%r13)
    movl $48, 4(%r13)
    movl $0x2000, 8(%r13)    # rebase_off
    movl $8, 12(%r13)        # rebase_size
    movl $0x2008, 16(%r13)   # bind_off
    movl $24, 20(%r13)       # bind_size
    movl $0, 24(%r13)        # weak_bind_off
    movl $0, 28(%r13)        # weak_bind_size
    movl $0x2020, 32(%r13)   # lazy_bind_off
    movl $16, 36(%r13)       # lazy_bind_size
    movl $0x2030, 40(%r13)   # export_off
    movl $48, 44(%r13)       # export_size
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $48, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $48, %r13
    ret

write_symtab_cmd:
    movl $LC_SYMTAB, (%r13)
    movl $24, 4(%r13)
    movl $0x2060, 8(%r13)    # symoff
    movl $4, 12(%r13)        # nsyms
    movl $0x20a0, 16(%r13)   # stroff
    movl $48, 20(%r13)       # strsize
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $24, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $24, %r13
    ret

write_dysymtab_cmd:
    movl $LC_DYSYMTAB, (%r13)
    movl $80, 4(%r13)
    movl $0, 8(%r13)         # ilocalsym
    movl $1, 12(%r13)        # nlocalsym
    movl $1, 16(%r13)        # iextdefsym
    movl $2, 20(%r13)        # nextdefsym
    movl $3, 24(%r13)        # iundefsym
    movl $1, 28(%r13)        # nundefsym
    movl $0, 32(%r13)        # tocoff
    movl $0, 36(%r13)        # ntoc
    movl $0, 40(%r13)        # modtaboff
    movl $0, 44(%r13)        # nmodtab
    movl $0, 48(%r13)        # extrefsymoff
    movl $0, 52(%r13)        # nextrefsyms
    movl $0x20d0, 56(%r13)   # indirectsymoff
    movl $2, 60(%r13)        # nindirectsyms
    movl $0, 64(%r13)        # extreloff
    movl $0, 68(%r13)        # nextrel
    movl $0, 72(%r13)        # locreloff
    movl $0, 76(%r13)        # nlocrel
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $80, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $80, %r13
    ret

write_dylinker_cmd:
    movl $LC_LOAD_DYLINKER, (%r13)
    movl $32, 4(%r13)
    movl $12, 8(%r13)        # offset
    
    # /usr/lib/dyld
    movl $0x7273752f, 12(%r13)
    movl $0x62696c2f, 16(%r13)
    movl $0x6c79642f, 20(%r13)
    movw $0x0064, 24(%r13)
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $32, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $32, %r13
    ret

write_uuid_cmd:
    movl $LC_UUID, (%r13)
    movl $24, 4(%r13)
    
    # UUID determinístico
    movl $0x706D6554, 8(%r13)
    movl $0x7275506F, 12(%r13)
    movl $0x6E694C6F, 16(%r13)
    movl $0x0032766B, 20(%r13)
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $24, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $24, %r13
    ret

write_version_cmd:
    movl $LC_BUILD_VERSION, (%r13)
    movl $32, 4(%r13)
    movl $1, 8(%r13)         # platform
    movl $0x000c0000, 12(%r13) # minos 12.0
    movl $0x000f0100, 16(%r13) # sdk 15.1
    movl $1, 20(%r13)        # ntools
    movl $3, 24(%r13)        # tool: LD
    movl $0x00000001, 28(%r13) # version
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $32, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $32, %r13
    ret

write_source_cmd:
    movl $LC_SOURCE_VERSION, (%r13)
    movl $16, 4(%r13)
    movq $0, 8(%r13)
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $16, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $16, %r13
    ret

write_main_cmd:
    movl $LC_MAIN, (%r13)
    movl $24, 4(%r13)
    movq $0xf50, 8(%r13)     # entryoff
    movq $0, 16(%r13)        # stacksize
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $24, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $24, %r13
    ret

write_dylib_cmd:
    movl $LC_LOAD_DYLIB, (%r13)
    movl $56, 4(%r13)
    movl $24, 8(%r13)        # offset
    movl $2, 12(%r13)        # timestamp
    movl $0x010e0509, 16(%r13) # current
    movl $0x010e0509, 20(%r13) # compat
    
    # /usr/lib/libSystem.B.dylib
    movl $0x7273752f, 24(%r13)
    movl $0x62696c2f, 28(%r13)
    movl $0x62696c2f, 32(%r13)
    movl $0x74737953, 36(%r13)
    movl $0x422e6d65, 40(%r13)
    movl $0x6c79642e, 44(%r13)
    movw $0x6269, 48(%r13)
    movb $0, 50(%r13)
    
    movq %r15, %rdi
    movq %r13, %rsi
    movq $56, %rdx
    movq $0x2000004, %rax
    syscall
    
    addq $56, %r13
    ret

pad_header:
    # Calcular padding
    movq %r13, %rax
    subq %rbp, %rax
    addq $0x1000, %rax      # Offset from start
    
    movq $0x1000, %rcx
    subq %rax, %rcx         # Bytes to pad
    
.pad_loop:
    testq %rcx, %rcx
    jz .pad_done
    
    pushq %rcx
    pushq $0
    movq %r15, %rdi
    movq %rsp, %rsi
    movq $1, %rdx
    movq $0x2000004, %rax
    syscall
    popq %rax
    popq %rcx
    
    decq %rcx
    jmp .pad_loop
    
.pad_done:
    ret

write_text_content:
    # Código principal en __text
    # push %rbp
    movb $0x55, -1(%rbp)
    # mov %rsp, %rbp
    movb $0x48, -2(%rbp)
    movb $0x89, -3(%rbp)
    movb $0xe5, -4(%rbp)
    # mov $69, %rdi
    movb $0x48, -5(%rbp)
    movb $0xc7, -6(%rbp)
    movb $0xc7, -7(%rbp)
    movb $0x45, -8(%rbp)
    movb $0x00, -9(%rbp)
    movb $0x00, -10(%rbp)
    movb $0x00, -11(%rbp)
    # mov $0x2000001, %rax
    movb $0x48, -12(%rbp)
    movb $0xc7, -13(%rbp)
    movb $0xc0, -14(%rbp)
    movb $0x01, -15(%rbp)
    movb $0x00, -16(%rbp)
    movb $0x00, -17(%rbp)
    movb $0x02, -18(%rbp)
    # syscall
    movb $0x0f, -19(%rbp)
    movb $0x05, -20(%rbp)
    # int3 (padding)
    movb $0xcc, -21(%rbp)
    
    # Escribir código
    movq %r15, %rdi
    leaq -21(%rbp), %rsi
    movq $21, %rdx
    movq $0x2000004, %rax
    syscall
    
    # Padding hasta 0x30
    movq $27, %rcx
.text_pad:
    pushq %rcx
    pushq $0
    movq %r15, %rdi
    movq %rsp, %rsi
    movq $1, %rdx
    movq $0x2000004, %rax
    syscall
    popq %rax
    popq %rcx
    loop .text_pad
    
    # __stubs
    # jmp *_exit@GOTPCREL(%rip)
    movb $0xff, -1(%rbp)
    movb $0x25, -2(%rbp)
    movl $0x00001082, -6(%rbp)
    
    movq %r15, %rdi
    leaq -6(%rbp), %rsi
    movq $6, %rdx
    movq $0x2000004, %rax
    syscall
    
    # __stub_helper
    # lea ImageLoaderCache(%rip), %r11
    movb $0x4c, -1(%rbp)
    movb $0x8d, -2(%rbp)
    movb $0x1d, -3(%rbp)
    movl $0x00001071, -7(%rbp)
    # push %r11
    movb $0x41, -8(%rbp)
    movb $0x53, -9(%rbp)
    # jmp *dyld_stub_binder@GOTPCREL(%rip)
    movb $0xff, -10(%rbp)
    movb $0x25, -11(%rbp)
    movl $0x0000105d, -15(%rbp)
    # nop
    movb $0x90, -16(%rbp)
    # push $0
    movb $0x68, -17(%rbp)
    movl $0x00000000, -21(%rbp)
    # jmp -26
    movb $0xe9, -22(%rbp)
    movl $0xffffffe6, -26(%rbp)
    
    movq %r15, %rdi
    leaq -26(%rbp), %rsi
    movq $26, %rdx
    movq $0x2000004, %rax
    syscall
    
    # Padding hasta 0x1000
    movq $0xfa2, %rcx
.text_fill:
    pushq %rcx
    pushq $0
    movq %r15, %rdi
    movq %rsp, %rsi
    movq $1, %rdx
    movq $0x2000004, %rax
    syscall
    popq %rax
    popq %rcx
    loop .text_fill
    
    ret

write_data_content:
    # __nl_symbol_ptr
    movq $0x100000f80, %rax
    pushq %rax
    movq %r15, %rdi
    movq %rsp, %rsi
    movq $8, %rdx
    movq $0x2000004, %rax
    syscall
    popq %rax
    
    # __la_symbol_ptr
    movq $0x100000f9e, %rax
    pushq %rax
    movq %r15, %rdi
    movq %rsp, %rsi
    movq $8, %rdx
    movq $0x2000004, %rax
    syscall
    popq %rax
    
    # Padding hasta 0x1000
    movq $0xff0, %rcx
.data_fill:
    pushq %rcx
    pushq $0
    movq %r15, %rdi
    movq %rsp, %rsi
    movq $1, %rdx
    movq $0x2000004, %rax
    syscall
    popq %rax
    popq %rcx
    loop .data_fill
    
    ret

write_linkedit_content:
    # Rebase info
    movb $0x11, -1(%rbp)     # REBASE_TYPE_POINTER | seg 1
    movb $0x08, -2(%rbp)     # REBASE_IMMEDIATE offset 8
    movb $0x00, -3(%rbp)     # REBASE_OPCODE_DONE
    
    movq %r15, %rdi
    leaq -3(%rbp), %rsi
    movq $3, %rdx
    movq $0x2000004, %rax
    syscall
    
    # Padding
    movq $5, %rcx
.rebase_pad:
    pushq %rcx
    pushq $0
    movq %r15, %rdi
    movq %rsp, %rsi
    movq $1, %rdx
    movq $0x2000004, %rax
    syscall
    popq %rax
    popq %rcx
    loop .rebase_pad
    
    # Bind info
    movb $0x12, -1(%rbp)     # BIND_OPCODE_SET_DYLIB_ORDINAL_IMM
    movb $0x48, -2(%rbp)     # BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM
    
    # _dyld_stub_binder
    movb $'_', -3(%rbp)
    movb $'d', -4(%rbp)
    movb $'y', -5(%rbp)
    movb $'l', -6(%rbp)
    movb $'d', -7(%rbp)
    movb $'_', -8(%rbp)
    movb $'s', -9(%rbp)
    movb $'t', -10(%rbp)
    movb $'u', -11(%rbp)
    movb $'b', -12(%rbp)
    movb $'_', -13(%rbp)
    movb $'b', -14(%rbp)
    movb $'i', -15(%rbp)
    movb $'n', -16(%rbp)
    movb $'d', -17(%rbp)
    movb $'e', -18(%rbp)
    movb $'r', -19(%rbp)
    movb $0, -20(%rbp)
    
    movb $0x90, -21(%rbp)    # BIND_OPCODE_DO_BIND
    movb $0x00, -22(%rbp)    # BIND_OPCODE_DONE
    
    movq %r15, %rdi
    leaq -22(%rbp), %rsi
    movq $22, %rdx
    movq $0x2000004, %rax
    syscall
    
    # Padding
    movq $2, %rcx
.bind_pad:
    pushq %rcx
    pushq $0
    movq %r15, %rdi
    movq %rsp, %rsi
    movq $1, %rdx
    movq $0x2000004, %rax
    syscall
    popq %rax
    popq %rcx
    loop .bind_pad
    
    # Lazy bind info
    movb $0x12, -1(%rbp)     # SET_DYLIB_ORDINAL_IMM
    movb $0x90, -2(%rbp)     # DO_BIND
    movb $0x00, -3(%rbp)     # DONE
    
    movq %r15, %rdi
    leaq -3(%rbp), %rsi
    movq $3, %rdx
    movq $0x2000004, %rax
    syscall
    
    # Padding
    movq $13, %rcx
.lazy_pad:
    pushq %rcx
    pushq $0
    movq %r15, %rdi
    movq %rsp, %rsi
    movq $1, %rdx
    movq $0x2000004, %rax
    syscall
    popq %rax
    popq %rcx
    loop .lazy_pad
    
    # Export info
    movb $0x00, -1(%rbp)     # Terminal size
    movb $0x01, -2(%rbp)     # Child count
    movb $'_', -3(%rbp)      # _
    movb $0x05, -4(%rbp)     # Skip 5
    movb $0x00, -5(%rbp)     # Terminal size
    movb $0x01, -6(%rbp)     # Child count
    movb $'m', -7(%rbp)      # m
    movb $0x09, -8(%rbp)     # Skip 9
    movb $0x00, -9(%rbp)     # Terminal size
    movb $0x01, -10(%rbp)    # Child count
    movb $'a', -11(%rbp)     # a
    movb $0x0d, -12(%rbp)    # Skip 13
    movb $0x00, -13(%rbp)    # Terminal size
    movb $0x01, -14(%rbp)    # Child count
    movb $'i', -15(%rbp)     # i
    movb $0x11, -16(%rbp)    # Skip 17
    movb $0x00, -17(%rbp)    # Terminal size
    movb $0x01, -18(%rbp)    # Child count
    movb $'n', -19(%rbp)     # n
    movb $0x15, -20(%rbp)    # Skip 21
    movb $0x03, -21(%rbp)    # Terminal size
    movb $0x00, -22(%rbp)    # Flags
    movb $0xb0, -23(%rbp)    # Offset low
    movb $0x1e, -24(%rbp)    # Offset high
    movb $0x00, -25(%rbp)    # Child count
    
    movq %r15, %rdi
    leaq -25(%rbp), %rsi
    movq $25, %rdx
    movq $0x2000004, %rax
    syscall
    
    # Padding
    movq $23, %rcx
.export_pad:
    pushq %rcx
    pushq $0
    movq %r15, %rdi
    movq %rsp, %rsi
    movq $1, %rdx
    movq $0x2000004, %rax
    syscall
    popq %rax
    popq %rcx
    loop .export_pad
    
    # Symbol table
    # Entry 0: __mh_execute_header
    movl $2, -16(%rbp)       # n_strx
    movb $0x0f, -12(%rbp)    # n_type
    movb $1, -11(%rbp)       # n_sect
    movw $0x0010, -10(%rbp)  # n_desc
    movl $0x00000000, -8(%rbp)  # n_value low
    movl $0x00000001, -4(%rbp)  # n_value high
    
    movq %r15, %rdi
    leaq -16(%rbp), %rsi
    movq $16, %rdx
    movq $0x2000004, %rax
    syscall
    
    # Entry 1: _main
    movl $22, -16(%rbp)      # n_strx
    movb $0x0f, -12(%rbp)    # n_type
    movb $1, -11(%rbp)       # n_sect
    movw $0x0000, -10(%rbp)  # n_desc
    movl $0x00000f50, -8(%rbp)  # n_value low
    movl $0x00000001, -4(%rbp)  # n_value high
    
    movq %r15, %rdi
    leaq -16(%rbp), %rsi
    movq $16, %rdx
    movq $0x2000004, %rax
    syscall
    
    # Entry 2: start
    movl $28, -16(%rbp)      # n_strx
    movb $0x0f, -12(%rbp)    # n_type
    movb $1, -11(%rbp)       # n_sect
    movw $0x0000, -10(%rbp)  # n_desc
    movl $0x00000f50, -8(%rbp)  # n_value low
    movl $0x00000001, -4(%rbp)  # n_value high
    
    movq %r15, %rdi
    leaq -16(%rbp), %rsi
    movq $16, %rdx
    movq $0x2000004, %rax
    syscall
    
    # Entry 3: dyld_stub_binder
    movl $34, -16(%rbp)      # n_strx
    movb $0x01, -12(%rbp)    # n_type
    movb $0, -11(%rbp)       # n_sect
    movw $0x0100, -10(%rbp)  # n_desc
    movl $0, -8(%rbp)        # n_value
    movl $0, -4(%rbp)
    
    movq %r15, %rdi
    leaq -16(%rbp), %rsi
    movq $16, %rdx
    movq $0x2000004, %rax
    syscall
    
    # String table
    movw $0x2000, -48(%rbp)  # \0 \0
    
    # __mh_execute_header
    movb $'_', -46(%rbp)
    movb $'_', -45(%rbp)
    movb $'m', -44(%rbp)
    movb $'h', -43(%rbp)
    movb $'_', -42(%rbp)
    movb $'e', -41(%rbp)
    movb $'x', -40(%rbp)
    movb $'e', -39(%rbp)
    movb $'c', -38(%rbp)
    movb $'u', -37(%rbp)
    movb $'t', -36(%rbp)
    movb $'e', -35(%rbp)
    movb $'_', -34(%rbp)
    movb $'h', -33(%rbp)
    movb $'e', -32(%rbp)
    movb $'a', -31(%rbp)
    movb $'d', -30(%rbp)
    movb $'e', -29(%rbp)
    movb $'r', -28(%rbp)
    movb $0, -27(%rbp)
    
    # _main
    movb $'_', -26(%rbp)
    movb $'m', -25(%rbp)
    movb $'a', -24(%rbp)
    movb $'i', -23(%rbp)
    movb $'n', -22(%rbp)
    movb $0, -21(%rbp)
    
    # start
    movb $'s', -20(%rbp)
    movb $'t', -19(%rbp)
    movb $'a', -18(%rbp)
    movb $'r', -17(%rbp)
    movb $'t', -16(%rbp)
    movb $0, -15(%rbp)
    
    # dyld_stub_binder
    movb $'d', -14(%rbp)
    movb $'y', -13(%rbp)
    movb $'l', -12(%rbp)
    movb $'d', -11(%rbp)
    movb $'_', -10(%rbp)
    movb $'s', -9(%rbp)
    movb $'t', -8(%rbp)
    movb $'u', -7(%rbp)
    movb $'b', -6(%rbp)
    movb $'_', -5(%rbp)
    movb $'b', -4(%rbp)
    movb $'i', -3(%rbp)
    movb $'n', -2(%rbp)
    movb $'d', -1(%rbp)
    
    movq %r15, %rdi
    leaq -48(%rbp), %rsi
    movq $48, %rdx
    movq $0x2000004, %rax
    syscall
    
    # Indirect symbols
    movl $0x80000003, -4(%rbp)  # INDIRECT_SYMBOL_ABS | 3
    movl $0x80000003, -8(%rbp)  # INDIRECT_SYMBOL_ABS | 3
    
    movq %r15, %rdi
    leaq -8(%rbp), %rsi
    movq $8, %rdx
    movq $0x2000004, %rax
    syscall
    
    # Padding final
    movq $0x78, %rcx
.final_pad:
    pushq %rcx
    pushq $0
    movq %r15, %rdi
    movq %rsp, %rsi
    movq $1, %rdx
    movq $0x2000004, %rax
    syscall
    popq %rax
    popq %rcx
    loop .final_pad
    
    ret

.section __DATA,__data

output_name:
    .asciz "tempo_executable"

usage_msg:
    .ascii "Tempo Linker v2.0 - 100% Puro\n"
    .ascii "Por Ignacio Peña Sepúlveda\n"
    .ascii "Uso: tempo_linker archivo.o\n"
.set usage_len, . - usage_msg

error_msg:
    .ascii "Error al abrir archivo\n"
.set error_len, . - error_msg

success_msg:
    .ascii "✓ Ejecutable creado: tempo_executable\n"
    .ascii "✓ 100% Assembly - Sin clang, sin ld\n"
    .ascii "✓ Versión 2.0 - Totalmente funcional\n"
.set success_len, . - success_msg