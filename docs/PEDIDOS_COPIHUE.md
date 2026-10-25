# Pedidos a Copihue

Lapageria rosea es el nombre científico del copihue; antes AtomicOS.

Compilador fijado en el Makefile: v0.27.1, commit
`e7564b3782ab2cbabf966311c2e42147d4c19d1c`. El renombre se midió sobre
v0.26.1, commit `54dc238f943968e676040e67bdac206ce201bc7f`.

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

## Sigue abierto: `cld()`

El arranque lo necesita antes de `lodsb` y de `rep_stosd`. No hay
`asm()` ahí: `clear_df` lo apaga con `pushfd` / `popfd`.

```
error: unknown function 'cld'
```

`std()` tampoco está. No hace falta para este kernel.

## No hizo falta dejar `asm()`

`jmp resume_rip` sigue siendo `unknown jump target 'resume_rip'`.
`jmp rax` es `unknown jump target 'rax'`. `restore_context` salta
solo. El PIT cambia de proceso con `frame = (saved_ksp + 120) as
*InterruptFrame`. No quedó un caso del planificador fuera de ese
diseño.

## Cabecera Ethernet: el arreglo ya mide

En v0.26.1 esta sonda salía en 0 sin mensaje. `EthHdr` medía 4 bytes,
`etype` quedaba en el offset 2 y `e.dst[0] = 255` no almacenaba:

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

```
; Struct local: e: EthHdr (4 bytes)
mov rax, 2048
; Field assignment: e.etype (offset 2)
mov word [rbp-2], cx
mov rax, 255
mov rax, 0
```

Debería medir 14 bytes: destino en 0, origen en 6, ethertype en 12.
v0.27.0 cierra ese caso (`[T; N]` ocupa `N * sizeof(T)` y el store
escribe el byte). ARP e IPv4 eran el mismo clavo. `net.lap` todavía
escribe esas cabeceras con offsets sobre un buffer; el descriptor del
e1000 ya es un `packed struct` de escalares (16 bytes, campo
`volatile`). El formato de la trama no cambia al pasar los offsets a
structs.
