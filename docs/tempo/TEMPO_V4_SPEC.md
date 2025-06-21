# Tempo v4 Language Specification

## Vision

Tempo v4 is a deterministic systems programming language that provides memory safety, predictable timing, and zero-cost abstractions. It's designed to replace C for kernel development while maintaining the ability to reason about every CPU cycle.

## Core Principles

1. **Zero Undefined Behavior** - Every operation has defined semantics
2. **Predictable Performance** - WCET analysis for all constructs
3. **Memory Safety** - Compile-time and runtime bounds checking
4. **Hardware Control** - Direct access when needed, safe abstractions when possible
5. **No Hidden Costs** - What you write is what executes

## New Features in v4

### 1. Structures with Layout Control

```tempo
// Basic struct
struct Point {
    x: int32,
    y: int32
}

// Packed struct (no padding)
@packed
struct PackedData {
    flag: bool,      // 1 byte
    value: int32     // 4 bytes, no padding
}

// C-compatible struct
@repr(C)
struct FileDescriptor {
    fd: int32,
    flags: int32,
    offset: int64
}

// Union type for low-level manipulation
union Register {
    full: int32,
    bytes: int8[4],
    halves: int16[2]
}
```

### 2. Advanced Pointer Operations

```tempo
// Safe pointer with bounds
ptr<T, N>  // Pointer to T with N elements

// Example usage
function process_buffer(buf: ptr<int32, 100>): void {
    // Bounds checked at compile time
    for (i: int32 = 0; i < 100; i++) {
        buf[i] = i * 2;  // Safe indexing
    }
    
    // Pointer arithmetic with bounds
    let p: ptr<int32, 50> = buf + 50;  // OK, within bounds
    // let bad: ptr<int32, 60> = buf + 50;  // Compile error!
}

// Raw pointers for kernel/hardware
@unsafe
function mmio_write(addr: raw_ptr<int32>, value: int32): void {
    *addr = value;  // Direct memory write
}
```

### 3. Inline Assembly

```tempo
@wcet(10)
function outb(port: int16, value: int8): void {
    asm {
        "mov al, {value}"
        "mov dx, {port}"
        "out dx, al"
        : // no outputs
        : "a"(value), "d"(port)  // inputs
        : // no clobbers
    }
}

@wcet(15)
function get_cr3(): int32 {
    let result: int32;
    asm {
        "mov eax, cr3"
        : "=a"(result)  // output
        : // no inputs
        : // no clobbers
    }
    return result;
}
```

### 4. System Call Interface

```tempo
// Declare external interrupt handler
@interrupt(0x80)
@wcet(100)
function syscall_handler(regs: ptr<RegisterState, 1>): void {
    match regs.eax {
        SYS_WRITE => sys_write(regs.ebx, regs.ecx, regs.edx),
        SYS_READ  => sys_read(regs.ebx, regs.ecx, regs.edx),
        _         => regs.eax = -ENOSYS
    }
}

// Port I/O intrinsics
@intrinsic("inb")
function inb(port: int16): int8;

@intrinsic("outb")
function outb(port: int16, value: int8): void;
```

### 5. Memory Management Primitives

```tempo
// Static allocation with alignment
@align(4096)  // Page aligned
static KERNEL_HEAP: int8[1024 * 1024];  // 1MB heap

// Memory regions with permissions
@memory_region(0x100000, 0x200000, READ | WRITE | EXEC)
static KERNEL_CODE: region;

// DMA-safe buffers
@dma_coherent
@align(64)  // Cache line aligned
static DMA_BUFFER: int8[4096];
```

### 6. Standard Library

```tempo
module mem {
    @wcet(n * 4)  // 4 cycles per byte
    @constant_time
    function copy(dst: ptr<int8, n>, src: ptr<int8, n>, n: int32): void {
        for (i: int32 = 0; i < n; i++) {
            dst[i] = src[i];
        }
    }
    
    @wcet(n * 2)
    @constant_time
    function set(dst: ptr<int8, n>, value: int8, n: int32): void {
        for (i: int32 = 0; i < n; i++) {
            dst[i] = value;
        }
    }
    
    @wcet(n * 3)
    @constant_time
    function compare(a: ptr<int8, n>, b: ptr<int8, n>, n: int32): bool {
        let equal: bool = true;
        for (i: int32 = 0; i < n; i++) {
            equal = equal & (a[i] == b[i]);
        }
        return equal;
    }
}

module string {
    @wcet(100)  // Bounded string length
    function length(s: ptr<int8, ?>, max: int32): int32 {
        let len: int32 = 0;
        while (len < max && s[len] != 0) [max] {
            len++;
        }
        return len;
    }
}
```

### 7. Error Handling

```tempo
// Result type for error handling
enum Result<T, E> {
    Ok(T),
    Err(E)
}

// Error codes
enum KernelError {
    OutOfMemory,
    InvalidAddress,
    PermissionDenied,
    DeviceError(int32)
}

// Usage
function allocate_page(): Result<ptr<int8, 4096>, KernelError> {
    if (pages_available > 0) {
        return Result::Ok(get_next_page());
    } else {
        return Result::Err(KernelError::OutOfMemory);
    }
}
```

### 8. Compile-Time Execution

```tempo
// Compile-time constants
const PAGE_SIZE: int32 = 4096;
const MAX_PROCS: int32 = 64;

// Compile-time functions
@const
function max(a: int32, b: int32): int32 {
    return if (a > b) a else b;
}

// Compile-time assertions
@static_assert(sizeof(ProcessControlBlock) <= PAGE_SIZE);
@static_assert(MAX_PROCS > 0 && MAX_PROCS <= 256);
```

### 9. Type System Enhancements

```tempo
// Type aliases
type pid_t = int32;
type size_t = int32;
type physaddr_t = int32;

// Phantom types for type safety
struct PhysAddr<T> {
    addr: physaddr_t
}

struct VirtAddr<T> {
    addr: int32
}

// Ensures physical/virtual addresses aren't mixed
function map_page(phys: PhysAddr<Page>, virt: VirtAddr<Page>): void {
    // Implementation
}
```

### 10. Modules and Visibility

```tempo
// Private by default
module kernel::memory {
    // Private to module
    static FREE_PAGES: int32 = 1024;
    
    // Public export
    pub function allocate(): Result<PhysAddr<Page>, KernelError> {
        // ...
    }
    
    // Public type
    pub struct Page {
        data: int8[4096]
    }
}

// Using modules
import kernel::memory::{allocate, Page};
```

## WCET Analysis for New Features

### Struct Access
- Field access: 1 cycle (compile-time offset)
- Nested struct: additive (1 cycle per level)

### Pointer Operations
- Dereference: 4 cycles (L1 cache hit assumed)
- Bounds check: 2 cycles (compile-time when possible)
- Arithmetic: 1 cycle

### Inline Assembly
- User-specified WCET
- Compiler validates against instruction timings

### System Calls
- Interrupt overhead: 50-100 cycles
- Plus handler WCET

## Safety Guarantees

1. **Memory Safety**
   - All array accesses bounds checked
   - No use-after-free (static lifetime analysis)
   - No null pointer dereferences

2. **Type Safety**
   - No implicit conversions
   - Strong typing for hardware addresses
   - Enum exhaustiveness checking

3. **Concurrency Safety**
   - No data races (ownership system)
   - Atomic operations explicit
   - Interrupt safety annotations

## Migration Path from C

```tempo
// C code
typedef struct {
    int pid;
    char name[16];
    void* stack;
} process_t;

// Tempo equivalent
struct Process {
    pid: pid_t,
    name: int8[16],
    stack: VirtAddr<Stack>
}

// C function
int create_process(const char* name, void* entry) {
    // ...
}

// Tempo equivalent
function create_process(name: ptr<int8, ?>, entry: VirtAddr<Code>)
    : Result<pid_t, KernelError> {
    // ...
}
```

## Comparison with Other Languages

| Feature | Tempo v4 | C | Rust | Zig |
|---------|----------|---|------|-----|
| Memory Safety | ✅ Compile + Runtime | ❌ | ✅ Compile-time | ✅ Compile-time |
| WCET Analysis | ✅ Built-in | ❌ External | ❌ | ❌ |
| Zero-cost Abstractions | ✅ | ❌ | ✅ | ✅ |
| Inline Assembly | ✅ Safe | ✅ Unsafe | ✅ Unsafe blocks | ✅ |
| Deterministic | ✅ Guaranteed | ⚠️ | ⚠️ | ✅ |
| Kernel Ready | ✅ | ✅ | ⚠️ std issues | ✅ |

## Implementation Plan

1. **Phase 1**: Parser and AST (Week 1-2)
   - Recursive descent parser
   - Complete AST representation
   - Error recovery

2. **Phase 2**: Type System (Week 3-4)
   - Type inference
   - Generics
   - Lifetime analysis

3. **Phase 3**: Code Generation (Week 5-6)
   - Direct to assembly
   - Optimization passes
   - WCET calculation

4. **Phase 4**: Standard Library (Week 7-8)
   - Core modules
   - Hardware abstractions
   - Testing framework

This is our path to making AtomicOS truly eat its own dog food!