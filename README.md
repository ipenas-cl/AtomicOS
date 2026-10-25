# AtomicOS

Sistema operativo de Ignacio Peña Sepúlveda. El kernel está escrito en [Copihue](https://github.com/ipenas-cl/copihue). El nombre del sistema puede cambiar; este repositorio sigue llamándose AtomicOS.

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.8-green)](VERSION)
[![CI](https://github.com/ipenas-cl/AtomicOS/actions/workflows/ci.yml/badge.svg)](https://github.com/ipenas-cl/AtomicOS/actions/workflows/ci.yml)

Versión de kernel **0.8**. Experimental.

## Qué arranca hoy

`make all` escribe `build/atomicos.img`, una imagen cruda de 2880 sectores de 512 bytes. El sector 0 es el arranque. El kernel empieza en LBA 1.

`src/bootloader/boot64.asm` (263 líneas de NASM, un sector de 512 bytes) trabaja en modo real: habilita A20, lee 64 sectores con INT 13h (LBA) a `0x10000`, guarda el mapa E820 en `0x8000` y entra en modo largo. La pila del kernel queda en `0x90000`.

El kernel es un binario plano de como mucho 32768 bytes. Lo emite `copihue --flat 0x10000` a partir de `src/kernel/copihue/*.lap` (1191 líneas). Esas líneas son el 100 % del fuente del kernel que entra en la imagen. El único archivo `.asm` del arranque es el sector. Dentro de los `.lap` hay `asm(...)` donde el hardware lo exige: `ud2`, el `retfq` que recarga CS, el marco de `iretq` y el cuerpo del programa de anillo 3.

Al arrancar, el kernel limpia la `.bss`, abre la consola VGA (el mismo texto sale por COM1), inicializa el asignador físico, carga su GDT e IDT, espera dos muestras del PIT y deja el shell `atomicos>`.

El shell responde a `help`, `echo`, `add`, `mul`, `clear`, `version`, `ticks`, `irqs`, `cr3`, `pf`, `pmm`, `user`, `ud2` y `reboot`. `make qemu-test` escribe los comandos con el `sendkey` del monitor de QEMU y lee COM1. Esa prueba no escribe `reboot` (reinicia la máquina) ni `clear`.

![Shell de AtomicOS en QEMU](docs/copihue-shell.png)

Vídeo de unos 40 s: [docs/atomicos-copihue-demo.mp4](docs/atomicos-copihue-demo.mp4).

`make qemu-test` comprueba:

- el PIT avanza unas 100 ticks por segundo (dos muestras al arrancar, y `ticks` sigue creciendo en el shell);
- `echo`, `add`, `mul`, `version`, `help` y un comando desconocido;
- la IRQ1 del teclado, y `cr3` del bootloader (`0x1000`, con el bit de acceso en PML4 y PDPT);
- un `#PF` de anillo 0 en `0x200000` que reserva un marco, lo pone a cero y reanuda;
- el resumen E820 y el asignador (`pmm`, `pmm high`);
- `user`: el syscall imprime `user ok`, la lectura de `0x10000` desde anillo 3 produce `#PF` con `err=0x5`, el kernel escribe `user blocked` y vuelve al shell;
- `ud2` imprime la excepción `#UD`.

Los textos exactos están en `tests/qemu_shell_test.sh`.

## Cómo compilar y probar

Hacen falta `make`, `gcc`, `git`, `nasm`, `qemu-system-x86_64` y `socat`. `ffmpeg` solo entra si se pide una captura (`tests/qemu_shell_test.sh imagen --screenshot salida.png`).

```bash
# Debian/Ubuntu
sudo apt-get install build-essential nasm qemu-system-x86 socat git

make all          # build/atomicos.img; la primera vez clona y compila Copihue
make qemu         # QEMU en curses. El shell lee el teclado PS/2
make qemu-test    # arranque mudo y comprobaciones por serie
make debug        # igual que qemu, esperando gdb en tcp::1234
make clean        # borra build/
make help
```

`scripts/run_atomicos.sh` es un atajo de `make qemu`.

La consola serie repite el texto de VGA. Con `-display none -serial stdio` se ve el registro de arranque. Las teclas de esa terminal no llegan al shell: el shell lee el controlador PS/2. Para escribir comandos a mano hace falta la pantalla de QEMU (`make qemu`). `make qemu-test` inyecta las teclas con `sendkey`.

```bash
qemu-system-x86_64 -drive format=raw,file=build/atomicos.img -display none -serial stdio
```

El compilador está fijado en el Makefile (`COPIHUE_COMMIT ?= v0.23.0`, repositorio `https://github.com/ipenas-cl/copihue.git`). `make` lo construye en `build/copihue`. Para un binario ya compilado:

```bash
make COPIHUE=/ruta/al/copihue all
```

`make debug` abre el mismo QEMU y espera un gdb en `tcp::1234`. La imagen es un binario plano, sin tabla de símbolos ELF.

CI (`.github/workflows/ci.yml`), en cada pull request: instala `nasm`, `qemu-system-x86` y `socat`, ejecuta `make all`, arranca la imagen 10 segundos (el proceso tiene que seguir vivo) y ejecuta `make qemu-test`.

`scripts/pre-commit.sh` repite esas comprobaciones. `scripts/release.sh MAYOR.MENOR` ejecuta `make all` y `make qemu-test` y actualiza `VERSION` y el Makefile. No crea commits, etiquetas ni hace push.

## Estructura

```
Makefile                      all, qemu, qemu-test, debug, clean, help
VERSION                       0.8
LICENSE                       MIT
CHANGELOG.md
.github/workflows/ci.yml
src/bootloader/boot64.asm     sector de arranque, modo largo, E820, lectura LBA
src/kernel/copihue/
  kernel.lap                  _start, banner, muestras del PIT, llamada al shell
  console.lap                 texto VGA y COM1
  keyboard.lap                PS/2 en IRQ1
  idt.lap                     IDT, PIC, PIT, excepciones
  pmm.lap                     bitmap E820, alloc y free
  paging.lap                  mapas de 4 KB y #PF de demanda en anillo 0
  gdt.lap                     GDT, TSS, ltr, vuelta del #PF de anillo 3
  usermode.lap                iretq, syscall, programa de anillo 3
  shell.lap                   editor de línea y comandos
tests/qemu_shell_test.sh
scripts/run_atomicos.sh
scripts/pre-commit.sh
scripts/release.sh
docs/copihue-shell.png
docs/atomicos-copihue-demo.mp4
```

`make` concatena los `.lap` en `build/kernel_all.lap` y compila ese archivo. `build/` no se versiona. El orden de concatenación es `COPIHUE_KERNEL_SRC` en el Makefile.

## Hoja de ruta

El estado de cada etapa es el de la imagen que arranca `make qemu-test`.

### 1. Arranque y consola — en la imagen

Sector de 512 bytes, modo largo, kernel en Copihue, VGA y COM1, shell con los comandos de arriba.

### 2. Interrupciones y temporizador — en la imagen

IDT de 48 entradas. Las excepciones de CPU 0–31 imprimen su nombre; `#PF` también imprime CR2. El PIC queda remapeado a los vectores 32–47. El PIT del canal 0 va a 100 Hz (`1193182/100`). El teclado usa IRQ1. La máscara del PIC maestro es `0xFC` (IRQ0 e IRQ1 abiertas). El esclavo queda enmascarado. El resto de líneas del maestro solo envía EOI. Esta etapa no incluye APIC.

### 3. Memoria virtual — parcial, en la imagen

- Mapa E820 (como mucho 16 entradas) y un bitmap de marcos de 4 KB para la RAM tipo 1 usable, desde 1 MB y hasta 128 MB.
- Tablas del bootloader: CR3 en `0x1000`, PML4, PDPT y PD. `PD[0]` es la página de identidad de 2 MB del kernel, de supervisor.
- `map_page` y `unmap_page` crean tablas de 4 KB por encima de esa ventana.
- Un `#PF` de anillo 0 por página no presente, con dirección por debajo de 128 MB, asigna un marco, lo mapea, lo pone a cero y reanuda.
- `user` marca el bit de usuario en PML4[0] y PDPT[0], mapea código en `0x400000` (lectura, usuario) y pila en `0x401000` (lectura y escritura, usuario). Sigue siendo el mismo CR3.
- Sigue pendiente en esta etapa: un segundo espacio de direcciones, el bit NX (`EFER.NXE` queda en claro) y unas tablas que no sean las del sector de arranque.

### 4. Procesos y syscalls — parcial, en la imagen

Hay un programa de anillo 3, y se ejecuta una sola vez (`user already` a la segunda). La GDT tiene segmentos de usuario, un TSS de 104 bytes, `ltr`, `STAR`, `LSTAR`, `SFMASK` y `EFER.SCE`, más `iretq` a anillo 3 y `syscall` / `sysret`. El syscall 1 escribe un carácter. El syscall 2 escribiría `user leak` si esa lectura del kernel llegara a ejecutarse. El `#PF` de anillo 3 vuelve al shell por la pila `rsp0` (`0x86000`) y `resume_shell`. El syscall usa otra pila (`0x81000`).

Sigue pendiente en esta etapa: una tabla de procesos, un planificador y el cambio de contexto entre tareas.

### 5. Disco y sistema de archivos — pendiente

El disco se usa en el sector de arranque para leer los 64 sectores del kernel (INT 13h). El kernel de la imagen no trae driver ATA, AHCI ni NVMe, ni sistema de archivos.

### 6. Red — pendiente

El árbol que arranca no trae driver de red ni pila de protocolos.

### 7. Zonas y observabilidad — pendiente

Lo que se puede observar hoy es la consola: VGA duplicada en COM1, los contadores `ticks` y `kbd_irqs`, y la línea de cada excepción. Esta etapa cubre zonas de aislamiento y trazas; ese código todavía no está en la imagen.

## Historial Tempo

El lenguaje Tempo, sus compiladores, los `.s` y `.inc` generados, el `link.ld` antiguo y el binario `tools/tempo_compiler` siguen en el historial de git. El commit anterior a esta limpieza es `355f4ec7438239f0062988512a5c9f585da8cc57` (punta de `cursor/copihue-v0.23-usermode-46c5` al abrir el cambio). Ahí puede crearse la etiqueta `legacy-tempo`. Este árbol no reescribe historia ni mueve `main`.

[LICENSE](LICENSE) es MIT y conserva el aviso de marca de Tempo.

## Autor

**Ignacio Peña Sepúlveda** ([@ipenas-cl](https://github.com/ipenas-cl))
