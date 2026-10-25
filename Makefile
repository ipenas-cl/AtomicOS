# AtomicOS — kernel en Copihue, sector de arranque en NASM.
#
# Targets:
#   all        Construye build/atomicos.img (sector de arranque + kernel plano).
#   qemu       Arranca esa imagen en QEMU con consola curses (teclado PS/2).
#   qemu-test  Arranque sin pantalla: sendkey y comprobaciones por COM1.
#   debug      Igual que qemu, y espera a gdb en tcp::1234 (-s -S).
#   clean      Borra build/.
#   help       Muestra esta lista.
#
# La primera vez `all` clona y compila Copihue en build/copihue.
# COPIHUE_COMMIT fija la etiqueta. COPIHUE=/ruta/copihue usa un binario local
# y no clona el repositorio.

PROJECT_NAME = AtomicOS
VERSION = 0.8
ARCH = x86_64

SRC_DIR = src
BUILD_DIR = build
TESTS_DIR = tests

KERNEL_SRC = $(SRC_DIR)/kernel
BOOT_SRC = $(SRC_DIR)/bootloader

AS = nasm
ASFLAGS = -f bin

BOOTLOADER = $(BUILD_DIR)/boot.bin
KERNEL = $(BUILD_DIR)/kernel.bin
OS_IMAGE = $(BUILD_DIR)/atomicos.img

# boot64.asm lee 64 sectores (32 KiB) desde LBA 1.
KERNEL_MAX_BYTES = 32768

COPIHUE_REPO   ?= https://github.com/ipenas-cl/copihue.git
COPIHUE_COMMIT ?= v0.23.0
COPIHUE_DIR     = $(BUILD_DIR)/copihue
COPIHUE       ?= $(COPIHUE_DIR)/copihue

# Orden de concatenación. `copihue --flat` coloca fn _start al inicio del binario.
COPIHUE_KERNEL_SRC = $(KERNEL_SRC)/copihue/console.lap \
                     $(KERNEL_SRC)/copihue/keyboard.lap \
                     $(KERNEL_SRC)/copihue/pmm.lap \
                     $(KERNEL_SRC)/copihue/paging.lap \
                     $(KERNEL_SRC)/copihue/gdt.lap \
                     $(KERNEL_SRC)/copihue/usermode.lap \
                     $(KERNEL_SRC)/copihue/idt.lap \
                     $(KERNEL_SRC)/copihue/shell.lap \
                     $(KERNEL_SRC)/copihue/kernel.lap

.PHONY: all qemu qemu-test debug clean help

# Imagen de arranque: sector 0 = boot.bin, el kernel empieza en LBA 1.
all: $(OS_IMAGE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(COPIHUE_DIR)/copihue: | $(BUILD_DIR)
	@echo "Building Copihue compiler @ $(COPIHUE_COMMIT)..."
	rm -rf $(COPIHUE_DIR)
	git init -q $(COPIHUE_DIR)
	git -C $(COPIHUE_DIR) fetch -q --depth 1 $(COPIHUE_REPO) $(COPIHUE_COMMIT)
	git -C $(COPIHUE_DIR) checkout -q FETCH_HEAD
	$(MAKE) -C $(COPIHUE_DIR) copihue

$(BOOTLOADER): $(BOOT_SRC)/boot64.asm | $(BUILD_DIR)
	@echo "Assembling 64-bit bootloader..."
	$(AS) $(ASFLAGS) $< -o $@

# kernel_all.lap se genera en build/ y no se versiona.
$(KERNEL): $(COPIHUE_KERNEL_SRC) $(COPIHUE) | $(BUILD_DIR)
	@echo "Compiling kernel with Copihue..."
	cat $(COPIHUE_KERNEL_SRC) > $(BUILD_DIR)/kernel_all.lap
	cd $(BUILD_DIR) && $(abspath $(COPIHUE)) --flat 0x10000 -o kernel.bin kernel_all.lap
	@size=$$(stat -c %s $@); echo "kernel.bin: $$size bytes (max $(KERNEL_MAX_BYTES))"; \
	  test $$size -le $(KERNEL_MAX_BYTES) || { echo "kernel too large for bootloader"; rm -f $@; exit 1; }

$(OS_IMAGE): $(BOOTLOADER) $(KERNEL) | $(BUILD_DIR)
	@echo "Creating OS image..."
	dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	dd if=$(BOOTLOADER) of=$@ conv=notrunc status=none
	dd if=$(KERNEL) of=$@ bs=512 seek=1 conv=notrunc status=none
	@echo "AtomicOS v$(VERSION) image created: $@"

# Consola curses: QEMU entrega el teclado al controlador PS/2 que lee el shell.
qemu: $(OS_IMAGE)
	qemu-system-$(ARCH) -drive format=raw,file=$(OS_IMAGE) -display curses

# tests/qemu_shell_test.sh exige qemu-system-x86_64 y socat.
qemu-test: $(OS_IMAGE)
	$(TESTS_DIR)/qemu_shell_test.sh $(OS_IMAGE)

# La imagen es un binario plano: gdb no tiene tabla de símbolos ELF.
debug: $(OS_IMAGE)
	qemu-system-$(ARCH) -drive format=raw,file=$(OS_IMAGE) -display curses -s -S

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "AtomicOS $(VERSION) — kernel Copihue, arranque NASM"
	@echo ""
	@echo "  make all        Imagen de arranque en build/atomicos.img"
	@echo "                  (nasm + copihue --flat; la primera vez compila Copihue $(COPIHUE_COMMIT))."
	@echo "  make qemu       Arranca la imagen en QEMU (curses, teclado PS/2)."
	@echo "  make qemu-test  Arranque mudo: sendkey y comprobaciones por COM1."
	@echo "  make debug      Igual que qemu, esperando gdb en tcp::1234."
	@echo "  make clean      Borra build/."
	@echo "  make help       Esta lista."
	@echo ""
	@echo "Compilador local: make COPIHUE=/ruta/al/copihue all"
