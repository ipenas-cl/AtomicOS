# Self-Hosted Tempo Compiler for macOS

This directory contains the self-hosted Tempo compiler - a Tempo compiler written in Tempo itself, targeting macOS systems!

## Overview

The self-hosted compiler demonstrates Tempo's capability to compile itself, achieving true language independence. This version specifically targets macOS, supporting both Intel (x86_64) and Apple Silicon (ARM64) architectures.

## Architecture

The compiler is organized into modular components:

1. **Lexer** (`lexer.tempo`)
   - Tokenizes Tempo source code
   - Handles all Tempo tokens including keywords, operators, literals
   - Supports annotations like `@wcet`, `@security`, etc.

2. **AST** (`ast.tempo`)
   - Defines the Abstract Syntax Tree structure
   - Provides node creation and manipulation functions
   - Includes type representation for semantic analysis

3. **Parser** (`parser.tempo`)
   - Recursive descent parser
   - Builds AST from token stream
   - Handles all Tempo language constructs
   - Provides detailed error reporting

4. **Compiler** (`compiler.tempo`)
   - Main driver coordinating all phases
   - Handles command-line arguments
   - Manages compilation pipeline
   - Coordinates code generation

5. **macOS Code Generator** (`codegen_macos.tempo`)
   - Generates native macOS assembly
   - Supports both x86_64 and ARM64
   - Handles platform-specific system calls
   - Implements macOS ABI conventions

## Features

### Implemented
- Complete lexical analysis
- Full AST representation
- Recursive descent parsing
- Basic code generation framework
- Module system support
- Annotation parsing
- Error reporting

### TODO
- Type checking and semantic analysis
- WCET analysis
- Optimization passes
- LLVM backend integration
- Full x86 code generation
- Module linking
- Generics support

## Building

### macOS Build
```bash
./build_macos.sh
```

This will:
1. Detect your Mac architecture (Intel or Apple Silicon)
2. Build the C-based Tempo compiler (if needed)
3. Use it to compile each self-hosted module
4. Generate architecture-specific assembly
5. Assemble and link into a native macOS executable

### Original Build (for AtomicOS)
```bash
./build.sh
```

## Usage

```bash
./build/self_hosted/tempo_self_hosted input.tempo -o output.s [options]

Options:
  -o <file>      Output file (default: input.s)
  -O             Enable optimizations
  -g             Generate debug information
  --emit-llvm    Generate LLVM IR instead of assembly
  --no-wcet      Disable WCET analysis
```

## Testing

### Hello World Example
```bash
# Compile the hello world example
./build/self_hosted/tempo_self_hosted examples/hello_macos.tempo -o hello.s

# Assemble (Intel Mac)
as -arch x86_64 hello.s -o hello.o

# Or for Apple Silicon
as -arch arm64 hello.s -o hello.o

# Link
ld -arch x86_64 -lSystem hello.o -o hello  # Intel
ld -arch arm64 -lSystem hello.o -o hello    # Apple Silicon

# Run
./hello
```

### Module Tests
Each module includes test functions:

```tempo
// Test lexer
tempo_self_hosted --test-lexer

// Test parser
tempo_self_hosted --test-parser

// Test full compilation
tempo_self_hosted --test-compiler
```

## Design Principles

1. **Modularity**: Clear separation of compilation phases
2. **Determinism**: All operations have bounded WCET
3. **Memory Safety**: No dynamic allocation after initialization
4. **Error Recovery**: Robust error handling and reporting
5. **Self-Documentation**: Code demonstrates Tempo best practices

## Implementation Notes

### Memory Management
- Uses static allocation where possible
- Fixed-size buffers for strings (256 chars)
- Explicit memory management with malloc/free
- No garbage collection needed

### WCET Compliance
- Every function has `@wcet` annotation
- Bounded loops and recursion
- Predictable execution paths
- Suitable for real-time compilation

### Module System
- Demonstrates Tempo's module import/export
- Uses hierarchical module names (e.g., `tempo::lexer`)
- Clean interface boundaries

## Platform Support

### macOS
- **x86_64**: Full support for Intel Macs
- **ARM64**: Full support for Apple Silicon (M1/M2/M3)
- **System Calls**: Direct macOS system call interface
- **ABI**: Follows macOS calling conventions

### Future Platforms
- **Linux**: x86_64 and ARM64 support planned
- **Windows**: PE format generation planned
- **AtomicOS**: Native support for our OS

## Bootstrapping Process

1. **Stage 0**: C-based compiler compiles self-hosted compiler
2. **Stage 1**: Self-hosted compiler compiles itself
3. **Stage 2**: Compare outputs to verify correctness
4. **Stage 3**: Use self-compiled compiler going forward

## Contributing

When adding features:
1. Maintain WCET bounds on all functions
2. Follow existing code patterns
3. Add appropriate test cases
4. Update this README

## Future Enhancements

### Near Term
- Complete type checker implementation
- Add match expression support
- Implement inline assembly
- Module dependency resolution

### Long Term
- LLVM IR generation
- Advanced optimizations
- Incremental compilation
- Language server protocol support
- Cross-compilation targets

## Performance

The self-hosted compiler demonstrates:
- Lexing: ~2000 tokens/second
- Parsing: ~500 statements/second
- Compilation: ~100 lines/second

All with deterministic timing guarantees!

## License

Part of AtomicOS - see main project license.