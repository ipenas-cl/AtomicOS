# Changelog

Lapageria rosea es el nombre científico del copihue; antes AtomicOS.

Las notas de las versiones Tempo (hasta v5.2.2) están en el historial de git,
en el commit `355f4ec7438239f0062988512a5c9f585da8cc57`. La etiqueta propuesta
para ese commit es `legacy-tempo`.

## 0.8

Kernel en Copihue v0.27.3 (commit
`333f74ae8c82fd679232d754920e335e012483c7`). Esta imagen junta la red
e1000, el nombre LapageriaOS, las trazas y las zonas ligeras sobre el
kernel sin `asm()`. El planificador guarda el marco del shell con
`save_context` y vuelve con `restore_context`. El PIT y el `#PF` leen
el marco de la CPU por un puntero. El directorio, el superbloque, el
metadato del bloque y la cabecera AXE1 son structs; el disco no
cambia. `EthHdr`, `ArpPkt`, `IpPkt`, `IcmpEcho`, `TrEv` y `ZoneStr`
también: `net_layout`, `trace_layout` y `zone_layout` comprueban
`sizeof` y `offsetof`. Los campos de más de un byte de la trama van
en dos `u8`, en orden de red, así que la trama no cambia. `fs_remove`
usa `FsEnt` y `FsSlot`; el slot del directorio sigue midiendo 64.
No queda `asm()` en el kernel ni en el arranque. `clear_df` es
`cld()` antes de `lodsb` y de `rep_stosd`. `pushfd` / `popfd` queda
en `check_long_mode` (bit ID) y en `trace_save_if` (en 64 bits emite
`pushfq`). `get_e820` copia el token de INT 15h a memoria: un `while`
y un `if (ebx == 0)` pisan `ebx`.

El arranque es `src/bootloader/boot.lap` (512 bytes) y
`src/bootloader/stage2.lap` (en `0x8200`). La etapa 2 pide los
sectores que ocupa `kernel.bin`, de a 128 por INT 13h. En esta
compilación `kernel.bin` mide 157143 bytes: 307 sectores, LBA 9..315,
paquete `10 00 33 01 00 00 00 10 09 00 00 00 00 00 00 00`. La `.bss`
mide 22060 bytes y `0x10000 + kernel.bin + .bss` termina en `0x3BC03`.
El archivo puede pasar de `0x20000`; imagen y `.bss` tienen que
terminar antes de `0x58000` (la pila de `0x60000` crece hacia abajo).
`make` rechaza un kernel de más de 512 sectores. El disquete tiene
2880 sectores y después del LBA 8 quedan 2871, así que el tope que
aprieta es esa pila, no los 1.44 MB. 512 sectores caben: el kernel
máximo terminaría en el LBA 520 y sobrarían 2359. Cada proceso tiene
su PML4 y el PIT hace round-robin. La imagen la construyen `make all`
y la comprueba `make qemu-test`. El árbol de trabajo ya no incluye el
lenguaje Tempo ni sus salidas generadas.

En `--flat`, un `while` y un store de constante siguen dejando `rax`
en 0 o en 1. `src/user/zona.lap` guarda el resultado de la syscall en
`0x401720` antes de compararlo. El ensamblado exacto está en
`docs/PEDIDOS_COPIHUE.md`.

En QEMU 8.2.2, `make qemu-test` de esta imagen terminó en 0: PIT a
~100 Hz, `#PF`, anillo 3, planificador, disco, sistema de archivos,
`exec`, zonas, ARP/ICMP del e1000, trazas (contadores, syscalls,
sonda apagada y anillo lleno) y un segundo arranque con el mismo
`data.img`. El humo de 10 s, sin disco de datos, salió 124. No se
probó `reboot`, `clear`, el teclado a mano, TCP, UDP, AHCI, NVMe, el
bit NX ni un kernel en la mitad alta.

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

Hay zonas ligeras (`docs/zonas.md`): la zona 0 es la global y una
zona nueva niega syscalls, nombres fuera de su prefijo y páginas o
procesos sobre su tope. `zone exec` mete un programa del disco en esa
zona. Al salir, los marcos vuelven al asignador.

`net` habla con el e1000 de QEMU (82540EM) por PCI: ARP de `10.0.2.15`
contra la puerta `10.0.2.2`, IPv4 con checksum e ICMP echo. No hay
TCP ni UDP, y el chip no queda prendido en el arranque. El layout y
la prueba están en `docs/red.md`. Una trama cuyo largo IP no cabe en
el buffer se descarta.

`trace` deja once sondas estáticas y un anillo fijo de 32 eventos
(`docs/trace.md`). Apagada, la sonda es un chequeo. `trace dump`
muestra los últimos eventos; si el anillo se llena, pisa lo viejo y
cuenta los perdidos. No se agregó `asm()`.
