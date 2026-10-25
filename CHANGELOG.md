# Changelog

Las notas de las versiones Tempo (hasta v5.2.2) están en el historial de git,
en el commit `355f4ec7438239f0062988512a5c9f585da8cc57`. La etiqueta propuesta
para ese commit es `legacy-tempo`.

## 0.8

Kernel en Copihue v0.26.1. El arranque es `src/bootloader/boot.lap`
(512 bytes) y `src/bootloader/stage2.lap` (en `0x8200`). La etapa 2 pide
los sectores que ocupa `kernel.bin`. Cada proceso tiene su PML4 y el PIT
hace round-robin. La imagen la construyen `make all` y la comprueba
`make qemu-test`. El árbol de trabajo ya no incluye el lenguaje Tempo
ni sus salidas generadas.

El esclavo IDE (`build/data.img`, QEMU index=1) tiene driver ATA PIO
LBA28 y bloques de 512 B con CRC-32 en un sector de metadatos. `disk`
escribe un patrón, lo relee y comprueba que un byte pisado con el
driver (sin pasar por `blk`) hace fallar la lectura. El layout está en
`docs/disco.md`.
