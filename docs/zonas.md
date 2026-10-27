# Zonas

Aislamiento liviano, al estilo de una jail: un proceso vive en una
zona y no sale de ahí. La zona 0 es la global. Ahí corre el shell.
Solo ella crea zonas y mete un programa en otra.

Hasta 8 zonas, contando la global. Cada una tiene:

| Campo | Qué es |
|---|---|
| id | 0 es la global. Las demás salen de `zone create` |
| nombre | hasta 8 letras o dígitos |
| tope de páginas | cuántos marcos de 4 KiB puede tener |
| máscara | syscalls permitidas. Lo que no está, se niega |
| prefijo | se antepone a cada nombre de archivo |
| tope de procesos | en una zona nueva son 2 |

El programa no elige el prefijo ni la zona. El kernel agrega el
prefijo y rechaza un nombre que no sea relativo (`/`, `.`, otro
prefijo, o un archivo que ya existe fuera de la zona).

## Syscalls

La máscara de una zona nueva deja pasar solo estas:

| Número | Qué hace |
|---|---|
| 1 | escribe un carácter |
| 5 | `exit` |
| 6 | escribe un archivo. El nombre está en `0x401800` y los datos en `0x401C00`. `rdi` es el largo |
| 7 | lee con el mismo convenio |
| 8 | pide una página. Si pasa el tope, devuelve -1 y no toma el marco |

Cualquier otra syscall de una zona distinta de 0 devuelve -1 y suma
`niega`. Cambiar de zona es la syscall 9: desde una zona distinta de
0 devuelve -2 y suma `fuga`, aunque alguien prenda ese bit. No crea
zonas y no toca el tope.

`exec` del shell hereda la zona 0. `zone exec` es el único camino que
pone un proceso en otra zona, y solo lo puede pedir la zona 0. Un
hijo usaría la zona del padre (`zone_for_new`).

## Cuota

El asignador no entrega un marco si la zona ya llegó a su tope. El
error vuelve al que pidió la página. Las otras zonas siguen igual.
Al hacer `exit` esos marcos vuelven al asignador global.

Si armar el proceso ya no cabe, `zone exec` imprime `zone fail 6` y
suelta lo que alcanzó a tomar. Si el tope de procesos está lleno,
`zone fail 4`.

## Comandos

Solo responden en la zona 0. Si no, `zone denied`.

```
zone create nombre prefijo paginas
zone ls
zone exec zona programa
zone plant
```

`zone plant` escribe el programa de prueba `ztest` con `fs_write`,
igual que `exec plant`. El directorio cabe 7 nombres; la prueba suelta
`malo` y `gordo` antes.

`zone ls` muestra `pre`, `cap`, `proc`, `pag`, `niega` y `fuga`.

## Errores del programa

| Código | Qué pasó |
|---|---|
| -1 | syscall fuera de la máscara, o página sobre el tope |
| -2 | quiso cambiar de zona |
| -12 | el nombre no pertenece a la zona |

## Prueba

`make qemu-test` crea las zonas `a` (prefijo `za`, 12 páginas) y `b`
(prefijo `zb`). `zone exec a ztest` tiene que imprimir, en orden,
`zone fs ok`, `zone fs denied`, `zone syscall denied`, `zone quota ok`
y `zone escape denied`. El archivo queda como `zan` (el programa pidió
`n`). `fs cat n` no lo encuentra. Las dos líneas `zone mem` tienen el
mismo número: la memoria libre global volvió al valor de antes. Si
falta una línea, el script termina en 1.
