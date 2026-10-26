# Changelog

Lapageria rosea es el nombre científico del copihue; antes AtomicOS.

Las notas de las versiones Tempo (hasta v5.2.2) están en el historial de git,
en el commit `355f4ec7438239f0062988512a5c9f585da8cc57`. La etiqueta propuesta
para ese commit es `legacy-tempo`.

## 0.8

Bit NX y W^X. Con `-cpu qemu64`, CPUID `0x80000001` EDX bit 20 está
puesto y el kernel prende EFER.NXE (MSR `0xC0000080`, bit 11) y
CR0.WP. La página de 2 MiB de PD[0] pasa a 512 páginas de 4 KiB:
`.text` (`0x10000`..`0x33000`, 35 páginas) queda ejecutable y sin
escritura; datos, `.bss`, pilas, heap y el MMIO llevan el bit 63.
`.data` y `.bss` comparten la página `0x34000`. El comando `nx` en
QEMU 8.2.2 imprimió fetch `err=0x00000011` `cr2=0x00033000`,
escritura `err=0x00000003` `cr2=0x00010000`, usuario
`err=0x00000015` en `0x401000` y `0x500000` con `nx ok sched` en
medio, y `nx ok audit wx=0 leaves=513 code=35 nx=478 huge=0`.
`make qemu-test` terminó en 0 e imprimió `ok   nx fetch=0x00000011
write=0x00000003 user=0x00000015 audit wx=0 leaves=513 code=35
nx=478`. El humo de 10 s con `-cpu qemu64` salió 124 y mostró
`nx efer on` y `nx wp on`. No hay `asm()` nuevo. Copihue ya tenía
`wrmsr`, `cpuid` y el desplazamiento de 63 bits. El detalle está en
`docs/nx.md`.

Kernel en Copihue v0.29.4 (commit
`cac6fc27453c0136711e5de4ead703f836eff987`). `git ls-remote` devuelve
ese commit para el tag y para `HEAD`. `git -C build/copihue rev-parse
HEAD` después del `make` también, y `copihue --version` imprime
`Copihue 0.29.4`. Esta imagen junta la red e1000, el nombre
LapageriaOS, las trazas y las zonas ligeras sobre el kernel sin
`asm()`. El planificador guarda el marco del shell con `save_context`
y vuelve con `restore_context`. El PIT y el `#PF` leen el marco de la
CPU por un puntero. El directorio, el superbloque, el metadato del
bloque y la cabecera AXE1 son structs; el disco no cambia. `EthHdr`,
`ArpPkt`, `IpPkt`, `IcmpEcho`, `TrEv` y `ZoneStr` también:
`net_layout`, `trace_layout` y `zone_layout` comprueban `sizeof` y
`offsetof`. Los campos de más de un byte de la trama van en dos
`u8`, en orden de red, así que la trama no cambia. `fs_remove` usa
`FsEnt` y `FsSlot`; el slot del directorio sigue midiendo 64. No
queda `asm()` en el kernel ni en el arranque. `clear_df` es `cld()`
antes de `lodsb` y de `rep_stosd`. `pushfd` / `popfd` queda en
`check_long_mode` (bit ID) y en `trace_save_if` (en 64 bits emite
`pushfq`). `get_e820` escribe el token de INT 15h en `ebx`. En
v0.29.2 un `+` fuera de la condición dejaba el sumando ahí (`di = di
+ 24` dejaba 24) y sin la copia QEMU imprimía `e820 n=1`. v0.29.4
hace `push ebx` / `pop ebx` alrededor de ese `+`. Con `ebx` directo,
QEMU 8.2.2 imprimió `e820 n=7` y `pmm free=32480`. Un store de
dirección no simple y `lgdt` / `lidt` / `invlpg` sin etiqueta todavía
dejan `ebx` modificado; `get_e820` no los usa entre el `int 0x15` y
el `if (ebx == 0)`. El caso está en `docs/PEDIDOS_COPIHUE.md`.

El arranque es `src/bootloader/boot.lap` (512 bytes, firma `55 aa`;
el último byte distinto de cero antes de la firma está en el offset
168) y `src/bootloader/stage2.lap` (en `0x8200`, 1678 bytes). La
etapa 2 pide los sectores que ocupa `kernel.bin`, de a 128 por INT
13h. En esta compilación `kernel.bin` mide 149053 bytes: 292
sectores, LBA 9..300, paquete `10 00 24 01 00 00 00 10 09 00 00 00
00 00 00 00`. `make` suma 22124 bytes de `.bss` y escribe fin de
imagen `0x39ca9`. `__bss_end` es `0x39cac`.

El archivo puede pasar de `0x20000`; imagen y `.bss` tienen que
terminar antes de `0x58000` (la pila de `0x60000` crece hacia abajo).
`make` rechaza un kernel de más de 512 sectores. El disquete tiene
2880 sectores y después del LBA 8 quedan 2871, así que el tope que
aprieta es esa pila, no los 1.44 MB. 512 sectores caben: el kernel
máximo terminaría en el LBA 520 y sobrarían 2359. Con esta `.bss`, ese
tope llegaría a `0x5566c`, todavía antes de `0x58000`. Cada proceso tiene
su PML4 y el PIT hace round-robin. La imagen la construyen `make all`
y la comprueba `make qemu-test`. El árbol de trabajo ya no incluye el
lenguaje Tempo ni sus salidas generadas.

En v0.28.1 un `while` y un `if` saltan por los flags, y un store de
constante no pasa por `rax`. `src/user/zona.lap` pide la cuota con
`r = syscall(8)` y `if (r == 0 - 1)`. `r` está en la pila. El
ensamblado está en `docs/PEDIDOS_COPIHUE.md`.

En QEMU 8.2.2, `make qemu-test` de esta imagen terminó en 0. El
script imprimió ticks 100 → 200 en 1037 ms, ticks 200 → 418,
`keyboard IRQ count=151`, `cr3=0x1000 pml4=0x2023 pdpt=0x3023`,
`e820 n=7`, `pmm free=32480 base=0x0000000000100000
end=0x0000000007fe0000`, `ok   nx fetch=0x00000011 write=0x00000003
user=0x00000015 audit wx=0 leaves=513 code=35 nx=478`, `pmm ok,
high page 0x00201000`, letras `baabaabababb`, `round-robin timer=28
yield=4`, `exit freed 7+6 pages, pmm back to 32472`, `trace
counters ok`, `ata ok, blk ok, blk corrupt detected`, `trace
syscall ok`, `trace off ok`, `exec desde disco, missing, bad magic,
bad size, corrupt`, `zone fs, denied, syscall, quota mem 32472,
escape`, `net arp, ping, drop`, `trace overflow ok`, `ud2 -> #UD`,
y en el segundo arranque `fs persist v2, snap v1, corrupt
detected`. El humo de 10 s (`timeout 10 qemu-system-x86_64 -cpu
qemu64 -drive format=raw,file=build/lapageriaos.img -display none
-no-reboot -serial file:…`, sin disco de datos) salió 124. La serie
mostró `e820 n=7`, las siete entradas, `pmm free=32480`, `nx efer
on`, `nx wp on` y `lapageria>`. Esa corrida no escribe comandos. No
se probó `reboot`, `clear`, el teclado a mano, TCP, UDP, AHCI, NVMe
ni un kernel en la mitad alta.

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
