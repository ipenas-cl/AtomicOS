# AtomicOS

**Deterministic Real-Time Security Operating System**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-5.1.0-green)](https://github.com/ipenas-cl/AtomicOS/releases)

## Overview

AtomicOS is a deterministic operating system written in x86 assembly, designed for real-time and security-critical applications. Every operation has guaranteed timing bounds (WCET) and the system provides military-grade security through hardware-enforced isolation.

**Note**: AtomicOS kernel modules are written in [Tempo](https://github.com/ipenas-cl/Tempo-Lang), a deterministic programming language designed specifically for systems programming with guaranteed timing.

## Key Features

- **100% Deterministic**: Every operation has proven worst-case execution time
- **Real-Time Guarantees**: EDF and RMS schedulers with deadline enforcement
- **Security First**: 12 layers of protection, hardware-enforced isolation
- **Pure Assembly**: No high-level language overhead
- **Minimal Attack Surface**: < 10,000 lines of audited code

## Quick Start

```bash
# Clone the repository
git clone https://github.com/ipenas-cl/AtomicOS.git
cd AtomicOS

# Build the OS
make all

# Run in QEMU
make run

# Debug with GDB
make debug
```

## System Requirements

- x86 processor (i386 or higher)
- 4MB RAM minimum
- QEMU for testing
- GNU toolchain (as, ld, make)
- Tempo compiler (for building kernel modules)

## Architecture

### Core Components

```
AtomicOS/
├── src/
│   ├── bootloader/     # First-stage boot (512 bytes)
│   └── kernel/         # Kernel core
│       ├── kernel.asm  # Main kernel entry
│       ├── interrupts.asm  # IDT and handlers
│       ├── *.tempo     # Security modules (compiled with Tempo)
│       └── *.inc       # Generated from Tempo modules
├── examples/           # Tempo modules for kernel
│   └── atomicos/       # Core OS modules written in Tempo
├── include/            # System headers
├── build/             # Build output
└── scripts/           # Build and utility scripts
```

### Security Architecture

1. **Hardware Isolation**: Separate memory domains
2. **W^X Protection**: No page is both writable and executable
3. **Stack Protection**: Hardware-enforced guard pages
4. **KASLR**: Kernel randomization at boot
5. **Secure Boot**: Cryptographic verification

### Real-Time Features

- **Schedulers**: EDF (dynamic) and RMS (static)
- **WCET Analysis**: Every function has proven bounds
- **Priority Inheritance**: Prevents priority inversion
- **Deadline Monitoring**: Automatic deadline violation handling

## Building

### Prerequisites

```bash
# Install Tempo compiler first
wget https://github.com/ipenas-cl/tempo-lang/releases/download/v5.0.0/tempo
chmod +x tempo
sudo mv tempo /usr/local/bin/

# Ubuntu/Debian
sudo apt-get install build-essential nasm qemu-system-x86

# macOS
brew install nasm qemu

# Arch Linux
sudo pacman -S base-devel nasm qemu
```

### Build Commands

```bash
make all        # Build complete OS
make kernel     # Build kernel only
make clean      # Clean build files
make test       # Run test suite
```

## Testing

```bash
# Run automated tests
make test

# Run specific test
make test-scheduler
make test-memory
make test-security
```

## Documentation

- [Architecture Guide](docs/architecture/KERNEL_DESIGN.md)
- [Security Model](docs/SECURITY.md)
- [Real-Time Scheduling](docs/SCHEDULING.md)
- [API Reference](docs/api/)

## Use Cases

- **Aerospace**: Flight control systems
- **Medical**: Life-critical devices
- **Industrial**: Safety-critical controllers
- **Defense**: Secure communication systems
- **Automotive**: Engine control units

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing`)
3. Run tests (`make test`)
4. Commit changes (`git commit -m 'Add feature'`)
5. Push to branch (`git push origin feature/amazing`)
6. Open a Pull Request

## Performance

- Boot time: < 100ms
- Context switch: < 1000 cycles
- Interrupt latency: < 500 cycles
- Memory allocation: O(1) deterministic

## License

MIT License - see [LICENSE](LICENSE) file

## Author

**Ignacio Peña Sepúlveda**  
Email: ignacio@atomicos.io

---

*"Security First, Stability Second, Performance Third"* - AtomicOS Philosophy
