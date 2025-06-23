# Tempo Compiler Development Issues

## Problemas Encontrados al Crear el Compilador Nativo

### 1. Limitaciones del Compilador C Actual (v1.3.1)

Durante el desarrollo del compilador Tempo 100% nativo para macOS, encontramos las siguientes limitaciones en el compilador C existente:

#### a) No soporta la palabra clave "type" como nombre de campo
- **Problema**: `struct Token { type: int32, ... }` falla
- **Error**: "Expected field name"
- **Solución**: Renombrar a `token_type`

#### b) No soporta arrays en structs
- **Problema**: `value: array<char, 256>` no es reconocido
- **Error**: "Expected ',' or '}' after field"
- **Solución**: Usar punteros en su lugar

#### c) No soporta "extern" 
- **Problema**: `extern function malloc(...)` no es reconocido
- **Error**: "Expected top-level declaration"
- **Solución**: Usar inline assembly o intrínsecos

#### d) Limitaciones en el sistema de módulos
- No soporta `module` keyword
- No soporta `import` statements
- No soporta `use` declarations
- No soporta `export` modifiers

### 2. Incompatibilidades con macOS

#### a) Convenciones de llamada diferentes
- macOS x86_64 usa System V AMD64 ABI
- Los primeros 6 argumentos van en registros: rdi, rsi, rdx, rcx, r8, r9
- El valor de retorno va en rax

#### b) Números de syscall específicos de macOS
```
SYS_exit:  0x2000001 (no 60 como en Linux)
SYS_write: 0x2000004 (no 1 como en Linux)
SYS_read:  0x2000003 (no 0 como en Linux)
```

#### c) Prefijo underscore en símbolos
- macOS requiere `_main` en lugar de `main`
- Todos los símbolos globales necesitan prefijo `_`

### 3. Soluciones Implementadas

Para crear un compilador Tempo 100% nativo, tuvimos que:

1. **Simplificar el lenguaje**: Usar solo características soportadas por el compilador C actual
2. **Usar intrínsecos**: `memory_read8`, `memory_write8` en lugar de arrays
3. **Inline assembly**: Para syscalls directas sin dependencias externas
4. **Bootstrapping gradual**: Primero compilar con C, luego recompilar con sí mismo

### 4. Próximos Pasos

1. **Mejorar el compilador C**: Agregar soporte para las características faltantes
2. **Crear compilador mínimo**: Que soporte justo lo necesario para auto-compilarse
3. **Bootstrap completo**: Compilador Tempo que se compile a sí mismo sin C

### 5. Lecciones Aprendidas

- El dogfooding revela limitaciones rápidamente
- Es crítico tener un subset del lenguaje que funcione para bootstrapping
- Las diferencias entre plataformas (Linux vs macOS) son significativas
- Un compilador debe poder compilar su propio código fuente

## Relevancia para la Comunidad

Estos problemas son importantes de documentar porque:

1. **Transparencia**: Mostrar el proceso real de desarrollo
2. **Educación**: Otros pueden aprender de nuestros errores
3. **Colaboración**: La comunidad puede ayudar a resolver estos problemas
4. **Historia**: Documentar la evolución del compilador

Cuando publiquemos Tempo v2.0.0, incluiremos esta documentación para mostrar el camino recorrido.