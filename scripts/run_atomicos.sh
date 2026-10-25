#!/bin/bash
# Arranca AtomicOS en QEMU. Atajo de `make qemu` (curses, teclado PS/2).
set -euo pipefail
cd "$(dirname "$0")/.."
exec make qemu
