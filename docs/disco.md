# Disco de datos

Lapageria rosea es el nombre científico del copihue; antes AtomicOS.

El arranque no cambia. `build/lapageriaos.img` sigue siendo la imagen de
2880 sectores (1,44 MB) en el IDE maestro (`ide0-hd0`). QEMU la presenta
como disco duro, no como disquete. El disco de datos es otro archivo,
`build/data.img`: 2048 sectores de 512 bytes (1 MiB), esclavo del canal
primario (`ide0-hd1`):

```
-drive file=build/data.img,format=raw,if=ide,index=1
```

`ata.lap` solo selecciona ese esclavo (bit 4 del registro 0x1F6, LBA28,
PIO). No escribe el disco de arranque. Cada espera de estado tiene tope;
si la unidad no responde, la función devuelve error.

## Layout

El bloque lógico `n` son 512 bytes de datos y ocupa dos sectores:

| LBA | Rol |
|---|---|
| `2*n` | datos |
| `2*n+1` | metadatos |

Con 2048 sectores entran los bloques 0..1023. `blk_read` / `blk_write`
rechazan un `n` que no quepa en lo que informó IDENTIFY.

Sector de metadatos (el resto va en cero):

| Offset | Bytes | Contenido |
|---|---|---|
| 0 | 4 | magic `ABLK` (`41 42 4C 4B`) |
| 4 | 4 | número de bloque, little-endian |
| 8 | 4 | CRC-32 de los 512 bytes de datos |

El CRC es el de ISO-HDLC (Ethernet, PNG, gzip): polinomio reflejado
`0xEDB88320`, valor inicial `0xFFFFFFFF`, XOR final `0xFFFFFFFF`. Se
calcula bit a bit, sin tabla. El vector `123456789` da `0xCBF43926`.

`blk_write(n)` escribe primero el sector de datos y después el de
metadatos, con el CRC nuevo. `blk_read(n)` lee los dos y revisa magic,
número y CRC. Si algo no calza, devuelve 5: el buffer no sirve.

## Errores

| Código | Significado |
|---|---|
| 0 | listo |
| 1 | se agotó el tiempo |
| 2 | el disco marcó ERR o DF |
| 3 | no hay unidad, o IDENTIFY no trae sectores |
| 4 | LBA fuera de 28 bits, o el bloque no cabe |
| 5 | magic, número o CRC no calzan |

## Prueba

`disk` en el shell (lo escribe `make qemu-test`):

1. IDENTIFY del esclavo y un sector de ida y vuelta por el driver (LBA 2).
2. `blk_write(0)` de un patrón y `blk_read(0)` que lo trae igual.
3. Se cambia un byte del sector de datos con `ata_write`, sin tocar el
   sector de metadatos. `blk_read(0)` tiene que devolver 5.

Por la serie salen, en ese orden, `ata ok`, `blk ok` y
`blk corrupt detected`. Si falta una, el script termina en 1.

Sin `build/data.img`, en QEMU 8.2.2 el mismo comando imprime `ata fail 1`
y vuelve al prompt. El estado no es 0xFF: la espera se agota y devuelve
timeout.

## Qué falta

El sistema de archivos copy-on-write está en [docs/fs.md](fs.md): el
bloque lógico sigue siendo el de esta capa, y un snapshot se queda con
los bloques que ya no apunta la raíz nueva. Sigue sin otro bus (AHCI,
NVMe).
