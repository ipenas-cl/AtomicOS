# Plan de Mejora del Compilador Tempo v3

## Análisis Actual

### Limitaciones Identificadas:
1. **Parser muy limitado**: Solo soporta funciones simples con return de números
2. **Sin gestión de memoria**: Memory leaks en AST y tokens
3. **Lexer básico**: No maneja errores de símbolos desconocidos
4. **Análisis de seguridad superficial**: Solo asigna niveles sin validación real
5. **Sin soporte para tipos**: Solo int32, sin parámetros de función
6. **Manejo de errores primitivo**: Aborta al primer error

## Plan de Mejora

### Fase 1: Parser Extendido (v3.1)
- [ ] Soporte para múltiples funciones
- [ ] Declaraciones `let` con tipos
- [ ] Estructuras de control: `if`, `while`, `for`
- [ ] Expresiones aritméticas y booleanas completas
- [ ] Parámetros de función con tipos
- [ ] Llamadas a funciones

### Fase 2: Análisis Semántico (v3.2)
- [ ] Tabla de símbolos para variables y funciones
- [ ] Verificación de tipos
- [ ] Detección de variables no declaradas
- [ ] Validación de llamadas a funciones
- [ ] Análisis de alcance (scope)

### Fase 3: WCET Mejorado (v3.3)
- [ ] Modelo más preciso considerando:
  - Ramas condicionales (peor caso)
  - Bucles con límites estáticos
  - Cache y pipeline del i386
- [ ] Anotaciones WCET por bloque
- [ ] Verificación de cumplimiento WCET

### Fase 4: Seguridad Robusta (v3.4)
- [ ] Verificación de acceso a memoria
- [ ] Validación de niveles de privilegio
- [ ] Detección de operaciones no determinísticas
- [ ] Análisis de flujo de información

### Fase 5: Optimizaciones (v3.5)
- [ ] Eliminación de código muerto
- [ ] Propagación de constantes
- [ ] Reducción de fuerza (strength reduction)
- [ ] Optimizaciones específicas para WCET

## Estructura Propuesta

```
tools/
├── tempo_compiler_v3/
│   ├── lexer.c          # Análisis léxico
│   ├── lexer.h
│   ├── parser.c         # Análisis sintáctico
│   ├── parser.h
│   ├── ast.c            # Manejo del AST
│   ├── ast.h
│   ├── semantic.c       # Análisis semántico
│   ├── semantic.h
│   ├── wcet.c           # Análisis WCET
│   ├── wcet.h
│   ├── security.c       # Análisis de seguridad
│   ├── security.h
│   ├── codegen.c        # Generación de código
│   ├── codegen.h
│   ├── error.c          # Manejo de errores
│   ├── error.h
│   └── main.c           # Punto de entrada
```

## Ejemplos de Mejoras

### Parser Mejorado
```c
// Soporte para expresiones complejas
ast_node_t* parse_expression() {
    ast_node_t* left = parse_term();
    
    while (current_token.type == TOK_PLUS || 
           current_token.type == TOK_MINUS) {
        token_type_t op = current_token.type;
        advance();
        ast_node_t* right = parse_term();
        left = create_binary_op(left, op, right);
    }
    
    return left;
}
```

### Análisis WCET Mejorado
```c
int calculate_wcet_block(ast_node_t* block) {
    int wcet = 0;
    
    for (ast_node_t* stmt = block->first; stmt; stmt = stmt->next) {
        switch (stmt->type) {
            case AST_IF:
                // Tomar el peor caso entre then y else
                wcet += max(calculate_wcet_block(stmt->then_branch),
                           calculate_wcet_block(stmt->else_branch));
                wcet += 2; // Ciclos para la comparación
                break;
                
            case AST_WHILE:
                // WCET = condición + (cuerpo * iteraciones_max)
                wcet += 2; // Condición
                wcet += stmt->max_iterations * 
                        calculate_wcet_block(stmt->body);
                break;
                
            default:
                wcet += get_instruction_cycles(stmt);
        }
    }
    
    return wcet;
}
```

### Manejo de Errores Mejorado
```c
typedef struct {
    char* filename;
    int line;
    int column;
    char* message;
    error_severity_t severity;
} error_t;

static error_t* errors = NULL;
static int error_count = 0;
static int error_capacity = 0;

void report_error(const char* msg, token_t* token) {
    add_error(ERROR_SEVERITY_ERROR, token->line, 
              token->column, msg);
    
    // No abortar inmediatamente
    if (can_recover()) {
        synchronize(); // Buscar siguiente punto de sincronización
    }
}

void print_error_summary() {
    for (int i = 0; i < error_count; i++) {
        fprintf(stderr, "%s:%d:%d: %s: %s\n",
                errors[i].filename,
                errors[i].line,
                errors[i].column,
                severity_to_string(errors[i].severity),
                errors[i].message);
    }
    
    if (error_count > 0) {
        fprintf(stderr, "\n%d error(s) found\n", error_count);
    }
}
```

## Tests Extendidos

### Test del Parser
```c
void test_parser_expressions() {
    const char* code = "function calc(): int32 { "
                      "let x: int32 = 10 + 20 * 3; "
                      "return x - 5; }";
    
    token_list_t* tokens = tokenize(code);
    ast_node_t* ast = parse(tokens);
    
    assert(ast->type == AST_PROGRAM);
    assert(ast->functions[0]->statements[0]->type == AST_LET);
    assert(ast->functions[0]->statements[1]->type == AST_RETURN);
    
    // Verificar que la expresión se parseó correctamente
    ast_node_t* expr = ast->functions[0]->statements[0]->init_expr;
    assert(expr->type == AST_BINARY_OP);
    assert(expr->op == TOK_PLUS);
}
```

### Test de WCET
```c
void test_wcet_if_statement() {
    const char* code = 
        "function test(): int32 wcet 20 { "
        "  if (x > 0) { "      // 2 ciclos
        "    return 10; "      // 5 ciclos
        "  } else { "
        "    return 20; "      // 5 ciclos
        "  } "
        "}";
    
    ast_node_t* ast = parse(tokenize(code));
    int wcet = calculate_wcet(ast->functions[0]);
    
    // WCET = comparación + max(then, else) = 2 + 5 = 7
    assert(wcet == 7);
    assert(wcet <= ast->functions[0]->declared_wcet);
}
```

## Cronograma Estimado

- **v3.1 (Parser Extendido)**: 2-3 semanas
- **v3.2 (Análisis Semántico)**: 2 semanas
- **v3.3 (WCET Mejorado)**: 1 semana
- **v3.4 (Seguridad Robusta)**: 2 semanas
- **v3.5 (Optimizaciones)**: 2 semanas

Total: ~10 semanas para un compilador robusto

## Beneficios Esperados

1. **Mayor Expresividad**: Programas Tempo más complejos y útiles
2. **Mejor Confiabilidad**: Detección temprana de errores
3. **WCET Preciso**: Garantías temporales más confiables
4. **Seguridad Real**: Validación efectiva de políticas
5. **Código Optimizado**: Mejor rendimiento sin perder determinismo