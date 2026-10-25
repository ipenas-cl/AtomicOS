# Pedidos a Copihue

Lapageria rosea es el nombre científico del copihue; antes AtomicOS.

Compilador fijado en el Makefile: v0.28.1, commit
`4f12ea1ea940d307eca2d2d2c5a914d650b88d00`. El tag es ligero: `git
ls-remote` y `gh` devuelven ese commit, y `git -C build/copihue
rev-parse HEAD` después del `make` también.

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

En QEMU 8.2.2, `make qemu-test` de esta imagen terminó en 0. La línea
de la zona fue `zone fs, denied, syscall, quota mem 32473, escape`:
la syscall 2 denegada devolvió -1 (`zone syscall denied`) y la cuota
también (`zone quota ok`). El resultado no se perdió.

## Sigue abierto: el comparando de `bits 16` pasa por `ebx`

v0.28.1 no escribe el booleano en `eax`, pero el otro lado de la
comparación sigue siendo `mov ebx, <valor>`. El token de INT 15h vive
en `ebx`. Este `while` lo pone a cero antes de `int 0x15`:

```
mov eax, 0
mov ebx, eax
pop edx
pop eax
cmp eax, ebx
je .L11
mov ecx, 24
mov edx, 1397571920
mov dword [di+20], 1
mov eax, 59424
int 21
```

Eso es `get_e820` sin `ebx_save`, compilado con este pin
(`copihue --flat 0x8200 -S`). La copia que está en `stage2.lap`
repone el token al entrar al cuerpo. Sin ella, el mismo kernel y
QEMU 8.2.2 imprimieron `e820 n=16`. Con la copia, el arranque corto y
`make qemu-test` imprimieron `e820 n=7`. El rodeo se queda.
