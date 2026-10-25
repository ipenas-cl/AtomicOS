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

# Two address spaces and the PIT round-robin. `sched` returns to the
# prompt; the spins are user-mode, so this waits longer than a shell command.
typeline sched
sched_ok=0
for _ in $(seq 200); do
    if tr -d '\r' < "$SERIAL" | grep -qx "sched ok"; then
        sched_ok=1
        break
    fi
    if tr -d '\r' < "$SERIAL" | grep -qx "sched fail"; then break; fi
    sleep 0.25
done
[ "$sched_ok" = 1 ] || fail "sched did not finish"
clean=$(tr -d '\r' < "$SERIAL")

pml4=$(printf '%s\n' "$clean" | sed -n 's/^pml4 a=\(0x[0-9a-f]*\) b=\(0x[0-9a-f]*\)$/\1 \2/p' | head -1)
half=$(printf '%s\n' "$clean" | sed -n 's/^half a=\(0x[0-9a-f]*\) b=\(0x[0-9a-f]*\)$/\1 \2/p' | head -1)
span=$(printf '%s\n' "$clean" | sed -n 's/^span a=\(0x[0-9a-f]*\) b=\(0x[0-9a-f]*\)$/\1 \2/p' | head -1)
low=$(printf '%s\n' "$clean" | sed -n 's/^low a=\(0x[0-9a-f]*\) b=\(0x[0-9a-f]*\)$/\1 \2/p' | head -1)
cr3a=$(printf '%s\n' "$clean" | sed -n 's/^cr3 a=\(0x[0-9a-f]*\)$/\1/p' | head -1)
cr3b=$(printf '%s\n' "$clean" | sed -n 's/^cr3 b=\(0x[0-9a-f]*\)$/\1/p' | head -1)
[ -n "$pml4" ] && [ -n "$half" ] && [ -n "$span" ] && [ -n "$low" ] || fail "missing page-table lines"
set -- $pml4; pa=$1; pb=$2
set -- $half; ha=$1; hb=$2
set -- $span; sa=$1; sb=$2
set -- $low; la=$1; lb=$2
[ "$pa" != "$pb" ] || fail "processes share one PML4 ($pa)"
[ "$pa" != "0x00001000" ] && [ "$pb" != "0x00001000" ] || fail "process PML4 is the bootloader table"
[ "$cr3a" = "$pa" ] || fail "write_cr3(A) read back $cr3a, PML4 is $pa"
[ "$cr3b" = "$pb" ] || fail "write_cr3(B) read back $cr3b, PML4 is $pb"
[ "$ha" = "$hb" ] && [ "$ha" = "$sa" ] && [ "$ha" = "$sb" ] || fail "kernel half is not one shared entry (half $ha $hb span $sa $sb)"
[ "$ha" != "0x00000000" ] || fail "kernel half entry is empty"
[ "$la" != "$lb" ] || fail "lower-half PML4[0] is not per process"
alias_n=$(printf '%s\n' "$clean" | grep -cx "alias ok")
[ "$alias_n" = 2 ] || fail "alias ok count=$alias_n, want 2 (shared kernel half not readable)"
echo "ok   per-process PML4, shared kernel half, write_cr3"

printf '%s\n' "$clean" | grep -qx "pf-cross R pid=2 cr2=0x00500000 err=0x00000004" || fail "process B read of A's page did not #PF"
printf '%s\n' "$clean" | grep -qx "pf-cross W pid=2 cr2=0x00500000 err=0x00000006" || fail "process B write of A's page did not #PF"
echo "ok   cross-process read and write #PF"

printf '%s\n' "$clean" | grep -qx "getpid 1" || fail "getpid 1 missing"
printf '%s\n' "$clean" | grep -qx "getpid 2" || fail "getpid 2 missing"
region=$(printf '%s\n' "$clean" | awk 'f&&/run end/{exit} /run begin/{f=1;next} f{print}')
printf '%s\n' "$region" | grep -q "M" || fail "process A did not print M from its private page"
ab=$(printf '%s\n' "$region" | awk 'BEGIN{s=""} {for(i=1;i<=length($0);i++){c=substr($0,i,1); if(c=="a"||c=="b") s=s c}} END{print s}')
case "$ab" in
    *a*b*a*|*b*a*b*) ;;
    *) fail "user letters not interleaved (got '$ab')" ;;
esac
echo "ok   A/B interleaved ($ab)"

sw=$(printf '%s\n' "$clean" | sed -n 's/^switch timer=\([0-9][0-9]*\) yield=\([0-9][0-9]*\)$/\1 \2/p' | head -1)
[ -n "$sw" ] || fail "no switch line"
set -- $sw; st=$1; sy=$2
[ "$st" -ge 1 ] || fail "timer switches=$st, want >= 1"
[ "$sy" -ge 1 ] || fail "yield switches=$sy, want >= 1"
echo "ok   round-robin timer=$st yield=$sy"

ex1=$(printf '%s\n' "$clean" | sed -n 's/^exit pid=1 freed=\([0-9][0-9]*\)$/\1/p' | head -1)
ex2=$(printf '%s\n' "$clean" | sed -n 's/^exit pid=2 freed=\([0-9][0-9]*\)$/\1/p' | head -1)
rel=$(printf '%s\n' "$clean" | sed -n 's/^sched released=\([0-9][0-9]*\)$/\1/p' | head -1)
bb=$(printf '%s\n' "$clean" | sed -n 's/^sched base=\([0-9][0-9]*\) back=\([0-9][0-9]*\)$/\1 \2/p' | head -1)
[ -n "$ex1" ] && [ -n "$ex2" ] && [ -n "$rel" ] && [ -n "$bb" ] || fail "missing exit accounting"
set -- $bb; base=$1; back=$2
[ "$ex1" -ge 1 ] && [ "$ex2" -ge 1 ] || fail "exit freed pid1=$ex1 pid2=$ex2"
[ "$rel" -eq $((ex1 + ex2)) ] || fail "released=$rel, want pid1+pid2=$((ex1 + ex2))"
[ "$back" -eq "$base" ] || fail "pmm free back=$back, base=$base (exit did not return every page)"
printf '%s\n' "$clean" | grep -qx "half kept" || fail "shared kernel half was freed"
printf '%s\n' "$clean" | grep -qx "pml4 free" || fail "process PML4 pages were not freed"
echo "ok   exit freed $ex1+$ex2 pages, pmm back to $base"

printf '%s\n' "$clean" | grep -qx "user leak" && fail "ring 3 read of kernel memory succeeded"
echo "ok   sched ok"

typeline ud2
sleep 0.5
if tr -d '\r' < "$SERIAL" | grep -q "exception: vector 6 #UD invalid opcode"; then
    echo "ok   ud2 -> #UD"
else
    fail "no #UD exception message"
fi
