# Tutorial: Crea Tu Propio Compilador Sin C

## De Cero a Compilador en 7 Pasos

### Introducción

Este tutorial te enseñará a crear un compilador **sin usar C**. Solo assembly y determinación.

**Requisitos**:
- Una computadora (macOS, Linux, o Windows con WSL)
- Ganas de aprender
- Nada más

### Paso 1: Entender Qué Hace Un Compilador

Un compilador transforma:
```
ENTRADA: print("Hola")
SALIDA:  movq $mensaje, %rsi
         call _print
```

**¡Eso es todo!** No hay magia. Solo transformación de texto.

### Paso 2: Tu Primer "Compilador" en Assembly

Crea `mini_compiler.s`:

```asm
# Un compilador que siempre genera "return 42"
.globl _main
.section __TEXT,__text

_main:
    # Imprimir el programa compilado
    movq $1, %rdi                    # stdout
    leaq program(%rip), %rsi         # nuestro "código compilado"
    movq $program_len, %rdx          # longitud
    movq $0x2000004, %rax           # write (macOS)
    syscall
    
    # Salir
    xorq %rax, %rax                 # return 0
    ret

.section __DATA,__data
program:
    .ascii ".globl _main\n"
    .ascii "_main:\n"
    .ascii "    movq $42, %rax\n"
    .ascii "    ret\n"
program_len = . - program
```

**Compilar y ejecutar**:
```bash
# Ensamblar
as mini_compiler.s -o mini_compiler.o

# Linkear
clang -o mini_compiler mini_compiler.o

# Ejecutar (genera assembly)
./mini_compiler > output.s

# Ver lo que generó
cat output.s
```

**¡Felicidades! Acabas de crear un compilador sin C.**

### Paso 3: Hacerlo Más Inteligente

Ahora hagamos que lea entrada y genere código según lo que lee:

```asm
# Compilador que lee un número y genera código para retornarlo
.globl _main
.section __TEXT,__text

_main:
    pushq %rbp
    movq %rsp, %rbp
    
    # Leer un carácter (el número)
    xorq %rdi, %rdi                 # stdin
    leaq input_buffer(%rip), %rsi   # buffer
    movq $1, %rdx                   # leer 1 byte
    movq $0x2000003, %rax          # read syscall
    syscall
    
    # Generar código según el input
    # ... (aquí va la lógica)
    
    popq %rbp
    ret

.section __DATA,__data
input_buffer: .byte 0
```

### Paso 4: Parser Básico Sin C

Conceptos clave:
1. **Lexer**: Convierte caracteres en tokens
2. **Parser**: Convierte tokens en acciones
3. **Codegen**: Genera assembly

Todo en assembly puro:

```asm
# Función: es_digito
# Entrada: %rdi = carácter
# Salida: %rax = 1 si es dígito, 0 si no
es_digito:
    cmpq $'0', %rdi
    jl no_es_digito
    cmpq $'9', %rdi
    jg no_es_digito
    movq $1, %rax
    ret
no_es_digito:
    xorq %rax, %rax
    ret
```

### Paso 5: Generar Código Real

```asm
# Generar instrucción MOV con el valor leído
generar_mov:
    # Imprimir "    movq $"
    movq $1, %rdi
    leaq mov_prefix(%rip), %rsi
    movq $mov_prefix_len, %rdx
    movq $0x2000004, %rax
    syscall
    
    # Imprimir el número
    # ... convertir y imprimir
    
    # Imprimir ", %rax\n"
    movq $1, %rdi
    leaq mov_suffix(%rip), %rsi
    movq $mov_suffix_len, %rdx
    movq $0x2000004, %rax
    syscall
    ret

.section __DATA,__data
mov_prefix: .ascii "    movq $"
mov_prefix_len = . - mov_prefix
mov_suffix: .ascii ", %rax\n"
mov_suffix_len = . - mov_suffix
```

### Paso 6: Tu Lenguaje, Tus Reglas

Ahora puedes definir tu propia sintaxis:

```
MI_LENGUAJE:
devolver 42     → movq $42, %rax; ret
mostrar "hola"  → call print_string
repetir 5 {...} → loop 5 times
```

Sin C diciéndote qué es "válido" o "inválido".

### Paso 7: Bootstrap - El Compilador Se Compila a Sí Mismo

El santo grial: Un compilador escrito en su propio lenguaje.

```tempo
// compiler.tempo - Un compilador Tempo escrito en Tempo
function compile_file(input: ptr<char>) -> void {
    let tokens = tokenize(input);
    let ast = parse(tokens);
    generate_code(ast);
}

function main() -> int32 {
    // Compilar otros programas Tempo
    compile_file("program.tempo");
    return 0;
}
```

### Ejercicios Prácticos

1. **Básico**: Modifica el compilador para soportar diferentes números
2. **Intermedio**: Agrega soporte para suma (ej: "5+3")
3. **Avanzado**: Implementa variables ("x = 5")
4. **Experto**: Agrega funciones ("fun suma(a,b) { a+b }")

### Recursos Adicionales

**Documentación Assembly**:
- Intel x64: [Link a manual gratuito]
- ARM64: [Link a manual gratuito]
- RISC-V: [Link a manual gratuito]

**Herramientas Gratuitas**:
- `as` - Ensamblador (viene con tu OS)
- `objdump` - Ver código máquina
- `gdb` - Debugger

**Comunidad**:
- Discord: AtomicOS Compiler Creators
- Forum: compiladores-sin-c.org
- GitHub: Comparte tu compilador

### Preguntas Frecuentes

**¿Es más difícil que usar C?**
No. Es diferente. C agrega complejidad. Assembly es directo.

**¿Es más lento?**
No. Tu compilador puede ser tan rápido como quieras.

**¿Funciona en producción?**
Tempo es la prueba. 100% assembly → 100% funcional.

**¿Por qué nadie lo hace?**
Tradición. Miedo. Desconocimiento. Tú puedes cambiar eso.

### Conclusión

Has aprendido que:
1. No necesitas C para hacer un compilador
2. Assembly es suficiente
3. Puedes innovar sin pedir permiso
4. El conocimiento es libre

**Tu turno**: Crea tu lenguaje. Sin C. Sin límites. Sin excusas.

---

*"El mejor momento para crear tu compilador fue hace 20 años. El segundo mejor momento es ahora."*

**Comparte tu compilador**: #MiCompiladorSinC

Creado con ❤️ por Ignacio Peña Sepúlveda
100% Conocimiento Libre