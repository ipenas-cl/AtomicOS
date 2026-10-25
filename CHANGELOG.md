# Changelog

Las notas de las versiones Tempo (hasta v5.2.2) están en el historial de git,
en el commit `355f4ec7438239f0062988512a5c9f585da8cc57`. La etiqueta propuesta
para ese commit es `legacy-tempo`.

## 0.8

Kernel en Copihue v0.23.0 y sector de arranque `src/bootloader/boot64.asm`.
La imagen la construyen `make all` y la comprueba `make qemu-test`.
El árbol de trabajo ya no incluye el lenguaje Tempo ni sus salidas generadas.
