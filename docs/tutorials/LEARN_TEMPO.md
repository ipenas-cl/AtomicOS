# Learn Tempo in 10 Minutes

Welcome to Tempo! If you know C, Rust, or Go, you'll feel right at home. Tempo combines the best of systems programming with unique features for security and real-time systems.

## 🚀 Quick Start

### Hello World

```tempo
module hello;

import io;

function main() -> int32 {
    io::println("Hello, Tempo!");
    return 0;
}
```

That's it! No headers, no boilerplate, just clean code.

## 📦 Basic Concepts

### 1. Variables and Types

```tempo
function examples() -> void {
    // Immutable by default (like Rust)
    let x = 42;              // Type inference
    let y: int32 = 100;      // Explicit type
    
    // Mutable variables
    let mut count = 0;
    count = count + 1;       // OK
    
    // Basic types
    let a: int8 = 127;       // 8-bit signed
    let b: uint32 = 0xFF;    // 32-bit unsigned (hex!)
    let c: bool = true;      // Boolean
    let d: char = 'A';       // Character
    let e: float64 = 3.14;   // Double precision
}
```

### 2. Functions

```tempo
// Simple function
function add(a: int32, b: int32) -> int32 {
    return a + b;
}

// Multiple return values (coming soon)
function divmod(a: int32, b: int32) -> (int32, int32) {
    return (a / b, a % b);
}

// No return value
function print_sum(a: int32, b: int32) -> void {
    io::print_int(a + b);
}
```

### 3. Control Flow

```tempo
function control_flow(x: int32) -> void {
    // If-else
    if x > 0 {
        io::println("Positive");
    } else if x < 0 {
        io::println("Negative");
    } else {
        io::println("Zero");
    }
    
    // While loop
    let mut i = 0;
    while i < 10 {
        io::print_int(i);
        i = i + 1;
    }
    
    // For loop
    for let j = 0; j < 10; j = j + 1 {
        io::print_int(j);
    }
    
    // Match expression (pattern matching!)
    match x {
        0 => io::println("Zero"),
        1..10 => io::println("Small"),
        _ => io::println("Large")
    }
}
```

### 4. Arrays and Pointers

```tempo
function arrays_and_pointers() -> void {
    // Fixed-size array
    let numbers: array<int32, 10> = [0; 10];  // All zeros
    let primes = [2, 3, 5, 7, 11];            // Type inferred
    
    // Accessing elements
    let first = primes[0];
    
    // Pointers (like C, but safer)
    let x = 42;
    let ptr: ptr<int32> = &x;      // Take address
    let value = *ptr;               // Dereference
    
    // Pointer arithmetic
    let arr = [1, 2, 3, 4, 5];
    let p = &arr[0];
    let second = *(p + 1);          // Points to arr[1]
}
```

### 5. Structs and Enums

```tempo
// Define a struct
struct Point {
    x: float32,
    y: float32
}

// Methods on structs
impl Point {
    function new(x: float32, y: float32) -> Point {
        return Point { x: x, y: y };
    }
    
    function distance(&self, other: &Point) -> float32 {
        let dx = self.x - other.x;
        let dy = self.y - other.y;
        return sqrt(dx * dx + dy * dy);
    }
}

// Enums with data
enum Result<T, E> {
    Ok { value: T },
    Err { error: E }
}

// Pattern matching on enums
function handle_result(r: Result<int32, String>) -> void {
    match r {
        Result::Ok { value } => {
            io::print("Success: ");
            io::print_int(value);
        },
        Result::Err { error } => {
            io::print("Error: ");
            io::println(error);
        }
    }
}
```

## 🌟 Unique Tempo Features

### 1. WCET (Worst-Case Execution Time)

```tempo
// Guarantee this function completes in 100 cycles
@wcet(100)
function process_data(data: ptr<int32>, len: int32) -> int32 {
    let sum = 0;
    // Compiler ensures this loop fits in 100 cycles
    for let i = 0; i < len && i < 10; i = i + 1 {
        sum = sum + data[i];
    }
    return sum;
}
```

### 2. Constant-Time Operations

```tempo
// Prevent timing attacks
@constant_time
function secure_compare(a: ptr<uint8>, b: ptr<uint8>, len: int32) -> bool {
    let diff = 0;
    for let i = 0; i < len; i = i + 1 {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}
```

### 3. Security Levels

```tempo
@security(kernel)
function privileged_operation() -> void {
    // Only callable from kernel security level
}

@security(user)
function user_function() -> void {
    // This would be a compile error:
    // privileged_operation();
}
```

### 4. Memory Safety

```tempo
function memory_safe() -> void {
    // Bounds checking (automatic)
    let arr = [1, 2, 3];
    // let x = arr[10];  // Compile error!
    
    // Null pointer checks
    let p: ptr<int32> = null;
    // let v = *p;  // Compile error!
    
    if p != null {
        let v = *p;  // OK, compiler knows it's safe
    }
}
```

### 5. Modules and Imports

```tempo
// math.tempo
module math;

export function abs(x: int32) -> int32 {
    return if x < 0 { -x } else { x };
}

export const PI: float64 = 3.14159265359;
```

```tempo
// main.tempo
module main;

import math;
import io;

function main() -> int32 {
    let x = math::abs(-42);
    io::print_int(x);
    return 0;
}
```

## 💪 Advanced Features

### Inline Assembly

```tempo
function fast_memcpy(dst: ptr<uint8>, src: ptr<uint8>, n: int32) -> void {
    asm {
        mov ecx, $n
        mov edi, $dst
        mov esi, $src
        rep movsb
    }
}
```

### Generic Functions

```tempo
function swap<T>(a: ptr<T>, b: ptr<T>) -> void {
    let temp = *a;
    *a = *b;
    *b = temp;
}
```

### Zero-Copy Channels

```tempo
// Coming soon: Deterministic IPC
channel<int32, 10> ch;  // Channel with capacity 10

task producer {
    ch.send(42);  // Non-blocking if space available
}

task consumer {
    let value = ch.receive();  // Blocks until data
}
```

## 🎯 Why Tempo?

### 1. **No Garbage Collection**
Unlike Go or Java, Tempo has predictable memory management.

### 2. **Safer than C**
Bounds checking, null safety, and type safety by default.

### 3. **Simpler than Rust**
No complex lifetime annotations, easier to learn.

### 4. **Real-Time Capable**
WCET guarantees make it perfect for embedded systems.

### 5. **Security First**
Constant-time operations and security levels built-in.

## 🚀 Getting Started

1. **Install Tempo Compiler**
```bash
git clone https://github.com/yourusername/tempo
cd tempo
make install
```

2. **Write Your First Program**
```tempo
// hello.tempo
module hello;
import io;

function main() -> int32 {
    io::println("Welcome to Tempo!");
    return 0;
}
```

3. **Compile and Run**
```bash
tempo build hello.tempo
./hello
```

## 📚 Next Steps

- Read the [full documentation](https://tempo-lang.org/docs)
- Explore [example programs](https://github.com/tempo-lang/examples)
- Join our [community](https://tempo-lang.org/community)

## 🎉 Welcome to the Future of Systems Programming!

Tempo gives you the power of C, the safety of Rust, and the simplicity of Go, with unique features for real-time and secure systems. 

Start writing deterministic, secure, and fast code today!