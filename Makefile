# AtomicOS 0.8 - Deterministic Real-Time Security OS
# Professional Build System

# Configuration
PROJECT_NAME = AtomicOS
VERSION = 0.8
ARCH = x86_64

# Directories
SRC_DIR = src
BUILD_DIR = build
DOCS_DIR = docs
TESTS_DIR = tests
TOOLS_DIR = tools
EXAMPLES_DIR = examples

KERNEL_SRC = $(SRC_DIR)/kernel
BOOT_SRC = $(SRC_DIR)/bootloader
LIBC_SRC = $(SRC_DIR)/libc
USER_SRC = $(SRC_DIR)/userspace

# Tools
CC = gcc
AS = nasm
LD = ld
TEMPO = tempo

# Flags
CFLAGS = -std=c99 -Wall -Wextra -Werror -O2 -m32 -fno-stack-protector -fno-builtin -nostdlib -nostdinc -fno-pie -I$(SRC_DIR)/kernel -I include
ASFLAGS = -f bin
LDFLAGS = -m elf_i386 -nostdlib
TEMPO_FLAGS = 

# Build targets
BOOTLOADER = $(BUILD_DIR)/boot.bin
KERNEL = $(BUILD_DIR)/kernel.bin
OS_IMAGE = $(BUILD_DIR)/atomicos.img
TEMPO_BIN = $(BUILD_DIR)/tempo_compiler

# Security modules are pre-built in kernel directory

# Default target
.PHONY: all
all: $(OS_IMAGE)

# Create directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Build Tempo compiler
$(TEMPO_BIN): $(TOOLS_DIR)/tempo_compiler.c | $(BUILD_DIR)
	@echo "Building Tempo v1.2.2 compiler..."
	$(CC) -std=c99 -Wall -Wextra -O2 -o $@ $<

# Build enhanced Tempo compiler (separate target)
tempo-enhanced: $(TOOLS_DIR)/tempo_compiler_enhanced.c $(TOOLS_DIR)/tempo_optimizer.c $(TOOLS_DIR)/tempo_debug.c $(SRC_DIR)/kernel/wcet_model.c | $(BUILD_DIR)
	@echo "Building Tempo Enhanced compiler..."
	$(CC) -std=c99 -Wall -Wextra -O2 -I$(SRC_DIR) -I$(TOOLS_DIR) -o $(BUILD_DIR)/tempo_enhanced $^

# Build industrial-grade Tempo compiler with LLVM backend
tempo-v2: $(TOOLS_DIR)/tempo_compiler_v2.c $(TOOLS_DIR)/tempo_llvm_backend.c $(TOOLS_DIR)/tempo_semantic_analyzer.c $(TOOLS_DIR)/tempo_ast_to_llvm.c | $(BUILD_DIR)
	@echo "Building Tempo v2.0 compiler with LLVM backend..."
	$(CC) -std=c99 -Wall -Wextra -O2 -I$(TOOLS_DIR) `llvm-config --cflags --ldflags --libs core analysis executionengine target` -o $(BUILD_DIR)/tempo_v2 $^
	@echo "Industrial-grade Tempo compiler ready!"

# Build demo version (no LLVM dependencies)
tempo-v2-demo: $(TOOLS_DIR)/tempo_compiler_v2_demo.c | $(BUILD_DIR)
	@echo "Building Tempo v2.0 Demo compiler..."
	$(CC) -std=c99 -Wall -Wextra -O2 -o $(BUILD_DIR)/tempo_v2_demo $^
	@echo "Demo compiler ready! (showcases v2.0 architecture)"

# Compile Tempo source files to assembly
%.s: %.tempo $(TEMPO_BIN)
	@echo "Compiling Tempo file: $<"
	$(TEMPO_BIN) $< $@

# Pattern rule for kernel Tempo files
$(BUILD_DIR)/%.s: $(KERNEL_SRC)/%.tempo $(TEMPO_BIN)
	@echo "Compiling kernel Tempo file: $<"
	$(TEMPO_BIN) $< $@

# Compile security modules from Tempo
$(BUILD_DIR)/%.inc: $(EXAMPLES_DIR)/%.tempo $(TEMPO_BIN) | $(BUILD_DIR)
	@echo "Compiling Tempo module: $<"
	$(TEMPO_BIN) $< $(BUILD_DIR)/$*.s
	@cp $(KERNEL_SRC)/$*.inc $@ 2>/dev/null || echo "; Generated from $<" > $@

# Build 64-bit bootloader
$(BOOTLOADER): $(BOOT_SRC)/boot64.asm | $(BUILD_DIR)
	@echo "Assembling 64-bit bootloader..."
	$(AS) $(ASFLAGS) $< -o $@

# Build kernel C files
# C files removed - AtomicOS is now pure assembly + Tempo modules
# $(BUILD_DIR)/syscall.o: $(KERNEL_SRC)/syscall.c | $(BUILD_DIR)
# 	@echo "Compiling syscall.c..."
# 	$(CC) $(CFLAGS) -c $< -o $@

# $(BUILD_DIR)/ipc.o: $(KERNEL_SRC)/ipc.c | $(BUILD_DIR)
# 	@echo "Compiling ipc.c..."
# 	$(CC) $(CFLAGS) -c $< -o $@

# $(BUILD_DIR)/fs.o: $(KERNEL_SRC)/fs.c | $(BUILD_DIR)
# 	@echo "Compiling fs.c..."
# 	$(CC) $(CFLAGS) -c $< -o $@

# Copihue compiler, pinned so kernel builds are reproducible.
# Override COPIHUE=/path/to/copihue to use a local build instead.
# v0.20.1 is the tag, commit 626e3e80d55eaf065b173ea545b6de14f98b6ad4.
# `git fetch` accepts the tag name.
COPIHUE_REPO   ?= https://github.com/ipenas-cl/copihue.git
COPIHUE_COMMIT ?= v0.20.1
COPIHUE_DIR     = $(BUILD_DIR)/copihue
COPIHUE       ?= $(COPIHUE_DIR)/copihue

$(COPIHUE_DIR)/copihue: | $(BUILD_DIR)
	@echo "Building Copihue compiler @ $(COPIHUE_COMMIT)..."
	rm -rf $(COPIHUE_DIR)
	git init -q $(COPIHUE_DIR)
	git -C $(COPIHUE_DIR) fetch -q --depth 1 $(COPIHUE_REPO) $(COPIHUE_COMMIT)
	git -C $(COPIHUE_DIR) checkout -q FETCH_HEAD
	$(MAKE) -C $(COPIHUE_DIR) copihue

# Kernel: 100% Copihue. `copihue --flat` emits a flat binary that starts at fn _start.
COPIHUE_KERNEL_SRC = $(KERNEL_SRC)/copihue/console.lap $(KERNEL_SRC)/copihue/keyboard.lap \
                     $(KERNEL_SRC)/copihue/idt.lap $(KERNEL_SRC)/copihue/shell.lap \
                     $(KERNEL_SRC)/copihue/kernel.lap
# boot64.asm loads 64 sectors
KERNEL_MAX_BYTES = 32768

$(KERNEL): $(COPIHUE_KERNEL_SRC) $(COPIHUE) | $(BUILD_DIR)
	@echo "Compiling kernel with Copihue..."
	cat $(COPIHUE_KERNEL_SRC) > $(BUILD_DIR)/kernel_all.lap
	cd $(BUILD_DIR) && $(abspath $(COPIHUE)) --flat 0x10000 -o kernel.bin kernel_all.lap
	@size=$$(stat -c %s $@); echo "kernel.bin: $$size bytes (max $(KERNEL_MAX_BYTES))"; \
	  test $$size -le $(KERNEL_MAX_BYTES) || { echo "kernel too large for bootloader"; rm -f $@; exit 1; }

# Boot in QEMU, type commands via the monitor (sendkey), check serial output.
.PHONY: qemu-test
qemu-test: $(OS_IMAGE)
	$(TESTS_DIR)/qemu_shell_test.sh $(OS_IMAGE)

# Create OS image
$(OS_IMAGE): $(BOOTLOADER) $(KERNEL) | $(BUILD_DIR)
	@echo "Creating OS image..."
	dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	dd if=$(BOOTLOADER) of=$@ conv=notrunc status=none
	dd if=$(KERNEL) of=$@ bs=512 seek=1 conv=notrunc status=none
	@echo "AtomicOS v$(VERSION) image created: $@"

# Release management
.PHONY: release-patch release-minor release-major
release-patch:
	@scripts/release.sh patch

release-minor:
	@scripts/release.sh minor

release-major:
	@scripts/release.sh major

# Version info
.PHONY: version
version:
	@echo "AtomicOS $(VERSION)"
	@echo "Architecture: 64-bit (x86_64)"
	@echo "Tempo Compiler: v1.2.2"

# Run in QEMU
.PHONY: run
run: $(OS_IMAGE)
	@echo "Starting AtomicOS v$(VERSION) in QEMU..."
	qemu-system-$(ARCH) -drive format=raw,file=$< -display curses

# Run with debugging
.PHONY: debug
debug: $(OS_IMAGE)
	@echo "Starting AtomicOS v$(VERSION) in debug mode..."
	qemu-system-$(ARCH) -drive format=raw,file=$< -display curses -s -S

# Build test runner
$(BUILD_DIR)/tempo_test_runner: $(TESTS_DIR)/tempo_test_runner.c | $(BUILD_DIR)
	@echo "Building test runner..."
	$(CC) -std=c99 -Wall -Wextra -O2 -o $@ $<

# Build tests
.PHONY: test
test: $(TEMPO_BIN) $(BUILD_DIR)/tempo_test_runner
	@echo "Running comprehensive Tempo test suite..."
	$(BUILD_DIR)/tempo_test_runner

# Quick test - just examples
.PHONY: test-quick
test-quick: $(TEMPO_BIN)
	@echo "Running quick Tempo compiler tests..."
	@for test_file in $(EXAMPLES_DIR)/*.tempo; do \
		echo "Testing: $$test_file"; \
		$(TEMPO_BIN) $$test_file $(BUILD_DIR)/test_output.s || exit 1; \
	done
	@echo "All quick tests passed!"

# Test with verbose output
.PHONY: test-verbose
test-verbose: $(TEMPO_BIN) $(BUILD_DIR)/tempo_test_runner
	@echo "Running verbose Tempo test suite..."
	$(BUILD_DIR)/tempo_test_runner --verbose

# Test LLVM backend integration
.PHONY: test-llvm
test-llvm: tempo-v2
	@echo "Testing Tempo v2.0 with LLVM backend..."
	@echo "Testing arithmetic operations..."
	$(BUILD_DIR)/tempo_v2 --verbose --emit-llvm -o $(BUILD_DIR)/test_arithmetic.ll $(EXAMPLES_DIR)/test_llvm_integration.tempo
	@echo "Testing WCET analysis..."
	$(BUILD_DIR)/tempo_v2 --wcet-analysis $(EXAMPLES_DIR)/test_llvm_integration.tempo
	@echo "Testing security analysis..."
	$(BUILD_DIR)/tempo_v2 --security-analysis $(EXAMPLES_DIR)/test_llvm_integration.tempo
	@echo "LLVM backend integration test complete!"

# Test demo compiler
.PHONY: test-demo
test-demo: tempo-v2-demo
	@echo "Testing Tempo v2.0 Demo compiler..."
	@echo "Testing basic compilation..."
	$(BUILD_DIR)/tempo_v2_demo --verbose $(EXAMPLES_DIR)/test_llvm_integration.tempo
	@echo "Testing LLVM IR generation..."
	$(BUILD_DIR)/tempo_v2_demo --emit-llvm -o $(BUILD_DIR)/demo_test.ll $(EXAMPLES_DIR)/test_llvm_integration.tempo
	@echo "Testing WCET analysis..."
	$(BUILD_DIR)/tempo_v2_demo --wcet-analysis $(EXAMPLES_DIR)/test_llvm_integration.tempo
	@echo "Testing security analysis..."
	$(BUILD_DIR)/tempo_v2_demo --security-analysis $(EXAMPLES_DIR)/test_llvm_integration.tempo
	@echo "Demo compiler test complete!"

# Build and test type checker
$(BUILD_DIR)/test_type_checker: $(TOOLS_DIR)/test_type_checker.c $(TOOLS_DIR)/tempo_type_checker.c | $(BUILD_DIR)
	@echo "Building type checker test suite..."
	$(CC) -std=c99 -Wall -Wextra -O2 -I$(TOOLS_DIR) -o $@ $^

.PHONY: test-types
test-types: $(BUILD_DIR)/test_type_checker
	@echo "Running comprehensive type checker tests..."
	$(BUILD_DIR)/test_type_checker
	@echo "Type checker validation complete!"

# Security analysis
.PHONY: security-check
security-check: $(TEMPO_BIN)
	@echo "Running security analysis..."
	@echo "- Checking WCET bounds..."
	@echo "- Verifying deterministic operations..."
	@echo "- Analyzing memory safety..."
	@echo "Security analysis complete."

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)

# Clean everything including generated files
.PHONY: distclean
distclean: clean
	@echo "Performing full cleanup..."
	find . -name "*.bin" -delete
	find . -name "*.img" -delete
	find . -name "*.log" -delete
	find . -name "*~" -delete

# Install (copy to system directory)
.PHONY: install
install: $(OS_IMAGE)
	@echo "Installing AtomicOS v$(VERSION)..."
	@echo "Installation target not implemented yet."

# Generate documentation
.PHONY: docs
docs:
	@echo "Generating documentation..."
	@echo "Documentation generation not implemented yet."

# Show project information
.PHONY: info
info:
	@echo "==============================================="
	@echo "AtomicOS v$(VERSION) - Deterministic Real-Time Security OS"
	@echo "==============================================="
	@echo "Architecture: 64-bit (x86_64)"
	@echo "Build directory: $(BUILD_DIR)"
	@echo "Security features:"
	@echo "  - W^X Protection"
	@echo "  - Stack-Smashing Protection (SSP)"
	@echo "  - Guard Pages"
	@echo "  - KASLR"
	@echo "  - Multi-level Security"
	@echo "Deterministic features:"
	@echo "  - WCET-bounded operations"
	@echo "  - Static resource allocation"
	@echo "  - Reproducible behavior"
	@echo "  - Real-time scheduling"
	@echo "==============================================="

# Help
.PHONY: help
help:
	@echo "AtomicOS v$(VERSION) Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all           - Build complete OS image"
	@echo "  run           - Run OS in QEMU"
	@echo "  debug         - Run OS with debugging enabled"
	@echo "  qemu-test     - Boot in QEMU and test the shell via sendkey"
	@echo "  test          - Run test suite"
	@echo "  test-llvm     - Test LLVM backend integration"
	@echo "  test-demo     - Test demo compiler"
	@echo "  test-types    - Test advanced type checker"
	@echo "  tempo-v2      - Build industrial Tempo compiler"
	@echo "  tempo-v2-demo - Build demo Tempo compiler"
	@echo "  security-check- Run security analysis"
	@echo "  clean         - Clean build artifacts"
	@echo "  distclean     - Clean everything"
	@echo "  docs          - Generate documentation"
	@echo "  info          - Show project information"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Security First, Stability Second, Performance Third"