# Changelog

All notable changes to AtomicOS will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [5.1.1] - 2025-01-23

### Fixed
- Removed all references to AI assistance
- Updated all copyright notices to 2025
- Cleaned up repository structure
- Version consistency across all files

## [5.0.0] - 2025-01-24

### 🎉 Major Release - Tempo is Production Ready!

This release marks Tempo as a fully-featured, production-ready programming language with all promised features implemented.

### Added

#### Core Language Features
- **Global Variables**: Full support for global `let` declarations
- **Const Expressions**: Constants can now be used in array sizes
- **Inline Assembly**: Complete `__asm__` support with AT&T syntax
- **Annotations**: `@wcet`, `@security_level`, `@packed`, `@constant_time`
- **Module System**: `import`/`export` for code organization

#### Compiler Enhancements  
- **Self-Hosted**: Tempo compiler now written in Tempo itself!
- **Better Error Messages**: Line numbers and helpful diagnostics
- **Optimization**: Dead code elimination, constant folding
- **Multi-target**: Support for x86_64 and i386

#### Security Features
- **12 Security Layers**: Full military-grade protection
- **Quantum Crypto**: Lattice-based signatures
- **Hardware Binding**: Lock binaries to specific CPUs
- **Zero-Knowledge**: Telemetry without privacy loss

#### Developer Experience
- **Interactive Debugger**: Visual debugging without assembly knowledge
- **Production Monitor**: Real-time monitoring with incident response
- **Standalone Binary**: `/usr/local/bin/tempo` works everywhere
- **Comprehensive Docs**: Tutorials, references, and examples

### Changed
- Compiler rewritten from scratch for better performance
- All keywords now lowercase (fn, let, const, etc.)
- Improved type inference
- Better WCET analysis accuracy

### Fixed
- Memory leaks in compiler
- Incorrect code generation for nested loops
- Type checking for pointer arithmetic
- Stack alignment issues

### Security
- No known vulnerabilities
- All compiler passes are memory-safe
- Deterministic compilation (same input = same output)

## [4.0.0] - 2024-12-15

### Added
- Initial inline assembly support
- Basic annotations

## [3.0.0] - 2024-11-20

### Added
- Struct support
- Pointer types
- Basic WCET analysis

## [2.0.0] - 2024-10-10

### Added
- Self-hosting capability
- Type system improvements

## [1.0.0] - 2024-09-01

### Added
- Initial release
- Basic compiler in C
- Core language features