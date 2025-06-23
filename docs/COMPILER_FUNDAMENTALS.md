# Fundamentos de Compiladores: De Assembly a Lenguaje de Alto Nivel

## Capítulo 1: ¿Qué es un Compilador?

### Definición Simple

Un compilador es un **traductor**:
- **Entrada**: Código que humanos pueden leer
- **Salida**: Código que la máquina puede ejecutar

### Ejemplo Real de Nuestro Compilador

```
ENTRADA:  return 69;
PROCESO:  [PARSER] → [ANALYZER] → [GENERATOR]
SALIDA:   movq $69, %rax
          ret
```

## Capítulo 2: Anatomía de un Compilador

### 2.1 Análisis Léxico (Lexer)

**Propósito**: Convertir caracteres en tokens

```
Entrada: "return 69;"
Salida:  [RETURN] [NUMBER:69] [SEMICOLON]
```

**Nuestro Código**:
```assembly
parse_num:
    xorq %rax, %rax         ; resultado = 0
digit_loop:
    movb (%r12), %cl        ; leer carácter
    cmpb $48, %cl           ; ¿es >= '0'?
    jb emit_value           ; no, terminar
    cmpb $57, %cl           ; ¿es <= '9'?
    ja emit_value           ; no, terminar
    
    imulq $10, %rax         ; resultado *= 10
    subb $48, %cl           ; convertir ASCII a número
    movzbq %cl, %rcx
    addq %rcx, %rax         ; resultado += dígito
    incq %r12               ; siguiente carácter
    jmp digit_loop
```

### 2.2 Análisis Sintáctico (Parser)

**Propósito**: Verificar que la estructura es correcta

```
Correcto:   return 42;
Incorrecto: 42 return;
```

**Nuestro Código**:
```assembly
check_r:
    cmpb $114, %al          ; ¿es 'r'?
    jne parse_error         ; no, error
    ; Si es 'r', esperamos "eturn"
```

### 2.3 Generación de Código

**Propósito**: Producir código ejecutable

**Nuestro Código**:
```assembly
emit_value:
    movq %rax, %r13         ; guardar valor parseado
    
    ; Imprimir "    movq $"
    movq $1, %rdi
    leaq ret_prefix(%rip), %rsi
    movq $ret_prefix_len, %rdx
    movq $0x2000004, %rax   ; syscall write
    syscall
    
    ; Imprimir el número
    movq %r13, %rdi
    call print_num
```

## Capítulo 3: Compiladores Determinísticos

### ¿Qué es Determinismo?

**Determinístico**: Mismo input → Mismo output → **Mismo tiempo**

### Por Qué Importa

1. **Seguridad**: No hay timing attacks
2. **Confiabilidad**: Comportamiento predecible
3. **Verificabilidad**: Se puede probar correctitud

### Ejemplo de No-Determinismo (MAL)

```c
// C con malloc - NO determinístico
char* buffer = malloc(size);  // ¿Cuánto tarda? ¡No sabemos!
```

### Ejemplo de Determinismo (BIEN)

```assembly
; Assembly con buffer fijo - Determinístico
subq $256, %rsp  ; SIEMPRE toma 1 ciclo
```

## Capítulo 4: Técnicas de Implementación

### 4.1 Registros como Variables

En lugar de memoria dinámica, usamos registros:

```assembly
; r12 = puntero al input
; r13 = valor parseado
; r14 = estado del parser
```

**Ventajas**:
- Acceso en 1 ciclo
- No cache misses
- Totalmente predecible

### 4.2 Parsing Directo

Sin estructuras intermedias:

```
Tradicional: Texto → Tokens → AST → IR → Assembly
Nuestro:     Texto → Assembly (directo)
```

**Ventajas**:
- Menos memoria
- Menos pasos
- Más rápido
- Más simple

### 4.3 Manejo de Errores Determinístico

```assembly
parse_error:
    movq $1, %rdi           ; código de error
    movq $0x2000001, %rax   ; exit syscall
    syscall                 ; SIEMPRE toma el mismo tiempo
```

## Capítulo 5: Syscalls y el Sistema Operativo

### ¿Qué es una Syscall?

Manera de pedirle al OS que haga algo por nosotros.

### Syscalls de macOS x86_64

```assembly
; Escribir a stdout
movq $1, %rdi              ; fd = 1 (stdout)
movq $buffer, %rsi         ; qué escribir
movq $length, %rdx         ; cuántos bytes
movq $0x2000004, %rax      ; número de syscall write
syscall                    ; hacer la llamada
```

### Por Qué No Usar libc

libc añade:
- Overhead impredecible
- Buffering oculto
- Comportamiento no determinístico

Syscalls directas dan:
- Control total
- Timing predecible
- Sin sorpresas

## Capítulo 6: El Proceso Completo

### Paso 1: Leer Input

```assembly
leaq input(%rip), %r12     ; r12 apunta al código fuente
```

### Paso 2: Parsear

```assembly
call parse_return          ; buscar y parsear "return X;"
```

### Paso 3: Generar

```assembly
call emit_header          ; .section __TEXT,__text
call emit_value           ; movq $X, %rax
call emit_footer          ; ret
```

### Paso 4: Resultado

```bash
$ ./tempo_compiler > output.s
$ cat output.s
.section __TEXT,__text
.globl _main
_main:
    pushq %rbp
    movq %rsp, %rbp
    movq $69, %rax
    popq %rbp
    ret
```

## Capítulo 7: Ejercicios Prácticos

### Ejercicio 1: Agregar Soporte para Comentarios

Modificar el parser para ignorar `// comentario`

### Ejercicio 2: Múltiples Statements

Soportar:
```
return 42;
return 69;  // usar el último
```

### Ejercicio 3: Expresiones Simples

Soportar:
```
return 40 + 2;
```

### Ejercicio 4: Variables

Soportar:
```
let x = 42;
return x;
```

## Capítulo 8: Medición y Verificación

### Medir Ciclos

```assembly
rdtsc                      ; leer timestamp counter
; ... código a medir ...
rdtsc                      ; leer de nuevo
; restar para obtener ciclos
```

### Verificar Determinismo

1. Ejecutar 1000 veces
2. Medir ciclos cada vez
3. Verificar que sea SIEMPRE igual

## Capítulo 9: Optimizaciones Determinísticas

### Lo Que SÍ Hacer

- Desenrollar loops con límite fijo
- Usar instrucciones SIMD
- Precalcular constantes

### Lo Que NO Hacer

- Cache dinámico
- Branches dependientes de datos
- Allocación dinámica

## Capítulo 10: El Futuro

### Próximas Características

1. **Type Checking**: Verificar tipos en compile-time
2. **WCET Analysis**: Calcular tiempo máximo
3. **Security Levels**: Enforce en hardware
4. **Formal Verification**: Probar correctitud

### Bootstrap

El objetivo final:
```
tempo_compiler.s → tempo_compiler (assembly)
tempo_compiler.tempo → tempo_compiler (Tempo)
Compilar el segundo con el primero
Compilar el segundo consigo mismo
¡Bootstrap completo!
```

## Recursos para Profundizar

### Libros Esenciales
- "Engineering a Compiler" - Cooper & Torczon
- "Modern Compiler Implementation" - Appel
- "The Dragon Book" - Aho, Sethi, Ullman

### Papers Importantes
- "Proof-Carrying Code" - Necula
- "CompCert: A Verified Compiler" - Leroy
- "Deterministic Parallel Programming" - Bocchino

### Nuestro Código
- `tools/tempo_compiler_full.s` - Compilador completo
- `docs/TEMPO_NATIVE_SUCCESS.md` - Historia del desarrollo
- `examples/` - Programas Tempo de ejemplo

## Conclusión

Crear un compilador no requiere magia, solo:
1. **Entender** qué debe hacer
2. **Diseñar** cómo hacerlo
3. **Implementar** paso a paso
4. **Verificar** que funcione
5. **Optimizar** manteniendo correctitud

Con 400 líneas de assembly creamos un compilador real.
Sin C. Sin dependencias. Sin excusas.

**El conocimiento es poder. El poder debe ser compartido.**

---

*"Un compilador es solo un programa que escribe programas. No hay magia, solo lógica."*

Ignacio Peña Sepúlveda
2025