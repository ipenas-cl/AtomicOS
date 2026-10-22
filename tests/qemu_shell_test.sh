#!/usr/bin/env bash
# Boot AtomicOS in QEMU, type shell commands with the monitor's sendkey and
# check the replies on the serial console (the kernel mirrors VGA to COM1).
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

for _ in $(seq 100); do grep -q "atomicos> " "$SERIAL" 2>/dev/null && break; sleep 0.2; done
grep -q "atomicos> " "$SERIAL" || { echo "FAIL: no shell prompt"; cat "$SERIAL"; exit 1; }

CASES=("echo hello from copihue|hello from copihue" "add 40 2|42" "mul 6 -7|-42"
       "version|Copihue" "help|echo <text>" "nosuchcmd|unknown command")
for c in "${CASES[@]}"; do typeline "${c%%|*}"; done
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
[ $fails -eq 0 ] || { echo "--- serial ---"; cat "$SERIAL"; exit 1; }
