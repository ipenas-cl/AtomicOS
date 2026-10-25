#!/bin/bash
# Deja la versión MAYOR.MENOR en Makefile y VERSION después de comprobar
# que la imagen arranca. No crea commits, etiquetas ni hace push.
#
# Uso: scripts/release.sh 0.9
#
# La cadena "AtomicOS 0.8" que imprime el shell está en
# src/kernel/copihue/shell.lap. Este script no la modifica.
set -euo pipefail
cd "$(dirname "$0")/.."

version="${1:-}"
if [[ ! "$version" =~ ^[0-9]+\.[0-9]+$ ]]; then
    echo "Uso: scripts/release.sh MAYOR.MENOR" >&2
    echo "Ejemplo: scripts/release.sh 0.9" >&2
    exit 1
fi

make all
make qemu-test

tmp=$(mktemp)
sed "s/^VERSION = .*/VERSION = ${version}/" Makefile > "$tmp"
mv "$tmp" Makefile
printf '%s\n' "$version" > VERSION

date_stamp=$(date +%Y-%m-%d)
entry=$(mktemp)
{
    echo "# Changelog"
    echo ""
    echo "## ${version} - ${date_stamp}"
    echo ""
    echo "### Cambios"
    echo ""
    echo "- "
    echo ""
    tail -n +2 CHANGELOG.md
} > "$entry"
mv "$entry" CHANGELOG.md

echo "Versión ${version} escrita en Makefile y VERSION."
echo "Revisa CHANGELOG.md y crea el commit a mano."
echo "El banner del shell sigue en src/kernel/copihue/shell.lap."
