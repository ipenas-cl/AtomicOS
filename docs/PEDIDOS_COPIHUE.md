# Pedidos a Copihue

Lapageria rosea es el nombre científico del copihue; antes AtomicOS.

Compilador fijado en el Makefile: v0.27.3, commit
`333f74ae8c82fd679232d754920e335e012483c7`.

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

`syscall(n, a, ...)` existe en `--flat`. El programa de la zona
sigue con `syscall()` sin argumentos: el resultado queda en `rax` y
el caso de abajo lo pisa igual.

## Sigue abierto: `rax` no sobrevive a un `if` ni a un `while`

No hay error: `copihue` termina en 0. La condición deja `rax` en 0 o
en 1 (`sete` / `movzx`). Un `while (rax == 0)` que sale porque `rax`
era -1 sigue con `rax == 0`. Un store de constante (`*(p as *u8) = 0`)
también es `mov rax, 0` y pisa el valor de la syscall de arriba.

Esto es lo que tendría que conservar el -1 de la syscall 8:

```copihue
rax = 8;
syscall();
while (rax == 0) {
    rax = 8;
    syscall();
}
if (rax == 0 - 1) { emit(33); }
```

Con v0.27.3 (`copihue --flat 0x400000 -S`, commit
`333f74ae8c82fd679232d754920e335e012483c7`) la salida del `while` es:

```
syscall
.L0:
    push rax
    mov rax, 0
    mov rbx, rax
    pop rax
    cmp rax, rbx
    sete al
    movzx rax, al
    test rax, rax
    jz .L1
```

Cuando la syscall devuelve -1, el salto sale con `rax == 0` y el
`if` de abajo no ve el -1. El store de constante, en la misma
corrida, es:

```
syscall
mov rax, 0
mov byte [4200200], al
```

`0x401708` queda en cero y `rax` también, antes del `if`. El programa
de la zona guarda el resultado en `0x401720` antes de comparar.
`src/user/zona.lap` no puede soltar ese rodeo.
