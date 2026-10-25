# Sistema de archivos

Copy-on-write chico sobre `blk_read` / `blk_write`. El checksum del
bloque sigue siendo el del sector de metadatos. Si no calza, la
lectura devuelve error y el shell no imprime esos bytes.

Los bloques 0 y 1 los usa el comando `disk` (el 0 es la prueba de CRC,
el LBA 2 es el sector de datos del bloque 1). El sistema de archivos
parte en el bloque 2.

## Superbloque

Hay dos copias, en los bloques 2 y 3. Cada operación escribe la copia
que no está publicada y le sube la generación. La copia viva queda
quieta hasta que la nueva cierra el CRC. Al montar se leen las dos y
gana la generación mayor cuyo magic y CRC calzan. Si la copia nueva
quedó a medias, sigue valiendo la anterior.

`fs_format` deja la generación 1 en el bloque 2 y la generación 0 en
el bloque 3, con el mismo árbol. Así un disco que ya tenía datos no
esconde el formato nuevo.

| Offset | Bytes | Contenido |
|---|---|---|
| 0 | 4 | magic `AFS1` (`41 46 53 31`) |
| 4 | 4 | versión 1 |
| 8 | 4 | generación |
| 12 | 4 | raíz: bloque del directorio vivo |
| 16 | 4 | cantidad de bloques del disco (tope 1024) |
| 20 | 4 | primer bloque asignable (4) |
| 24 | 4 | cantidad de snapshots |
| 28 | 4 | reservado |
| 32 | 128 | bitmap. El bit 1 significa ocupado. Bit `n` en el byte `32 + n/8` |
| 160 | 192 | 8 snapshots de 24 bytes |

Snapshot (desde el offset 160):

| Offset | Bytes | Contenido |
|---|---|---|
| 0 | 12 | nombre, con cero al final |
| 12 | 4 | bloque del directorio |
| 16 | 4 | generación al momento del snapshot |
| 20 | 4 | cero |

## Directorio

Un solo bloque, plano. Magic `ADIR` (`41 44 49 52`). Hasta 7 archivos.
La entrada mide 64 bytes y parte en el offset 8.

| Offset | Bytes | Contenido |
|---|---|---|
| 0 | 16 | nombre |
| 16 | 4 | tamaño en bytes |
| 20 | 4 | cantidad de bloques de datos |
| 24 | 32 | hasta 8 números de bloque |
| 56 | 8 | cero |

Un archivo llega a 8 × 512 = 4096 bytes. El bloque de datos es el
contenido, sin cabecera: el CRC cubre los bytes del archivo.

## Copy-on-write

Escribir reserva bloques libres, los llena y arma un directorio nuevo.
Recién al final se publica la raíz en el otro superbloque. Un bloque
que todavía cuelga de la raíz viva o de algún snapshot sigue con el
bit puesto: no se vuelve a entregar. Si ya no cuelga de ninguno, el
bit vuelve a cero.

La única escritura en un bloque ya publicado es la copia vieja del
superbloque, y solo cuando ya hay una copia más nueva.

## Llamadas

| Función | Qué hace |
|---|---|
| `fs_format` | bitmap, directorio vacío, las dos copias del superbloque |
| `fs_create(name)` | archivo de tamaño 0 |
| `fs_write(name, buf, len)` | reemplaza el contenido; si no existe, lo crea |
| `fs_read(name, buf, max)` | bytes leídos, o un código negativo |
| `fs_list` | imprime `fs file <nombre>` |
| `fs_snapshot(name)` | guarda la raíz actual |
| `fs_read_snap(snap, name, buf, max)` | lee esa raíz, no la viva |
| `fs_rollback(snap)` | la raíz viva pasa a ser la del snapshot |

Los códigos positivos de `fs_write` y compañía, y el valor absoluto de
los de `fs_read`: 1 a 5 son los de ATA y de `blk` (5 es CRC, magic o
número de bloque), 6 sin espacio, 7 no está, 8 superbloque o
directorio malo, 9 no cabe, 11 nombre malo o repetido.

## Shell

`fs format`, `fs create <nombre>`, `fs write <nombre> <texto>`,
`fs cat <nombre>`, `fs ls`, `fs snap <nombre>`,
`fs snap <nombre> <archivo>`, `fs rollback <nombre>`.
`fs corrupt <archivo>` pisa un byte del dato con `ata_write` y no
toca el sector de metadatos. `fs blocks` escribe 600 bytes y los
relee. `fs check` solo relee, para el segundo arranque.
`fs rm <nombre>` saca la entrada del directorio vivo. Un bloque que
sigue colgando de un snapshot no se libera.

## Prueba

`make qemu-test` formatea, escribe `a` con `v1`, saca el snapshot
`s1`, reescribe `a` con `v2` y comprueba las dos lecturas. También
crea `c` vacío, hace rollback de `b` y un archivo de dos bloques.
Apaga QEMU, abre el mismo `build/data.img` y vuelve a leer. Después
corrompe un bloque de `a`: tiene que salir `fs corrupt detected` y
`s1` sigue en `v1`. Si falta una línea, el script termina en 1.
