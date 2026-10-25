# AtomicOS

**Deterministic Real-Time Security Operating System**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.8-green)](VERSION) [![CI](https://github.com/ipenas-cl/AtomicOS/actions/workflows/ci.yml/badge.svg)](https://github.com/ipenas-cl/AtomicOS/actions/workflows/ci.yml)

**Written in [Copihue](https://github.com/ipenas-cl/copihue).**

## Overview

AtomicOS is a hobby operating system. Its kernel is written in [Copihue](https://github.com/ipenas-cl/copihue); only the 512-byte bootloader is x86-64 assembly. The long-term idea is a deterministic, real-time kernel for security-critical use, where every operation has a known worst-case execution time (WCET) and isolation is enforced by hardware.

> **Status: experimental hobby OS, kernel 0.8.** `make all` builds a bootable 64-bit image. A NASM bootloader switches to long mode and loads a kernel that is **written entirely in [Copihue](https://github.com/ipenas-cl/copihue) v0.22.1**: entry and .bss, VGA text and COM1, an IDT (CPU exceptions print a line on the console, which is mirrored to serial; `#PF` also prints `read_cr2()`; exception names are `-> *u8`), the PIC remapped to vectors 32-47, a PIT at 100 Hz (`let volatile ticks`), a PS/2 keyboard on IRQ1, and a shell. `make qemu-test` checks the tick counter, the keyboard IRQ, `read_cr3()` against the bootloader page tables, a read of unmapped `0x200000` (the handler resumes and `pf resumed` prints the byte `0x00000000`, which is the byte at that address), a deliberate `#UD`, and the shell replies for `echo`, `add`, `mul`, `version`, `help` and an unknown command. `clear` and `reboot` are still in the shell; the test does not type `reboot`, because that resets the machine. The features under "Design goals" are goals, not implemented features.

![AtomicOS shell running a Copihue kernel in QEMU](docs/copihue-shell.png)

Demo video (40 s): [docs/atomicos-copihue-demo.mp4](docs/atomicos-copihue-demo.mp4) — compiling the kernel with `copihue --flat`, booting it in QEMU and typing shell commands.

## How much of AtomicOS is written in Copihue

Measured by source lines that end up in the boot image:

| | Before (v5.2.2) | Now (kernel 0.8) |
|---|---|---|
| Bootloader (NASM) | 231 | 233 (`boot64.asm`, reads the kernel with LBA; 64-bit code selector is 0x18) |
| Kernel, assembly | 75 (`kernel_simple64.asm`) | 0 (no entry stub; `asm("ud2")` is the only inline instruction, used by the `ud2` shell command) |
| Kernel, Copihue | 0 | 659 (`src/kernel/copihue/*.lap`, built with `copihue --flat 0x10000`) |
| **Share in Copihue** | **0%** | **100% of the kernel; ~74% of the image by source lines (the rest is the bootloader)** |

The compiler is pinned to the tag `v0.22.1` (`COPIHUE_COMMIT` in the `Makefile`; that tag is commit `e630cd025f7d03bca89e94a068a47addf34194c2`). `make` fetches and builds it into `build/copihue`. To use a local compiler: `make COPIHUE=/path/to/copihue`. The older `*.asm`, `*.inc` and `*.tempo` files under `src/kernel/` are not part of the current image (see `src/kernel/LEGACY.md`).

## Co-evolution roadmap

AtomicOS and Copihue grow together. Done in this tree means it is in the boot image and covered by `make qemu-test`.

| OS milestone | State |
|---|---|
| IDT, CPU exceptions, PIC remapped to 32-47, PIT at 100 Hz, IRQ1 keyboard | done (`interrupt fn`, `packed struct`, `let volatile`, `lidt`). The long-mode code selector is 0x18 |
| Page-fault address and the bootloader page tables | done. `#PF` prints `read_cr2()`. `cr3` prints `read_cr3()` and the first PML4 and PDPT entries. A not-present fault in the bootloader's PD is filled with a 2MB identity page and `invlpg`, then the instruction retries. After `mul 6 -7`, `pf resumed` prints the byte just read (`0x00000000` at `0x200000`). This kernel does not call `write_cr3` |
| Physical page allocator and a new address space | planned. `write_cr3` has been in the compiler since v0.20.1, but there is no allocator and no second set of tables |
| Bootloader stage 2 in Copihue | planned. Needs 16/32-bit codegen, or the 512-byte stub stays |
| Separate compilation of kernel modules | planned. Needs modules and relocatable ELF objects. The kernel is still one concatenated `.lap` file |
| User mode and syscalls | planned. The compiler has `syscall fn`, but this kernel does not program `STAR` / `LSTAR` / `EFER` or a TSS |

`fn exc_name(vec: i64) -> *u8` compiles on v0.22.1 (on v0.20.1 the same signature failed with `expected '{', found 'u8'`). One request is still open for the language: in one function, a second `let` of a name already bound is stored in a new stack slot, but a use of that name still reads the first binding. Copihue v0.22.2 is expected to fix that lexical scope; this tree does not patch the compiler. `run_command` therefore uses distinct names (`rhs` and `fault_byte`; `i` and `name_i`).

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
│   ├── bootloader/boot64.asm   # 512-byte stub: long mode, GDT, loads the kernel
│   └── kernel/copihue/         # the boot image (Copihue)
│       ├── kernel.lap          # _start, banner, tick samples, shell
│       ├── idt.lap             # IDT, PIC, PIT, CPU exceptions
│       ├── keyboard.lap        # PS/2 on IRQ1
│       ├── console.lap         # VGA text and COM1
│       └── shell.lap           # line editor and commands
├── build/                      # boot.bin, kernel.bin, atomicos.img (generated)
└── tests/qemu_shell_test.sh    # QEMU sendkey checks
```

Files under `src/kernel/` other than `copihue/` are not linked into the image.

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

CI ([.github/workflows/ci.yml](.github/workflows/ci.yml)) builds the image, boots it in QEMU for 10 seconds (the process must still be running), and runs `make qemu-test`. That test types the six original shell commands with `sendkey` and checks their replies, checks that `ticks` grows by 100 in about one second and again while the shell runs, checks that `irqs` reports keyboard interrupts, checks that `cr3` prints `0x1000` and the bootloader's PML4 and PDPT entries (with the CPU accessed bit set), checks that `pf` prints `cr2=0x00200000` and that `pf resumed` after `mul 6 -7` prints `0x00000000`, and checks that `ud2` prints `exception: vector 6 #UD invalid opcode`.
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
