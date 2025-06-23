# Tempo Examples

This directory contains example programs written in the Tempo programming language, organized by category:

## Directory Structure

### `/atomicos/`
AtomicOS-specific examples including kernel modules, interrupt handlers, memory management, and real-time scheduling. These examples are compiled and included in the AtomicOS kernel.

### `/tempo/`
Pure Tempo language examples demonstrating language features like structs, pattern matching, concurrency, and transactions. These showcase Tempo's capabilities independent of AtomicOS.

### `/tests/`
Test cases for the Tempo compiler and language features. These are used by the automated test suite to ensure compiler correctness.

### `/modules/`
Modular Tempo code demonstrating the module system with separate compilation units for collections, core functionality, and I/O operations.

### `/tempo_v1/`
Legacy examples from Tempo v1, kept for historical reference and compatibility testing.

## Quick Start

To compile any example:
```bash
../build/tempo_compiler example.tempo output.s
```

For AtomicOS integration, examples are automatically compiled during the build process:
```bash
make all
```

## Learning Tempo

1. **New to Tempo?** Start with examples in `/tempo/` to learn the language
2. **Building for AtomicOS?** Check `/atomicos/` for kernel integration patterns
3. **Testing the compiler?** Use examples in `/tests/` for validation

## Key Concepts

All Tempo examples follow these principles:
- **Deterministic execution** with bounded WCET
- **Static memory allocation** (no malloc/free)
- **Security-first design** with capability-based access control
- **Real-time guarantees** for critical operations

See individual directory README files for detailed information.