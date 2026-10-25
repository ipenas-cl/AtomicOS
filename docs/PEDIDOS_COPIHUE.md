# Pedidos a Copihue

Lapageria rosea es el nombre científico del copihue; antes AtomicOS.

Compilador fijado en el Makefile: v0.29.4, commit
`cac6fc27453c0136711e5de4ead703f836eff987`. El tag es ligero: `git
ls-remote` devuelve ese commit para `refs/tags/v0.29.4` y para
`HEAD`. `git -C build/copihue rev-parse HEAD` después del `make`
también, y `copihue --version` imprime `Copihue 0.29.4`.

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
línea de resumen y terminó en 0. El de v0.29.4 también: `zone fs,
denied, syscall, quota mem 32473, escape`, código 0.

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

## Cerrado en v0.29.4: un `+` fuera de la condición restaura `ebx`

En v0.29.2, `di = di + 24` fuera de un `if` o un `while` dejaba 24 en
`ebx`. Este pin hace `push ebx` antes del sumando y `pop ebx` después
del `add`. Esta función se compiló con `copihue --freestanding` de este
pin. No se arrancó:

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
push ebx
push eax
push edx
mov eax, 24
mov ebx, eax
pop edx
pop eax
add eax, ebx
pop ebx
mov di, ax
```

El `pop ebx` devuelve el 7. El changelog de v0.29.4 dice que `-`, `&`,
`|`, `^`, `*`, `/`, `%` y un desplazamiento hacen el mismo par (el
desplazamiento también guarda `ecx`), y que una comparación usada como
valor hace el `pop` después del `cmp` y antes del `sete`. Esas otras
formas no se compilaron en este árbol.

`get_e820` ya no copia el token. `ebx = 0` abre el mapa. Después de
`int 0x15` el conteo y `di = di + 24` restauran `ebx`, y `if (ebx == 0)`
mira el token. `copihue --flat 0x8200 -S` de este pin, sobre
`build/stage2.lap`, escribe esto entre el `int` y ese uso:

```
mov dword [di+20], 1
mov eax, 59424
int 21
```

```
movzx eax, word [32768]
push ebx
push eax
push edx
mov eax, 1
mov ebx, eax
pop edx
pop eax
add eax, ebx
pop ebx
mov word [32768], ax
movzx eax, di
push ebx
push eax
push edx
mov eax, 24
mov ebx, eax
pop edx
pop eax
add eax, ebx
pop ebx
mov di, ax
```

```
mov eax, ebx
push ebx
```

`[di+20]` y `[32768]` son un operando de memoria. El `pop ebx` del `+`
ocurre antes del store. `lgdt` de esta etapa es `lgdt [gdtr]`, después
de que `get_e820` vuelve.

En QEMU 8.2.2, `make qemu-test` de esta imagen terminó en 0 e imprimió
`e820 n=7` y `pmm free=32480 base=0x0000000000100000
end=0x0000000007fe0000`. El humo de 10 s
(`timeout 10 qemu-system-x86_64 -drive format=raw,file=build/lapageriaos.img
-display none -no-reboot -serial file:…`, sin disco de datos) salió
124 y la serie mostró `e820 n=7`, las siete entradas (la usable de
1 MB termina en `0x7fe0000`) y el mismo `pmm free=32480`. No fue 1 ni
16.

## Sigue abierto: un store de dirección no simple, y `lgdt` / `lidt` / `invlpg` sin etiqueta

Un store cuya dirección no es un número, un registro desnudo o
`reg±disp` sigue dejando esa dirección en `ebx`. `lgdt`, `lidt` e
`invlpg` de un puntero que no es una etiqueta de `data` mueven la
dirección a `bx` o a `ebx`. Una etiqueta (`lgdt(&gdtr)`) sigue siendo
`lgdt [gdtr]` y no toca `ebx`.

Estas funciones se compilaron con `copihue --freestanding` de este pin
y se ensamblaron con nasm. No se arrancaron:

```copihue
data gdtr {
    dw 39;
    dd 0;
}

bits 16 naked fn store_clobber() {
    ebx = 7;
    *((di + si) as *u16) = 1;
}

bits 16 naked fn lgdt_label() {
    ebx = 7;
    lgdt(&gdtr);
}

bits 16 naked fn lgdt_reg() {
    ebx = 7;
    lgdt(di);
}
```

```
store_clobber:
mov ebx, 7
movzx eax, di
push ebx
push eax
push edx
movzx eax, si
mov ebx, eax
pop edx
pop eax
add eax, ebx
pop ebx
push eax
mov eax, 1
pop ebx
mov word [ebx], ax
```

```
lgdt_label:
mov ebx, 7
lgdt [gdtr]
```

```
lgdt_reg:
mov ebx, 7
movzx eax, di
mov bx, ax
lgdt [bx]
```

El `+` de `di + si` restaura el 7. El store de después hace `pop ebx`
con la dirección y `mov word [ebx], ax`. `ebx` sale con esa dirección,
no con 7. `mov bx, ax` pisa la mitad baja del token. `lidt(di)` e
`invlpg(di)` salen igual (`lidt [bx]`, `invlpg [bx]`). En `bits 32`,
`*((edi + esi) as *u32) = 1` termina en `pop ebx` / `mov dword [ebx], eax`.

`get_e820` no usa esos casos entre el `int 0x15` y el `if (ebx == 0)`.
`*((di + 20) as *u32) = 1` es `[di+20]` y va antes del `int`. El conteo
escribe en `[32768]`. Los stores `kern_packet+N` sí pasan por `ebx`, y
están en `load_kernel`, antes de `get_e820`. `lgdt(&gdtr)` es la
etiqueta y va después. En `protected_mode` (`bits 32`) no hay
`int 0x15` ni un uso del token.
