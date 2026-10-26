# Bit NX y W^X

QEMU arranca con `-cpu qemu64` (`make qemu`, `make debug`, `make qemu-test`, el humo de CI y `scripts/pre-commit.sh`). Ese modelo deja puesto el bit 20 de EDX en la hoja CPUID `0x80000001`. Sin ese bit el kernel no toca el MSR.

`nx_init` corre después de la IDT. Lee el bit, prende EFER.NXE (MSR `0xC0000080`, bit 11) y lo vuelve a leer. En el humo de 10 s y en `make qemu-test` la serie dice `nx efer on`.

La etapa 2 deja PD[0] en `0x83`: una página de 2 MiB, presente y escribible, que cubre código y datos. Con NXE prendido el kernel la parte. Pide un marco entre 1 MiB y 2 MiB, escribe 512 entradas de 4 KiB y guarda `tabla|3` en PD[0]. El bit 63 va solo en las hojas. Un directorio con ese bit ensuciaría la dirección, porque `>>` es aritmético. Después pone CR0.WP (bit 16) y recarga CR3. La serie dice `nx wp on`.

`.text` termina alineado a 4096 (`data nx_text_pad { align 4096; }`). En este binario los inmediatos de `nx_ranges` son:

| Símbolo | Dirección |
|---|---|
| `__text_start` | `0x10000` |
| `__text_end` | `0x33000` |
| `__data_start` | `0x33000` |
| `__data_end` | `0x3463d` |
| `__bss_start` | `0x34640` |
| `__bss_end` | `0x39cac` |

Las páginas completas dentro de `[0x10000, 0x33000)` quedan presentes, sin escritura y ejecutables (35 páginas). El resto del primer 2 MiB lleva el bit 63: datos, `.bss`, pilas, VGA, tablas y los huecos. `.data` y `.bss` comparten la página `0x34000`. Las dos son NX. No queda ninguna hoja de 2 MiB: la auditoría imprimió `huge=0`.

El código de usuario en `0x400000` queda presente, de usuario, sin escritura y sin NX. La pila (`0x401000`), la página privada (`0x500000`), el heap de la zona y el MMIO de la red llevan el bit 63. Durante la copia el código está NX; al terminar se remapea sin escritura y sin NX.

El comando `nx` corre en la prueba después de `pf`, así que ya existe la página de demanda en `0x200000`. Esta corrida imprimió:

```
nx map text 0x00010000 0x00033000 data 0x00033000 0x0003463d bss 0x00034640 0x00039cac
nx ok efer
nx ok wp
exception: vector 14 #PF page fault err=0x00000011 cr2=0x00033000
nx ok fetch err=0x00000011 cr2=0x00033000
exception: vector 14 #PF page fault err=0x00000003 cr2=0x00010000
nx ok write err=0x00000003 cr2=0x00010000
exception: vector 14 #PF page fault err=0x00000015 cr2=0x00401000
nx ok sched
exception: vector 14 #PF page fault err=0x00000015 cr2=0x00500000
nx ok user stack err=0x00000015 cr2=0x00401000
nx ok user data err=0x00000015 cr2=0x00500000
nx ok audit wx=0 leaves=513 code=35 nx=478 huge=0
```

El fetch es de una página presente (bit 0) y de instrucción (bit 4). La escritura en `.text` es presente y de escritura (bits 0 y 1), con CR0.WP. El usuario que ejecuta su pila o sus datos es presente, anillo 3 e instrucción (bits 0, 2 y 4). El segundo proceso alcanza a imprimir `nx ok sched` antes de su `#PF`. El shell sigue. `35 + 478 = 513`: 512 hojas del primer 2 MiB más la página de `pf`. `wx=0`.

`make qemu-test` imprimió `ok   nx fetch=0x00000011 write=0x00000003 user=0x00000015 audit wx=0 leaves=513 code=35 nx=478`.

Copihue v0.29.4 alcanza para esto: `cpuid` / `cpuid_edx`, `rdmsr` / `wrmsr`, `read_cr0` / `write_cr0`, `read_cr2` y un `<<` de 63 bits guardado en 8 bytes (`mov qword`). No se agregó un pedido ni un `asm()`.
