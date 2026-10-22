# AtomicOS

**Deterministic Real-Time Security Operating System**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-5.2.2-green)](VERSION) [![CI](https://github.com/ipenas-cl/AtomicOS/actions/workflows/ci.yml/badge.svg)](https://github.com/ipenas-cl/AtomicOS/actions/workflows/ci.yml)

**Written in [Copihue](https://github.com/ipenas-cl/copihue).**

## Overview

AtomicOS is a hobby operating system. Its kernel is written in [Copihue](https://github.com/ipenas-cl/copihue); only the 512-byte bootloader is x86-64 assembly. The long-term idea is a deterministic, real-time kernel for security-critical use, where every operation has a known worst-case execution time (WCET) and isolation is enforced by hardware.

> **Status: experimental hobby OS.** `make all` builds a bootable 64-bit image: a NASM bootloader switches to long mode and loads a kernel that is **written entirely in [Copihue](https://github.com/ipenas-cl/copihue)**: entry point and .bss setup, VGA/serial console, PS/2 keyboard driver and shell (the language formerly called Chronos, which grew out of Tempo). In QEMU you can type commands (`help`, `echo`, `add`, `mul`, `clear`, `version`, `reboot`); CI boots the image, types commands with QEMU's `sendkey` and checks the replies. The features under "Design goals" are goals, not implemented features.

![AtomicOS shell running a Copihue kernel in QEMU](docs/copihue-shell.png)

Demo video (40 s): [docs/atomicos-copihue-demo.mp4](docs/atomicos-copihue-demo.mp4) — compiling the kernel with `copihue --flat`, booting it in QEMU and typing shell commands.

## How much of AtomicOS is written in Copihue

Measured by source lines that end up in the boot image:

| | Before (v5.2.2) | Now |
|---|---|---|
| Bootloader (NASM) | 231 | 233 (`boot64.asm`, now reads the kernel with LBA) |
| Kernel, assembly | 75 (`kernel_simple64.asm`) | 0 (no entry stub, no inline `asm()`) |
| Kernel, Copihue | 0 | 318 (`src/kernel/copihue/*.lap`, built with `copihue --flat 0x10000`) |
| **Share in Copihue** | **0%** | **100% of the kernel; ~58% of the image by source lines (the rest is the bootloader)** |

The compiler is pinned to an exact commit (`COPIHUE_COMMIT` in the `Makefile`); `make` fetches and builds it into `build/copihue`. To use a local compiler: `make COPIHUE=/path/to/copihue`. The older `*.asm`, `*.inc` and `*.tempo` files under `src/kernel/` are not part of the current image (see `src/kernel/LEGACY.md`).

## Co-evolution roadmap (planned)

AtomicOS and Copihue grow together: each OS milestone names the compiler feature it needs. Nothing below is done yet.

| OS milestone | Copihue feature it needs |
|---|---|
| IDT, exceptions and interrupt-driven keyboard | interrupt-handler functions (`iretq` epilogue, saved registers) or naked functions |
| PIT timer and a tick counter | same as above, plus `volatile`-style memory access |
| Physical page allocator and paging in Copihue | `u64` bit manipulation helpers, section/address control |
| Bootloader stage 2 in Copihue | 16/32-bit code generation (or keep the asm stub) |
| Separate compilation of kernel modules | modules/imports and a linker-friendly output (ELF objects) |
| User mode and syscalls | `syscall`/`sysret` entry stubs, a calling-convention attribute |

## Design goals

- **100% Deterministic**: Every operation has proven worst-case execution time
- **Real-Time Guarantees**: EDF and RMS schedulers with deadline enforcement
- **Security First**: 12 layers of protection, hardware-enforced isolation
- **Small, auditable code**: assembly only where hardware requires it, the rest in Copihue
- **Minimal Attack Surface**: < 10,000 lines of audited code

## Quick Start

```bash
# Clone the repository
git clone https://github.com/ipenas-cl/AtomicOS.git
cd AtomicOS

# Build the OS
make all

# Run in QEMU
make run

# Boot headless, type shell commands via sendkey, check the output
make qemu-test

# Run headless (no curses window), e.g. over SSH
qemu-system-x86_64 -drive format=raw,file=build/atomicos.img -display none -serial stdio

# Debug with GDB (QEMU waits for gdb on :1234)
make debug
```

## System Requirements

- x86-64 machine or QEMU (`qemu-system-x86_64`)
- `nasm`, `make`, `gcc` and `git` (to build the pinned Copihue compiler), `socat` for `make qemu-test` (`ffmpeg` only for screenshots)

## Architecture

### Core Components

```
AtomicOS/
├── src/
│   ├── bootloader/     # First-stage boot (512 bytes)
│   └── kernel/         # Kernel core
│       ├── kernel.asm  # Main kernel entry
│       ├── interrupts.asm  # IDT and handlers
│       ├── *.tempo     # Security modules (compiled with Tempo)
│       └── *.inc       # Generated from Tempo modules
├── examples/           # Tempo modules for kernel
│   └── atomicos/       # Core OS modules written in Tempo
├── include/            # System headers
├── build/             # Build output
└── scripts/           # Build and utility scripts
```

### Security Architecture

1. **Hardware Isolation**: Separate memory domains
2. **W^X Protection**: No page is both writable and executable
3. **Stack Protection**: Hardware-enforced guard pages
4. **KASLR**: Kernel randomization at boot
5. **Secure Boot**: Cryptographic verification

### Real-Time Features

- **Schedulers**: EDF (dynamic) and RMS (static)
- **WCET Analysis**: Every function has proven bounds
- **Priority Inheritance**: Prevents priority inversion
- **Deadline Monitoring**: Automatic deadline violation handling

## Building

### Prerequisites

```bash
# Only nasm, make and QEMU are needed for `make all` / `make run`.
# Ubuntu/Debian
sudo apt-get install build-essential nasm qemu-system-x86

# macOS
brew install nasm qemu

# Arch Linux
sudo pacman -S base-devel nasm qemu
```

### Build Commands

```bash
make all        # Build the bootable image: build/atomicos.img
make run        # Boot it in QEMU
make clean      # Remove build output
```

## Testing

CI ([.github/workflows/ci.yml](.github/workflows/ci.yml)) builds the image on every push and boots it in QEMU as a smoke test.
The kernel unit tests under `src/tests/` depend on the old Tempo toolchain and are not wired into the Makefile yet.

## Documentation

- [Architecture Guide](docs/architecture/KERNEL_DESIGN.md)
- [Architecture overview](docs/architecture/overview.md)

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing`)
3. Check it still builds and boots (`make all && make run`)
4. Commit changes (`git commit -m 'Add feature'`)
5. Push to branch (`git push origin feature/amazing`)
6. Open a Pull Request

## Performance targets (not measured yet)

- Boot time: < 100ms
- Context switch: < 1000 cycles
- Interrupt latency: < 500 cycles
- Memory allocation: O(1) deterministic

## License

MIT License - see [LICENSE](LICENSE) file

## Author

**Ignacio Peña Sepúlveda** ([@ipenas-cl](https://github.com/ipenas-cl))

---

*"Security First, Stability Second, Performance Third"* - AtomicOS Philosophy
