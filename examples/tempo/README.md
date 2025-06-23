# Pure Tempo Language Examples

This directory contains examples that demonstrate the Tempo programming language features, independent of AtomicOS-specific functionality.

## Language Feature Examples

### Basic Constructs
- **simple_test.tempo** - Basic language features and syntax
- **test_assignment.tempo** - Variable assignment and initialization
- **test_reassignment.tempo** - Variable reassignment patterns

### Data Structures
- **test_struct_simple.tempo** - Basic struct usage
- **test_struct_debug.tempo** - Struct debugging examples
- **test_struct_offsets.tempo** - Struct field offset calculations
- **test_field_access.tempo** - Struct field access patterns

### Control Flow
- **test_match.tempo** - Pattern matching expressions
- **test_cast.tempo** - Type casting operations

### Concurrency and Parallelism
- **deterministic_concurrency.tempo** - Deterministic concurrency patterns
- **test_task.tempo** - Task-based parallelism
- **test_transaction.tempo** / **test_transaction_simple.tempo** - Transactional memory

### Memory Management
- **test_cache.tempo** - Cache-aware programming
- **test_vm_simple.tempo** - Virtual memory basics

### Advanced Features
- **demo_ideal_os.tempo** - Comprehensive language feature demonstration
- **ideal_os_extensions.tempo** - Advanced security and OS features
- **test_llvm_integration.tempo** - LLVM backend integration

## Compilation

To compile any example:
```bash
../../build/tempo_compiler example.tempo output.s
```

## Language Principles

These examples demonstrate Tempo's core principles:

1. **Deterministic Execution** - All operations have predictable behavior
2. **Static Memory Management** - No dynamic allocation
3. **WCET Analysis** - Compiler-enforced execution time bounds
4. **Type Safety** - Strong static typing with compile-time checks
5. **Security by Design** - Built-in security levels and capabilities

## Learning Path

For those new to Tempo, we recommend exploring examples in this order:

1. Start with `simple_test.tempo` for basic syntax
2. Move to `test_assignment.tempo` and `test_reassignment.tempo`
3. Explore struct examples for data structures
4. Study `test_match.tempo` for control flow
5. Advanced users can explore concurrency and transaction examples