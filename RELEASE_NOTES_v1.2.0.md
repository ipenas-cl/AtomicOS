# AtomicOS v1.2.0 Release Notes

## Release Date: 2025-06-22

## What's New

### Tempo Compiler v1.2.0 - Memory & I/O Intrinsics

This release introduces hardware intrinsics to Tempo, enabling direct memory and I/O port access without C dependencies. This is a crucial step towards making AtomicOS 100% Tempo-based.

#### 🆕 Memory Intrinsics

**Memory Read Operations:**
- `memory_read8(address)` - Read a byte from memory
- `memory_read16(address)` - Read a 16-bit word from memory  
- `memory_read32(address)` - Read a 32-bit dword from memory

**Memory Write Operations:**
- `memory_write8(address, value)` - Write a byte to memory
- `memory_write16(address, value)` - Write a 16-bit word to memory
- `memory_write32(address, value)` - Write a 32-bit dword to memory

#### 🆕 I/O Port Intrinsics

**Port Input Operations:**
- `io_in8(port)` - Read a byte from I/O port
- `io_in16(port)` - Read a 16-bit word from I/O port
- `io_in32(port)` - Read a 32-bit dword from I/O port

**Port Output Operations:**
- `io_out8(port, value)` - Write a byte to I/O port
- `io_out16(port, value)` - Write a 16-bit word to I/O port
- `io_out32(port, value)` - Write a 32-bit dword to I/O port

#### 🆕 CPU Control Intrinsics

- `cpu_cli()` - Disable interrupts (CLI instruction)
- `cpu_sti()` - Enable interrupts (STI instruction)
- `cpu_hlt()` - Halt CPU (HLT instruction)

#### 📊 WCET Analysis

All intrinsics have defined Worst-Case Execution Times:
- Memory operations: 4 cycles
- I/O operations: 20 cycles
- CPU control: 1-2 cycles

#### 📝 Example Usage

```tempo
// Write "HELLO" to VGA text buffer
function vga_hello() -> int32 {
    let base = 0xB8000;  // VGA text buffer
    
    memory_write8(base, 72);      // 'H'
    memory_write8(base + 1, 0x0F); // White on black
    
    memory_write8(base + 2, 69);   // 'E'
    memory_write8(base + 3, 0x0F);
    
    return 0;
}

// Read keyboard status
function keyboard_status() -> int32 {
    return io_in8(0x64);  // Keyboard status port
}
```

### Benefits

- **No C Dependencies**: Write drivers and low-level code in pure Tempo
- **Direct Hardware Access**: Access memory-mapped devices and I/O ports
- **Type Safety**: Intrinsics are type-checked at compile time
- **WCET Guarantees**: All operations have predictable timing

### Version Information

- **AtomicOS**: v1.2.0
- **Tempo Compiler**: v1.2.0
- **Architecture**: i386

### Next Steps

With these intrinsics, we can now:
- Rewrite C modules (`syscall.c`, `ipc.c`, `fs.c`) in Tempo
- Implement device drivers in pure Tempo
- Create hardware abstraction layers without C

---

**Security First, Stability Second, Performance Third**