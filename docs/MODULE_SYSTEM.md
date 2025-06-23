# Tempo Module System

## Overview

The Tempo module system provides a mechanism for organizing code into reusable, composable units with clear boundaries and explicit dependencies. It supports deterministic linking, WCET analysis across module boundaries, and security isolation.

## Module Declaration

Every Tempo source file must declare its module:

```tempo
module mymodule;

// Module contents...
```

Module names follow a hierarchical naming convention:
- `core` - Core system functionality
- `io` - Input/output operations
- `net::tcp` - TCP networking (submodule)
- `crypto::aes` - AES cryptography (submodule)

## Imports

### Basic Import

Import all public symbols from a module:

```tempo
import core;
import io;

function main() {
    io::println("Hello, World!");
}
```

### Aliased Import

Import a module with an alias:

```tempo
import io as console;

function main() {
    console::println("Hello with alias!");
}
```

### Selective Import

Import specific symbols:

```tempo
import core::{malloc, free};
import io::{print, println};

function test() {
    let ptr = malloc(100);
    println("Allocated memory");
    free(ptr);
}
```

### Use Declarations

Bring symbols into the current namespace:

```tempo
import io;
use io::println;

function main() {
    println("No prefix needed!");  // Direct use
}
```

## Exports

### Function Exports

```tempo
// Public function - available to other modules
export function public_api() -> int32 {
    return 42;
}

// Private function - module internal only
function internal_helper() -> void {
    // Not visible outside module
}
```

### Type Exports

```tempo
// Export struct
export struct Point {
    x: int32,
    y: int32
}

// Export enum
export enum Status {
    Ok,
    Error { code: int32 }
}

// Export type alias
export type NodeId = int32;
```

### Constant Exports

```tempo
export const MAX_SIZE: int32 = 1024;
export const VERSION: ptr<char> = "1.0.0";
```

### Re-exports

Re-export symbols from other modules:

```tempo
module wrapper;

import core;
export use core::{malloc, free};  // Re-export
```

## Visibility Rules

### Access Levels

1. **Private** (default): Only accessible within the module
2. **Public** (`export`): Accessible to any module that imports
3. **Protected** (`export(submodules)`): Only accessible to submodules

```tempo
module parent;

// Public to everyone
export function public_fn() -> void {}

// Only submodules can access
export(submodules) function protected_fn() -> void {}

// Private to this module
function private_fn() -> void {}
```

### Submodule Access

```tempo
module parent::child;

import parent;
use parent::protected_fn;  // OK - submodule access

function test() {
    protected_fn();  // Allowed
}
```

## Module Properties

### Deterministic Modules

Modules can be marked as deterministic:

```tempo
@deterministic
module crypto;

// All functions must have WCET bounds
// No non-deterministic operations allowed
```

### Safe Modules

Safe modules cannot use unsafe operations:

```tempo
@safe
module userlib;

// No raw pointers
// No inline assembly
// No unsafe casts
```

### Module Metadata

```tempo
@version("1.2.0")
@author("AtomicOS Team")
@license("MIT")
module mylib;
```

## Compilation and Linking

### Separate Compilation

Each module compiles to a separate object file:

```bash
tempo_compiler core.tempo -o core.o
tempo_compiler io.tempo -o io.o
tempo_compiler app.tempo -o app.o
```

### Module Interface Files

The compiler generates `.tmi` (Tempo Module Interface) files:

```
core.tmi:
  exports:
    - malloc: (size: int32) -> ptr<void> @wcet(50)
    - free: (ptr: ptr<void>) -> void @wcet(20)
  
  properties:
    deterministic: true
    safe: true
    total_wcet: 1000
```

### Linking

The linker resolves module dependencies:

```bash
tempo_link app.o core.o io.o -o app.exe
```

## WCET Analysis

### Cross-Module WCET

The module system tracks WCET across module boundaries:

```tempo
module app;
import io;  // io::print has WCET 100

@wcet(200)
function process() -> void {
    io::print("Processing...");  // WCET 100
    compute();                    // WCET 50
    // Total: 150 < 200 ✓
}
```

### Module-Level WCET

Modules can declare total WCET budget:

```tempo
@wcet_budget(10000)
module realtime;

// Sum of all exported functions must be ≤ 10000
```

## Security

### Security Levels

Modules have security levels that restrict imports:

```tempo
@security(kernel)
module kernel_crypto;

// Can only be imported by kernel-level modules
```

### Capability-Based Imports

```tempo
@requires_capability(NET_ACCESS)
module network;

// Importing module must have NET_ACCESS capability
```

## Module Search Paths

The compiler searches for modules in:

1. Current directory
2. Project `modules/` directory
3. System module path (`/usr/local/tempo/modules`)
4. Paths specified by `TEMPO_MODULE_PATH`

## Example: Complete Module

```tempo
// File: math/statistics.tempo

@version("1.0.0")
@deterministic
@safe
module math::statistics;

import core;
use core::{min, max};

// Private helper
function square(x: int32) -> int32 {
    return x * x;
}

// Calculate mean
@wcet(100)
export function mean(data: ptr<int32>, len: int32) -> int32 {
    if len == 0 {
        return 0;
    }
    
    let sum = 0;
    for let i = 0; i < len; i = i + 1 {
        sum = sum + data[i];
    }
    
    return sum / len;
}

// Calculate variance
@wcet(200)
export function variance(data: ptr<int32>, len: int32) -> int32 {
    let m = mean(data, len);
    let sum_sq = 0;
    
    for let i = 0; i < len; i = i + 1 {
        let diff = data[i] - m;
        sum_sq = sum_sq + square(diff);
    }
    
    return sum_sq / len;
}

// Find range
@wcet(150)
export function range(data: ptr<int32>, len: int32) -> int32 {
    if len == 0 {
        return 0;
    }
    
    let min_val = data[0];
    let max_val = data[0];
    
    for let i = 1; i < len; i = i + 1 {
        min_val = min(min_val, data[i]);
        max_val = max(max_val, data[i]);
    }
    
    return max_val - min_val;
}
```

## Best Practices

### 1. Module Granularity

- Keep modules focused on a single responsibility
- Avoid circular dependencies
- Prefer many small modules over few large ones

### 2. Interface Design

- Export minimal necessary interface
- Use types to enforce constraints
- Document WCET for all exports

### 3. Dependency Management

- Explicitly list all dependencies
- Use selective imports when possible
- Avoid transitive dependency exposure

### 4. Versioning

- Use semantic versioning
- Maintain backward compatibility
- Document breaking changes

## Module System Internals

### Symbol Resolution

1. Check current module scope
2. Check imported symbols
3. Check used namespaces
4. Error if not found

### Compilation Order

Modules compile in dependency order:
1. Leaf modules (no dependencies)
2. Modules depending only on leaves
3. Continue until all compiled

### Binary Format

Module object files contain:
- Export table
- Import table
- Symbol relocations
- WCET metadata
- Security attributes