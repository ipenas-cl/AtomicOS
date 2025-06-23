# Herramientas de Desarrollo de AtomicOS

## Compiladores de Tempo

### tempo_compiler_v3.c
- **Estado**: Estable (v3.0.0)
- **Uso en producción**: Sí
- **Características**: 
  - Lexer y parser básicos
  - Análisis WCET
  - Análisis de seguridad
  - Generación de código x86
- **Compilación**: `gcc -o tempo_compiler tempo_compiler_v3.c`
- **Uso**: `./tempo_compiler input.tempo output.s`

### tempo_compiler_v3_enhanced.c
- **Estado**: Experimental
- **Características**: 
  - Basado en v3.0.0
  - Optimizaciones de código
  - Mejor análisis WCET
- **Notas**: En evaluación para fusión con v3

### tempo_compiler_v4.c
- **Estado**: En desarrollo (no funcional)
- **Problemas conocidos**: 
  - Parser incompleto
  - Errores en análisis sintáctico
- **Objetivo**: Soporte completo del lenguaje Tempo

## Módulos de Soporte

### tempo_optimizer.c/h
- Optimizaciones del código generado
- Reducción de instrucciones redundantes
- Mejora del WCET

### tempo_debug.c/h
- Generación de información de depuración
- Soporte para símbolos de debug
- Integración con herramientas de desarrollo

## Herramientas Adicionales

### validate_wcet.sh
- Script para validar límites WCET
- Verifica que el código generado cumpla las garantías temporales

## Compilación

```bash
# Compilar el compilador principal
make build/tempo_compiler

# Compilar con optimizador (experimental)
gcc -o tempo_enhanced tempo_compiler_v3_enhanced.c tempo_optimizer.c tempo_debug.c

# Ejecutar tests
make test
```

## Roadmap

1. **v3.1**: Parser extendido (en planificación)
2. **v3.2**: Análisis semántico
3. **v3.3**: WCET mejorado
4. **v3.4**: Seguridad robusta
5. **v3.5**: Optimizaciones avanzadas

Ver `docs/tempo_improvement_plan.md` para detalles.