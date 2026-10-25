#!/usr/bin/env bash
# Boot AtomicOS in QEMU, type shell commands with the monitor's sendkey and
# check the replies on the serial console (the kernel mirrors VGA to COM1).
# Also checks that the PIT counter advances by 100 ticks in about one second,
# that a later `ticks` read is larger, that IRQ1 delivered keystrokes, and
# that a deliberate #UD prints its exception line.
# Usage: tests/qemu_shell_test.sh [image] [--screenshot out.png]
# Needs: qemu-system-x86_64, socat; ffmpeg only for --screenshot.
set -u
IMAGE=${1:-build/atomicos.img}
SHOT=""; [ "${2:-}" = "--screenshot" ] && SHOT=$3
TMP=$(mktemp -d); MON=$TMP/mon.sock; SERIAL=$TMP/serial.log
qemu-system-x86_64 -drive format=raw,file="$IMAGE" -display none -no-reboot \
    -serial file:"$SERIAL" -monitor unix:"$MON",server,nowait &
QPID=$!
trap 'kill $QPID 2>/dev/null; rm -rf "$TMP"' EXIT

mon() { echo "$1" | socat - UNIX-CONNECT:"$MON" >/dev/null; }
typeline() {
    local s=$1 i c k
    for ((i = 0; i < ${#s}; i++)); do
        c=${s:i:1}
        case "$c" in " ") k=spc ;; "-") k=minus ;; *) k=$c ;; esac
        mon "sendkey $k"
    done
    mon "sendkey ret"
    sleep 0.3
}
fail() { echo "FAIL: $*"; echo "--- serial ---"; tr -d '\r' < "$SERIAL" 2>/dev/null || true; exit 1; }

# The kernel prints ticks once the counter reaches 100, then again after 100
# more interrupts. Stamp each line so the gap can be compared to 1 second.
t1=0
for _ in $(seq 200); do
    if grep -q "timer: ticks=" "$SERIAL" 2>/dev/null; then t1=$(date +%s%N); break; fi
    sleep 0.1
done
[ "$t1" != 0 ] || fail "no PIT sample on serial"
t2=0
for _ in $(seq 80); do
    n=$(grep -c "timer: ticks=" "$SERIAL" || true)
    if [ "${n:-0}" -ge 2 ]; then t2=$(date +%s%N); break; fi
    sleep 0.1
done
[ "$t2" != 0 ] || fail "PIT did not publish a second sample"
sleep 0.1
samples=$(tr -d '\r' < "$SERIAL" | sed -n 's/^timer: ticks=\([0-9][0-9]*\)$/\1/p')
first=$(printf '%s\n' "$samples" | sed -n '1p')
second=$(printf '%s\n' "$samples" | sed -n '2p')
[ -n "$first" ] && [ -n "$second" ] || fail "could not parse timer samples"
[ "$first" -ge 100 ] || fail "first ticks=$first, want >= 100"
[ "$second" -ge $((first + 100)) ] || fail "second ticks=$second, first=$first, want second >= first+100"
dt_ms=$(( (t2 - t1) / 1000000 ))
echo "ok   ticks $first -> $second in ${dt_ms} ms"
# 100 ticks at 100 Hz is 1 s. The legacy 18.2 Hz PIT would take ~5.5 s.
if [ "$dt_ms" -lt 400 ] || [ "$dt_ms" -gt 4000 ]; then
    fail "100 ticks took ${dt_ms} ms, want 400..4000 (PIT not ~100 Hz)"
fi

for _ in $(seq 100); do grep -q "atomicos> " "$SERIAL" 2>/dev/null && break; sleep 0.2; done
grep -q "atomicos> " "$SERIAL" || fail "no shell prompt"

CASES=("echo hello from copihue|hello from copihue" "add 40 2|42" "mul 6 -7|-42"
       "version|Copihue" "help|echo <text>" "nosuchcmd|unknown command")
for c in "${CASES[@]}"; do typeline "${c%%|*}"; done
typeline ticks
typeline irqs
sleep 1
if [ -n "$SHOT" ]; then
    mon "screendump $TMP/s.ppm"; sleep 1
    ffmpeg -loglevel error -y -i "$TMP/s.ppm" "$SHOT" && echo "screenshot: $SHOT"
fi

fails=0
for c in "${CASES[@]}"; do
    cmd=${c%%|*}; want=${c#*|}
    # the reply must appear after the echoed command line
    if awk -v c="atomicos> $cmd" -v w="$want" 'f && index($0, w) {ok=1; exit} index($0, c) {f=1} END {exit !ok}' "$SERIAL"; then
        echo "ok   '$cmd' -> '$want'"
    else
        echo "FAIL '$cmd' -> '$want'"; fails=$((fails + 1))
    fi
done
[ $fails -eq 0 ] || fail "shell cases"

shell_ticks=$(tr -d '\r' < "$SERIAL" | sed -n 's/^ticks \([0-9][0-9]*\)$/\1/p' | tail -1)
[ -n "${shell_ticks:-}" ] || fail "no ticks command reply"
[ "$shell_ticks" -gt "$second" ] || fail "ticks command=$shell_ticks, not greater than boot sample $second"
echo "ok   ticks still growing ($second -> $shell_ticks)"

kbd=$(tr -d '\r' < "$SERIAL" | sed -n 's/^kbd_irqs \([0-9][0-9]*\)$/\1/p' | tail -1)
[ -n "${kbd:-}" ] || fail "no irqs command reply"
[ "$kbd" -ge 8 ] || fail "kbd_irqs=$kbd, want >= 8 (keyboard IRQ not delivering keys)"
echo "ok   keyboard IRQ count=$kbd"

typeline ud2
sleep 0.5
if tr -d '\r' < "$SERIAL" | grep -q "exception: vector 6 #UD invalid opcode"; then
    echo "ok   ud2 -> #UD"
else
    fail "no #UD exception message"
fi
