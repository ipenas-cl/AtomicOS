# Changelog

Lapageria rosea es el nombre científico del copihue; antes AtomicOS.

Las notas de las versiones Tempo (hasta v5.2.2) están en el historial de git,
en el commit `355f4ec7438239f0062988512a5c9f585da8cc57`. La etiqueta propuesta
para ese commit es `legacy-tempo`.

## 0.8

Kernel en Copihue v0.27.1. El planificador guarda el marco del shell
con `save_context` y vuelve con `restore_context`. El PIT y el `#PF`
leen el marco de la CPU por un puntero. El directorio, el superbloque,
el metadato del bloque y la cabecera AXE1 son structs; el disco no
cambia. No queda `asm()` en el kernel. El arranque es `src/bootloader/boot.lap`
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

`exec` carga un ejecutable plano de 16 bytes de cabecera (`AXE1`,
tamaño, entrada) desde ese disco a un PML4 nuevo (`docs/exec.md`).
`scripts/mkaxe.sh` arma la imagen; `exec plant` la escribe con
`fs_write`. Un magic malo, un tamaño de más o un bloque con el CRC
roto no arrancan el proceso.

`net` habla con el e1000 de QEMU (82540EM) por PCI: ARP de `10.0.2.15`
contra la puerta `10.0.2.2`, IPv4 con checksum e ICMP echo. No hay
TCP ni UDP, y el chip no queda prendido en el arranque. El layout y
la prueba están en `docs/red.md`. Una trama cuyo largo IP no cabe en
el buffer se descarta.

`trace` deja once sondas estáticas y un anillo fijo de 32 eventos
(`docs/trace.md`). Apagada, la sonda es un chequeo. `trace dump`
muestra los últimos eventos; si el anillo se llena, pisa lo viejo y
cuenta los perdidos. No se agregó `asm()`.
