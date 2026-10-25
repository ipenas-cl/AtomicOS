#!/bin/bash
# Mismas comprobaciones que .github/workflows/ci.yml:
# la imagen se construye, sigue en pie a los 10 s y pasa el shell test.
# Para usarlo como gancho: cp scripts/pre-commit.sh .git/hooks/pre-commit
set -euo pipefail
cd "$(dirname "$0")/.."

make clean
make all

code=0
timeout 10 qemu-system-x86_64 \
    -drive format=raw,file=build/atomicos.img \
    -display none -no-reboot || code=$?
test "${code}" = "124"

make qemu-test
