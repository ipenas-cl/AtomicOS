# Trazas mínimas

Once sondas estáticas, un anillo fijo en memoria y cinco subcomandos
de shell. No hay asignación en el camino de la sonda y no se agregó
`asm()`.

## Sondas

Apagadas al arranque. El llamador hace un solo chequeo del indicador;
si está en cero no llama. El contador sube solo cuando la sonda está
prendida y el evento entra al anillo.

| Id | Nombre | Dónde | Argumentos |
|---|---|---|---|
| 0 | `sys-enter` | entrada de `sys_user` | número, argumento |
| 1 | `sys-exit` | vuelta de `sys_user` | número, resultado |
| 2 | `switch` | `switch_to` | pid que sale, pid que entra |
| 3 | `pf` | `#PF` | CR2, código de error |
| 4 | `pit` | tick del PIT | tick, 0 |
| 5 | `pg-alloc` | página entregada | dirección física, 0 |
| 6 | `pg-free` | página devuelta | dirección física, 0 |
| 7 | `blk-read` | lectura de bloque | número de bloque, código |
| 8 | `blk-write` | escritura de bloque | número de bloque, código |
| 9 | `net-rx` | descriptor de recepción listo | largo, error |
| 10 | `net-tx` | paquete entregado al e1000 | largo, estado |

`yield` y `exit` no vuelven al epílogo del syscall, así que queda la
entrada y no la salida. `pit` anota un evento por tick mientras está
prendida: es el reloj de muestreo. El proceso del registro es
`proc_pid` del que corre; en el shell, antes de `exec` o `sched`, es 0.
En `switch` es el pid que deja la CPU.

## Anillo

32 registros, potencia de 2. Cada uno es un struct de cinco `i64`:
tick, id, proceso y dos argumentos. Vive en la `.bss`. Al llenarse
pisa el más viejo y `tr_lost` cuenta ese piso. Un anidamiento (la
sonda ya estaba tomada) también suma un perdido y no escribe.

El registro guarda los flags con `pushfq` / `cli` / `pop` y los
devuelve con `popfq`. No hace `sti()` a ciegas: el syscall entra con
IF en 0 y el PIT también. Si la pila de flags (cuatro niveles) se
llena, el evento se cuenta perdido y se sale. No hay ciclo de espera.

## Comandos

```
trace ls
trace on sys-enter
trace off sys-enter
trace dump
trace dump 8
trace count
trace clear
```

`ls` muestra id, nombre, on/off y contador, y al final los perdidos.
`dump` sin número muestra lo que hay, del más viejo al más nuevo.
`dump n` muestra los últimos n. `count` es el resumen por sonda.
`clear` vacía el anillo, los perdidos y los contadores; no toca el
indicador.

El costo de una sonda apagada se midió en QEMU 8.2.2, con el PIT a
100 Hz. Ocho millones de veces el chequeo `tr_en[4] != 0` (la sonda
`pit` apagada, sin entrar a `trace_hit`) tardaron 6 o 7 ticks. El
mismo lazo, sin ese chequeo, tardó 4 ticks en las cuatro corridas. La
diferencia es 2 o 3 ticks (20 a 30 ms en total), del orden de 3 ns por
chequeo. El tick mide 10 ms: el número no es más fino que eso.

`trace dump` imprime `trace begin`, una línea `t <tick> <nombre> pid <pid> a <n> b <n>`
por evento y `trace end n <cuántos> lost <perdidos>`.

## Prueba

`make qemu-test` deja estas líneas, y falla si falta alguna:

| Línea | Qué pasó |
|---|---|
| `trace counters ok` | el contador de `pf` coincide con las `#PF` que imprimió `sched`, y el de `switch` con `timer` + `yield` |
| `trace syscall ok` | `exec hola` (pid 1) dejó `sys-enter` / `sys-exit` de «desde disco» en orden, y la entrada de `exit` |
| `trace off ok` | con las dos sondas apagadas, otro `exec hola` no graba |
| `trace overflow ok` | `pit` llena el anillo, `lost` sube y el shell sigue |
