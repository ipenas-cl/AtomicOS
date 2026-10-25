# Red mínima

El dispositivo es el e1000 de QEMU, el 82540EM (`8086:100e`; también
se acepta `8086:100f`). No es virtio-net.

virtio-net legacy pide la cola available/used, el header de 10 bytes
y la negociación de features. El e1000 se programa con registros MMIO
y descriptores de 16 bytes que están en el manual de Intel. Para esta
etapa eso alcanza, y se puede revisar en un volcado. El costo es más
CPU en polling; no hay IRQ de red. La máscara del PIC sigue en `0xFC`
(solo el PIT y el teclado).

## PCI

Mecanismo 1: `outl` a `0xCF8`, `inl` de `0xCFC`. Se recorre el bus 0.
El BAR0 es memoria de 32 bits y 128 KiB. Se apaga el decode, se mide
el BAR, y se reescribe en `0x10000000`.

Esa dirección está sobre los 128 MiB de RAM y dentro del primer GiB.
El PDPT del arranque solo tiene presente la entrada 0, y `map_page` no
crea un PDPT nuevo. `0x10000000` sí entra en ese GiB: una tabla de
páginas cubre los 32 marcos. Los bits de la PTE son presente, escritura,
PWT y PCD (`0x1B`), sin bit de usuario.

Después se prenden memoria y bus master en el comando PCI (el byte de
estado se escribe en 0 para no borrarlo). INTx queda apagado: no se
usa la IRQ.

## Anillos

Ocho descriptores de recepción y ocho de transmisión, más un buffer de
envío y ocho de recepción. Cada uno sale de `alloc_low_page`. Se
rechaza la página si no está alineada a 4 KiB, si pasa de 4 GiB, o si
no cae en la identidad de 2 MiB (ahí la CPU puede escribirla). El alto
del descriptor de 64 bits queda en 0. `RDLEN` y `TDLEN` son 128, que es
múltiplo de 128.

El descriptor es un `packed struct` de escalares (16 bytes). La trama
no: un arreglo adentro del struct no tiene ancho real. Ver
`docs/PEDIDOS_COPIHUE.md`.

## Protocolos

IP fija `10.0.2.15`. La puerta de la red de usuario es `10.0.2.2`.

- ARP who-has para esa IP se contesta. El comando también pregunta por
  la puerta.
- IPv4 lleva checksum de cabecera. Se tira el fragmento y el checksum
  malo.
- ICMP echo request a nuestra IP se contesta. El comando manda un echo
  a la puerta (id `0x4242`, seq 1, datos `ATOM`) y espera el reply.

No hay TCP ni UDP. Nada de esto escucha en el arranque: `net` prende el
chip y al salir apaga `RCTL` y `TCTL`.

El largo que manda el paquete no es el tope. El tope es el largo del
descriptor (o el de la prueba), y no puede pasar de 2048. Si el total
IPv4 no cabe ahí, o es menor que la cabecera, la trama se descarta y
se sigue.

QEMU, al escribir `RCTL`, deja de recibir durante un segundo de reloj
virtual y encola lo que llega. `net` espera 120 ticks (el PIT va a
100 Hz) antes de mandar el ARP. Las esperas usan `hlt` y tienen tope:
si no hay respuesta, se imprime el fallo y vuelve el prompt.

## Prueba

`make qemu-test` arranca con `-nic none -netdev user,id=n0 -device
e1000,netdev=n0`. Si el monitor no muestra esa red, el script falla
al tiro.

| Línea | Qué pasó |
|---|---|
| `net arp ok` | llegó el ARP reply de `10.0.2.2` |
| `net ping ok` | llegó el ICMP echo reply de la puerta |
| `net drop ok` | una trama con total IP de 200 y buffer de 48 se descartó |
| `net fail pci` | no apareció el 82540 |
| `net arp fail` | no hubo reply dentro del tope |
| `net ping fail` | el ARP anduvo y el echo no |
| `net drop fail` | el parser aceptó la trama mala |
