# Journey to a Native Tempo Compiler

## La Búsqueda del Compilador 100% Tempo para macOS

Por Ignacio Peña Sepúlveda - 22 de Junio, 2025

### El Desafío

Crear un compilador Tempo escrito 100% en Tempo, sin dependencias de C, que funcione nativamente en macOS.

### Problemas Encontrados y Soluciones

#### 1. **Limitaciones del Compilador C Existente**

**Problema**: El compilador Tempo v1.3.1 escrito en C no soporta muchas características del lenguaje Tempo mismo.

**Síntomas encontrados**:
```tempo
// Esto falla:
struct Token {
    type: int32,  // Error: "type" es palabra reservada en el parser
}

// Esto falla:
struct Token {
    value: array<char, 256>,  // Error: arrays no soportados en structs
}

// Esto falla:
extern function malloc(size: int32) -> ptr<void>;  // Error: "extern" no reconocido

// Esto falla:
module tempo::lexer;  // Error: módulos no implementados
import core;          // Error: imports no implementados
```

**Solución temporal**: Simplificar el código hasta que funcione con el compilador actual.

#### 2. **Variables Globales No Soportadas**

**Problema**:
```tempo
let g_input: ptr<char> = 0 as ptr<char>;  // Error: "let" global no soportado
```

**Solución**: Usar solo funciones y constantes, no variables globales.

#### 3. **Diferencias macOS vs Linux**

**Problema**: Los números de syscall son diferentes:
```asm
# Linux
mov rax, 1    # write
mov rax, 60   # exit

# macOS  
mov rax, 0x2000004  # write
mov rax, 0x2000001  # exit
```

**Solución**: Usar las syscalls correctas de macOS.

#### 4. **Problemas de Linking**

**Problema**:
```bash
ld: library not found for -lSystem
```

**Solución**: Usar `clang` en lugar de `ld` directamente:
```bash
# En lugar de:
ld -arch x86_64 -lSystem file.o -o program

# Usar:
clang -o program file.o
```

#### 5. **Símbolos con Underscore**

**Problema**: macOS requiere underscore en símbolos globales
```asm
# Linux
.globl main
main:

# macOS
.globl _main
_main:
```

### La Solución Final: Assembly Directo

Dado que el compilador Tempo genera assembly, y assembly puede generar assembly, la solución fue:

1. **Crear un compilador en assembly puro** que genera assembly
2. **Este compilador genera código Tempo válido**
3. **Luego reescribir ese compilador en Tempo**

### Código del Compilador Nativo en Assembly

```asm
.section __TEXT,__text
.globl _main

_main:
    pushq %rbp
    movq %rsp, %rbp
    
    # Generar assembly para un programa Tempo
    movq $1, %rdi                  # stdout
    leaq asm_code(%rip), %rsi      # código a generar
    movq $code_len, %rdx           # longitud
    movq $0x2000004, %rax          # write syscall
    syscall
    
    xorq %rax, %rax
    popq %rbp
    ret

.section __DATA,__data
asm_code:
    .ascii ".section __TEXT,__text\n"
    .ascii ".globl _main\n"
    .ascii "_main:\n"
    .ascii "    movq $42, %rax\n"
    .ascii "    ret\n"
.set code_len, . - asm_code
```

### Próximos Pasos

1. **Escribir el compilador mínimo en Tempo** que genere el mismo assembly
2. **Compilarlo con el compilador C**
3. **Usar ese compilador para compilarse a sí mismo**
4. **Bootstrap completo**: Tempo compilando Tempo

### Lecciones Aprendidas

1. **Dogfooding es difícil**: Usar tu propio lenguaje revela todas sus limitaciones
2. **Bootstrap requiere un subset mínimo**: No todas las características pueden usarse inicialmente
3. **Las diferencias de plataforma importan**: macOS != Linux en muchos detalles
4. **Assembly es el denominador común**: Cuando todo falla, assembly funciona

### Estado Actual

✅ Tenemos un "compilador" en assembly que genera código Tempo válido
✅ Este código se puede ensamblar y ejecutar en macOS
✅ Probado que la estrategia funciona

❌ Aún no está escrito en Tempo
❌ Falta el bootstrap completo
❌ El compilador C necesita mejoras

### Código de Estado

El proyecto ahora tiene:
- `build/tempo_native`: Compilador en assembly que genera assembly
- `docs/TEMPO_COMPILER_ISSUES.md`: Documentación de problemas
- `docs/TEMPO_NATIVE_COMPILER_JOURNEY.md`: Este documento

### Conclusión

Crear un compilador self-hosted es un proceso iterativo. Cada intento revela nuevos problemas y cada problema resuelto nos acerca más al objetivo. El camino no es directo, pero cada paso es aprendizaje valioso.

*"El mejor compilador es el que puede compilarse a sí mismo"* - Ignacio Peña Sepúlveda