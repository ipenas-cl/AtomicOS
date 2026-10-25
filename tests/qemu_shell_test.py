#!/usr/bin/env python3
"""Boot AtomicOS in QEMU, type shell commands with the monitor's sendkey,
and check the replies on the serial console (the kernel mirrors VGA
output to COM1). Optional: --screenshot PATH saves a PNG of the screen."""
import os, socket, subprocess, sys, tempfile, time

IMAGE = sys.argv[1] if len(sys.argv) > 1 and not sys.argv[1].startswith("--") else "build/atomicos.img"
SHOT = sys.argv[sys.argv.index("--screenshot") + 1] if "--screenshot" in sys.argv else None

KEYS = {" ": "spc", "\n": "ret", "-": "minus", "'": "apostrophe"}
CASES = [
    ("echo hello from copihue", "hello from copihue"),
    ("add 40 2", "42"),
    ("mul 6 -7", "-42"),
    ("version", "Copihue"),
    ("help", "echo"),
    ("nosuchcmd", "unknown command"),
]

def main():
    tmp = tempfile.mkdtemp()
    mon, serial = os.path.join(tmp, "mon.sock"), os.path.join(tmp, "serial.log")
    qemu = subprocess.Popen(["qemu-system-x86_64", "-drive", f"format=raw,file={IMAGE}",
        "-display", "none", "-no-reboot", "-serial", f"file:{serial}",
        "-monitor", f"unix:{mon},server,nowait"])
    try:
        def log():
            return open(serial, errors="replace").read() if os.path.exists(serial) else ""
        deadline = time.time() + 20
        while "atomicos> " not in log():
            if time.time() > deadline or qemu.poll() is not None:
                print("FAIL: no shell prompt\n" + log()); return 1
            time.sleep(0.2)
        s = socket.socket(socket.AF_UNIX); s.connect(mon); s.settimeout(0.05)
        def cmd(c):
            s.send((c + "\n").encode()); time.sleep(0.08)
            try: s.recv(65536)
            except socket.timeout: pass
        for line, _ in CASES:
            for ch in line + "\n":
                cmd("sendkey " + KEYS.get(ch, ch))
            time.sleep(0.3)
        time.sleep(1)
        if SHOT:
            ppm = os.path.join(tmp, "screen.ppm"); cmd("screendump " + ppm); time.sleep(1)
            from PIL import Image
            Image.open(ppm).save(SHOT); print("screenshot:", SHOT)
        out, fails = log(), 0
        pos = 0
        for line, want in CASES:
            i = out.find("atomicos> " + line, pos)
            j = out.find(want, i + len(line) + 10) if i >= 0 else -1
            ok = i >= 0 and j >= 0
            print(("ok   " if ok else "FAIL ") + f"{line!r} -> {want!r}")
            fails += not ok
            if ok: pos = i + 1
        if fails: print("--- serial ---\n" + out)
        return 1 if fails else 0
    finally:
        qemu.kill()

sys.exit(main())
