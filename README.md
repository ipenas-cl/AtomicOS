# AtomicOS v0.6.1

**Deterministic Real-Time Security Operating System**

AtomicOS is a deterministic operating system designed for security-critical applications requiring predictable timing and robust security guarantees.

## Features

### 🔒 Security
- **W^X Protection**: Memory pages cannot be both writable and executable
- **Stack-Smashing Protection (SSP)**: Deterministic canary-based stack corruption detection
- **Guard Pages**: Empty pages between memory regions to detect buffer overflows
- **KASLR**: Kernel Address Space Layout Randomization with deterministic PRNG
- **Multi-level Security**: Bell-LaPadula inspired access control with 4 security levels

### ⚡ Determinism
- **Predictable Execution Timing**: WCET-bounded operations for real-time guarantees
- **Static Resource Allocation**: Fixed 64KB memory pool with deterministic allocation
- **Reproducible Behavior**: Deterministic PRNG with fixed seed for testing
- **Real-Time Scheduling**: Fixed priority scheduling with deadline guarantees
- **Constant-Time Operations**: No timing side-channels in cryptographic operations

### 🛠️ Tempo Programming Language
- **Deterministic**: All operations have bounded execution time
- **Memory Safe**: Static allocation with compile-time bounds checking
- **Verified**: Formal verification support for critical code paths
- **Real-Time**: WCET analysis integrated into compiler

## Quick Start

```bash
# Build the operating system
make all

# Run in QEMU
make run

# Run tests
make test
```

## Project Structure

```
AtomicOS/
├── src/
│   ├── kernel/          # Kernel source code
│   ├── bootloader/      # Boot loaders
│   ├── libc/           # C standard library
│   └── userspace/      # User applications
├── include/            # Header files
├── build/             # Build artifacts
├── docs/              # Documentation
├── tests/             # Test suites
├── tools/             # Development tools
└── examples/          # Example programs
```

## Security Philosophy

**"Security First, Stability Second, Performance Third"**

AtomicOS prioritizes security over performance, ensuring that:
1. All security subsystems must initialize successfully or the system halts
2. Memory operations are bounds-checked and deterministic
3. Timing side-channels are eliminated through constant-time operations
4. Process isolation is enforced through multiple security layers

## Real-Time Guarantees

- **WCET Analysis**: All code paths have provable worst-case execution times
- **Fixed Priority Scheduling**: Deterministic task scheduling with deadline guarantees
- **Static Memory Management**: No dynamic allocation to prevent unpredictable delays
- **Bounded Interrupt Latency**: Hardware interrupts serviced within fixed time windows

## License

MIT License - See LICENSE file for details.

## Contributing

AtomicOS follows strict coding standards for security and determinism:
- All code must have WCET analysis
- Memory operations must be bounds-checked
- Cryptographic operations must be constant-time
- Security features must be formally verified where possible

For detailed contributing guidelines, see `docs/CONTRIBUTING.md`.