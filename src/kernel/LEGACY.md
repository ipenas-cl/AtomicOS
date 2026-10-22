# Legacy kernel sources

The current boot image is built only from:

- `../bootloader/boot64.asm`
- `entry64.asm`
- `copihue/*.cph` (compiled with the pinned Copihue compiler)

Everything else in this directory (`*.asm`, `*.inc`, `*.tempo`, `link.ld`) is
older work kept for reference. It is not assembled or linked into the image.
