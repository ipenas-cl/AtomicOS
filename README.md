# AtomicOS v1.3.0

**Deterministic Real-Time Security Operating System**

🌐 **[Documentation](https://ipenas-cl.github.io/AtomicOS)** | 📦 **[Downloads](https://github.com/ipenas-cl/AtomicOS/releases)** | 📚 **[Tempo Language](https://ipenas-cl.github.io/AtomicOS/tempo)**

---

## What is AtomicOS?

AtomicOS is a deterministic operating system designed for **security-critical applications** requiring predictable timing and robust security guarantees. Written in x86 assembly and our custom **Tempo programming language**, it follows the principle: *"Security First, Stability Second, Performance Third"*.

### Key Features

- 🔒 **Multi-layered Security**: W^X protection, stack-smashing protection, guard pages, KASLR
- ⏱️ **Real-Time Guarantees**: WCET analysis, EDF/RMS scheduling, bounded interrupt latency
- 🎯 **Deterministic Execution**: Reproducible behavior, static allocation, no timing variations
- 🛡️ **Zero Trust Architecture**: Bell-LaPadula security model with 4 privilege levels

## What is Tempo?

**Tempo** is AtomicOS's systems programming language - think C, but deterministic and secure by design.

### Tempo Features (v1.3.0)

```tempo
// NEW in v1.3.0: Assignment expressions!
struct Point { x: int32, y: int32 }

function update_position(p: ptr<Point>, dx: int32, dy: int32) {
    p->x = p->x + dx;  // Struct field assignment
    p->y = p->y + dy;  // Automatic offset calculation
}

// Direct hardware access without C!
function vga_write(pos: int32, ch: int32) -> int32 {
    let vga = 0xB8000;
    memory_write8(vga + pos * 2, ch);      // Character
    memory_write8(vga + pos * 2 + 1, 0x0F); // Attribute
    return 0;
}

// Security annotations
function crypto_compare(a: ptr<int8, 32>, b: ptr<int8, 32>) -> bool @constant_time {
    let diff = 0;
    for i in 0..32 {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}
```

- ✨ **Hardware Intrinsics**: Direct memory/IO access (`memory_read32`, `io_out8`, etc.)
- 🔢 **Modern Syntax**: Hex literals (`0xFF`), inline assembly, type inference
- 🔐 **Security Annotations**: `@constant_time`, `@wcet(100)`, `@trusted`
- ⚡ **Zero Overhead**: Compiles to optimized x86 assembly
- 📊 **WCET Analysis**: Compile-time timing guarantees

## Quick Start

### 1. Download & Build

```bash
# Clone the repository
git clone https://github.com/ipenas-cl/AtomicOS.git
cd AtomicOS

# Build everything (OS + Tempo compiler)
make all

# Run in QEMU
make run
```

### 2. Try Tempo

```bash
# Compile a Tempo program
./build/tempo_compiler examples/hello.tempo output.s

# Run tests
make test
```

### 3. Explore Examples

Check the `examples/` directory for Tempo code:
- `intrinsics_vga_hello.tempo` - Hardware access demo
- `deterministic_core.tempo` - Core OS functions
- `realtime_scheduler.tempo` - Real-time scheduling
- `kaslr.tempo` - Security features

## Project Structure

```
AtomicOS/
├── src/
│   ├── kernel/          # Kernel (assembly + Tempo)
│   ├── bootloader/      # x86 bootloader
│   └── libc/           # Minimal C library
├── tools/
│   └── tempo_compiler.c # Tempo compiler v1.2.2
├── examples/           # Tempo example programs
├── docs/              # Documentation & website
├── build/             # Build output
└── tests/             # Test suite
```

## Documentation

- 📖 **[Getting Started Guide](https://ipenas-cl.github.io/AtomicOS)**
- 🔧 **[Tempo Language Reference](https://ipenas-cl.github.io/AtomicOS/tempo)**
- 📚 **[API Documentation](docs/API.md)**
- 🏗️ **[Building from Source](docs/BUILD.md)**
- 🔒 **[Security Model](docs/SECURITY.md)**

## Latest Release: v1.3.0

### What's New
- **v1.3.x**: Assignment expressions, automatic struct field offsets, enhanced type system
- **v1.2.x**: Hardware intrinsics for memory/IO access
- **v1.1.x**: Hex literals, inline assembly, security annotations
- **v1.0.x**: Initial release with complete kernel and Tempo compiler

[View all releases](https://github.com/ipenas-cl/AtomicOS/releases)

## Requirements

- **Architecture**: i386 (32-bit x86)
- **Build Tools**: GCC, NASM, Make
- **Runtime**: QEMU (for testing)
- **Memory**: 64KB minimum

## Contributing

AtomicOS is an open-source project. We welcome contributions!

1. Fork the repository
2. Create a feature branch
3. Run `scripts/pre-commit.sh` before committing
4. Submit a pull request

See [CONTRIBUTING.md](CONTRIBUTING.md) for details.

## Security

Found a security issue? Please email security@atomicos.org (or open an issue if non-critical).

## License

AtomicOS is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Philosophy

> "Security First, Stability Second, Performance Third"

We prioritize correctness and security over performance. Every line of code must be:
- **Deterministic**: Same input → same output → same timing
- **Verifiable**: Provable security and timing properties
- **Minimal**: No unnecessary complexity

---

**Ready to build secure, real-time systems?** [Get Started →](https://ipenas-cl.github.io/AtomicOS)