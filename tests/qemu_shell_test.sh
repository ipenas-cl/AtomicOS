#!/usr/bin/env bash
# Boot AtomicOS in QEMU, type shell commands with the monitor's sendkey and
# check the replies on the serial console (the kernel mirrors VGA to COM1).
# Also checks that the PIT counter advances by 100 ticks in about one second,
# that a later `ticks` read is larger, that IRQ1 delivered keystrokes, and
# that a deliberate #UD prints its exception line, that read_cr3 matches
# the bootloader page table, and that a read of unmapped 0x200000 prints
# that address from CR2 and then the loaded byte 0x00000000.
# Also checks the E820 map, alloc/free, a mapped page at or above 2MB,
# and a ring-3 program that syscalls "user ok" and then #PF on a kernel read.
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
typeline cr3
typeline pf
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

clean=$(tr -d '\r' < "$SERIAL")
# boot64.asm writes CR3=0x1000, PML4[0]=0x2003, PDPT[0]=0x3003.
# The CPU sets the accessed bit (bit 5) on the entries it walks, so they
# read back as 0x2023 and 0x3023. CR3 itself stays 0x1000.
printf '%s\n' "$clean" | grep -qx "cr3=0x00001000" || fail "read_cr3 is not the bootloader PML4 at 0x1000"
printf '%s\n' "$clean" | grep -qx "pml4=0x00002023" || fail "PML4[0] is not 0x2003 with the accessed bit"
printf '%s\n' "$clean" | grep -qx "pdpt=0x00003023" || fail "PDPT[0] is not 0x3003 with the accessed bit"
echo "ok   cr3=0x1000 pml4=0x2023 pdpt=0x3023"

if printf '%s\n' "$clean" | grep -qx "exception: vector 14 #PF page fault err=0x00000000 cr2=0x00200000"; then
    echo "ok   pf -> cr2=0x200000"
else
    fail "page fault did not report cr2=0x200000"
fi
# `mul 6 -7` runs before `pf`. Both blocks bind `b`. The fault allocates
# a frame and zeros it, so the resumed line is `pf resumed 0x00000000`.
# Before block scope, the second `b` printed the mul operand -7 as 0xfffffff9.
mul_at=$(printf '%s\n' "$clean" | grep -n "^-42$" | head -1 | cut -d: -f1)
pf_line=$(printf '%s\n' "$clean" | grep -n "^pf resumed " | head -1)
pf_at=$(printf '%s\n' "$pf_line" | cut -d: -f1)
pf_byte=$(printf '%s\n' "$pf_line" | sed -n 's/^[0-9]*:pf resumed //p')
[ -n "$mul_at" ] && [ -n "$pf_at" ] && [ "$pf_at" -gt "$mul_at" ] || fail "pf did not run after mul 6 -7"
[ "$pf_byte" = "0x00000000" ] || fail "pf resumed printed '$pf_byte', want the byte 0 at 0x200000 (not 0xfffffff9)"
echo "ok   pf resumed 0x00000000 after mul 6 -7"

# E820 and the page allocator. The exact entry count depends on the
# firmware; QEMU's default 128MB map has usable RAM above 1MB, which is
# more than the 256-page identity window.
en=$(tr -d '\r' < "$SERIAL" | sed -n 's/^e820 n=\([0-9][0-9]*\)$/\1/p' | head -1)
[ -n "${en:-}" ] || fail "no e820 count"
[ "$en" -ge 1 ] || fail "e820 n=$en"
echo "ok   e820 n=$en"
pfree=$(tr -d '\r' < "$SERIAL" | sed -n 's/^pmm free=\([0-9][0-9]*\) base=\(0x[0-9a-f]*\) end=\(0x[0-9a-f]*\)$/\1/p' | head -1)
pbase=$(tr -d '\r' < "$SERIAL" | sed -n 's/^pmm free=\([0-9][0-9]*\) base=\(0x[0-9a-f]*\) end=\(0x[0-9a-f]*\)$/\2/p' | head -1)
pend=$(tr -d '\r' < "$SERIAL" | sed -n 's/^pmm free=\([0-9][0-9]*\) base=\(0x[0-9a-f]*\) end=\(0x[0-9a-f]*\)$/\3/p' | head -1)
[ -n "${pfree:-}" ] && [ -n "${pbase:-}" ] && [ -n "${pend:-}" ] || fail "no pmm free line"
[ "$pfree" -gt 256 ] || fail "pmm free=$pfree, want > 256 (E820 did not add RAM above 1MB)"
[ "$pbase" = "0x0000000000100000" ] || fail "pmm base=$pbase, want 0x0000000000100000"
[ $((pend)) -gt $((0x200000)) ] || fail "pmm end=$pend, want above 2MB"
echo "ok   pmm free=$pfree base=$pbase end=$pend"
tr -d '\r' < "$SERIAL" | grep -qx "gdt load" || fail "gdt was not loaded"
tr -d '\r' < "$SERIAL" | grep -qx "gdt ok" || fail "ltr/syscall setup did not finish"
echo "ok   gdt ok"

typeline pmm
sleep 0.4
typeline user
sleep 0.4
clean=$(tr -d '\r' < "$SERIAL")
printf '%s\n' "$clean" | grep -qx "pmm ok" || fail "pmm alloc/free test did not pass"
high=$(printf '%s\n' "$clean" | sed -n 's/^pmm high ok \(0x[0-9a-f]*\)$/\1/p' | head -1)
[ -n "${high:-}" ] || fail "no pmm high ok line"
[ $((high)) -ge $((0x200000)) ] || fail "pmm high page $high is below 2MB"
echo "ok   pmm ok, high page $high"
printf '%s\n' "$clean" | grep -qx "user ok" || fail "ring 3 syscall did not print user ok"
printf '%s\n' "$clean" | grep -qx "exception: vector 14 #PF page fault err=0x00000005 cr2=0x00010000" || fail "ring 3 kernel read did not #PF"
printf '%s\n' "$clean" | grep -qx "user blocked" || fail "user #PF was not reported"
printf '%s\n' "$clean" | grep -qx "user returned" || fail "shell did not resume after user #PF"
printf '%s\n' "$clean" | grep -qx "user leak" && fail "ring 3 read of kernel memory succeeded"
if ! printf '%s\n' "$clean" | awk '
    /^user ok$/ { if (!a) a=NR }
    /^exception: vector 14 #PF page fault err=0x00000005 cr2=0x00010000$/ { if (a && !b) b=NR }
    /^user blocked$/ { if (b && !c) c=NR }
    /^user returned$/ { if (c && !d) d=NR }
    END { exit !(a && b && c && d && a<b && b<c && c<d) }
'; then
    fail "user ok / #PF / user blocked / user returned were out of order"
fi
echo "ok   user syscall, #PF on kernel read, returned to shell"

typeline ud2
sleep 0.5
if tr -d '\r' < "$SERIAL" | grep -q "exception: vector 6 #UD invalid opcode"; then
    echo "ok   ud2 -> #UD"
else
    fail "no #UD exception message"
fi
