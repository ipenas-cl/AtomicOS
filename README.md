# Tempo v5.0.0 - The Deterministic Programming Language 🚀

**A revolutionary systems programming language with guaranteed execution times**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-5.0.0-green)](https://github.com/ipenas-cl/AtomicOS/releases)

---

## 🎯 What is Tempo?

Tempo is a deterministic systems programming language designed for applications requiring **predictable timing**, **memory safety**, and **security guarantees**. Every Tempo program executes with the same timing, every time.

### ✨ Key Features

- **🕐 WCET Analysis** - Every function has guaranteed worst-case execution time
- **🔒 Memory Safe** - No buffer overflows, no null pointers, no use-after-free
- **🛡️ Security Levels** - Built-in security annotations for privilege separation
- **⚡ Zero Dependencies** - Self-hosted compiler with no external requirements
- **🔧 Inline Assembly** - Full control when you need it
- **📦 Module System** - Modern import/export for code organization

---

## 🚀 Quick Start

### Installation

```bash
# Install Tempo globally
sudo cp /usr/local/bin/tempo /usr/local/bin/
tempo --version  # Should show v5.0.0
```

### Your First Program

```tempo
// hello.tempo
fn main() -> i32 {
    print("Hello from Tempo!");
    return 0;
}
```

Compile and run:
```bash
tempo hello.tempo hello.s
as hello.s -o hello.o
ld hello.o -o hello
./hello
```

---

## 💪 Advanced Features

### WCET Annotations
```tempo
@wcet(1000)  // Guaranteed to complete in 1000 cycles
fn process_data(data: *u8, size: u32) -> void {
    // Deterministic processing
}
```

### Security Levels
```tempo
@security_level(3)  // Kernel level
fn critical_operation() -> void {
    // Privileged code
}
```

### Global Variables
```tempo
let global_counter: u32;
let buffer: [1024]u8;
```

### Inline Assembly
```tempo
fn get_cpu_cycles() -> u64 {
    let cycles: u64;
    __asm__(
        "rdtsc\n"
        "shl $32, %%rdx\n"
        "or %%rdx, %%rax\n"
        : "=a" (cycles)
        :
        : "rdx"
    );
    return cycles;
}
```

---

## 📚 Language Reference

### Data Types
- **Integers**: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`
- **Boolean**: `bool` (true/false)
- **Characters**: `char`
- **Pointers**: `*T`, `*mut T`
- **Arrays**: `[N]T` where N can be a const
- **Void**: `void`

### Control Flow
- `if` / `else`
- `while` loops
- `for` loops
- `match` expressions (pattern matching)
- `break` / `continue`
- `return`

### Memory Management
- Static allocation by default
- No garbage collector
- Deterministic memory usage
- Bounds checking on arrays

---

## 🛠️ Compiler Features

The Tempo v5.0.0 compiler includes:

- ✅ Complete self-hosting (written in Tempo!)
- ✅ Const expressions in array sizes
- ✅ Global variable support
- ✅ Inline assembly with AT&T syntax
- ✅ Module system with import/export
- ✅ Comprehensive type checking
- ✅ WCET analysis and verification
- ✅ Security level enforcement

---

## 📊 Real-World Examples

### Web Server Handler
```tempo
@wcet(50000)
fn handle_request(req: *Request) -> Response {
    let start = get_cycles();
    
    // Process request
    let response = route_request(req);
    
    // Ensure deterministic timing
    wait_until(start + 50000);
    
    return response;
}
```

### Crypto Operation
```tempo
@wcet(10000)
@constant_time  // No timing side channels
fn compare_hash(a: *[32]u8, b: *[32]u8) -> bool {
    let diff: u32 = 0;
    let i: u32 = 0;
    
    while (i < 32) {
        diff = diff | (a[i] ^ b[i]);
        i = i + 1;
    }
    
    return diff == 0;
}
```

---

## 🏗️ Building from Source

```bash
# Clone the repository
git clone https://github.com/ipenas-cl/AtomicOS.git
cd AtomicOS

# Build Tempo compiler
make tempo

# Install
sudo make install-tempo

# Run tests
make test-tempo
```

---

## 🧪 Testing

Tempo includes a comprehensive test suite:

```bash
# Run all tests
make test

# Run specific test category
make test-wcet      # WCET verification
make test-security  # Security features
make test-codegen   # Code generation
```

---

## 📖 Documentation

- [Language Tutorial](docs/tempo/LEARN_TEMPO.md)
- [Language Reference](docs/tempo/TEMPO_REFERENCE.md)
- [WCET Analysis](docs/tempo/WCET_GUIDE.md)
- [Security Levels](docs/tempo/SECURITY_LEVELS.md)
- [Module System](docs/tempo/MODULES.md)

---

## 🤝 Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Development Setup
1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

## 🚗 Roadmap

### v5.1.0 (Next)
- [ ] Pattern matching improvements
- [ ] Async/await with deterministic scheduling
- [ ] Package manager (tpm - Tempo Package Manager)
- [ ] LSP support for IDEs

### Future
- [ ] LLVM backend option
- [ ] Formal verification tools
- [ ] Hardware synthesis support
- [ ] RISC-V target

---

## 📜 License

Tempo is licensed under the MIT License - see [LICENSE](LICENSE) for details.

**Tempo™** is a trademark of Ignacio Peña Sepúlveda.

---

## 🌟 Why Tempo?

In a world where software timing is unpredictable and security vulnerabilities abound, Tempo offers a different path:

- **Predictable**: Same code, same timing, always
- **Secure**: Security levels built into the language
- **Fast**: No runtime overhead, no garbage collector
- **Simple**: Easy to learn, hard to misuse

---

## 👨‍💻 Author

**Ignacio Peña Sepúlveda**  
Email: ignacio@tempo-lang.org  
GitHub: [@ipenas-cl](https://github.com/ipenas-cl)

---

## ⭐ Star History

If you find Tempo useful, please star it on GitHub!

[![Star History Chart](https://api.star-history.com/svg?repos=ipenas-cl/AtomicOS&type=Date)](https://github.com/ipenas-cl/AtomicOS)

---

*"Deterministic by design, secure by default."* - Tempo Philosophy