# Legacy kernel sources

The current boot image is built only from:

- `../bootloader/boot.lap` (stage 1, `copihue --boot`) and `../bootloader/stage2.lap` (stage 2, `copihue --flat 0x8200`)
- `copihue/*.lap` (compiled with the pinned Copihue compiler, `copihue --flat`)

Everything else in this directory (`*.asm`, `*.inc`, `*.tempo`, `link.ld`) is
older work kept for reference. It is not assembled or linked into the image.
