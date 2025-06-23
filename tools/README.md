# AtomicOS Tools

This directory contains the development tools for AtomicOS, primarily focused on the Tempo compiler and related utilities.

## Active Tools

### Core Compiler
- `tempo_compiler.c` - **Main Tempo compiler** (current active version)
- `tempo_compiler` - Compiled binary of the Tempo compiler

### Compiler Components
- `tempo_type_checker.c/h` - Type checking system
- `tempo_semantic_analyzer.c` - Semantic analysis
- `tempo_optimizer.c/h` - Code optimization
- `tempo_match_codegen.c/h` - Match expression code generation
- `tempo_module_system.c/h` - Module system support

### Backend Components
- `tempo_ast_to_llvm.c/h` - LLVM backend (experimental)
- `tempo_llvm_backend.c/h` - LLVM code generation
- `tempo_instruction_patterns.c` - Instruction pattern matching

### Specialized Tools
- `wcet_scheduler.c` - Worst-Case Execution Time scheduler
- `auto_parallelizer.c` - Automatic parallelization tool

### Testing
- `test_type_checker.c` - Type checker tests
- `tests/` - Additional test files
- `validate_wcet.sh` - WCET validation script

## Subdirectories

### `self_hosted/`
Contains the self-hosted Tempo compiler implementation written in Tempo itself. This is an ongoing effort to bootstrap the compiler.

### `compiler/`
Assembly implementations of compiler components for research and optimization.

### `linker/`
Various linker implementations in assembly for object file linking and executable generation.

### `archive/`
Historical versions, demos, and experimental tools no longer in active use.

## Usage

The main tool is the Tempo compiler:
```bash
./tempo_compiler input.tempo output.s
```

This compiles Tempo source files to assembly that can be integrated into AtomicOS.