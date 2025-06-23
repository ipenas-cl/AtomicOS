# Tempo Quick Start Guide

¡Comienza a programar en Tempo en menos de 5 minutos!

## 🚀 Instalación Rápida

### Opción 1: Instalación Global (Recomendada)
```bash
# Clonar el repositorio
git clone https://github.com/atomicos/tempo
cd tempo

# Instalar globalmente
./scripts/install.sh

# Verificar instalación
tempo --version
```

### Opción 2: Uso Local
```bash
# Clonar y usar directamente
git clone https://github.com/atomicos/tempo
cd tempo
./demo_tempo_toolchain.sh  # Ver demostración
```

## 🎯 Tu Primer Programa

### 1. Crear archivo
```tempo
// hello.tempo
fn main() -> i32 {
    return 42;
}
```

### 2. Ejecutar
```bash
# Con instalación global
tempo run hello.tempo

# Con instalación local
./build/tempo hello.tempo
as -arch x86_64 hello.s -o hello.o
clang -o hello hello.o
./hello
```

### 3. Verificar resultado
```bash
echo $?  # Debe mostrar: 42
```

## 📚 Ejemplos Listos para Usar

### Ejecutar ejemplos
```bash
# Después de la instalación global
tempo examples
tempo run /usr/local/share/tempo/examples/beginner/01_hello_world.tempo
tempo run /usr/local/share/tempo/examples/projects/calculator.tempo

# O copiar a directorio local
cp -r /usr/local/share/tempo/examples .
tempo run examples/beginner/02_variables.tempo
```

### Ejemplos incluidos
- 🟢 **Principiante**: Variables, funciones, control de flujo
- 🟡 **Intermedio**: Structs, arrays, punteros seguros  
- 🔴 **Avanzado**: Tiempo real, seguridad, sistemas
- 🚀 **Proyectos**: Calculadora completa, más por venir

## 🎓 Aprender Tempo

### Tutorial Interactivo
```bash
tempo learn  # Tutorial paso a paso
```

### Documentación Completa
```bash
# Ver documentación
ls /usr/local/share/tempo/docs/

# O en línea
open docs/learning/README.md
```

### Ruta de Aprendizaje
1. **[Conceptos Básicos](docs/learning/01-introduction.md)** (30 min)
2. **[Variables y Tipos](examples/beginner/02_variables.tempo)** (30 min)
3. **[Funciones](examples/beginner/03_functions.tempo)** (30 min)
4. **[Estructuras](examples/intermediate/05_structs.tempo)** (1 hora)
5. **[Proyecto Completo](examples/projects/calculator.tempo)** (2 horas)

## 🔧 Comandos Principales

```bash
# Compilar solamente
tempo compile program.tempo program.s

# Compilar y ejecutar
tempo run program.tempo

# Verificar sintaxis
tempo check program.tempo

# Ver ejemplos
tempo examples

# Tutorial interactivo
tempo learn

# Información de versión
tempo version

# Ayuda
tempo help
```

## ✨ Características Únicas de Tempo

### 🛡️ Seguridad de Memoria
```tempo
// Imposible buffer overflow
let array: [10]i32;
array[15] = 42;  // Error automático
```

### ⏱️ Tiempo Real
```tempo
@wcet(100)  // Máximo 100 microsegundos
fn critical_task() -> void {
    // Timing garantizado
}
```

### 🔒 Seguridad por Niveles
```tempo
@security_level(3)  // Solo kernel
fn kernel_function() -> void {
    // Código privilegiado
}
```

### 🎯 Determinismo
```tempo
// SIEMPRE produce el mismo resultado
fn deterministic_calc(x: i32) -> i32 {
    return x * 2 + 1;  // Sin undefined behavior
}
```

## 🔍 Debugging y Desarrollo

### Verificar compilación
```bash
# Verificar que compile
tempo check my_program.tempo

# Ver assembly generado
tempo compile my_program.tempo my_program.s
cat my_program.s
```

### Common Patterns
```tempo
// Estructura típica de programa Tempo
fn helper_function(param: i32) -> i32 {
    return param * 2;
}

fn main() -> i32 {
    let value: i32 = 21;
    let result: i32 = helper_function(value);
    return result;  // 42
}
```

## 🆘 Solución de Problemas

### Problema: "Command not found: tempo"
```bash
# Verificar PATH
echo $PATH | grep /usr/local/bin

# Reinstalar si es necesario
./scripts/install.sh
```

### Problema: "Assembly failed"
```bash
# Verificar que tienes Xcode Command Line Tools
xcode-select --install

# Verificar assembler
which as
```

### Problema: "Linking failed"
```bash
# Verificar compilador C
which clang
which gcc

# Instalar si es necesario
xcode-select --install
```

## 🚀 Siguientes Pasos

### Para Principiantes
1. Complete todos los ejemplos en `examples/beginner/`
2. Lea la documentación en `docs/learning/`
3. Construya el proyecto calculadora

### Para Programadores Experimentados
1. Revise `examples/advanced/` para características únicas
2. Lea sobre tiempo real y seguridad
3. Contribuya al proyecto AtomicOS

### Para Desarrolladores de Sistemas
1. Estudie `src/kernel/kernel_tempo_pure.tempo`
2. Entienda las anotaciones @wcet y @security_level
3. Experimente con drivers en Tempo

## 🎉 ¡Felicidades!

Ahora tienes instalado el primer compilador determinístico self-hosted del mundo. Estás programando con tecnología que:

- ✅ **Elimina clases enteras de bugs**
- ✅ **Garantiza timing en tiempo real**  
- ✅ **Provee seguridad de memoria automática**
- ✅ **Permite verificación formal**

¡Bienvenido al futuro de la programación! 🌟

## 📞 Ayuda

- **Documentación**: `docs/` o `tempo help`
- **Ejemplos**: `examples/` o `tempo examples`  
- **Tutorial**: `tempo learn`
- **Comunidad**: GitHub Issues/Discussions

**¡Happy deterministic programming!** 🎯