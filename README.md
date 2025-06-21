# AtomicOS v0.8.0

**Deterministic Real-Time Security Operating System**

🌐 **Website**: [ipenas-cl.github.io/AtomicOS](https://ipenas-cl.github.io/AtomicOS)

AtomicOS is a deterministic operating system designed for security-critical applications requiring predictable timing and robust security guarantees.

## Features

### 🔒 Security
- **W^X Protection**: Memory pages cannot be both writable and executable
- **Stack-Smashing Protection (SSP)**: Deterministic canary-based stack corruption detection
- **Guard Pages**: Empty pages between memory regions to detect buffer overflows
- **KASLR**: Kernel Address Space Layout Randomization with deterministic PRNG
- **Multi-level Security**: Bell-LaPadula inspired access control with 4 security levels

### ⚡ Determinism & Real-Time
- **Predictable Execution Timing**: WCET-bounded operations for real-time guarantees
- **Static Resource Allocation**: Fixed 64KB memory pool with deterministic allocation
- **Reproducible Behavior**: Deterministic PRNG with fixed seed for testing
- **Real-Time Scheduling**: EDF (Earliest Deadline First) and RMS (Rate Monotonic Scheduling)
- **Constant-Time Operations**: No timing side-channels in cryptographic operations
- **Interrupt System**: Full IDT with ISRs, IRQs, and deterministic interrupt handling
- **Process Management**: Context switching with real-time task support

### 🛠️ Tempo Programming Language v3.0.0

📚 **[Tempo Language Documentation](https://ipenas-cl.github.io/AtomicOS/tempo)**

- **Current Status**: Full-featured compiler with advanced OS capabilities
- **Working**: Functions, variables, arithmetic, WCET analysis, security annotations
- **Security**: Process sandboxing, capability restrictions, trusted execution
- **Real-Time**: EDF/RMS scheduling, bounded execution, interrupt handling  
- **Analysis**: Automatic WCET calculation, security verification, deterministic guarantees
- **Testing**: Comprehensive test suite with regression testing and quality assurance

## Quick Start

```bash
# Build the operating system
make all

# Run in QEMU
make run

# Run comprehensive test suite
make test

# Run pre-commit checks
scripts/pre-commit.sh
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

## New in v0.8.0

### Interrupt System Foundation
- **IDT Implementation**: Complete Interrupt Descriptor Table with 256 entries
- **Exception Handlers**: All 32 CPU exceptions with deterministic handling
- **IRQ Management**: Hardware interrupt handlers for timer, keyboard, and devices
- **PIC Configuration**: 8259A controller remapped to avoid conflicts
- **Real-Time Timer**: 1000Hz timer interrupt for scheduling foundation
- **WCET Tracking**: Interrupt cycle counting for deterministic guarantees
- **Nested Interrupts**: Support for up to 3 levels with overflow protection

## New in v0.7.0

### Enhanced Tempo Compiler
- **Advanced Parser**: Complete syntax validation and error reporting
- **WCET Analysis**: Automatic worst-case execution time calculation
- **Security Levels**: Multi-level security verification at compile time
- **Code Generation**: Optimized assembly output with security annotations

### Comprehensive Testing
- **Test Suite**: 16 automated tests covering all compiler features
- **Regression Testing**: All example files tested on every change
- **Pre-commit Hooks**: Quality assurance before code commits
- **Coverage**: Syntax, WCET, security, and real-time feature validation

### Professional Development Tools
- **Release Management**: Semantic versioning with automated releases
- **Version Control**: `make release-patch/minor/major` commands
- **Quality Checks**: Pre-commit validation for code quality
- **Documentation**: Comprehensive developer and API documentation

## License

MIT License - See LICENSE file for details.

## Contributing

AtomicOS follows strict coding standards for security and determinism:
- All code must have WCET analysis
- Memory operations must be bounds-checked
- Cryptographic operations must be constant-time
- Security features must be formally verified where possible

For detailed contributing guidelines, see `docs/CONTRIBUTING.md`.

## Links

- 🌐 **Official Website**: [ipenas-cl.github.io/AtomicOS](https://ipenas-cl.github.io/AtomicOS)
- 📚 **Tempo Language Documentation**: [ipenas-cl.github.io/AtomicOS/tempo](https://ipenas-cl.github.io/AtomicOS/tempo)
- 🔧 **GitHub Repository**: [github.com/ipenas-cl/AtomicOS](https://github.com/ipenas-cl/AtomicOS)
- 📦 **Releases**: [github.com/ipenas-cl/AtomicOS/releases](https://github.com/ipenas-cl/AtomicOS/releases)