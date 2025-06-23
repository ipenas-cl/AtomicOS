# Tempo Programming Language

**Current Version**: v1.3.0

Tempo is a deterministic systems programming language designed specifically for AtomicOS. It combines the low-level control of C with modern safety features and real-time guarantees.

## Language Philosophy

1. **Deterministic by Design**: Every operation has predictable timing
2. **Memory Safe**: Compile-time bounds checking and type safety
3. **Zero Overhead**: Abstractions compile to optimal assembly
4. **Real-Time Ready**: Built-in WCET analysis and timing guarantees

## Key Features

### 🛡️ Memory Safety
```tempo
// Bounded pointers prevent overflows
function safe_copy(dst: ptr<int8, 256>, src: ptr<int8, 256>, len: int32) {
    for i in 0..len {
        if i >= 256 { break; }  // Bounds check
        dst[i] = src[i];
    }
}
```

### ⏱️ Real-Time Guarantees
```tempo
// WCET (Worst-Case Execution Time) annotations
function process_sensor_data() -> int32 @wcet(1000) {
    let sum = 0;
    
    // Loop bounds are required
    while @wcet(100) i < 100 {
        sum = sum + read_sensor(i);
        i = i + 1;
    }
    
    return sum;
}
```

### 🔒 Security Features
```tempo
// Constant-time operations for crypto
function secure_compare(a: ptr<int8, 32>, b: ptr<int8, 32>) -> bool @constant_time {
    let diff = 0;
    
    for i in 0..32 {
        diff = diff | (a[i] ^ b[i]);
    }
    
    return diff == 0;
}

// Security level annotations
function kernel_only() @security(3) {
    // Only callable from kernel level
}
```

### 💾 Hardware Access
```tempo
// Direct memory and I/O access (v1.2.0+)
function init_vga() {
    let vga_base = 0xB8000;
    
    // Clear screen
    for i in 0..4000 {
        memory_write8(vga_base + i, 0);
    }
    
    // Set cursor position
    io_out8(0x3D4, 0x0F);
    io_out8(0x3D5, 0);
}
```

### 🔧 Inline Assembly
```tempo
// Inline assembly for low-level operations (v1.1.0+)
function get_cpu_id() -> int32 {
    let id: int32;
    
    asm {
        "mov eax, 1"
        "cpuid"
        "mov %0, eax" : "=r"(id)
    }
    
    return id;
}
```

## Type System

### Basic Types
- `int8`, `int16`, `int32`, `int64` - Signed integers
- `bool` - Boolean values
- `void` - No value

### Composite Types
```tempo
// Structures
struct Point {
    x: int32,
    y: int32
}

// Arrays
let buffer: [256]int8;

// Pointers
let p: ptr<int32>;        // Safe pointer
let rp: raw_ptr<int32>;   // Raw pointer (unsafe)
```

### Type Annotations
```tempo
// Function with full type annotations
function calculate(a: int32, b: int32) -> int32 {
    return a + b;
}

// Variable with type
let result: int32 = calculate(10, 20);

// Type inference
let value = 42;  // Inferred as int32
```

## Control Flow

### Conditionals
```tempo
if x > 0 {
    positive_action();
} else if x < 0 {
    negative_action();
} else {
    zero_action();
}
```

### Loops
```tempo
// For loop with range
for i in 0..10 {
    process(i);
}

// While loop with WCET bound
while @wcet(100) condition() {
    do_work();
}

// Loop with explicit bounds
let i = 0;
while i < limit && i < 1000 {
    array[i] = value;
    i = i + 1;
}
```

### Pattern Matching
```tempo
match value {
    0 => handle_zero(),
    1..10 => handle_small(),
    _ => handle_default()
}
```

## Memory Management

### Static Allocation
```tempo
// All memory is statically allocated
let buffer: [1024]int8;  // Stack allocated
static shared: [64]int32; // Static storage

// No malloc/free - all memory is predictable
```

### Pointer Safety
```tempo
// Bounded pointers include size information
function process(data: ptr<int32, 100>) {
    // Compiler ensures bounds are respected
    for i in 0..100 {
        data[i] = compute(i);
    }
}
```

## Modules and Organization

### Module System
```tempo
// Define a module
module crypto {
    pub function hash(data: ptr<int8, 64>) -> int32 {
        // Implementation
    }
    
    // Private helper
    function mix(a: int32, b: int32) -> int32 {
        return a ^ b;
    }
}

// Use modules
use crypto;

function main() {
    let result = crypto::hash(data);
}
```

## Compiler Intrinsics

### Memory Operations (v1.2.0+)
```tempo
memory_read8(addr)   // Read byte
memory_read16(addr)  // Read word
memory_read32(addr)  // Read dword
memory_write8(addr, value)
memory_write16(addr, value)
memory_write32(addr, value)
```

### I/O Operations (v1.2.0+)
```tempo
io_in8(port)    // Read from I/O port
io_in16(port)
io_in32(port)
io_out8(port, value)
io_out16(port, value)
io_out32(port, value)
```

### CPU Control (v1.2.0+)
```tempo
cpu_cli()  // Disable interrupts
cpu_sti()  // Enable interrupts
cpu_hlt()  // Halt CPU
```

## Best Practices

### 1. Always Specify Bounds
```tempo
// Good
while @wcet(100) i < count && i < 100 {
    process(i);
}

// Bad - unbounded loop
while i < count {  // Error: missing WCET bound
    process(i);
}
```

### 2. Use Type Annotations
```tempo
// Good - clear types
function add(a: int32, b: int32) -> int32 {
    return a + b;
}

// Less clear - avoid when possible
function add(a, b) {  // Types inferred
    return a + b;
}
```

### 3. Handle Errors Explicitly
```tempo
function safe_divide(a: int32, b: int32) -> int32 {
    if b == 0 {
        return 0;  // Or handle error appropriately
    }
    return a / b;
}
```

### 4. Clear Sensitive Data
```tempo
function process_password(pwd: ptr<int8, 32>) {
    // Use password
    authenticate(pwd);
    
    // Clear when done
    for i in 0..32 {
        pwd[i] = 0;
    }
}
```

## Common Patterns

### State Machines
```tempo
enum State {
    IDLE,
    PROCESSING,
    ERROR
}

function state_machine(input: int32) -> State {
    static state: State = IDLE;
    
    match state {
        IDLE => {
            if input > 0 {
                state = PROCESSING;
            }
        },
        PROCESSING => {
            if process(input) {
                state = IDLE;
            } else {
                state = ERROR;
            }
        },
        ERROR => {
            reset();
            state = IDLE;
        }
    }
    
    return state;
}
```

### Ring Buffers
```tempo
struct RingBuffer {
    data: [256]int8,
    head: int32,
    tail: int32
}

function rb_write(rb: ptr<RingBuffer>, value: int8) -> bool {
    let next = (rb->head + 1) % 256;
    
    if next == rb->tail {
        return false;  // Buffer full
    }
    
    rb->data[rb->head] = value;
    rb->head = next;
    return true;
}
```

## Version History

### v1.3.0 (Current)
- Assignment expressions (`x = y`)
- Struct field offset calculation
- Improved type propagation

### v1.2.0
- Hardware intrinsics (memory/IO access)
- Direct hardware programming without C

### v1.1.0
- Hexadecimal literals (`0xFF`)
- Inline assembly blocks
- Security annotations (`@constant_time`)

### v1.0.0
- Initial release
- Core language features
- WCET analysis
- Basic code generation

## Resources

- [Language Specification](v4-spec.md)
- [Examples](../../examples/)
- [Compiler Usage](../tools/compiler.md)
- [API Reference](../api/tempo-stdlib.md)

---
*Tempo: Deterministic Systems Programming for the Modern Era*