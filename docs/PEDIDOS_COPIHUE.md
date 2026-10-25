# Pedidos a Copihue

Lapageria rosea es el nombre científico del copihue; antes AtomicOS.

Compilador fijado en el Makefile: v0.29.2, commit
`584f731121744c3683ad610a9dab0b021e702ca5`. El tag es ligero: `git
ls-remote` devuelve ese commit para `refs/tags/v0.29.2` y para
`HEAD`. `git -C build/copihue rev-parse HEAD` después del `make`
también, y `copihue --version` imprime `Copihue 0.29.2`.

## Cerrado en v0.27.0 y v0.27.1

Un arreglo adentro de `packed struct` ahora mide `N * sizeof(T)`.
`FsEnt` (nombre `[u8; 16]`, tamaño, cantidad, `[u32; 8]`) sale en 56
bytes, no en 13. `sizeof` y `offsetof` lo comprueban en `fs_layout`,
`blk_layout` y `axe_layout`. El sector en disco sigue siendo el de
`docs/fs.md`.

`save_context(&ctx)` / `restore_context(&ctx)` y `label` /
`label_addr` reemplazan el marco de `sched_go` y de `enter_user`.
`interrupt fn` con `*InterruptFrame` o `*PfFrame` (primer campo
`error_code`) reemplaza el `rbp` del PIT y del `#PF`. El kernel no
tiene `asm()`.

En v0.26.1 esta sonda de Ethernet salía en 0 sin mensaje. `EthHdr`
medía 4 bytes, `etype` quedaba en el offset 2 y `e.dst[0] = 255` no
almacenaba. v0.27.0 lo cierra. En v0.27.3, `copihue --flat 0x10000 -S`
mide `EthHdr` 14, `ArpPkt` 42, `IpPkt` 34, `IcmpEcho` 12, `ZoneStr`
16, `TrEv` 40 y `RxD` 16, con los `offsetof` del fuente. El store
`e.dst[0] = 255` escribe el byte. La red, las trazas y las zonas
leen esos structs. La trama y el disco no cambian: los campos de más
de un byte de la trama van en dos `u8`, en orden de red.

## Cerrado en v0.27.3

`cld()` y `std()` ensamblan, también en `bits 16`. `clear_df` es
`cld()` antes de `lodsb` y de `rep_stosd`. `pushfd` / `popfd` queda
en `check_long_mode`, porque ahí se prueba el bit ID, y en
`trace_save_if`, porque ahí se guardan los flags (en 64 bits
`pushfd()` emite `pushfq`).

`jmp rax` y `jmp [resume_rip]` existen dentro de `naked`. El
planificador no los usa: `restore_context` salta solo y el PIT
cambia de proceso con `frame = (saved_ksp + 120) as *InterruptFrame`.

`ebx` se puede asignar en `bits 16`. El token de INT 15h vive ahí.
Un `if (ebx == 0)` igual lo pisa (`mov ebx, eax` con el cero del
comparando), así que `get_e820` copia el token a memoria antes de
mirarlo y lo restaura al volver al lazo.

`syscall(n, a, ...)` existe en `--flat`. En v0.27.3 el programa de la
zona seguía con `syscall()` sin argumentos: el resultado quedaba en
`rax` y el caso de abajo lo pisaba igual.

## Cerrado en v0.28.1: `rax` sobrevive al `if` y al `while`

Un `if` o un `while` cuya condición es una comparación salta por los
flags. No deja 0 o 1 en `rax`. Un store de una constante que cabe en
el apuntado es `mov tamaño [addr], imm` y no pasa por `rax`.

La cuota de `src/user/zona.lap` quedó así. `r` es un local de
`zone_pages` (pila de `0x402000`). Un `let` dentro del `_start` naked
caería en la página de código, y el kernel la mapea solo lectura:

```copihue
let r: i64 = 0;
let n: i64 = 0;
while (r == 0 && n <= 8) {
    r = syscall(8);
    if (r == 0) { n = n + 1; }
}
if (r == 0 - 1) { say_quota(); }
```

`copihue --flat 0x400000 -S` con este pin escribe la syscall y guarda
`r` antes de comparar. Al salir del `while` recarga ese slot, resta 1
y salta con `jne`. No hay `sete`:

```
mov rax, 8
syscall
mov [rbp-8], rax
```

```
mov rax, [rbp-8]
push rax
mov rax, 0
push rax
mov rax, 1
mov rbx, rax
pop rax
sub rax, rbx
mov rbx, rax
pop rax
cmp rax, rbx
jne .L23
call say_quota
```

En QEMU 8.2.2, `make qemu-test` de la imagen con el pin v0.28.1
terminó en 0. La línea de la zona fue `zone fs, denied, syscall,
quota mem 32473, escape`: la syscall 2 denegada devolvió -1 (`zone
syscall denied`) y la cuota también (`zone quota ok`). El resultado
no se perdió. El `make qemu-test` de v0.29.2 también imprimió esa
línea de resumen y terminó en 0.

## Cerrado en v0.29.2: el `while` y el `if` restauran `ebx`

En `bits 16` la comparación sigue siendo `cmp eax, ebx`. Alrededor
hay `push ebx` y `pop ebx`. `pop` no toca los flags, así que un
`while (c != 0)` ya no deja el cero del comparando en `ebx`.

Esta función se compiló con `copihue --freestanding` de este pin. No
se ensambló y no se arrancó:

```copihue
bits 16 naked fn while_keeps() {
    ebx = 7;
    let c: u16 = 1;
    while (c != 0) { c = 0; }
}
```

```
mov ebx, 7
mov eax, 1
mov [__loc_while_keeps_0], eax
.L0:
mov eax, [__loc_while_keeps_0]
push ebx
push eax
push edx
mov eax, 0
mov ebx, eax
pop edx
pop eax
cmp eax, ebx
pop ebx
je .L1
mov eax, 0
mov [__loc_while_keeps_0], eax
```

El cuerpo escribe el cero en memoria y no en `ebx`. El `pop ebx`
devuelve el 7. El fallo de v0.28.1 (el `while` dejaba `ebx` en 0, el
siguiente `int 0x15` reiniciaba el mapa y QEMU imprimía `e820 n=16`)
queda cerrado por ese `push` / `pop`.

## Sigue abierto: un `+` fuera de la condición deja el sumando en `ebx`

El mismo operador, fuera del `if` o del `while`, sigue usando `ebx`
de borrador y no lo restaura. Caso mínimo, misma corrida de
`--freestanding`, sin ensamblar y sin arrancar:

```copihue
bits 16 naked fn add_clobber() {
    ebx = 7;
    di = di + 24;
}
```

```
add_clobber:
mov ebx, 7
movzx eax, di
push eax
push edx
mov eax, 24
mov ebx, eax
pop edx
pop eax
add eax, ebx
mov di, ax
ret
```

`ebx` sale en 24. `get_e820` hace esa suma, y otra de 1 para el
conteo, después de `int 0x15` y antes de la vuelta al lazo. Sin
`ebx_save`, `copihue --flat 0x8200 -S` de este pin escribe:

```
mov eax, 59424
int 21
jnc .L13
```

```
.L15:
movzx eax, word [32768]
push eax
push edx
mov eax, 1
mov ebx, eax
pop edx
pop eax
add eax, ebx
mov word [32768], ax
movzx eax, di
push eax
push edx
mov eax, 24
mov ebx, eax
pop edx
pop eax
add eax, ebx
mov di, ax
```

El `if (ebx == 0)` que sigue compara ese 24, no el token de la BIOS.
La próxima `int 0x15` también lo recibe en 24.

Esa imagen, sin la copia y con el resto del kernel igual, se arrancó
en QEMU 8.2.2 (`timeout 6`, `-display none -no-reboot`, serie a
archivo, código 124). Imprimió una sola entrada y el fallback:

```
e820 n=1
e820[0] type=1 base=0x0000000000000000 len=0x000000000009fc00 acpi=1
e820 fallback 0x00100000-0x00200000
pmm free=256 base=0x0000000000100000 end=0x0000000000200000
```

No fue `e820 n=7` y tampoco el `e820 n=16` del pin anterior: el
`while` ya no pone el token a cero, pero el `+` lo reemplaza por el
sumando y el mapa se corta. Con `ebx_save` en `stage2.lap`, el mismo
QEMU y un arranque de 6 s imprimieron `e820 n=7`, las siete entradas
(la usable de 1 MB termina en `0x7fe0000`) y `pmm free=32480
base=0x0000000000100000 end=0x0000000007fe0000`. El rodeo se queda.
