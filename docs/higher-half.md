# Kernel en la mitad alta

El kernel se enlaza en `0xFFFFFFFF80010000` y se carga en el físico `0x10000`. La base de la ventana es `0xFFFFFFFF80000000`: PML4[511], PDPT[510], PD[0], los 2 MiB de arriba de los 2 GiB altos. Esa dirección cabe en un desplazamiento con signo de 32 bits. Copihue v0.31.0 la acepta con `--flat`; no hay un pedido nuevo en [PEDIDOS_COPIHUE.md](PEDIDOS_COPIHUE.md).

`0xFFFFFFFF80000000` es un `i64` negativo. `(va >> 39) & 511` igual da 511, y el índice de PDPT da 510. `hh_phys(va)` resta la base y recupera el físico.

## Mapa

| Qué | Virtual | Físico | Después de retirar la identidad |
|---|---|---|---|
| Etapa 2 | `0x8200` | `0x8200` | Sigue. El salto ya ocurrió |
| Tablas del arranque | `0x1000`..`0x5FFF` | igual | Siguen. CR3 es `0x1000` |
| E820 | `0x8000` | `0x8000` | Sigue |
| Imagen (`.text`, `.data`, `.bss`) | `0xFFFFFFFF80010000` + offset | `0x10000` + offset | Solo la ventana alta |
| VGA | `0xB8000` | `0xB8000` | Sigue, supervisor |
| Pilas de proceso, syscall y `TSS.rsp0` | `0x60000`, `0x68000`, `0x70000`, `0x78000`, `0x81000`, `0x86000` | igual | Siguen, supervisor, NX |
| Pila del shell | `0xFFFFFFFF80090000` | `0x90000` | Las dos vistas; RSP usa la alta |
| Heap de demanda | `0x200000` en adelante | el marco que asigne el PMM | NX, anillo 0, por debajo de 128 MB |
| MMIO e1000 | `0x10000000` | igual | Fuera de esta ventana |
| Código y pila de usuario | `0x400000`, `0x401000`, `0x402000` | marcos del PMM | Mitad baja, bit de usuario |
| Página privada de `sched` | `0x500000` | un marco | Mitad baja, bit de usuario |

En este binario (`ndisasm -b 64` de `build/kernel.bin`, org `0xFFFFFFFF80010000`):

| Símbolo | Virtual | Físico |
|---|---|---|
| `__text_start` | `0xFFFFFFFF80010000` | `0x10000` |
| `__text_end` | `0xFFFFFFFF80035000` | `0x35000` |
| `__data_start` | `0xFFFFFFFF80035000` | `0x35000` |
| `__data_end` | `0xFFFFFFFF8003684F` | `0x3684F` |
| `__bss_start` | `0xFFFFFFFF80036850` | `0x36850` |
| `__bss_end` | `0xFFFFFFFF8003BEFC` | `0x3BEFC` |

El archivo mide 157775 bytes (`0x2684F`) y empieza en `_start` (`55 48 89 E5`). `.text` llega alineado a 4096 en el offset `0x25000`. Entre `__data_end` y `__bss_start` hay un byte de alineación. `put_hex` imprime 32 bits, así que el mapa del comando `nx` muestra `0x80010000` para el inicio de `.text`.

GDT, IDT y TSS están en la `.bss`. `lgdt` y `lidt` reciben esa dirección alta. Al retirar la identidad, esas páginas dejan de estar en la tabla baja y el CPU las sigue por la ventana alta.

## Qué queda en identidad, y hasta cuándo

La etapa 2 arma las dos vistas antes de saltar.

- Identidad baja, como antes: PML4[0] = `0x2003`, PDPT[0] = `0x3003`, PD[0] = `0x83` (2 MiB, físico 0).
- Ventana alta, tablas distintas: PML4[511] en `0x1FF8` = `0x4003` (PDPT en `0x4000`), PDPT[510] en `0x4FF0` = `0x5003` (PD en `0x5000`), PD[0] = `0x83`. El `rep_stosd` de arriba limpia hasta `0x4FFF`. `0x5000` se limpia aparte (1024 dwords).

Hace falta otra tabla de 4 KiB. El alias viejo `0xFFFF800000000000` (PML4[256], PDPT[0]) comparte la tabla baja. Borrar ahí la imagen también borraría ese alias.

`long_mode` deja RSP en `0x90000` para el `push`/`pop` del desplazamiento, luego:

```
rax = 0xFFFFFFFF; rax = rax << 32; rax = rax | 0x80090000; rsp = rax;
rax = 0xFFFFFFFF; rax = rax << 32; rax = rax | 0x80010000; jmp rax;
```

En `build/stage2.bin` (1789 bytes) el final es `mov rsp, rax` / `jmp rax` (`FF E0` en el offset `0x6FA`). `mov eax, 0x80010000` extiende con ceros y el `or` lo junta con `0xFFFFFFFF00000000`.

Desde ahí hasta el final de `nx_init` conviven las dos vistas. `_start`, la consola, el PMM, la GDT y la IDT corren ya en la dirección alta. `nx_split` pide dos marcos bajos, llena las dos tablas de 4 KiB con los mismos flags a partir del rango físico, y apunta PD[0] (`0x3000`) y la PD alta (`0x5000`) a cada una. Después de CR0.WP, `hh_retire` pone en cero las entradas de la tabla baja desde el físico `0x10000` hasta `__bss_end` redondeado a página (`0x3C000`), hace `invlpg` y recarga CR3. La tabla alta no se toca. La serie del humo dice `hh on text=0xffffffff80010000` antes del prompt.

VGA (`0xB8000`), E820, las tablas, la etapa 2 y las pilas de `0x60000` para arriba quedan en la identidad: están fuera de ese rango. La pila del shell también se ve en `0xFFFFFFFF80090000`, porque la PD alta cubre los primeros 2 MiB.

## NX, W^X y el anillo 3

Las páginas cuyo físico cae entero en `.text` llevan flags `1`: presentes, sin escritura, sin usuario, sin NX. El resto de esos 2 MiB lleva `3` y el bit 63. La página de 2 MiB existe solo hasta `nx_split`, antes de cualquier auditoría. La auditoría clasifica por la dirección física de la hoja. El `.text` alto cuenta una vez; el bajo ya no está presente. En las tres corridas, `wx=0` y `huge=0`.

La cuenta de esta imagen: la imagen ocupa `0x10000`..`0x3C000`, 44 páginas. Abajo quedan 512 − 44 = 468 hojas presentes. Arriba, 512. `pf` ya mapeó `0x200000`. Total 981. `.text` es `0x10000`..`0x35000`, 37 páginas. 981 − 37 = 944. Las tres corridas imprimieron `leaves=981 code=37 nx=944`.

Un proceso no reutiliza el PML4 del arranque. PML4[256..511] apuntan a un PDPT compartido, sin bit de usuario. Ese PDPT tiene en [510] la PD física `0x5000`, así que un `#PF` o un `syscall` con el CR3 del proceso sigue pudiendo ejecutar el kernel. El bit de usuario no está en ningún nivel de esa caminata: leer o escribir `0xFFFFFFFF80010000` desde anillo 3 es `#PF` presente, con el bit 2. `hh` mide la lectura con bits 0 y 2 (`(err & 5) == 5`, sin escritura ni fetch) y la escritura con bits 0, 1 y 2 (`(err & 7) == 7`). El planificador pasa al otro proceso (`hh ok sched`) y el shell sigue.

La lectura de anillo 3 de `0x10000` (el comando `user`) ahora es no presente: la identidad de esa página ya no está. El error es `0x4` (bit 2, bit 0 en cero). Antes de retirar la imagen era `0x5`.

`demand_map` sigue rechazando una dirección negativa. La ventana del kernel no se pagina por demanda.

PML4[511] del proceso queda con el bit de acceso (bit 5) porque el kernel corre por ahí. PML4[256] no. La prueba de `sched` compara esas entradas con el bit 5 enmascarado.

## Comando `hh`

`hh` pide `nx_on` y la tabla alta. Imprime RIP y RSP con `put_hex64` y exige los dos dentro de la ventana de 2 MiB. Después comprueba que la hoja de `0x10000` no está presente, lee ese byte y espera `#PF` con error 0 y CR2 `0x10000` (`hh_kernel_fault` vuelve con `restore_context`, sin mapear). Copia a `0x400000` dos programas desnudos: uno lee `0xFFFFFFFF80010000` y otro, después de `syscall(11)`, lo escribe. El acceso es absoluto (`movzx` / `mov byte` con desplazamiento de 32 bits), así que los bytes copiados siguen apuntando al kernel. El segundo proceso imprime `hh ok sched` antes de su falta. Al volver, `nx_audit` tiene que dar `wx=0`, `huge=0` y `leaves == code + nx`. `nx` y `hh` sueltan sus procesos y restauran CR3 `0x1000` antes de auditar.

## Qué se midió

Copihue v0.31.0, commit `495bb4b54d3fba77ed83aa51c0d2362ea9ebd621`. `build/kernel.bin` 157775 bytes, 309 sectores, LBA 9..317. El paquete de la etapa 2 es `10 00 35 01 00 00 00 10 09 00 00 00 00 00 00 00`. `make` suma 22188 bytes de `.bss` y escribe fin `0x3befb`. `__bss_end` físico es `0x3befc`. `stage2.bin` mide 1789 bytes. `src/kernel/copihue/*.lap` sin `exec_img.lap` ni `zone_img.lap` son 6265 líneas; `hh.lap` son 256. No hay `asm()` en `src/`.

Tres `make COPIHUE=/tmp/copihue qemu-test` seguidos, QEMU 8.2.2, `-cpu qemu64`, terminaron en 0. Los tres imprimieron `ok   nx fetch=0x00000011 write=0x00000003 user=0x00000015 audit wx=0 leaves=981 code=37 nx=944` y `ok   hh rip rsp low=#PF user wx=0 leaves=981 code=37 nx=944`. También `keyboard IRQ count=151`, `cr3=0x1000 pml4=0x2023 pdpt=0x3023`, `e820 n=7`, `pmm free=32480`, `pmm back to 32471`, `zone quota mem 32471`. Los ticks 100 → 200 tardaron 935 ms, 938 ms y 1035 ms; la muestra siguiente fue 415, 419 y 418. Las letras fueron `babababababa`, `abbababababa` y `babababababa`. El round-robin fue `timer=25`, `33` y `27`, las tres con `yield=4`.

El humo de 10 s (`timeout 10 qemu-system-x86_64 -cpu qemu64 -drive format=raw,file=build/lapageriaos.img -display none -no-reboot -serial file:…`) salió 124. La serie mostró `e820 n=7`, las siete entradas, `pmm free=32480`, `gdt load`, `gdt ok`, `nx efer on`, `nx wp on`, `hh on text=0xffffffff80010000`, `timer: ticks=100`, `timer: ticks=200` y `lapageria>`. Esa corrida no escribe comandos.
