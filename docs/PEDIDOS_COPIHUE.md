# Pedidos a Copihue

Compilador fijado en el Makefile: v0.26.1, commit
`54dc238f943968e676040e67bdac206ce201bc7f`.

## Arreglo dentro de una struct

Un `struct` sin `packed` guarda cada campo escalar en 8 bytes. Un
`packed struct` sí usa el ancho de `u8`, `u16` y `u32`, pero un campo
que es un arreglo no. El parser se queda con el tipo base y tira el
largo: `[u8; 16]` ocupa 1 byte y `[u32; 8]` ocupa 4. No hay mensaje de
error. `copihue` termina en 0.

Esto es lo que habría descrito el directorio:

```copihue
packed struct FsEnt {
    name: [u8; 16],
    size: u32,
    nblocks: u32,
    blocks: [u32; 8]
}

fn fs_probe() -> i64 {
    let e: FsEnt;
    e.size = 2;
    e.nblocks = 1;
    e.blocks[0] = 4;
    e.name[0] = 65;
    return e.size;
}
```

Esa entrada debería medir 56 bytes: nombre en 0, tamaño en 16,
cantidad de bloques en 20 y punteros en 24. Con v0.26.1 el ensamblado
(`copihue --flat 0x10000 -S`) dice lo mismo que v0.25.0:

```
; Struct local: e: FsEnt (13 bytes)
; Field assignment: e.size (offset 1)
    mov dword [rbp-12], ecx
; Field assignment: e.nblocks (offset 5)
    mov dword [rbp-8], ecx
    mov rax, 4
    push rax
    mov rax, 0
    mov rax, 65
    push rax
    mov rax, 0
```

`e.size` quedó en el offset 1, justo después de un solo byte de
nombre. `e.name[0] = 65` no genera un store.

El sistema de archivos anota el sector con offsets sobre un buffer
`[u8; 512]`, el mismo criterio que el sector de metadatos de `blk`.
Cuando un arreglo adentro de un `packed struct` mida `N` por el ancho
del elemento, y `e.name[i] = c` guarde el byte, estas entradas pueden
pasar a ser structs.

## Cabecera Ethernet

La misma falla, ahora con una trama. No hay mensaje: `copihue` termina
en 0. Esto es lo que habría descrito la cabecera:

```copihue
packed struct EthHdr {
    dst: [u8; 6],
    src: [u8; 6],
    etype: u16
}

fn probe() -> i64 {
    let e: EthHdr;
    e.etype = 0x0800;
    e.dst[0] = 255;
    return e.etype;
}
```

Debería medir 14 bytes: destino en 0, origen en 6, ethertype en 12.
Con v0.26.1 (`copihue --flat 0x10000 -S`) el ensamblado dice:

```
; Struct local: e: EthHdr (4 bytes)
mov rax, 2048
; Field assignment: e.etype (offset 2)
mov word [rbp-2], cx
mov rax, 255
mov rax, 0
```

`etype` quedó en el offset 2, detrás de un byte por arreglo. `e.dst[0]
= 255` carga 255 y lo tira: no hay store.

ARP e IPv4 pisan el mismo clavo (direcciones de 6 y 4 bytes). La red
las escribe con offsets sobre un buffer. Un `packed struct` de solo
escalares sí mide bien: el descriptor del e1000 (`u64`, `u16`, `u8`,
16 bytes, campo `volatile`) ensambla el ancho real y por eso el anillo
sí es un struct.

## `rax` no sobrevive a un `if` ni a un `while`

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

Con v0.26.1 (`copihue --flat 0x400000 -S`) la salida del `while` es:

```
push rax
mov rax, 0
mov rbx, rax
pop rax
cmp rax, rbx
sete al
movzx rax, al
test rax, rax
jz .L17
```

Cuando la syscall devuelve -1, el salto sale con `rax == 0` y el
`if` de abajo no ve el -1. Lo mismo pasa si entre la syscall y el
`if` hay `*(0x401708 as *u8) = 0`. El programa de la zona guarda el
resultado en memoria antes de comparar.
