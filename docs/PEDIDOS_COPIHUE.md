# Pedidos a Copihue

Lapageria rosea es el nombre científico del copihue; antes AtomicOS.

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
