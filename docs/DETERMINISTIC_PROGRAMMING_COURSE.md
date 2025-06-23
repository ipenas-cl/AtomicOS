# Curso: Programación Determinística y Compiladores

## Módulo 1: Fundamentos de Determinismo

### Clase 1.1: ¿Qué es el Determinismo?

#### Definición
**Determinismo**: Un sistema donde el mismo input SIEMPRE produce:
- El mismo output
- En el mismo tiempo
- Con los mismos recursos

#### Ejemplo Visual

```
NO DETERMINÍSTICO (Mal):
Input: 42 → Proceso → Output: 84 (3ms)
Input: 42 → Proceso → Output: 84 (7ms)  ← ¡Tiempo variable!
Input: 42 → Proceso → Output: 84 (2ms)  ← ¡Impredecible!

DETERMINÍSTICO (Bien):
Input: 42 → Proceso → Output: 84 (5ms)
Input: 42 → Proceso → Output: 84 (5ms)  ← ¡Siempre igual!
Input: 42 → Proceso → Output: 84 (5ms)  ← ¡Predecible!
```

### Clase 1.2: Por Qué Importa el Determinismo

#### 1. Seguridad
```c
// VULNERABLE - Timing attack
if (password[i] != input[i]) {
    return false;  // Retorna RÁPIDO si no coincide
}

// SEGURO - Constant time
diff |= password[i] ^ input[i];
// Siempre revisa TODO, mismo tiempo
```

#### 2. Sistemas Críticos
- **Aviones**: El piloto automático debe responder en X ms SIEMPRE
- **Medicina**: El marcapasos debe latir EXACTAMENTE cada segundo
- **Industria**: El robot debe parar en EXACTAMENTE 100ms

#### 3. Debugging
```
Determinístico: Bug reproducible 100% de las veces
No determinístico: "Funciona en mi máquina" 🤷
```

### Clase 1.3: Enemigos del Determinismo

#### 1. Memoria Dinámica
```c
malloc(size);  // ¿Cuánto tarda? ¿Quién sabe?
```

#### 2. Garbage Collection
```java
System.gc();  // ¿Cuándo corre? ¿Cuánto demora? ¡Misterio!
```

#### 3. Cache
```c
array[random_index];  // ¿Cache hit o miss? ¡Lotería!
```

#### 4. Interrupciones
```
Usuario mueve mouse → Interrupción → ¡Timing arruinado!
```

## Módulo 2: Diseñando un Lenguaje Determinístico

### Clase 2.1: Decisiones de Diseño

#### Memoria: Estática vs Dinámica

```tempo
// MAL - No determinístico
let buffer = malloc(size);  // NO EXISTE en Tempo

// BIEN - Determinístico
let buffer: array<u8, 1024>;  // Tamaño fijo, tiempo fijo
```

#### Loops: Acotados vs Infinitos

```tempo
// MAL - Potencialmente infinito
while (condition) {  // ¿Cuántas veces? No sabemos
    process();
}

// BIEN - Acotado
@wcet(1000)  // Máximo 1000 ciclos
for i in 0..100 {  // Exactamente 100 iteraciones
    process();
}
```

### Clase 2.2: Sistema de Tipos para Determinismo

#### Tipos con Garantías Temporales

```tempo
// Tipo que garantiza ejecución en tiempo constante
@constant_time
function crypto_compare(a: secure<u8, 32>, b: secure<u8, 32>) -> bool {
    let diff = 0;
    for i in 0..32 {  // Loop fijo
        diff |= a[i] ^ b[i];  // Sin branches
    }
    return diff == 0;
}
```

#### Tipos con Límites de Recursos

```tempo
// Función que garantiza usar máximo 1KB de stack
@stack_limit(1024)
function process_data(input: bounded_array<u8, 512>) -> result<u32> {
    let temp: array<u8, 512>;  // Compilador verifica: 512 + 512 < 1024 ✓
    // ...
}
```

### Clase 2.3: Análisis Estático

#### WCET (Worst Case Execution Time)

```tempo
@wcet(100)  // Esta función DEBE terminar en 100 ciclos
function quick_process(x: u32) -> u32 {
    // Compilador cuenta:
    let y = x * 2;     // 1 ciclo
    let z = y + 5;     // 1 ciclo
    return z;          // 1 ciclo
    // Total: 3 ciclos ✓ (< 100)
}
```

#### Verificación en Compile-Time

```tempo
@wcet(50)
function bad_example(n: u32) -> u32 {
    for i in 0..n {  // ERROR: Loop no acotado
        process();   // Compilador rechaza
    }
}

@wcet(50)
function good_example() -> u32 {
    for i in 0..10 {  // OK: Máximo 10 iteraciones
        process();    // Compilador puede verificar
    }
}
```

## Módulo 3: Construcción del Compilador

### Clase 3.1: Arquitectura Sin Capas Ocultas

#### Tradicional (Muchas Capas)
```
Código → Lexer → Parser → AST → Optimizer → IR → Backend → Assembly
         ↓       ↓        ↓      ↓          ↓     ↓
      (malloc) (malloc) (malloc) (cache)  (???) (malloc)
```

#### Nuestro Enfoque (Directo)
```
Código → Parser/Generator → Assembly
         ↓
    (registros fijos)
```

### Clase 3.2: Parser Determinístico

#### Sin Backtracking
```assembly
; MAL - Backtracking no determinístico
try_parse_function:
    ; intenta...
    jne try_parse_variable  ; falla? prueba otra cosa
    
; BIEN - Decisión directa
check_first_char:
    cmpb $'f', %al         ; ¿empieza con 'f'?
    je parse_function      ; sí: es función
    cmpb $'l', %al         ; ¿empieza con 'l'?  
    je parse_let           ; sí: es let
    jmp error              ; no: error
```

#### Memoria Fija
```assembly
; Toda la memoria del parser en el stack
subq $256, %rsp           ; 256 bytes, siempre
; No malloc, no sorpresas
```

### Clase 3.3: Generación de Código Predecible

#### Plantillas Fijas
```assembly
; Para "return N;" SIEMPRE generamos:
emit_return:
    ; 1. Prólogo (siempre igual)
    print("    pushq %rbp\n")
    print("    movq %rsp, %rbp\n")
    
    ; 2. Valor (solo cambia el número)
    print("    movq $")
    print_number(N)
    print(", %rax\n")
    
    ; 3. Epílogo (siempre igual)
    print("    popq %rbp\n")
    print("    ret\n")
```

## Módulo 4: Técnicas Avanzadas

### Clase 4.1: Optimizaciones Determinísticas

#### Loop Unrolling Estático
```tempo
// Original
@wcet(40)
for i in 0..4 {
    process(i);  // 4 * 10 = 40 ciclos
}

// Optimizado (unrolled)
@wcet(36)
process(0);  // 9 ciclos (sin overhead de loop)
process(1);  // 9 ciclos
process(2);  // 9 ciclos  
process(3);  // 9 ciclos
// Total: 36 ciclos (¡más rápido Y determinístico!)
```

#### Precálculo de Constantes
```tempo
// Compile-time
const TABLE: array<u32, 256> = compute_table();  // Se ejecuta AL COMPILAR

// Runtime  
function lookup(index: u8) -> u32 {
    return TABLE[index];  // Solo 1 acceso a memoria, tiempo fijo
}
```

### Clase 4.2: Manejo de Hardware

#### Cache Partitioning
```tempo
@cache_partition(1)  // Usa partición de cache 1
function critical_path() {
    // Esta función tiene su propia cache
    // Sin interferencia de otras funciones
}
```

#### Instrucciones de Tiempo Fijo
```assembly
; MAL - Tiempo variable
div %rcx  ; División: 20-80 ciclos según valor

; BIEN - Tiempo fijo
; Usar shift para división por potencia de 2
shr $3, %rax  ; Dividir por 8: SIEMPRE 1 ciclo
```

### Clase 4.3: Verificación y Testing

#### Test de Determinismo
```c
// Test que tu código es determinístico
void test_determinism() {
    uint64_t times[1000];
    
    for (int i = 0; i < 1000; i++) {
        uint64_t start = rdtsc();
        function_to_test(42);
        uint64_t end = rdtsc();
        times[i] = end - start;
    }
    
    // Verificar que todos los tiempos son IGUALES
    for (int i = 1; i < 1000; i++) {
        assert(times[i] == times[0]);
    }
}
```

#### Proof by Construction
```tempo
@wcet(100)
@proves("returns positive")
function abs(x: i32) -> u32 {
    if x >= 0 {
        return x as u32;
    } else {
        return (-x) as u32;
    }
    // Compilador verifica:
    // 1. WCET ≤ 100 ✓
    // 2. Siempre retorna ≥ 0 ✓
}
```

## Módulo 5: Proyecto Final

### Construir Tu Propio Compilador Determinístico

#### Requisitos
1. **Parser** para subset de Tempo
2. **WCET analysis** básico
3. **Code generator** para tu arquitectura
4. **Tests** de determinismo

#### Entregables
1. Código fuente (Assembly o Tempo)
2. Documentación de decisiones
3. Suite de tests
4. Demostración funcionando

#### Criterios de Evaluación
- ✓ Determinismo demostrable
- ✓ Código claro y documentado
- ✓ Tests comprehensivos
- ✓ Innovación en diseño

## Recursos del Curso

### Código Base
- `tools/tempo_compiler_full.s` - Compilador de referencia
- `examples/` - Programas Tempo de ejemplo
- `tests/` - Suite de testing

### Lecturas Obligatorias
1. "Fundamentos de Compiladores" (este documento)
2. "Why Skip C" - Filosofía del proyecto
3. "Tempo Native Success" - Case study

### Lecturas Complementarias
- "CompCert: Verified Compilation" 
- "seL4: Verified OS Kernel"
- "Timing Attacks and Defenses"

### Herramientas
- Assembly debugger (gdb)
- Cycle counter (rdtsc)
- Static analyzer (custom)

## Evaluación

### Parcial 1 (30%)
- Teoría de determinismo
- Análisis de código
- Identificar problemas

### Parcial 2 (30%)  
- Implementar parser simple
- Agregar WCET checking
- Demostrar determinismo

### Proyecto Final (40%)
- Compilador completo
- Documentación
- Presentación

## Mensaje Final

> "El determinismo no es una limitación, es una liberación.
> Cuando todo es predecible, todo es posible."

**Bienvenidos al futuro de la programación segura.**

---

Profesor: Ignacio Peña Sepúlveda
Curso: Programación Determinística
Universidad del Futuro, 2025