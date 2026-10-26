#!/bin/bash
# Mismas comprobaciones que .github/workflows/ci.yml:
# la imagen se construye, sigue en pie a los 10 s y pasa el shell test.
# Para usarlo como gancho: cp scripts/pre-commit.sh .git/hooks/pre-commit
set -euo pipefail
cd "$(dirname "$0")/.."

make clean
make all

code=0
# -cpu qemu64: el mismo modelo que CI y el Makefile (bit NX).
timeout 10 qemu-system-x86_64 -cpu qemu64 \
    -drive format=raw,file=build/lapageriaos.img \
    -display none -no-reboot || code=$?
test "${code}" = "124"

make qemu-test
