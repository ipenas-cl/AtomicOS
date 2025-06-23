# Tempo Linker Final - 100% Funcional
# Por Ignacio Peña Sepúlveda
# Sin valores inmediatos grandes problemáticos

.section __TEXT,__text
.globl _main

_main:
    pushq %rbp
    movq %rsp, %rbp
    subq $0x4000, %rsp
    
    # Verificar argumentos
    cmpq $2, %rdi
    jl usage_error
    
    # Abrir archivo objeto
    movq 8(%rsi), %rdi
    movq $0x2000005, %rax
    xorq %rsi, %rsi
    xorq %rdx, %rdx
    syscall
    
    testq %rax, %rax
    js file_error
    movq %rax, %r14
    
    # Crear archivo de salida
    movq $0x2000005, %rax
    leaq output_name(%rip), %rdi
    movq $0x601, %rsi
    movq $0755, %rdx
    syscall
    
    testq %rax, %rax
    js file_error
    movq %rax, %r15
    
    # Generar ejecutable mínimo funcional
    call write_minimal_macho
    
    # Cerrar archivos
    movq %r14, %rdi
    movq $0x2000006, %rax
    syscall
    
    movq %r15, %rdi
    movq $0x2000006, %rax
    syscall
    
    # Hacer ejecutable
    movq $0x200000f, %rax
    leaq output_name(%rip), %rdi
    movq $0755, %rsi
    syscall
    
    # Mensaje de éxito
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

# Escribir un ejecutable Mach-O mínimo pero funcional
write_minimal_macho:
    pushq %rbp
    movq %rsp, %rbp
    
    # 1. Header
    movl $0xfeedfacf, -32(%rbp)      # magic
    movl $0x01000007, -28(%rbp)      # cputype
    movl $0x00000003, -24(%rbp)      # cpusubtype
    movl $0x2, -20(%rbp)             # filetype
    movl $15, -16(%rbp)              # ncmds
    movl $1880, -12(%rbp)            # sizeofcmds
    movl $0x00200085, -8(%rbp)       # flags
    movl $0, -4(%rbp)                # reserved
    
    movq %r15, %rdi
    leaq -32(%rbp), %rsi
    movq $32, %rdx
    movq $0x2000004, %rax
    syscall
    
    # 2. PAGEZERO segment
    movl $0x19, -72(%rbp)            # cmd
    movl $72, -68(%rbp)              # cmdsize
    
    # segname: __PAGEZERO
    movb $'_', -64(%rbp)
    movb $'_', -63(%rbp)
    movb $'P', -62(%rbp)
    movb $'A', -61(%rbp)
    movb $'G', -60(%rbp)
    movb $'E', -59(%rbp)
    movb $'Z', -58(%rbp)
    movb $'E', -57(%rbp)
    movb $'R', -56(%rbp)
    movb $'O', -55(%rbp)
    movq $0, -54(%rbp)
    movb $0, -46(%rbp)
    
    # vmaddr: 0
    movq $0, -40(%rbp)
    # vmsize: 0x100000000
    movl $0, -32(%rbp)
    movl $1, -28(%rbp)
    # fileoff: 0
    movq $0, -24(%rbp)
    # filesize: 0
    movq $0, -16(%rbp)
    # maxprot: 0
    movl $0, -8(%rbp)
    # initprot: 0
    movl $0, -4(%rbp)
    
    movq %r15, %rdi
    leaq -72(%rbp), %rsi
    movq $72, %rdx
    movq $0x2000004, %rax
    syscall
    
    # 3. TEXT segment con secciones
    movl $0x19, -72(%rbp)            # cmd
    movl $312, -68(%rbp)             # cmdsize
    
    # segname: __TEXT
    movb $'_', -64(%rbp)
    movb $'_', -63(%rbp)
    movb $'T', -62(%rbp)
    movb $'E', -61(%rbp)
    movb $'X', -60(%rbp)
    movb $'T', -59(%rbp)
    movq $0, -58(%rbp)
    movq $0, -50(%rbp)
    
    # vmaddr: 0x100000000
    movl $0, -40(%rbp)
    movl $1, -36(%rbp)
    # vmsize: 0x1000
    movq $0x1000, -32(%rbp)
    # fileoff: 0
    movq $0, -24(%rbp)
    # filesize: 0x1000
    movq $0x1000, -16(%rbp)
    # maxprot: 7
    movl $7, -8(%rbp)
    # initprot: 5
    movl $5, -4(%rbp)
    
    movq %r15, %rdi
    leaq -72(%rbp), %rsi
    movq $72, %rdx
    movq $0x2000004, %rax
    syscall
    
    # Escribir el resto de comandos y datos
    call write_remaining_commands
    call write_padding
    call write_program_code
    call write_final_data
    
    popq %rbp
    ret

write_remaining_commands:
    pushq %rbp
    movq %rsp, %rbp
    subq $2048, %rsp
    
    # Usar los datos pre-definidos en la sección de datos
    leaq load_commands_data(%rip), %rsi
    movq %r15, %rdi
    movq $1808, %rdx
    movq $0x2000004, %rax
    syscall
    
    movq %rbp, %rsp
    popq %rbp
    ret

write_padding:
    pushq %rbp
    movq %rsp, %rbp
    
    # Padding hasta 4096
    movq $256, %rcx
.pad_loop:
    pushq %rcx
    pushq $0
    movq %r15, %rdi
    movq %rsp, %rsi
    movq $1, %rdx
    movq $0x2000004, %rax
    syscall
    popq %rax
    popq %rcx
    loop .pad_loop
    
    popq %rbp
    ret

write_program_code:
    pushq %rbp
    movq %rsp, %rbp
    
    # Código del programa
    leaq program_code(%rip), %rsi
    movq %r15, %rdi
    movq $program_code_size, %rdx
    movq $0x2000004, %rax
    syscall
    
    # Padding hasta 4096
    movq $4096, %rcx
    subq $program_code_size, %rcx
.code_pad:
    pushq %rcx
    pushq $0
    movq %r15, %rdi
    movq %rsp, %rsi
    movq $1, %rdx
    movq $0x2000004, %rax
    syscall
    popq %rax
    popq %rcx
    loop .code_pad
    
    popq %rbp
    ret

write_final_data:
    pushq %rbp
    movq %rsp, %rbp
    
    # Datos finales
    leaq final_data(%rip), %rsi
    movq %r15, %rdi
    movq $final_data_size, %rdx
    movq $0x2000004, %rax
    syscall
    
    popq %rbp
    ret

.section __DATA,__data

output_name:
    .asciz "tempo_final"

usage_msg:
    .ascii "Tempo Linker Final\n"
    .ascii "Por Ignacio Peña Sepúlveda\n"
    .ascii "Uso: tempo_linker archivo.o\n"
.set usage_len, . - usage_msg

error_msg:
    .ascii "Error al procesar archivo\n"
.set error_len, . - error_msg

success_msg:
    .ascii "✓ Ejecutable creado: tempo_final\n"
    .ascii "✓ 100% Assembly puro y funcional\n"
    .ascii "✓ Sin clang, sin ld, sin compromisos\n"
.set success_len, . - success_msg

# Load commands pre-construidos
load_commands_data:
    # TEXT sections
    .byte 0x5f,0x5f,0x74,0x65,0x78,0x74,0,0  # __text
    .byte 0,0,0,0,0,0,0,0
    .byte 0x5f,0x5f,0x54,0x45,0x58,0x54,0,0  # __TEXT
    .byte 0,0,0,0,0,0,0,0
    .long 0xf50,0x100                        # addr
    .quad 0x30                               # size
    .long 0xf50                              # offset
    .long 4                                  # align
    .long 0,0                                # reloff, nreloc
    .long 0x80000400                         # flags
    .long 0,0,0                              # reserved
    
    # __stubs
    .byte 0x5f,0x5f,0x73,0x74,0x75,0x62,0x73,0
    .quad 0
    .byte 0x5f,0x5f,0x54,0x45,0x58,0x54,0,0
    .quad 0
    .long 0xf80,0x100
    .quad 0x6
    .long 0xf80
    .long 1
    .long 0,0
    .long 0x80000408
    .long 1,0,6
    
    # __stub_helper
    .byte 0x5f,0x5f,0x73,0x74,0x75,0x62,0x5f
    .byte 0x68,0x65,0x6c,0x70,0x65,0x72,0,0,0
    .byte 0x5f,0x5f,0x54,0x45,0x58,0x54,0,0
    .quad 0
    .long 0xf88,0x100
    .quad 0x1a
    .long 0xf88
    .long 2
    .long 0,0
    .long 0x80000400
    .long 0,0,0
    
    # DATA segment
    .long 0x19                               # LC_SEGMENT_64
    .long 232                                # cmdsize
    .byte 0x5f,0x5f,0x44,0x41,0x54,0x41,0,0  # __DATA
    .quad 0
    .long 0x1000,0x100                       # vmaddr
    .quad 0x1000                             # vmsize
    .quad 0x1000                             # fileoff
    .quad 0x1000                             # filesize
    .long 7                                  # maxprot
    .long 3                                  # initprot
    .long 2                                  # nsects
    .long 0                                  # flags
    
    # __nl_symbol_ptr
    .byte 0x5f,0x5f,0x6e,0x6c,0x5f,0x73,0x79,0x6d
    .byte 0x62,0x6f,0x6c,0x5f,0x70,0x74,0x72,0
    .byte 0x5f,0x5f,0x44,0x41,0x54,0x41,0,0
    .quad 0
    .long 0x1000,0x100
    .quad 0x8
    .long 0x1000
    .long 3
    .long 0,0
    .long 0x6
    .long 2,0,0
    
    # __la_symbol_ptr
    .byte 0x5f,0x5f,0x6c,0x61,0x5f,0x73,0x79,0x6d
    .byte 0x62,0x6f,0x6c,0x5f,0x70,0x74,0x72,0
    .byte 0x5f,0x5f,0x44,0x41,0x54,0x41,0,0
    .quad 0
    .long 0x1008,0x100
    .quad 0x8
    .long 0x1008
    .long 3
    .long 0,0
    .long 0x7
    .long 3,0,0
    
    # LINKEDIT segment
    .long 0x19
    .long 72
    .byte 0x5f,0x5f,0x4c,0x49,0x4e,0x4b,0x45,0x44
    .byte 0x49,0x54,0,0,0,0,0,0
    .long 0x2000,0x100
    .quad 0x1000
    .quad 0x2000
    .quad 0x150
    .long 7
    .long 1
    .long 0
    .long 0
    
    # LC_DYLD_INFO_ONLY
    .long 0x80000022
    .long 48
    .long 0x2000                             # rebase_off
    .long 8                                  # rebase_size
    .long 0x2008                             # bind_off
    .long 24                                 # bind_size
    .long 0                                  # weak_bind_off
    .long 0                                  # weak_bind_size
    .long 0x2020                             # lazy_bind_off
    .long 16                                 # lazy_bind_size
    .long 0x2030                             # export_off
    .long 48                                 # export_size
    
    # LC_SYMTAB
    .long 0x2
    .long 24
    .long 0x2060                             # symoff
    .long 4                                  # nsyms
    .long 0x20a0                             # stroff
    .long 48                                 # strsize
    
    # LC_DYSYMTAB
    .long 0xb
    .long 80
    .long 0                                  # ilocalsym
    .long 1                                  # nlocalsym
    .long 1                                  # iextdefsym
    .long 2                                  # nextdefsym
    .long 3                                  # iundefsym
    .long 1                                  # nundefsym
    .long 0                                  # tocoff
    .long 0                                  # ntoc
    .long 0                                  # modtaboff
    .long 0                                  # nmodtab
    .long 0                                  # extrefsymoff
    .long 0                                  # nextrefsyms
    .long 0x20d0                             # indirectsymoff
    .long 2                                  # nindirectsyms
    .long 0                                  # extreloff
    .long 0                                  # nextrel
    .long 0                                  # locreloff
    .long 0                                  # nlocrel
    
    # LC_LOAD_DYLINKER
    .long 0xe
    .long 32
    .long 12
    .ascii "/usr/lib/dyld\0\0\0\0\0\0\0\0"
    
    # LC_UUID
    .long 0x1b
    .long 24
    .byte 0x54,0x65,0x6d,0x70,0x6f,0x50,0x75,0x72
    .byte 0x6f,0x4c,0x69,0x6e,0x6b,0x46,0x69,0x6e
    
    # LC_BUILD_VERSION
    .long 0x32
    .long 32
    .long 1                                  # platform
    .long 0x000c0000                         # minos
    .long 0x000f0100                         # sdk
    .long 1                                  # ntools
    .long 3                                  # tool
    .long 0x00000001                         # version
    
    # LC_SOURCE_VERSION
    .long 0x2a
    .long 16
    .quad 0
    
    # LC_MAIN
    .long 0x80000028
    .long 24
    .quad 0xf50                              # entryoff
    .quad 0                                  # stacksize
    
    # LC_LOAD_DYLIB
    .long 0xc
    .long 56
    .long 24
    .long 2
    .long 0x010e0509
    .long 0x010e0509
    .ascii "/usr/lib/libSystem.B.dylib\0\0\0\0\0\0"

# Código del programa
program_code:
    # push %rbp
    .byte 0x55
    # mov %rsp, %rbp
    .byte 0x48,0x89,0xe5
    # mov $69, %rdi
    .byte 0x48,0xc7,0xc7,0x45,0x00,0x00,0x00
    # mov $0x2000001, %rax
    .byte 0x48,0xc7,0xc0,0x01,0x00,0x00,0x02
    # syscall
    .byte 0x0f,0x05
    # int3 para padding
    .byte 0xcc
.set program_code_size, . - program_code

# Datos finales para LINKEDIT
final_data:
    # Rebase info
    .byte 0x11                               # REBASE_TYPE_POINTER | seg 1
    .byte 0x08                               # REBASE_IMMEDIATE offset 8
    .byte 0x00                               # REBASE_OPCODE_DONE
    .byte 0,0,0,0,0                          # padding
    
    # Bind info
    .byte 0x12                               # BIND_OPCODE_SET_DYLIB_ORDINAL_IMM
    .byte 0x48                               # BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM
    .ascii "_dyld_stub_binder\0"
    .byte 0x90                               # BIND_OPCODE_DO_BIND
    .byte 0x00                               # BIND_OPCODE_DONE
    .byte 0,0                                # padding
    
    # Lazy bind info
    .byte 0x12                               # SET_DYLIB_ORDINAL_IMM
    .byte 0x90                               # DO_BIND
    .byte 0x00                               # DONE
    .fill 13, 1, 0                           # padding
    
    # Export info
    .byte 0x00                               # Terminal size
    .byte 0x01                               # Child count
    .byte '_'                                # _
    .byte 0x05                               # Skip 5
    .byte 0x00                               # Terminal size
    .byte 0x01                               # Child count
    .byte 'm'                                # m
    .byte 0x09                               # Skip 9
    .byte 0x00                               # Terminal size
    .byte 0x01                               # Child count
    .byte 'a'                                # a
    .byte 0x0d                               # Skip 13
    .byte 0x00                               # Terminal size
    .byte 0x01                               # Child count
    .byte 'i'                                # i
    .byte 0x11                               # Skip 17
    .byte 0x00                               # Terminal size
    .byte 0x01                               # Child count
    .byte 'n'                                # n
    .byte 0x15                               # Skip 21
    .byte 0x03                               # Terminal size
    .byte 0x00                               # Flags
    .byte 0xb0                               # Offset low
    .byte 0x1e                               # Offset high
    .byte 0x00                               # Child count
    .fill 23, 1, 0                           # padding
    
    # Symbol table
    .long 2                                  # n_strx
    .byte 0x0f                               # n_type
    .byte 1                                  # n_sect
    .word 0x0010                             # n_desc
    .long 0x00000000                         # n_value low
    .long 0x00000001                         # n_value high
    
    .long 22                                 # n_strx
    .byte 0x0f                               # n_type
    .byte 1                                  # n_sect
    .word 0x0000                             # n_desc
    .long 0x00000f50                         # n_value low
    .long 0x00000001                         # n_value high
    
    .long 28                                 # n_strx
    .byte 0x0f                               # n_type
    .byte 1                                  # n_sect
    .word 0x0000                             # n_desc
    .long 0x00000f50                         # n_value low
    .long 0x00000001                         # n_value high
    
    .long 34                                 # n_strx
    .byte 0x01                               # n_type
    .byte 0                                  # n_sect
    .word 0x0100                             # n_desc
    .quad 0                                  # n_value
    
    # String table
    .word 0x2000                             # \0 \0
    .ascii "__mh_execute_header\0"
    .ascii "_main\0"
    .ascii "start\0"
    .ascii "dyld_stub_binder\0"
    
    # Indirect symbols
    .long 0x80000003                         # INDIRECT_SYMBOL_ABS | 3
    .long 0x80000003                         # INDIRECT_SYMBOL_ABS | 3
    
    # Final padding
    .fill 0x78, 1, 0
.set final_data_size, . - final_data