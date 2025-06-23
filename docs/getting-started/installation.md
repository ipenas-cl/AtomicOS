# AtomicOS Installation Guide

## System Requirements

### Minimum Requirements
- **Architecture**: x86 (i386) processor
- **Memory**: 64KB RAM minimum
- **Storage**: 10MB free disk space

### Build Requirements
- **Operating System**: Linux, macOS, or WSL2 on Windows
- **Compiler**: GCC (with 32-bit support)
- **Assembler**: NASM 2.x or later
- **Build Tools**: GNU Make
- **Emulator**: QEMU (for testing)

## Installing Dependencies

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential nasm qemu-system-x86 gcc-multilib
```

### Fedora/RHEL
```bash
sudo dnf install gcc nasm qemu-system-x86 glibc-devel.i686
```

### macOS
```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install nasm qemu
```

### Arch Linux
```bash
sudo pacman -S base-devel nasm qemu gcc-multilib
```

## Building AtomicOS

### 1. Clone the Repository
```bash
git clone https://github.com/ipenas-cl/AtomicOS.git
cd AtomicOS
```

### 2. Build the Tempo Compiler
```bash
make build/tempo_compiler
```

### 3. Build the Complete OS
```bash
make all
```

This will:
- Compile the Tempo compiler
- Compile all Tempo security modules
- Assemble the bootloader
- Build the kernel
- Create the OS image (`build/atomicos.img`)

### 4. Verify the Build
```bash
make version
```

Expected output:
```
AtomicOS 1.3.0
Architecture: i386
Tempo Compiler: v1.3.0
```

## Running AtomicOS

### In QEMU (Recommended)
```bash
make run
```

This launches AtomicOS in QEMU with a curses-based display.

### With Debugging Support
```bash
make debug
```

In another terminal:
```bash
gdb build/kernel.elf
(gdb) target remote :1234
(gdb) continue
```

### On Real Hardware

**WARNING**: Only attempt this on dedicated test hardware!

1. Write the image to a USB drive:
```bash
sudo dd if=build/atomicos.img of=/dev/sdX bs=512
```
Replace `/dev/sdX` with your USB device.

2. Boot from the USB drive

## Troubleshooting

### Build Errors

**Error**: `nasm: command not found`
- **Solution**: Install NASM using your package manager

**Error**: `gcc: error: unrecognized command line option '-m32'`
- **Solution**: Install 32-bit development libraries (`gcc-multilib`)

**Error**: `make: *** No rule to make target 'all'`
- **Solution**: Ensure you're in the AtomicOS root directory

### Runtime Issues

**QEMU won't start**
- Check QEMU is installed: `qemu-system-i386 --version`
- Ensure the OS image was built: `ls -la build/atomicos.img`

**OS crashes immediately**
- Run with debugging: `make debug`
- Check the kernel was built correctly
- Verify all dependencies are installed

### Common Problems

**Can't build on Apple Silicon (M1/M2)**
- Use Rosetta 2 or an x86 emulation layer
- Consider using a Linux VM

**Windows-specific issues**
- Use WSL2 for best compatibility
- Native Windows builds are not supported

## Verifying Installation

Run the test suite to ensure everything is working:

```bash
make test
```

All tests should pass. If any fail, check:
1. All dependencies are correctly installed
2. You have the latest version of the code
3. Your system meets the requirements

## Next Steps

- Read the [Quick Start Guide](quick-start.md)
- Explore [example Tempo programs](../../examples/)
- Learn about the [architecture](../architecture/overview.md)
- Start [developing](../development/dev-guide.md)

## Getting Help

If you encounter issues:
1. Check this guide again
2. Search existing [GitHub issues](https://github.com/ipenas-cl/AtomicOS/issues)
3. Ask in [discussions](https://github.com/ipenas-cl/AtomicOS/discussions)
4. Open a new issue with details

---
*Security First, Stability Second, Performance Third*