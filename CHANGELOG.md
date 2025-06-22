# Changelog

All notable changes to AtomicOS will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.3.0] - 2025-06-22

### Added
- **Tempo v1.3.0 - Assignment Expressions**
  - Assignment operator support (`x = y`)
  - Field assignment for structs (`obj.field = value`)
  - Array element assignment (`arr[i] = value`)
  - Proper lvalue handling in code generation
- **Struct Enhancements**
  - Automatic field offset calculation
  - Support for `@packed` structs
  - Struct size computation with alignment
  - Field type propagation for type checking
- **Documentation Reorganization**
  - New organized docs structure under `docs/`
  - Comprehensive developer guide
  - Installation and quick start guides
  - Architecture overview
  - Consolidated release notes in `docs/releases/`

### Changed
- Tempo compiler architecture improvements
  - Better type information propagation
  - Enhanced struct handling
  - Improved error messages
- Documentation structure completely reorganized
- Makefile updated to version 1.3.0

### Fixed
- Struct field access now properly uses calculated offsets
- Type information correctly attached to identifiers
- Assignment to struct fields generates correct assembly

## [1.2.2] - 2025-06-22

### Added
- Documentation updates for v1.2.0 features
- Updated GitHub Pages website

### Changed
- Version references updated throughout documentation
- Improved release notes formatting

## [1.2.1] - 2025-06-22

### Fixed
- Multiple syntax issues in example Tempo files
- Missing semicolons after statements
- Function return type syntax (`:` → `->`)

### Known Issues
- Some complex example files still have syntax errors

## [1.2.0] - 2025-06-22

### Added
- **Hardware Intrinsics** - Direct memory and I/O access without C
  - `memory_read8/16/32()` - Read from memory
  - `memory_write8/16/32()` - Write to memory
  - `io_in8/16/32()` - Read from I/O ports
  - `io_out8/16/32()` - Write to I/O ports
  - `cpu_cli()`, `cpu_sti()`, `cpu_hlt()` - CPU control
- New example: `intrinsics_vga_hello.tempo` demonstrating hardware access

### Changed
- Tempo can now write complete device drivers without C dependencies
- Updated compiler to v1.2.0

## [1.1.0] - 2025-06-22

### Added
- **Tempo Compiler Enhancements**
  - Hexadecimal number support (`0xFF`, `0xDEADBEEF`)
  - Inline assembly with `asm { }` blocks
  - Security annotation `@constant_time` for timing-safe operations
  - Modern function syntax with `->` return types
- **New Keywords**
  - `packed`, `repr`, `align` for struct attributes
  - `inline`, `no_inline` for functions
  - `must_use`, `deprecated` for API design

### Changed
- Updated all example files to use new `->` syntax for return types
- Improved tokenizer to handle hex numbers correctly
- Enhanced parser for inline assembly support

### Fixed
- Hex number tokenization bug
- Missing keyword recognition in tokenizer

## [1.0.0] - 2025-06-22

### Added
- **Production-Ready Kernel**
  - Complete interrupt system with IDT, ISRs, and IRQs
  - Process management with context switching
  - Real-time scheduler (EDF/RMS) with deadline guarantees
  - System call interface with WCET bounds
  - File system with VFS abstraction
  - IPC system with message passing
- **Tempo v1.0.0 - Complete Systems Language**
  - Struct support with memory layout control
  - Pointer types (bounded and raw)
  - Inline assembly for hardware control
  - Complete type system (all integer types, bool, arrays)
  - Module system with imports/exports
  - Enhanced control flow (for loops, match, break/continue)
- **Security Enhancements**
  - Full W^X protection enforcement
  - Enhanced stack protection with canaries
  - Guard pages between all memory regions
  - KASLR with deterministic PRNG
  - Multi-level security validation
- **Developer Experience**
  - Professional documentation site
  - Comprehensive test suite
  - Clean repository structure
  - Improved build system
  - Better error messages

### Changed
- Tempo can now replace C entirely for OS development
- Improved kernel stability and performance
- Enhanced security subsystem integration
- Cleaner codebase organization
- Better documentation structure

### Removed
- All C dependencies for core OS functionality
- Temporary and backup files
- Redundant test scripts
- Legacy code paths

## [0.8.0] - 2025-06-21

### Added
- Complete interrupt system implementation
  - IDT (Interrupt Descriptor Table) with 256 entries
  - ISRs for all 32 CPU exceptions
  - IRQ handlers for hardware interrupts
  - PIC (8259A) remapping to avoid conflicts
  - Timer interrupt at 1000Hz for real-time scheduling
  - WCET tracking for interrupt handlers
  - Nested interrupt support (max 3 levels)
- Interrupt statistics and monitoring
  - Cycle counting per interrupt
  - Maximum cycle tracking
  - Overflow detection and reporting
- Visual indicators
  - Exception messages on screen
  - Timer tick indicator (rotating character)
- Deterministic interrupt handling
  - Bounded execution times
  - Predictable latency
  - No dynamic allocation in handlers

### Changed
- Kernel size increased to 16KB to accommodate interrupt system
- Updated version messages throughout codebase

### Technical Notes
- Interrupt vectors 0-31: CPU exceptions
- Interrupt vectors 32-47: Hardware IRQs (remapped)
- Timer configured for 1ms ticks (1000Hz)
- All handlers respect WCET bounds

## [0.7.0] - 2024-06-21

### Added
- Tempo v3.0 compiler with advanced features
  - WCET (Worst-Case Execution Time) analysis
  - Security level verification
  - Enhanced error handling and syntax validation
  - Support for security annotations
  - Real-time scheduling constructs
- Comprehensive test suite
  - 16 automated tests with 100% pass rate
  - Regression testing for all examples
  - Pre-commit validation hooks
  - Test categories: compilation, WCET, security, real-time
- Professional development tools
  - Release management scripts (patch/minor/major)
  - Pre-commit quality checks
  - Version consistency validation
  - Automated changelog generation
- Advanced security features
  - Process sandboxing framework
  - Capability restriction system
  - Multi-level security (Bell-LaPadula)
  - Trusted execution with signatures
  - Enhanced memory protection
- Real-time capabilities
  - EDF/RMS scheduling algorithms
  - Bounded interrupt latency
  - Time-Sensitive Networking support
  - Watchdog deadline monitoring
- New example programs
  - ideal_os_extensions.tempo - Advanced security features
  - realtime_scheduler.tempo - RT scheduling framework
  - demo_ideal_os.tempo - Comprehensive feature demo

### Changed
- Updated Tempo compiler from v2.0 to v3.0
- Enhanced Makefile with testing and release targets
- Improved error messages and compiler diagnostics
- Cleaned codebase removing external OS references
- Updated all documentation to reflect new features

### Fixed
- Compiler syntax error detection
- Memory allocation determinism
- Security framework integration
- Build system dependency handling

### Security
- Removed all hardcoded values and potential secrets
- Enhanced compile-time security verification
- Added constant-time operation support
- Improved stack protection mechanisms

## [0.6.1] - Previous Release

### Added
- Basic Tempo compiler v2.0
- Initial deterministic core implementation
- Guard pages for memory protection
- Stack-smashing protection (SSP)
- Basic real-time scheduling
- W^X memory protection
- KASLR implementation

### Changed
- Improved kernel architecture
- Enhanced security subsystems
- Updated bootloader

### Fixed
- Memory alignment issues
- Interrupt handling bugs
- Compilation warnings