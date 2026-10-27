#!/usr/bin/env bash
# Graba docs/lapageriaos-demo.mp4: make construye la imagen y QEMU
# arranca el shell. QEMU va sin ventana; las teclas entran por el
# monitor (sendkey) y la consola sale de screendump, como
# tests/qemu_shell_test.sh. El tramo de make se graba con Xvfb, xterm
# y ffmpeg. Los rótulos son drawtext. No usa Python.
#
# Uso: scripts/record_demo.sh [salida.mp4]
# Hacen falta make, nasm, qemu-system-x86_64, socat, ffmpeg, Xvfb y xterm.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:-"$ROOT/docs/lapageriaos-demo.mp4"}
FONT=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
FPS=10
MAKE_HOLD=8
INTRO_SECS=3
OUTRO_SECS=3.5
DISPLAY_NUM=98
GEOM_W=1280
GEOM_H=664
HEADER=56

need() {
    command -v "$1" >/dev/null 2>&1 || { echo "falta el comando: $1" >&2; exit 1; }
}
need make
need nasm
need qemu-system-x86_64
need socat
need ffmpeg
need ffprobe
need Xvfb
need xterm
[ -f "$FONT" ] || { echo "falta la fuente $FONT" >&2; exit 1; }

TMP=$(mktemp -d)
MON=$TMP/mon.sock
SERIAL=$TMP/serial.log
KEEP=0
QPID=""
FFPID=""
XPID=""
XTERM_PID=""
CUR=""
FI=0
SEC_N=0

stop_qemu() {
    if [ -n "${QPID:-}" ] && kill -0 "$QPID" 2>/dev/null; then
        echo quit | socat - UNIX-CONNECT:"$MON" >/dev/null 2>&1 || true
        local i
        for i in $(seq 50); do
            if ! kill -0 "$QPID" 2>/dev/null; then
                wait "$QPID" 2>/dev/null || true
                QPID=""
                return 0
            fi
            sleep 0.1
        done
        kill "$QPID" 2>/dev/null || true
        wait "$QPID" 2>/dev/null || true
        QPID=""
    fi
}
stop_display() {
    if [ -n "${FFPID:-}" ] && kill -0 "$FFPID" 2>/dev/null; then
        kill -INT "$FFPID" 2>/dev/null || true
        wait "$FFPID" 2>/dev/null || true
        FFPID=""
    fi
    if [ -n "${XTERM_PID:-}" ] && kill -0 "$XTERM_PID" 2>/dev/null; then
        kill "$XTERM_PID" 2>/dev/null || true
        wait "$XTERM_PID" 2>/dev/null || true
        XTERM_PID=""
    fi
    if [ -n "${XPID:-}" ] && kill -0 "$XPID" 2>/dev/null; then
        kill "$XPID" 2>/dev/null || true
        wait "$XPID" 2>/dev/null || true
        XPID=""
    fi
}
cleanup() {
    stop_qemu
    stop_display
    if [ "${OK_DONE:-0}" = 1 ]; then
        rm -rf "$TMP"
    else
        echo "se conserva $TMP" >&2
    fi
}
on_err() {
    KEEP=1
    echo "error en la línea $1" >&2
}
trap cleanup EXIT
trap 'on_err $LINENO' ERR

prompt_count() {
    local n
    n=$(grep -c "lapageria> " "$SERIAL" 2>/dev/null || true)
    echo "${n:-0}"
}
mon() { echo "$1" | socat - UNIX-CONNECT:"$MON" >/dev/null; }
typeline() {
    local s=$1 i c k
    for ((i = 0; i < ${#s}; i++)); do
        c=${s:i:1}
        case "$c" in
            " ") k=spc ;;
            "-") k=minus ;;
            *) k=$c ;;
        esac
        mon "sendkey $k"
        sleep 0.012
    done
    mon "sendkey ret"
    sleep 0.05
}
wait_prompt() {
    local before=$1 limit=$2 i n
    for i in $(seq "$limit"); do
        n=$(prompt_count)
        if [ "$n" -gt "$before" ]; then
            return 0
        fi
        sleep 0.1
    done
    return 1
}
# Busca needle en las líneas posteriores a la última vez que se tecleó cmd.
reply_has() {
    local cmd=$1 needle=$2
    tr -d '\r' < "$SERIAL" | awk -v c="lapageria> $cmd" -v n="$needle" '
        index($0, c) { seen = 1; hit = 0; next }
        seen && index($0, n) { hit = 1 }
        END { exit hit ? 0 : 1 }
    '
}
reply_unknown() {
    reply_has "$1" "unknown command"
}
wait_ppm() {
    local f=$1 i s1 s2
    for i in $(seq 40); do
        if [ -s "$f" ]; then
            s1=$(stat -c %s "$f")
            sleep 0.05
            s2=$(stat -c %s "$f")
            if [ "$s1" = "$s2" ] && [ "$s1" -gt 100000 ]; then
                return 0
            fi
        fi
        sleep 0.05
    done
    echo "screendump incompleto: $f" >&2
    return 1
}

begin() {
    local label=$1
    CUR=$(printf '%s/sec-%02d' "$TMP" "$SEC_N")
    SEC_N=$((SEC_N + 1))
    mkdir -p "$CUR"
    printf '%s' "$label" > "$CUR/label.txt"
    : > "$CUR/list"
    FI=0
}
snap() {
    local tenths=$1 ppm png
    ppm=$(printf '%s/%03d.ppm' "$CUR" "$FI")
    png=$(printf '%s/%03d.png' "$CUR" "$FI")
    FI=$((FI + 1))
    rm -f "$ppm" "$png"
    mon "screendump $ppm"
    wait_ppm "$ppm"
    # La ruta se arma aquí. No se relee de un manifiesto: un campo partido
    # perdía la barra inicial y ffmpeg buscaba un archivo que no existe.
    if ! ffmpeg -y -loglevel error -i "$ppm" "$png"; then
        echo "no pude leer el fotograma $ppm" >&2
        return 1
    fi
    rm -f "$ppm"
    echo "$tenths" >> "$CUR/list"
}
# Teclea, espera el prompt y, si la salida real contiene needle, guarda el fotograma.
cmd() {
    local line=$1 tenths=$2 needle=${3:-}
    local before
    before=$(prompt_count)
    typeline "$line"
    if ! wait_prompt "$before" 200; then
        echo "OMITIDO (sin prompt): $line" | tee -a "$TMP/omit.txt"
        return 0
    fi
    if reply_unknown "$line"; then
        echo "OMITIDO (no existe): $line" | tee -a "$TMP/omit.txt"
        return 0
    fi
    if [ -n "$needle" ] && ! reply_has "$line" "$needle"; then
        echo "OMITIDO (sin '$needle'): $line" | tee -a "$TMP/omit.txt"
        return 0
    fi
    echo "OK $line" >> "$TMP/ok.txt"
    snap "$tenths"
}

console_vf() {
    local label=$1
    # 720x400 cabe en 1196x664. La franja de arriba es el rótulo.
    printf '%s' \
        "scale=1196:664:flags=neighbor,setsar=1,pad=${GEOM_W}:720:42:${HEADER}:black,drawbox=x=0:y=0:w=${GEOM_W}:h=${HEADER}:color=0x102820:t=fill,drawtext=fontfile=${FONT}:textfile=${label}:fontsize=30:fontcolor=white:x=(w-text_w)/2:y=12"
}

encode_section() {
    local sec=$1 clip frames n=0 tenths count i src
    frames=$sec/frames
    rm -rf "$frames"
    mkdir -p "$frames"
    local fn=0
    while read -r tenths; do
        src=$(printf '%s/%03d.png' "$sec" "$n")
        if [ ! -f "$src" ]; then
            echo "falta el fotograma $src" >&2
            return 1
        fi
        count=$((tenths * FPS / 10))
        if [ "$count" -lt 1 ]; then count=1; fi
        for ((i = 0; i < count; i++)); do
            ln -f "$src" "$(printf '%s/%05d.png' "$frames" "$fn")"
            fn=$((fn + 1))
        done
        n=$((n + 1))
    done < "$sec/list"
    if [ "$fn" -lt 1 ]; then
        echo "sección vacía: $sec" >&2
        return 0
    fi
    clip=$(printf '%s/clip-%s.mp4' "$TMP" "$(basename "$sec")")
    ffmpeg -y -loglevel error -framerate "$FPS" -i "$frames/%05d.png" \
        -vf "$(console_vf "$sec/label.txt")" \
        -c:v libx264 -pix_fmt yuv420p -preset medium -crf 18 -r "$FPS" -g "$FPS" \
        -fps_mode cfr "$clip"
    echo "file $clip" >> "$TMP/concat.txt"
}

record_make() {
    echo "Grabando make..."
    cat > "$TMP/do-make.sh" << EOF
#!/bin/bash
cd "$ROOT"
rm -f build/lapageriaos.img build/boot.bin build/stage2.bin build/kernel.bin \\
    build/kernel.asm build/stage2.lap \\
    build/hola.bin build/hola.axe build/zona.bin build/zona.axe \\
    src/kernel/copihue/exec_img.lap src/kernel/copihue/zone_img.lap
make all
echo \$? > "$TMP/make.rc"
touch "$TMP/make.done"
EOF
    chmod +x "$TMP/do-make.sh"
    Xvfb ":$DISPLAY_NUM" -screen 0 ${GEOM_W}x${GEOM_H}x24 >/tmp/xvfb-demo.log 2>&1 &
    XPID=$!
    sleep 0.4
    export DISPLAY=:$DISPLAY_NUM
    ffmpeg -y -loglevel error -f x11grab -video_size ${GEOM_W}x${GEOM_H} -framerate "$FPS" \
        -i ":$DISPLAY_NUM" -c:v libx264 -pix_fmt yuv420p -preset medium -crf 18 \
        -r "$FPS" -g "$FPS" -fps_mode cfr "$TMP/make-raw.mp4" &
    FFPID=$!
    sleep 0.3
    xterm -fa 'DejaVu Sans Mono' -fs 20 +sb -bw 0 -bg black -fg '#e8eef7' \
        -geometry 104x26+8+4 -hold -e bash "$TMP/do-make.sh" &
    XTERM_PID=$!
    local i
    for i in $(seq 180); do
        [ -f "$TMP/make.done" ] && break
        sleep 0.25
    done
    if [ ! -f "$TMP/make.done" ]; then
        echo "make no terminó durante la grabación" >&2
        return 1
    fi
    local rc
    rc=$(cat "$TMP/make.rc")
    if [ "$rc" != 0 ]; then
        echo "make terminó con código $rc" >&2
        return 1
    fi
    sleep "$MAKE_HOLD"
    kill -INT "$FFPID" 2>/dev/null || true
    wait "$FFPID" 2>/dev/null || true
    FFPID=""
    kill "$XTERM_PID" 2>/dev/null || true
    wait "$XTERM_PID" 2>/dev/null || true
    XTERM_PID=""
    kill "$XPID" 2>/dev/null || true
    wait "$XPID" 2>/dev/null || true
    XPID=""
    printf '%s' "Construcción de la imagen" > "$TMP/make-label.txt"
    ffmpeg -y -loglevel error -i "$TMP/make-raw.mp4" \
        -vf "setsar=1,pad=${GEOM_W}:720:0:${HEADER}:black,drawbox=x=0:y=0:w=${GEOM_W}:h=${HEADER}:color=0x102820:t=fill,drawtext=fontfile=${FONT}:textfile=${TMP}/make-label.txt:fontsize=30:fontcolor=white:x=(w-text_w)/2:y=12" \
        -c:v libx264 -pix_fmt yuv420p -preset medium -crf 18 -r "$FPS" -g "$FPS" \
        -fps_mode cfr "$TMP/clip-make.mp4"
    echo "file $TMP/clip-make.mp4" >> "$TMP/concat.txt"
}

card() {
    local text=$1 secs=$2 name=$3 size=$4
    printf '%s' "$text" > "$TMP/$name.txt"
    ffmpeg -y -loglevel error -f lavfi -i "color=c=0x0c1410:s=${GEOM_W}x720:r=${FPS}:d=${secs}" \
        -vf "drawtext=fontfile=${FONT}:textfile=${TMP}/$name.txt:fontsize=${size}:fontcolor=white:x=(w-text_w)/2:y=(h-text_h)/2" \
        -c:v libx264 -pix_fmt yuv420p -preset medium -crf 18 -r "$FPS" -g "$FPS" \
        -fps_mode cfr -t "$secs" "$TMP/clip-$name.mp4"
}

cd "$ROOT"
: > "$TMP/concat.txt"
: > "$TMP/ok.txt"
: > "$TMP/omit.txt"

if [ ! -x "$ROOT/build/copihue/copihue" ]; then
    echo "Compilando Copihue antes de grabar; ese paso no entra en el video."
    make -C "$ROOT" build/copihue/copihue
fi

card "LapageriaOS" "$INTRO_SECS" intro 78
echo "file $TMP/clip-intro.mp4" >> "$TMP/concat.txt"
record_make

echo "Arrancando QEMU..."
dd if=/dev/zero of="$ROOT/build/data.img" bs=512 count=2048 status=none
: > "$SERIAL"
qemu-system-x86_64 -cpu qemu64 \
    -drive format=raw,file="$ROOT/build/lapageriaos.img" \
    -drive file="$ROOT/build/data.img",format=raw,if=ide,index=1 \
    -display none -no-reboot \
    -nic none -netdev user,id=n0 -device e1000,netdev=n0 \
    -serial file:"$SERIAL" -monitor unix:"$MON",server,nowait &
QPID=$!
sleep 0.3
if ! kill -0 "$QPID" 2>/dev/null; then
    echo "QEMU salió al arrancar" >&2
    exit 1
fi
if ! wait_prompt 0 150; then
    echo "no apareció el prompt" >&2
    tr -d '\r' < "$SERIAL" >&2 || true
    exit 1
fi
sleep 0.2

begin "Arranque"
snap 40

begin "Ayuda"
cmd clear 4
cmd help 65 "Commands:"

begin "Versión, ticks, IRQ, CR3, #PF y PMM"
cmd clear 4
cmd version 12 "LapageriaOS 0.8"
cmd ticks 12 "ticks "
cmd irqs 12 "kbd_irqs "
cmd cr3 18 "cr3="
cmd pf 20 "pf resumed"
cmd pmm 45 "pmm high ok"

begin "Bit NX"
cmd clear 4
cmd nx 50 "nx ok audit"

begin "Mitad alta"
cmd clear 4
cmd hh 50 "hh ok audit"

begin "Anillo 3"
cmd clear 4
cmd user 40 "user ok"

begin "Planificador"
cmd clear 4
cmd sched 55 "sched ok"

begin "Disco de datos"
cmd clear 4
cmd disk 40 "ata ok"

begin "Archivos y snapshot"
cmd clear 4
cmd "fs format" 12 "fs format ok"
cmd "fs create nota" 14 "fs create ok"
cmd "fs write nota v1" 14 "fs write ok"
cmd "fs cat nota" 16 "fs live v1"
cmd "fs snap s1" 14 "fs snap ok"
cmd "fs write nota v2" 14 "fs write ok"
cmd "fs cat nota" 16 "fs live v2"
cmd "fs snap s1 nota" 36 "fs snap v1"

begin "Programa del disco"
cmd clear 4
cmd "exec plant" 16 "exec plant ok"
cmd "exec hola" 45 "desde disco"

begin "Zonas"
cmd clear 4
cmd "zone plant" 14 "zone plant ok"
cmd "zone create a za 12" 14 "zone created"
cmd "zone create b zb 12" 16 "zone created"
cmd "zone ls" 28 "pre=zb"
cmd clear 4
cmd "zone exec a ztest" 55 "zone exec ok"

begin "Trazas"
cmd clear 4
cmd "trace ls" 42 "trace lost"
cmd clear 4
cmd "trace on sys-enter" 10 "sys-enter"
cmd "trace on sys-exit" 10 "sys-exit"
cmd "exec hola" 18 "desde disco"
cmd clear 4
cmd "trace count" 42 "trace count lost"

begin "Red: ARP y ping"
cmd clear 4
cmd net 50 "net ping ok"

stop_qemu
cp "$SERIAL" /tmp/lapageria-demo-serial.txt
cp "$TMP/ok.txt" /tmp/lapageria-demo-ok.txt
cp "$TMP/omit.txt" /tmp/lapageria-demo-omit.txt

echo "Codificando fotogramas..."
sec_i=0
while [ "$sec_i" -lt "$SEC_N" ]; do
    encode_section "$(printf '%s/sec-%02d' "$TMP" "$sec_i")"
    sec_i=$((sec_i + 1))
done

card "github.com/ipenas-cl/LapageriaOS" "$OUTRO_SECS" outro 42
echo "file $TMP/clip-outro.mp4" >> "$TMP/concat.txt"

mkdir -p "$(dirname "$OUT")"
ffmpeg -y -loglevel error -f concat -safe 0 -i "$TMP/concat.txt" \
    -c:v libx264 -pix_fmt yuv420p -preset medium -tune stillimage -crf 18 \
    -r "$FPS" -g "$FPS" -movflags +faststart "$OUT"

dur=$(ffprobe -v error -show_entries format=duration -of csv=p=0 "$OUT")
bytes=$(stat -c %s "$OUT")
echo "duración: ${dur}s"
echo "tamaño: ${bytes} bytes"
echo "--- comandos grabados ---"
cat "$TMP/ok.txt"
echo "--- omitidos ---"
cat "$TMP/omit.txt"
awk -v d="$dur" -v s="$bytes" 'BEGIN {
    if (d + 0 < 60 || d + 0 > 120) {
        print "duración fuera de 60..120" > "/dev/stderr"
        exit 1
    }
    if (s + 0 >= 12 * 1024 * 1024) {
        print "el video pasa de 12 MB" > "/dev/stderr"
        exit 1
    }
}'
echo "video: $OUT"
OK_DONE=1
