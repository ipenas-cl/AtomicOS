#!/bin/bash
# Script to add/update copyright headers in source files

YEAR=2025
AUTHOR="Ignacio Peña Sepúlveda"

# C/C++ header
C_HEADER="/*
 * Copyright (c) $YEAR $AUTHOR
 * Part of AtomicOS Project - https://github.com/ipenas-cl/AtomicOS
 * Licensed under MIT License - see LICENSE file for details
 */"

# Assembly header  
ASM_HEADER="; Copyright (c) $YEAR $AUTHOR
; Part of AtomicOS Project - https://github.com/ipenas-cl/AtomicOS
; Licensed under MIT License - see LICENSE file for details"

# Tempo header
TEMPO_HEADER="// Copyright (c) $YEAR $AUTHOR
// Part of AtomicOS Project - https://github.com/ipenas-cl/AtomicOS  
// Licensed under MIT License - see LICENSE file for details"

echo "This script will add copyright headers to source files."
echo "Files that already have copyright will be skipped."
echo ""
echo "Continue? (y/n)"
read -n 1 confirm
echo ""

if [ "$confirm" != "y" ]; then
    echo "Cancelled."
    exit 0
fi

# Process C files
echo "Processing C files..."
for file in $(find . -name "*.c" -o -name "*.h" | grep -v build/); do
    if ! grep -q "Copyright" "$file"; then
        echo "Adding header to: $file"
        echo "$C_HEADER" | cat - "$file" > temp && mv temp "$file"
    fi
done

# Process Assembly files
echo "Processing Assembly files..."
for file in $(find . -name "*.asm" -o -name "*.s" | grep -v build/); do
    if ! grep -q "Copyright" "$file"; then
        echo "Adding header to: $file"
        echo "$ASM_HEADER" | cat - "$file" > temp && mv temp "$file"
    fi
done

# Process Tempo files
echo "Processing Tempo files..."
for file in $(find . -name "*.tempo" | grep -v build/); do
    if ! grep -q "Copyright" "$file"; then
        echo "Adding header to: $file"
        echo "$TEMPO_HEADER" | cat - "$file" > temp && mv temp "$file"
    fi
done

echo "Done!"