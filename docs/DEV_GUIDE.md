# AtomicOS Development Guide

## Overview

AtomicOS is a deterministic real-time security operating system designed with the principles: "Security First, Stability Second, Performance Third". This guide covers the development environment, architecture, and best practices for contributing to AtomicOS.

## Project Structure

```
AtomicOS/
├── src/
│   ├── bootloader/         # x86 assembly bootloader
│   │   └── boot.asm        # Stage 1 bootloader
│   ├── kernel/            # Kernel implementation
│   │   ├── kernel.asm     # Main kernel entry
│   │   ├── *.tempo        # Tempo source files
│   │   └── *.inc          # Generated assembly includes
│   └── userspace/         # User programs (future)
├── tools/
│   ├── tempo_compiler.c   # Tempo v1.3.1 compiler
│   ├── tempo_compiler_v2.c # Industrial LLVM backend
│   ├── tempo_type_checker.c # Advanced type system
│   └── tempo_*.c          # Additional compiler tools
├── examples/              # Tempo example programs
├── tests/                 # Test suite
├── docs/                  # Documentation
├── build/                 # Build artifacts (generated)
└── scripts/              # Build and utility scripts
```

## Development Environment

### Prerequisites

- **Operating System**: Linux or macOS
- **Architecture**: x86/x86_64 host for development
- **Required Tools**:
  - GCC or Clang (C99 support)
  - NASM (Netwide Assembler)
  - QEMU (for testing)
  - Make
  - Git
  - LLVM (optional, for v2.0 compiler)

### Setup

```bash
# Clone the repository
git clone https://github.com/ipenas-cl/AtomicOS
cd AtomicOS

# Build the OS
make all

# Run in QEMU
make run
```

## Tempo Programming Language

Tempo is AtomicOS's deterministic systems programming language, designed for real-time and security-critical applications.

### Current Version: v1.3.1

**Key Features**:
- Complete type system with structs, pointers, arrays
- WCET (Worst-Case Execution Time) analysis
- Security level annotations
- Inline assembly support
- Constant-time operations
- Parallel programming constructs
- Hardware transactional memory
- Cache partitioning directives

### Basic Syntax

```tempo
// Function with WCET annotation
@wcet(100)
function add(a: int32, b: int32) -> int32 {
    return a + b;
}

// Struct definition
struct ProcessInfo {
    pid: int32,
    priority: int32,
    state: int32
}

// Constant-time function
@wcet(50)
@constant_time
function secure_compare(a: ptr<int8>, b: ptr<int8>, len: int32) -> bool {
    let result = 0;
    for let i = 0; i < len; i = i + 1 {
        result = result | (a[i] ^ b[i]);
    }
    return result == 0;
}
```

### Parallel Programming

```tempo
// Parallel task
@wcet(200)
@task
function process_data(data: ptr<int32>, start: int32, end: int32) -> int32 {
    let sum = 0;
    @parallel_for(start, end, 4) {
        sum = sum + data[i];
    }
    return sum;
}

// Transactional memory
@wcet(150)
@transaction
function atomic_update(counter: ptr<int32>) -> void {
    @atomic {
        let current = *counter;
        *counter = current + 1;
    }
}
```

## Building AtomicOS

### Make Targets

```bash
# Core build targets
make all          # Build complete OS image
make clean        # Clean build artifacts
make distclean    # Clean everything

# Running and debugging
make run          # Run OS in QEMU
make debug        # Run with GDB debugging

# Testing
make test         # Run full test suite
make test-quick   # Quick compilation tests
make test-llvm    # Test LLVM backend
make test-types   # Test type checker
make test-demo    # Test demo compiler

# Compiler variants
make tempo-enhanced  # Build enhanced compiler
make tempo-v2       # Build LLVM backend compiler
make tempo-v2-demo  # Build demo compiler

# Quality assurance
make security-check  # Security analysis
make performance    # Performance analysis
make verify        # Formal verification

# Release management
make version       # Show version info
make release-patch # Increment patch version
make release-minor # Increment minor version
make release-major # Increment major version
```

## Kernel Architecture

### Memory Layout

```
0x00000000 - 0x00000FFF : Real mode IVT
0x00001000 - 0x00007BFF : Bootloader stack
0x00007C00 - 0x00007DFF : Bootloader
0x00010000 - 0x0001FFFF : Kernel stack
0x00100000 - 0x001FFFFF : Kernel code/data
0x00200000 - 0x002FFFFF : Kernel heap
0x00300000 - 0x003FFFFF : Process memory
0x00400000 - 0x004FFFFF : Shared memory
0x00500000 - 0x005FFFFF : Device memory
0x00600000 - 0x00FFFFFF : User space
```

### Security Architecture

**Multi-Layer Security Model**:
1. **Security Levels**: Guest (0), User (1), System (2), Kernel (3), Crypto (4)
2. **Bell-LaPadula Model**: No read up, no write down
3. **W^X Protection**: Write XOR Execute enforcement
4. **Stack Protection**: Deterministic canary-based SSP
5. **KASLR**: Kernel Address Space Layout Randomization
6. **Memory Domains**: Isolated security contexts

### Real-Time Features

**Deterministic Guarantees**:
- Static memory allocation (no malloc/free)
- Bounded WCET for all operations
- Fixed-priority preemptive scheduling
- EDF (Earliest Deadline First) support
- RMS (Rate Monotonic Scheduling) support
- Constant-time cryptographic operations

## Compiler Development

### Tempo Compiler v1.3.1

The main compiler (`tempo_compiler.c`) features:
- Complete parser with full language support
- x86 assembly code generation
- WCET analysis and verification
- Security level checking
- Inline assembly support

### Tempo Compiler v2.0 (LLVM Backend)

The industrial-grade compiler (`tempo_compiler_v2.c`) includes:
- LLVM IR generation
- Advanced optimizations
- Multiple target architectures
- Formal verification support
- Profile-guided optimization

### Type System

The advanced type checker (`tempo_type_checker.c`) provides:
- Complete type inference
- Security flow analysis
- WCET compatibility checking
- Constant-time verification
- Memory safety validation

## Testing

### Test Framework

```bash
# Run all tests
make test

# Run specific test categories
./build/tempo_test_runner --category=compilation
./build/tempo_test_runner --category=wcet
./build/tempo_test_runner --category=security
```

### Writing Tests

Add test files to `examples/test_*.tempo`:

```tempo
// test_example.tempo
@wcet(50)
function test_addition() -> int32 {
    let a = 10;
    let b = 20;
    return a + b;
}
```

## Contributing

### Code Style

**Assembly (NASM)**:
- Use consistent indentation (4 spaces)
- Comment all non-obvious code
- Use meaningful label names

**Tempo**:
- Follow Rust-like syntax conventions
- Always include WCET annotations
- Document security assumptions
- Use descriptive variable names

**C (Compiler/Tools)**:
- C99 standard
- No external dependencies
- Comprehensive error handling
- Memory leak free

### Git Workflow

1. Create feature branch from main
2. Make atomic commits with clear messages
3. Run pre-commit checks: `scripts/pre-commit.sh`
4. Submit pull request with detailed description

### Security Considerations

- **Never** include timing-dependent code paths
- **Always** validate bounds for array/pointer access
- **Ensure** constant-time for crypto operations
- **Verify** WCET bounds are maintained
- **Check** security level transitions

## Advanced Topics

### Virtual Memory Management

The deterministic VM system features:
- Page-based memory with 4KB pages
- Software-managed TLB for predictability
- Buddy allocator for physical frames
- Memory domains for isolation

### Process Management

- Fixed-size Process Control Blocks (PCBs)
- Priority-based scheduling
- Context switching with full state preservation
- Inter-process communication via channels

### Interrupt Handling

- Complete IDT with 256 entries
- WCET-bounded interrupt handlers
- Nested interrupt support
- Deterministic interrupt latency

## Debugging

### QEMU Debugging

```bash
# Start QEMU in debug mode
make debug

# In another terminal, connect GDB
gdb
(gdb) target remote :1234
(gdb) symbol-file build/kernel.elf
(gdb) break kernel_main
(gdb) continue
```

### Tempo Debugging

Use the debug-enabled compiler:
```bash
./build/tempo_enhanced --debug examples/test.tempo
```

## Performance Optimization

### WCET Optimization

1. Use Tempo's `@wcet` annotations accurately
2. Minimize loop iterations
3. Avoid recursive functions
4. Use constant-time algorithms

### Cache Optimization

```tempo
// Use cache partitioning
@cache_partition(ways: 4)
function critical_function() -> void {
    // Function gets dedicated cache ways
}
```

## Future Roadmap

### Planned Features

1. **Tempo v1.4**: Module system, generics
2. **Kernel**: Networking stack, device drivers
3. **Tools**: IDE integration, profiler
4. **Platform**: ARM/RISC-V ports

### Research Areas

- Formal verification integration
- Hardware security module support
- Quantum-resistant cryptography
- Real-time garbage collection

## Resources

- **Documentation**: `docs/` directory
- **Examples**: `examples/` directory
- **Tests**: `tests/` directory
- **Issues**: GitHub issue tracker

## License

AtomicOS is licensed under the MIT License. See LICENSE file for details.

---

*"Security First, Stability Second, Performance Third"* - The AtomicOS Philosophy