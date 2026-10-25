#!/bin/sh
# Arma un ejecutable plano AXE1 y el fuente Copihue que lo embebe.
# Uso: scripts/mkaxe.sh hola.bin hola.axe exec_img.lap
# Sin Python: printf, od y awk.
set -eu
if [ "$#" -ne 3 ]; then
    echo "usage: mkaxe.sh payload.bin salida.axe exec_img.lap" >&2
    exit 1
fi
bin=$1
axe=$2
lap=$3
size=$(wc -c < "$bin")
size=$((size))
if [ "$size" -lt 1 ] || [ "$size" -gt 4080 ]; then
    echo "mkaxe: payload de $size bytes; cabe entre 1 y 4080" >&2
    exit 1
fi
{
    # magic AXE1, tamaño u32, entrada u64 = 0x400000
    printf '\101\130\105\061'
    b0=$((size & 255))
    b1=$(((size >> 8) & 255))
    b2=$(((size >> 16) & 255))
    b3=$(((size >> 24) & 255))
    o0=$(printf '%03o' "$b0")
    o1=$(printf '%03o' "$b1")
    o2=$(printf '%03o' "$b2")
    o3=$(printf '%03o' "$b3")
    printf "\\${o0}\\${o1}\\${o2}\\${o3}"
    printf '\000\000\100\000\000\000\000\000'
    cat "$bin"
} > "$axe"
total=$(wc -c < "$axe")
total=$((total))
if [ "$total" -ne $((size + 16)) ]; then
    echo "mkaxe: el axe mide $total; esperaba $((size + 16))" >&2
    exit 1
fi
magic=$(od -An -tx1 -N4 "$axe" | tr -d ' \n')
if [ "$magic" != "41584531" ]; then
    echo "mkaxe: magic $magic, no es AXE1" >&2
    exit 1
fi
{
    echo "// Generado por scripts/mkaxe.sh. No editar."
    echo "fn exec_fill(p: *u8) -> i64 {"
    od -An -tu1 -v "$axe" | awk '
        {
            for (i = 1; i <= NF; i++) {
                printf "    p[%d] = %d;\n", n, $i
                n++
            }
        }
    '
    echo "    return $total;"
    echo "}"
} > "$lap"
assigns=$(grep -c 'p\[' "$lap" || true)
if [ "$assigns" -ne "$total" ]; then
    echo "mkaxe: quedaron $assigns bytes en el fuente; son $total" >&2
    exit 1
fi
echo "mkaxe: $axe ($total bytes, codigo $size, entrada 0x400000)"
