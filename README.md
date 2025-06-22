# AtomicOS v1.2.1

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

### 🛠️ Tempo Programming Language

📚 **[Tempo Language Documentation](https://ipenas-cl.github.io/AtomicOS/tempo)**

AtomicOS features Tempo, a deterministic systems programming language designed for security-critical applications:

- **Type Safety**: Strong static typing with compile-time verification
- **Memory Safety**: Bounded operations with no buffer overflows
- **Deterministic**: WCET analysis for all operations
- **Real-Time**: Built-in support for deadline scheduling
- **Security Levels**: Compile-time security verification
- **Zero Overhead**: Compiles directly to optimized assembly

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

## What's New in v1.0.0

### 🎉 Production Ready
- **Stable Kernel**: Fully functional with interrupt handling and process management
- **Complete Tempo Language**: Production-ready systems programming language
- **Real-Time Scheduler**: EDF and RMS scheduling with deadline guarantees
- **Comprehensive Testing**: Extensive test suite ensuring reliability
- **Professional Documentation**: Complete user and developer guides

### Key Components
- **Interrupt System**: Full IDT with 256 handlers, nested interrupt support
- **Process Management**: Context switching, PCB management, priority scheduling
- **Memory Protection**: W^X, guard pages, KASLR, stack protection
- **File System**: Basic VFS with deterministic operations
- **IPC System**: Message passing with security validation
- **System Calls**: Complete syscall interface with WCET bounds

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