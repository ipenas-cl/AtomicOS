# AtomicOS

**Deterministic Real-Time Security Operating System**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.8-green)](VERSION) [![CI](https://github.com/ipenas-cl/AtomicOS/actions/workflows/ci.yml/badge.svg)](https://github.com/ipenas-cl/AtomicOS/actions/workflows/ci.yml)

**Written in [Copihue](https://github.com/ipenas-cl/copihue).**

## Overview

AtomicOS is a hobby operating system. The kernel and the bootloader are written in [Copihue](https://github.com/ipenas-cl/copihue). The bootloader is two stages, both Copihue: a 512-byte sector and a second stage loaded at 0x8200. The long-term idea is a deterministic, real-time kernel for security-critical use, where every operation has a known worst-case execution time (WCET) and isolation is enforced by hardware.

> **Status: experimental hobby OS, kernel 0.8.** `make all` builds a bootable 64-bit image. A Copihue bootloader (stage 1 at 0x7C00, stage 2 at 0x8200) switches to long mode, records the BIOS E820 map, and loads a kernel **written entirely in [Copihue](https://github.com/ipenas-cl/copihue) v0.23.0**: entry and .bss, VGA text and COM1, an IDT (CPU exceptions print a line on the console, which is mirrored to serial; `#PF` also prints `read_cr2()`; exception names are `-> *u8`), the PIC remapped to vectors 32-47, a PIT at 100 Hz (`let volatile ticks`), a PS/2 keyboard on IRQ1, a physical page allocator, a GDT with user segments and a TSS (`ltr`), two process address spaces, a PIT round-robin, and a shell. `make qemu-test` checks the previous shell, timer, IRQ, `cr3`, `#PF` and `#UD` cases, alloc/free, a mapped page above 2MB, a ring-3 program that syscalls `user ok` and then faults on a kernel read, and `sched` (two processes, separate PML4s, shared kernel half, `write` / `getpid` / `yield` / `exit`). `clear` and `reboot` are still in the shell; the test does not type `reboot`, because that resets the machine. The features under "Design goals" are goals, not implemented features.

![AtomicOS shell running a Copihue kernel in QEMU](docs/copihue-shell.png)

Demo video (40 s): [docs/atomicos-copihue-demo.mp4](docs/atomicos-copihue-demo.mp4) — compiling the kernel with `copihue --flat`, booting it in QEMU and typing shell commands.

## How much of AtomicOS is written in Copihue

Measured by source lines that end up in the boot image:

| | Before (v5.2.2) | Now (kernel 0.8) |
|---|---|---|
| Bootloader, NASM source | 231 | 0 (no `.asm` is linked; `boot64.asm` is gone) |
| Bootloader, Copihue | 0 | 334 (`boot.lap` 59 + `stage2.lap` 275, `wc -l`). 147 of those lines are `asm("...")` |
| Kernel, assembly files | 75 (`kernel_simple64.asm`) | 0 (no `.asm` in the kernel image) |
| Kernel, Copihue | 0 | 1879 (`src/kernel/copihue/*.lap`, `wc -l`, built with `copihue --flat 0x10000`) |
| **Share in Copihue** | **0%** | **every file linked into the image is `.lap` (2213 lines). 147 bootloader lines and 193 kernel lines are still `asm()` strings** |

Inline `asm` in the kernel `.lap` sources is the `ud2` shell command, the `retfq` that reloads CS after `lgdt`, the pushes that build an `iretq` frame, the jump back from a ring-3 `#PF`, the body of the `user` program, the two `sched` programs, and the register restore in `resume_frame`. Those are Copihue `asm` builtins, the same ones `tests/usermode` in the compiler uses. There is no kernel `.asm` file. The bootloader's `asm()` lines are the real-mode and protected-mode operations v0.23.0 cannot express any other way (segment loads, BIOS register setup, `cpuid`, the carry flag, and the 64-bit forms of `read_cr*` / `inb` / `rdmsr` / `lgdt`). `cli`, `hlt`, `int` and `ljmp` are calls. The compiler and nasm errors are quoted in the header of `stage2.lap`. Stage 2 does not hardcode a kernel sector count: `make` writes `ceil(kernel.bin / 512)` into the DAP and refuses to build if that exceeds 128 sectors (64 KiB from `0x10000`).

The compiler is pinned to the tag `v0.23.0` (`COPIHUE_COMMIT` in the `Makefile`; that tag is commit `7dfde0473916501d45840719bf13f8749c5071eb`). `make` fetches and builds it into `build/copihue`. To use a local compiler: `make COPIHUE=/path/to/copihue`. The older `*.asm`, `*.inc` and `*.tempo` files under `src/kernel/` are not part of the current image (see `src/kernel/LEGACY.md`).

## Co-evolution roadmap

AtomicOS and Copihue grow together. Done in this tree means it is in the boot image and covered by `make qemu-test`.

| OS milestone | State |
|---|---|
| IDT, CPU exceptions, PIC remapped to 32-47, PIT at 100 Hz, IRQ1 keyboard | done (`interrupt fn`, `packed struct`, `let volatile`, `lidt`). The long-mode code selector is still 0x18 |
| Page-fault address and demand pages | done. `#PF` prints `read_cr2()`. A not-present fault from ring 0 allocates a 4KB frame, maps it, zeros it, and retries. `pf resumed` then prints `0x00000000`. `cr3` still prints the bootloader PML4 before `user` runs |
| Physical page allocator | done. Bitmap over E820 type-1 RAM from 1MB up to 128MB. `pmm` checks alloc, write, free, double-free, and a mapped page at or above 2MB |
| User mode and one syscall | done. Own GDT, `ltr` of a 104-byte TSS, `STAR`/`LSTAR`/`SFMASK`/`EFER.SCE`, `iretq` to ring 3, `syscall`/`sysret`. A ring-3 read of `0x10000` raises `#PF` and the shell resumes |
| A PML4 per process, shared kernel half | done. `sched` builds two PML4s. PML4[256..511] store one shared kernel PDPT. `write_cr3` switches to each root; a load through `0xFFFF800000000000` matches the byte at `0x10000`. The lower-half root is private |
| Round-robin and `write` / `getpid` / `yield` / `exit` | done. The PIT IRQ from ring 3 uses that process's `TSS.rsp0`. Two user programs print `a` and `b` on COM1. `exit` returns their pages to the physical allocator |
| Bootloader stage 2 in Copihue | done. `boot.lap` (`--boot`) loads `stage2.lap` (`--flat 0x8200`, 8 sectors from LBA 1). The kernel sector count is `ceil(kernel.bin / 512)` from LBA 9, capped at 128 sectors. `asm()` remains where v0.23.0's 16-bit and 32-bit codegen is rejected by nasm; the exact errors are in the header of `stage2.lap` |
| Separate compilation of kernel modules | planned. Needs modules and relocatable ELF objects. The kernel is still one concatenated `.lap` file |

On QEMU 8.2.2 with the default 128MB, `make qemu-test` reported `e820 n=7`, `pmm free=32480 base=0x0000000000100000 end=0x0000000007fe0000`, and `pmm high ok 0x00201000`. The physical page behind the earlier `#PF` is whichever high page `alloc_high_page` returned first; the test does not print that address. A boot of this image on that QEMU printed this map (the automated test checks the summary line, not each entry):

```
e820[0] type=1 base=0x0000000000000000 len=0x000000000009fc00 acpi=1
e820[1] type=2 base=0x000000000009fc00 len=0x0000000000000400 acpi=1
e820[2] type=2 base=0x00000000000f0000 len=0x0000000000010000 acpi=1
e820[3] type=1 base=0x0000000000100000 len=0x0000000007ee0000 acpi=1
e820[4] type=2 base=0x0000000007fe0000 len=0x0000000000020000 acpi=1
e820[5] type=2 base=0x00000000fffc0000 len=0x0000000000040000 acpi=1
e820[6] type=2 base=0x000000fd00000000 len=0x0000000300000000 acpi=1
```

32480 pages is the type-1 range from 1MB up to `0x7fe0000` (the low type-1 range is left reserved). The test does not pin that count: it requires `n >= 1`, `free > 256`, base `0x100000`, and an end above 2MB.

`user` copies a naked program to `0x400000` (present, user, not writable) and a stack page to `0x401000` (present, user, writable). PML4[0] and PDPT[0] gain the user bit so that walk can reach those pages; the 2MB identity page that covers the kernel does not, so a ring-3 `mov al, [0x10000]` prints `exception: vector 14 #PF page fault err=0x00000005 cr2=0x00010000`, then `user blocked`, then `user returned`. The syscall that printed `user ok` uses `set_syscall_stack(0x81000)`, not `TSS.rsp0`. `rsp0` is `0x86000` and is what the CPU loads for that `#PF`. There is no NX bit: user code is executable because EFER.NXE is left clear.

`sched` then builds two more address spaces and does not reuse that bootloader PML4. Each process gets its own PML4, PDPT and PD in low RAM. PD[0] is the same supervisor 2MB identity page (`0x83`), because the kernel is linked at `0x10000` and stage 2 still jumps there. PML4[256] through PML4[511] all hold the physical address of one shared kernel PDPT (`present|write`, no user bit). That PDPT's first directory maps physical 0 as a 2MB page at `0xFFFF800000000000`. `write_cr3` of each process root is followed by `read_cr3`, which must match that root, and by a load of the high alias of `0x10000`, which must match the identity byte (`alias ok`, once per process). Process A has a user page at `0x500000` whose first byte is `0xA5` and prints `M` when it reads it. Process B does not map `0x500000`. Its `mov al, [0x500000]` prints `pf-cross R pid=2 cr2=0x00500000 err=0x00000004`. The following `mov byte [0x500000], 90` prints `pf-cross W pid=2 cr2=0x00500000 err=0x00000006`. The handler writes the saved RIP (`[rbp+136]` in the v0.23.0 `#PF` frame) and the same process continues.

The runnable state is pid, state, PML4, kernel stack, syscall stack and a saved `rbp`. Kernel stacks are `0x60000` and `0x68000`; syscall stacks are `0x70000` and `0x78000`. `SFMASK` is `0x200`, so a syscall runs with interrupts clear and the PIT does not preempt it. A timer IRQ from ring 3 (saved CS.RPL is 3, selector at `[rbp+128]`) is the preemption path: the CPU has already loaded `TSS.rsp0`. The handler stores that frame's `rbp`, points `TSS.rsp0`, the syscall stack and CR3 at the other process, and the `iretq` epilogue pops the other frame. `yield` reads the user RIP, RSP and RFLAGS the compiler saved at the top of the syscall stack, forges the same 160-byte frame, and jumps to that restore. `exit` switches CR3 away, then `free_page`s every page recorded for that process (its PML4, the private tables, the user frames). The shared kernel PDPT and PD stay allocated.

On the same QEMU 8.2.2, `make qemu-test` typed `sched` after `user` and checked those `#PF` lines, `getpid 1`, `getpid 2`, and a COM1 stream `abababababab`. That run printed `switch timer=14 yield=4`, `exit pid=1 freed=7`, `exit pid=2 freed=6`, `sched released=13`, `sched base=32473 back=32473`, `half kept` and `pml4 free`. The test requires at least one timer switch and one yield switch, both letter orders, and the free count to return to the snapshot taken after the shared kernel tables were allocated. `32473` is that snapshot on this run, after `pf`, `pmm` and `user` had already kept a few pages; it is not a second copy of the boot-time `32480`. There is no open language request.

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
│   ├── bootloader/boot.lap     # stage 1: 512 bytes, loads stage 2
│   ├── bootloader/stage2.lap   # stage 2 at 0x8200: E820, long mode, loads the kernel
│   └── kernel/copihue/         # the boot image (Copihue)
│       ├── kernel.lap          # _start, banner, tick samples, shell
│       ├── pmm.lap             # E820 bitmap, alloc/free
│       ├── paging.lap          # 4KB map/unmap and demand #PF
│       ├── gdt.lap             # GDT, TSS, ltr
│       ├── sched.lap           # per-process PML4, round-robin, two programs
│       ├── usermode.lap        # iretq, syscall, ring-3 program
│       ├── idt.lap             # IDT, PIC, PIT, CPU exceptions
│       ├── keyboard.lap        # PS/2 on IRQ1
│       ├── console.lap         # VGA text and COM1
│       └── shell.lap           # line editor and commands
├── build/                      # boot.bin, stage2.bin, kernel.bin, atomicos.img (generated)
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
# nasm, make and QEMU. nasm is what `copihue --boot` and `--flat` invoke.
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

CI ([.github/workflows/ci.yml](.github/workflows/ci.yml)) builds the image, boots it in QEMU for 10 seconds (the process must still be running), and runs `make qemu-test`. That test types the six original shell commands with `sendkey` and checks their replies, checks that `ticks` grows by 100 in about one second and again while the shell runs, checks that `irqs` reports keyboard interrupts, checks that `cr3` prints `0x1000` and the bootloader's PML4 and PDPT entries (with the CPU accessed bit set, before `user` sets the user bit), checks that `pf` prints `cr2=0x00200000` and that `pf resumed` after `mul 6 -7` prints `0x00000000`, checks the E820 summary and `pmm` / `pmm high`, checks that `user` prints `user ok`, then `#PF` with `err=0x00000005` and `cr2=0x00010000`, then `user blocked` and `user returned`, types `sched` and checks distinct PML4s, one shared upper-half entry, `write_cr3`, both `alias ok` lines, the two `pf-cross` lines, `getpid 1` and `getpid 2`, interleaved `a`/`b`, at least one timer switch and one yield switch, `exit` returning every tracked page, and checks that `ud2` prints `exception: vector 6 #UD invalid opcode`.
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
