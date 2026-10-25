# Pedidos a Copihue

Compilador fijado en el Makefile: v0.27.1, commit
`e7564b3782ab2cbabf966311c2e42147d4c19d1c`.

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
