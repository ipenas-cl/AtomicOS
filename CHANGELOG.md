# Changelog

Las notas de las versiones Tempo (hasta v5.2.2) están en el historial de git,
en el commit `355f4ec7438239f0062988512a5c9f585da8cc57`. La etiqueta propuesta
para ese commit es `legacy-tempo`.

## 0.8

Kernel en Copihue v0.26.1. El arranque es `src/bootloader/boot.lap`
(512 bytes) y `src/bootloader/stage2.lap` (en `0x8200`). La etapa 2 pide
los sectores que ocupa `kernel.bin`, de a 128 por INT 13h. El archivo
puede pasar de `0x20000`; imagen y `.bss` tienen que terminar antes de
`0x58000` (la pila de `0x60000` crece hacia abajo). `make` rechaza un
kernel de más de 512 sectores. Cada proceso tiene su PML4 y el PIT
hace round-robin. La imagen la construyen `make all` y la comprueba
`make qemu-test`. El árbol de trabajo ya no incluye el lenguaje Tempo
ni sus salidas generadas.

El esclavo IDE (`build/data.img`, QEMU index=1) tiene driver ATA PIO
LBA28 y bloques de 512 B con CRC-32 en un sector de metadatos. `disk`
escribe un patrón, lo relee y comprueba que un byte pisado con el
driver (sin pasar por `blk`) hace fallar la lectura. El layout está en
`docs/disco.md`.

Encima de esos bloques hay un sistema de archivos copy-on-write, con
directorio plano, snapshots y rollback (`docs/fs.md`). `make qemu-test`
formatea, escribe, saca un snapshot, reabre el mismo `data.img` y
comprueba que un bloque pisado no se entrega como dato.
