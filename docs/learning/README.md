# Aprende Tempo - Guía Completa

¡Bienvenido al mundo de la programación determinística! Esta guía te llevará desde los conceptos básicos hasta proyectos avanzados en Tempo.

## 🎯 ¿Por qué Tempo?

Tempo es el primer lenguaje de programación que combina:
- **Determinismo**: Resultados predecibles siempre
- **Seguridad**: Imposible tener buffer overflows
- **Tiempo Real**: Garantías matemáticas de timing
- **Simplicidad**: Fácil de aprender y usar

## 📚 Ruta de Aprendizaje

### Nivel Principiante (1-2 semanas)
1. **[Introducción](01-introduction.md)** - ¿Qué es programación determinística?
2. **[Conceptos Básicos](02-basics.md)** - Tipos, variables, funciones
3. **[Control de Flujo](03-control-flow.md)** - if, while, loops
4. **[Práctica](../examples/beginner/)** - Ejercicios hands-on

### Nivel Intermedio (2-3 semanas)
5. **[Estructuras](04-structs.md)** - Datos complejos y organización
6. **[Arrays y Memoria](05-arrays.md)** - Manejo seguro de memoria
7. **[Punteros Seguros](06-pointers.md)** - Referencias sin peligro
8. **[Práctica](../examples/intermediate/)** - Proyectos más complejos

### Nivel Avanzado (3-4 semanas)
9. **[Tiempo Real](07-real-time.md)** - @wcet y sistemas críticos
10. **[Seguridad](08-security.md)** - @security_level y capabilities
11. **[Sistemas](09-systems.md)** - Drivers y kernel modules
12. **[Proyectos](../examples/projects/)** - Aplicaciones completas

## 🛠️ Configuración del Entorno

### Instalación Rápida
```bash
# Clonar el repositorio
git clone https://github.com/atomicos/tempo
cd tempo

# Instalar herramientas
./scripts/install.sh

# Verificar instalación
tempo --version
```

### Primer Programa
```tempo
// hello.tempo
fn main() -> i32 {
    return 42;
}
```

```bash
# Compilar y ejecutar
tempo run hello.tempo
echo $?  # Muestra: 42
```

## 📖 Ejemplos por Categoría

### 🟢 Principiante
- **[Hello World](../examples/beginner/01_hello_world.tempo)** - Tu primer programa
- **[Variables](../examples/beginner/02_variables.tempo)** - Tipos y operaciones
- **[Funciones](../examples/beginner/03_functions.tempo)** - Definición y uso
- **[Control de Flujo](../examples/beginner/04_control_flow.tempo)** - if, while, loops

### 🟡 Intermedio  
- **[Estructuras](../examples/intermediate/05_structs.tempo)** - Datos complejos
- **[Arrays](../examples/intermediate/06_arrays.tempo)** - Manejo de memoria

### 🔴 Avanzado
- **[Tiempo Real](../examples/advanced/07_real_time.tempo)** - Sistemas críticos
- **[Seguridad](../examples/advanced/08_security.tempo)** - Programación segura

### 🚀 Proyectos
- **[Calculadora](../examples/projects/calculator.tempo)** - Proyecto completo

## 🎓 Metodología de Aprendizaje

### 1. Lee el Concepto
Cada capítulo explica un concepto con ejemplos claros.

### 2. Ejecuta los Ejemplos
Todos los ejemplos son ejecutables:
```bash
tempo run examples/beginner/01_hello_world.tempo
```

### 3. Modifica y Experimenta
Cambia los ejemplos para entender mejor:
```tempo
// Cambia esto:
fn main() -> i32 {
    return 42;
}

// Por esto:
fn main() -> i32 {
    let result: i32 = 42 + 8;
    return result;
}
```

### 4. Resuelve Ejercicios
Cada capítulo incluye ejercicios prácticos.

### 5. Construye Proyectos
Aplica lo aprendido en proyectos reales.

## 🔍 Conceptos Clave de Tempo

### Determinismo
```tempo
// Este código SIEMPRE produce el mismo resultado
fn deterministic_function(x: i32) -> i32 {
    return x * 2 + 1;
}
```

### Seguridad de Memoria
```tempo
// Imposible buffer overflow
let array: [10]i32;
array[15] = 42;  // Error en compile-time o runtime
```

### Tiempo Real
```tempo
@wcet(100)  // Máximo 100 microsegundos
fn critical_task() -> void {
    // Código con timing garantizado
}
```

### Seguridad por Niveles
```tempo
@security_level(3)  // Solo kernel puede ejecutar
fn kernel_function() -> void {
    // Código privilegiado
}
```

## 🤔 Preguntas Frecuentes

### ¿Es Tempo difícil de aprender?
No más que C, pero mucho más seguro. Si sabes C, Java o Python, puedes aprender Tempo en unas semanas.

### ¿Qué aplicaciones puedo hacer?
- Sistemas operativos
- Controladores de dispositivos
- Sistemas embebidos
- Aplicaciones de tiempo real
- Software crítico para seguridad

### ¿Es Tempo lento?
No. Tempo genera código assembly optimizado y tiene mejor performance predecible que C.

### ¿Puedo usar Tempo para aplicaciones web?
Sí, especialmente para backends que requieren alta confiabilidad y baja latencia.

## 🎯 Objetivos de Aprendizaje

Al completar esta guía, podrás:

✅ **Escribir programas seguros** sin buffer overflows  
✅ **Crear sistemas de tiempo real** con garantías matemáticas  
✅ **Desarrollar software crítico** para seguridad  
✅ **Entender programación determinística** profundamente  
✅ **Construir aplicaciones complejas** en Tempo  

## 🚀 ¡Comienza Ahora!

1. **[Lee la Introducción](01-introduction.md)**
2. **[Configura tu entorno](#configuración-del-entorno)**
3. **[Ejecuta tu primer programa](#primer-programa)**
4. **[Sigue la ruta de aprendizaje](#ruta-de-aprendizaje)**

## 📞 Ayuda y Comunidad

- **Documentación**: [docs/reference/](../reference/)
- **Ejemplos**: [examples/](../examples/)
- **Issues**: GitHub Issues
- **Discusiones**: GitHub Discussions

¡Bienvenido al futuro de la programación segura y determinística! 🎉