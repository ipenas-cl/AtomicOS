# AtomicOS-Specific Examples

This directory contains Tempo code examples that are specifically designed for AtomicOS kernel and system-level functionality.

## Core System Components

- **deterministic_core.tempo** - Core deterministic functions for the kernel
- **guard_pages.tempo** - Memory protection using guard pages
- **stack_protection.tempo** / **stack_protection_v2.tempo** - Stack-smashing protection implementations
- **kaslr.tempo** - Kernel Address Space Layout Randomization

## Interrupt and Process Management

- **interrupt_core.tempo** - Core interrupt handling functionality
- **interrupt_system.tempo** - Full interrupt system implementation
- **realtime_scheduler.tempo** - Real-time scheduling (EDF/RMS)

## Memory Management

- **virtual_memory_demo.tempo** - Virtual memory management demonstration
- **multicore_demo.tempo** - Multi-core processor support

## I/O and Communication

- **intrinsics_vga_hello.tempo** - VGA display output using intrinsics
- **fs_demo.tempo** - File system demonstration
- **ipc_demo.tempo** - Inter-process communication demonstration

## Usage

These examples are compiled as part of the AtomicOS build process and included in the kernel as `.inc` files. They demonstrate:

1. **Security-first design** - All code follows AtomicOS security principles
2. **WCET compliance** - Each function has bounded worst-case execution time
3. **Deterministic behavior** - No dynamic allocation or unpredictable operations
4. **Real-time guarantees** - Support for hard real-time constraints

To compile these examples individually:
```bash
../../build/tempo_compiler example.tempo output.s
```

Note: These examples require AtomicOS kernel environment and are not meant to run standalone.