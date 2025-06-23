; Tempo Native Compiler v2.0 - Real Parser
; 100% Assembly, 0% C
; Por Ignacio Peña Sepúlveda

.section __TEXT,__text
.globl _main

_main:
    pushq %rbp
    movq %rsp, %rbp
    subq $256, %rsp              ; Space for parsing
    
    ; Print compiler banner
    movq $1, %rdi
    leaq banner(%rip), %rsi
    movq $banner_len, %rdx
    movq $0x2000004, %rax
    syscall
    
    ; Parse test program: "return 42;"
    leaq test_input(%rip), %r12   ; r12 = input pointer
    call parse_and_compile
    
    ; Exit
    xorq %rax, %rax
    movq %rbp, %rsp
    popq %rbp
    ret

; Main parser/compiler
parse_and_compile:
    pushq %rbp
    movq %rsp, %rbp
    
    ; Emit assembly header
    call emit_header
    
    ; Skip whitespace
    movq %r12, %rsi
    call skip_spaces
    movq %rsi, %r12
    
    ; Parse "return"
    movq %r12, %rsi
    leaq return_str(%rip), %rdi
    call match_keyword
    test %rax, %rax
    jz .error
    addq $6, %r12                ; Skip "return"
    
    ; Skip whitespace
    movq %r12, %rsi
    call skip_spaces
    movq %rsi, %r12
    
    ; Parse number
    movq %r12, %rsi
    call parse_number
    movq %rsi, %r12
    movq %rax, %r13              ; r13 = parsed number
    
    ; Generate code for return
    call emit_main_start
    movq %r13, %rdi
    call emit_return_value
    call emit_main_end
    
    movq %rbp, %rsp
    popq %rbp
    ret
    
.error:
    ; Print error and exit
    movq $1, %rdi
    leaq error_msg(%rip), %rsi
    movq $error_len, %rdx
    movq $0x2000004, %rax
    syscall
    movq $1, %rdi
    movq $0x2000001, %rax
    syscall

; Skip whitespace
skip_spaces:
    movb (%rsi), %al
    cmpb $32, %al                ; space
    je .skip
    cmpb $9, %al                 ; tab
    je .skip
    cmpb $10, %al                ; newline
    je .skip
    ret
.skip:
    incq %rsi
    jmp skip_spaces

; Match keyword
; rsi = input, rdi = keyword
; returns rax = 1 if match, 0 if not
match_keyword:
    pushq %rsi
    pushq %rdi
.loop:
    movb (%rsi), %al
    movb (%rdi), %cl
    testb %cl, %cl
    jz .match                    ; End of keyword
    cmpb %al, %cl
    jne .nomatch
    incq %rsi
    incq %rdi
    jmp .loop
.match:
    movq $1, %rax
    popq %rdi
    popq %rsi
    ret
.nomatch:
    xorq %rax, %rax
    popq %rdi
    popq %rsi
    ret

; Parse decimal number
; rsi = input
; returns: rax = number, rsi = updated position
parse_number:
    xorq %rax, %rax              ; result = 0
.loop:
    movb (%rsi), %cl
    cmpb $48, %cl                ; '0'
    jb .done
    cmpb $57, %cl                ; '9'
    ja .done
    
    ; result = result * 10 + (cl - '0')
    imulq $10, %rax
    subb $48, %cl
    movzbq %cl, %rcx
    addq %rcx, %rax
    incq %rsi
    jmp .loop
.done:
    ret

; Emit assembly header
emit_header:
    movq $1, %rdi
    leaq asm_header(%rip), %rsi
    movq $asm_header_len, %rdx
    movq $0x2000004, %rax
    syscall
    ret

; Emit main function start
emit_main_start:
    movq $1, %rdi
    leaq main_start(%rip), %rsi
    movq $main_start_len, %rdx
    movq $0x2000004, %rax
    syscall
    ret

; Emit return with value in rdi
emit_return_value:
    pushq %rdi                   ; Save value
    
    ; Print "    movq $"
    movq $1, %rdi
    leaq movq_prefix(%rip), %rsi
    movq $movq_prefix_len, %rdx
    movq $0x2000004, %rax
    syscall
    
    ; Print the number
    popq %rdi
    call print_decimal
    
    ; Print ", %rax\n"
    movq $1, %rdi
    leaq movq_suffix(%rip), %rsi
    movq $movq_suffix_len, %rdx
    movq $0x2000004, %rax
    syscall
    ret

; Emit main function end
emit_main_end:
    movq $1, %rdi
    leaq main_end(%rip), %rsi
    movq $main_end_len, %rdx
    movq $0x2000004, %rax
    syscall
    ret

; Print decimal number in rdi
print_decimal:
    pushq %rbp
    movq %rsp, %rbp
    subq $32, %rsp               ; Buffer
    
    ; Handle zero
    testq %rdi, %rdi
    jnz .not_zero
    movb $48, -1(%rbp)           ; '0'
    movq $1, %rdi
    leaq -1(%rbp), %rsi
    movq $1, %rdx
    movq $0x2000004, %rax
    syscall
    jmp .done
    
.not_zero:
    ; Convert to decimal (backwards)
    movq %rdi, %rax
    leaq -1(%rbp), %rdi
    movb $0, (%rdi)              ; null terminator
    
.convert:
    testq %rax, %rax
    jz .print
    
    xorq %rdx, %rdx
    movq $10, %rcx
    divq %rcx                    ; rax/10, remainder in rdx
    
    addb $48, %dl                ; Convert to ASCII
    decq %rdi
    movb %dl, (%rdi)
    jmp .convert
    
.print:
    ; Calculate length
    leaq -1(%rbp), %rax
    subq %rdi, %rax
    movq %rax, %rdx              ; length
    
    movq %rdi, %rsi              ; string start
    movq $1, %rdi                ; stdout
    movq $0x2000004, %rax        ; write
    syscall
    
.done:
    movq %rbp, %rsp
    popq %rbp
    ret

; Data section
.section __DATA,__data

banner:
    .ascii "=== Tempo Native Compiler v2.0 ===\n"
    .ascii "100% Assembly - A masterpiece\n"
    .ascii "By Ignacio Peña Sepúlveda\n\n"
    .ascii "Compiling: return 42;\n\n"
    .ascii "Generated assembly:\n"
    .ascii "-------------------\n"
.set banner_len, . - banner

test_input:
    .asciz "return 42;"

return_str:
    .asciz "return"

error_msg:
    .ascii "Parse error!\n"
.set error_len, . - error_msg

asm_header:
    .ascii ".section __TEXT,__text\n"
    .ascii ".globl _main\n\n"
.set asm_header_len, . - asm_header

main_start:
    .ascii "_main:\n"
    .ascii "    pushq %rbp\n"
    .ascii "    movq %rsp, %rbp\n"
.set main_start_len, . - main_start

movq_prefix:
    .ascii "    movq $"
.set movq_prefix_len, . - movq_prefix

movq_suffix:
    .ascii ", %rax\n"
.set movq_suffix_len, . - movq_suffix

main_end:
    .ascii "    popq %rbp\n"
    .ascii "    ret\n"
.set main_end_len, . - main_end