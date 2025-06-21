# Developer Guide

This file provides guidance for developers working with the AtomicOS codebase.

## Project Overview

AtomicOS v0.6.1 is a deterministic real-time security operating system written primarily in assembly (x86) and C, with a custom programming language called Tempo for security modules. The project prioritizes "Security First, Stability Second, Performance Third" and targets i386 architecture.

## Build System & Commands

The project uses a comprehensive Makefile with the following key commands:

- `make all` - Build complete OS image (atomicos.img)
- `make run` - Run OS in QEMU with curses display
- `make debug` - Run OS in QEMU with debugging enabled (-s -S flags)
- `make test` - Run Tempo compiler tests on all .tempo files in examples/
- `make security-check` - Run security analysis (WCET bounds, deterministic operations, memory safety)
- `make clean` - Clean build artifacts
- `make distclean` - Clean everything including generated files
- `make info` - Show project information and security features
- `make help` - Display all available targets

### Tempo Compiler Commands

The Tempo compiler is built as part of the build process and used to compile security modules:
- Tempo compiler source: `tools/tempo_compiler.c`
- Usage: `./build/tempo_compiler input.tempo output.s`
- Examples are in `examples/` directory (.tempo files)

## Architecture

### Core Components

1. **Bootloader** (`src/bootloader/boot.asm`) - Initial boot code in x86 assembly
2. **Kernel** (`src/kernel/`) - Main kernel in assembly with security modules
   - `kernel.asm` - Main kernel entry point
   - `mmu_security.c/h` - Memory management unit security
   - `wx_protection.c/h` - W^X (Write XOR Execute) protection
   - Security modules: `deterministic_core.inc`, `guard_pages.inc`, `ssp_functions.inc`
3. **Tempo Language** (`tools/tempo_compiler.c`) - Custom deterministic language compiler
4. **Headers** (`include/`) - System headers for memory, security, types
5. **User Space** (`src/userspace/`) and **LibC** (`src/libc/`) - User applications and C library

### Security Features

The OS implements multiple security layers:
- **W^X Protection**: Memory pages cannot be both writable and executable
- **Stack-Smashing Protection (SSP)**: Deterministic canary-based detection
- **Guard Pages**: Empty pages between memory regions for overflow detection
- **KASLR**: Kernel Address Space Layout Randomization with deterministic PRNG
- **Multi-level Security**: Bell-LaPadula inspired access control (4 levels)

### Deterministic Features

- **WCET Analysis**: All operations have provable worst-case execution times
- **Static Memory Management**: Fixed 64KB memory pool, no dynamic allocation
- **Reproducible Behavior**: Deterministic PRNG with fixed seed for testing
- **Real-Time Scheduling**: Fixed priority scheduling with deadline guarantees
- **Constant-Time Operations**: No timing side-channels in crypto operations

## Development Guidelines

### Code Standards

- All code must have WCET (Worst-Case Execution Time) analysis
- Memory operations must be bounds-checked
- Cryptographic operations must be constant-time
- Security features should be formally verified where possible
- Use C99 standard for C code
- Assembly code targets x86 (i386) architecture

### Build Process

1. Tempo compiler is built first from `tools/tempo_compiler.c`
2. Security modules (.tempo files) are compiled to assembly (.inc files)
3. Bootloader is assembled from `src/bootloader/boot.asm`
4. Kernel is assembled with included security modules
5. Final OS image combines bootloader and kernel into `build/atomicos.img`

### Testing

- Tempo compiler tests run on all `.tempo` files in `examples/`
- Security analysis includes WCET bounds verification
- No traditional unit testing framework - uses custom validation

## Tempo Language v3.0

Tempo is a deterministic programming language for AtomicOS that integrates enterprise-grade security and real-time features:

**Current Status**: Extended compiler with ideal OS features
- **Working**: Functions, variables, arithmetic, WCET analysis, security annotations
- **Security Features**: Process sandboxing, security framework hooks, trusted execution
- **Real-Time**: EDF/RMS scheduling, bounded execution, interrupt handling
- **Deterministic**: Constant-time operations, static memory allocation, formal verification support

**Key Tempo v3.0 Features:**
- **WCET Analysis**: Automatic worst-case execution time calculation and verification
- **Security Levels**: Multi-level security with Bell-LaPadula access control
- **Pledge System**: Process sandboxing and capability restriction framework
- **Real-Time Scheduling**: Rate Monotonic and Earliest Deadline First algorithms
- **Trusted Execution**: Cryptographic signature verification and secure enclaves
- **Time-Sensitive Networking**: Deterministic network scheduling for real-time
- **Guard Pages**: Memory protection with deterministic overflow detection
- **Copy-on-Write FS**: File system integrity with constant-time checksums

**Example Files:**
- `deterministic_core.tempo` - Core deterministic system functions
- `guard_pages.tempo` - Memory protection implementation
- `stack_protection.tempo` - Stack-smashing protection
- `ideal_os_extensions.tempo` - Advanced security and enterprise features
- `realtime_scheduler.tempo` - Advanced real-time scheduling framework
- `demo_ideal_os.tempo` - Comprehensive demonstration of all features

**Compiler Usage:**
- Build: `make all` (builds tempo_compiler_v3)
- Compile: `./build/tempo_compiler input.tempo output.s`
- Test: `make test` (tests all example files)

The Tempo compiler performs WCET analysis, security verification, and generates optimized assembly code for the AtomicOS kernel.

## Testing and Quality Assurance

**Comprehensive Test Suite:**
- `make test` - Run full test suite with regression testing
- `make test-quick` - Run quick compilation tests only  
- `make test-verbose` - Run tests with detailed output
- `scripts/pre-commit.sh` - Pre-commit validation checks

**Test Coverage:**
- Basic compilation and syntax validation
- WCET analysis verification  
- Security feature testing
- Regression testing of all example files
- Error handling and edge cases
- Code quality and external reference checks

**Release Management:**
- `make version` - Show current version information
- `make release-patch` - Increment patch version (0.7.0 -> 0.7.1)
- `make release-minor` - Increment minor version (0.7.0 -> 0.8.0)  
- `make release-major` - Increment major version (0.7.0 -> 1.0.0)
- `scripts/release.sh` - Manual release management with changelog generation

**Development Workflow:**
1. Make changes to code
2. Run `make test` to verify no regressions
3. Run `scripts/pre-commit.sh` to validate commit readiness
4. Use release scripts for version management and tagging
5. All external OS references removed for clean codebase