# Legacy kernel sources

The current boot image is built only from:

- `../bootloader/boot64.asm`
- `copihue/*.cph` (compiled with the pinned Copihue compiler, `copihue --flat`)

Everything else in this directory (`*.asm`, `*.inc`, `*.tempo`, `link.ld`) is
older work kept for reference. It is not assembled or linked into the image.
