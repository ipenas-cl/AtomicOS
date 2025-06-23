# Tempo: A Deterministic Programming Language

## What is Tempo?

Tempo is a deterministic programming language designed for writing safety-critical components with guaranteed worst-case execution time (WCET). Originally created for AtomicOS, it can be used for any system requiring predictable timing behavior.

## Key Features

### 🕒 **WCET Guarantees**
Every function declares its maximum execution time, verified at compile-time:
```tempo
@wcet(30)  // Maximum 30 CPU cycles
function calculate(x: int32, y: int32): int32 {
    return x * 2 + y;
}
```

### 🔒 **Security by Design**
- Multi-level security annotations
- Constant-time operations for crypto
- Memory safety without garbage collection
- No undefined behavior

### 🎯 **Real Determinism**
- No dynamic allocation
- Bounded loops only
- No function pointers
- Static dispatch only

## Quick Start

### Installation
```bash
git clone https://github.com/ipenas-cl/AtomicOS.git
cd AtomicOS/tools
make tempo_compiler
```

### Your First Program
```tempo
// hello.tempo
@wcet(100)
function main(): int32 {
    let x: int32 = 10;
    let y: int32 = 20;
    return x + y;  // Returns 30
}
```

### Compile
```bash
./tempo_compiler hello.tempo hello.s
```

## Real-World Examples

Tempo is used to build critical components of AtomicOS:

- **Stack Protection** (`stack_protection.tempo`)
  ```tempo
  @wcet(50)
  @security(kernel)
  function check_stack_canary(canary: int32): bool {
      return canary == STACK_CANARY_VALUE;
  }
  ```

- **Real-time Scheduler** (`realtime_scheduler.tempo`)
  ```tempo
  @wcet(200)
  function schedule_next_task(): int32 {
      // Deterministic task selection
  }
  ```

- **Memory Protection** (`guard_pages.tempo`)
  ```tempo
  @wcet(100)
  @constant_time
  function validate_memory_access(addr: int32): bool {
      // Constant-time bounds checking
  }
  ```

## WCET Analysis

### Simple Model (v3)
Basic cycle counting per instruction:
- Arithmetic: 1 cycle
- Multiplication: 3 cycles  
- Division: 10 cycles
- Memory access: 4 cycles

### Advanced Model (v4 - in development)
Considers hardware effects:
- Cache hits/misses
- Pipeline stalls
- Branch prediction
- TLB effects

## Language Specification

### Types
- `int32`, `int64` - Fixed-size integers
- `bool` - Boolean values
- `void` - No return value
- Arrays: `int32[10]` - Fixed size only
- Pointers: `int32*` - With bounds checking

### Control Flow
```tempo
// Bounded loops only
for (i: int32 = 0; i < 10; i++) {
    // Loop body
}

while (condition) [max 100] {
    // Maximum 100 iterations
}

if (x > 0) {
    // Then branch
} else {
    // Else branch
}
```

### Annotations
- `@wcet(n)` - Maximum n cycles
- `@security(level)` - Security level (0-3)
- `@constant_time` - No timing variations
- `@pure` - No side effects

## Comparison with Other Languages

| Feature | Tempo | C | Rust | Ada SPARK |
|---------|-------|---|------|-----------|
| WCET Analysis | ✅ Built-in | ❌ External tools | ❌ | ⚠️ Limited |
| Memory Safety | ✅ Static only | ❌ | ✅ | ✅ |
| Determinism | ✅ Guaranteed | ⚠️ Possible | ⚠️ Possible | ✅ |
| Real-time | ✅ | ⚠️ | ❌ | ✅ |
| Learning Curve | Easy | Easy | Hard | Hard |

## Academic Value

Tempo demonstrates several research concepts:
- Static WCET analysis in compiler
- Language-level security annotations
- Deterministic compilation strategies
- Integration with OS kernel development

Suitable for:
- Embedded systems courses
- Real-time systems research
- Compiler construction classes
- Security-focused OS development

## Limitations

Current limitations (by design):
- No heap allocation
- No recursion
- No function pointers
- No unbounded loops
- No external libraries

These ensure determinism but limit general-purpose use.

## Future Development

### Version 3.0 (Current)
- ✅ Basic WCET analysis
- ✅ Code generation
- ✅ Used in AtomicOS production

### Version 4.0 (In Progress)
- 🚧 Advanced parser
- 🚧 Hardware-aware WCET
- 🚧 Optimization passes
- 🚧 Debug symbols

### Roadmap
- [ ] LLVM backend option
- [ ] Formal verification
- [ ] ARM Cortex-M target
- [ ] VSCode extension

## Contributing

We welcome contributions! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Publications & Talks

- "Tempo: Deterministic Programming for Real-Time Systems" (in preparation)
- AtomicOS: Building Secure Systems with Domain-Specific Languages

## License

MIT License - See [LICENSE](LICENSE) for details.

## Contact

- GitHub: [@ipenas-cl](https://github.com/ipenas-cl)
- Project: [AtomicOS](https://github.com/ipenas-cl/AtomicOS)
- Documentation: [Tempo Language](https://ipenas-cl.github.io/AtomicOS/tempo/)

---

*"In a world of uncertainty, Tempo brings determinism."*