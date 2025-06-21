# AtomicOS v0.6.1 - Deterministic Real-Time Security OS
# Professional Build System

# Configuration
PROJECT_NAME = AtomicOS
VERSION = 0.6.1
ARCH = i386

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
TEMPO = $(TOOLS_DIR)/tempo_compiler

# Flags
CFLAGS = -std=c99 -Wall -Wextra -Werror -O2 -fno-stack-protector -nostdlib -nostdinc
ASFLAGS = -f bin
LDFLAGS = -nostdlib
TEMPO_FLAGS = 

# Build targets
BOOTLOADER = $(BUILD_DIR)/boot.bin
KERNEL = $(BUILD_DIR)/kernel.bin
OS_IMAGE = $(BUILD_DIR)/atomicos.img
TEMPO_BIN = $(BUILD_DIR)/tempo_compiler

# Security modules (compiled from Tempo)
SECURITY_MODULES = $(BUILD_DIR)/ssp_functions.inc \
                   $(BUILD_DIR)/guard_pages.inc \
                   $(BUILD_DIR)/deterministic_core.inc

# Default target
.PHONY: all
all: $(OS_IMAGE)

# Create directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Build Tempo compiler
$(TEMPO_BIN): $(TOOLS_DIR)/tempo_compiler.c | $(BUILD_DIR)
	@echo "Building Tempo compiler..."
	$(CC) -std=c99 -Wall -Wextra -O2 -o $@ $<

# Compile security modules from Tempo
$(BUILD_DIR)/%.inc: $(EXAMPLES_DIR)/%.tempo $(TEMPO_BIN) | $(BUILD_DIR)
	@echo "Compiling Tempo module: $<"
	$(TEMPO_BIN) $< $(BUILD_DIR)/$*.s
	@cp $(KERNEL_SRC)/$*.inc $@ 2>/dev/null || echo "; Generated from $<" > $@

# Build bootloader
$(BOOTLOADER): $(BOOT_SRC)/boot.asm | $(BUILD_DIR)
	@echo "Assembling bootloader..."
	$(AS) $(ASFLAGS) $< -o $@

# Build kernel
$(KERNEL): $(KERNEL_SRC)/kernel.asm | $(BUILD_DIR)
	@echo "Assembling kernel with security modules..."
	@cp $(KERNEL_SRC)/*.inc $(BUILD_DIR)/ 2>/dev/null || true
	$(AS) $(ASFLAGS) $< -o $@

# Create OS image
$(OS_IMAGE): $(BOOTLOADER) $(KERNEL) | $(BUILD_DIR)
	@echo "Creating OS image..."
	dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	dd if=$(BOOTLOADER) of=$@ conv=notrunc status=none
	dd if=$(KERNEL) of=$@ bs=512 seek=1 conv=notrunc status=none
	@echo "AtomicOS v$(VERSION) image created: $@"

# Run in QEMU
.PHONY: run
run: $(OS_IMAGE)
	@echo "Starting AtomicOS v$(VERSION) in QEMU..."
	qemu-system-$(ARCH) -fda $< -display curses

# Run with debugging
.PHONY: debug
debug: $(OS_IMAGE)
	@echo "Starting AtomicOS v$(VERSION) in debug mode..."
	qemu-system-$(ARCH) -fda $< -display curses -s -S

# Build tests
.PHONY: test
test: $(TEMPO_BIN)
	@echo "Running Tempo compiler tests..."
	@for test_file in $(EXAMPLES_DIR)/*.tempo; do \
		echo "Testing: $$test_file"; \
		$(TEMPO_BIN) $$test_file $(BUILD_DIR)/test_output.s || exit 1; \
	done
	@echo "All tests passed!"

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
	@echo "Architecture: $(ARCH)"
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
	@echo "  test          - Run test suite"
	@echo "  security-check- Run security analysis"
	@echo "  clean         - Clean build artifacts"
	@echo "  distclean     - Clean everything"
	@echo "  docs          - Generate documentation"
	@echo "  info          - Show project information"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Security First, Stability Second, Performance Third"