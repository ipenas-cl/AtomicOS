#!/bin/bash
# Build script for self-hosted Tempo compiler

set -e

echo "Building self-hosted Tempo compiler..."

# Create build directory
mkdir -p ../../build/self_hosted

# First, ensure the C-based compiler is built
echo "Building C-based Tempo compiler..."
(cd ../.. && make tempo_compiler)

# Compile each module
echo "Compiling lexer module..."
../../build/tempo_compiler lexer.tempo ../../build/self_hosted/lexer.s

echo "Compiling AST module..."
../../build/tempo_compiler ast.tempo ../../build/self_hosted/ast.s

echo "Compiling parser module..."
../../build/tempo_compiler parser.tempo ../../build/self_hosted/parser.s

echo "Compiling main compiler module..."
../../build/tempo_compiler compiler.tempo ../../build/self_hosted/compiler.s

# Assemble all modules
echo "Assembling modules..."
nasm -f elf32 ../../build/self_hosted/lexer.s -o ../../build/self_hosted/lexer.o
nasm -f elf32 ../../build/self_hosted/ast.s -o ../../build/self_hosted/ast.o
nasm -f elf32 ../../build/self_hosted/parser.s -o ../../build/self_hosted/parser.o
nasm -f elf32 ../../build/self_hosted/compiler.s -o ../../build/self_hosted/compiler.o

# Link into executable
echo "Linking self-hosted compiler..."
ld -m elf_i386 -o ../../build/self_hosted/tempo_self_hosted \
   ../../build/self_hosted/compiler.o \
   ../../build/self_hosted/parser.o \
   ../../build/self_hosted/ast.o \
   ../../build/self_hosted/lexer.o

echo "Self-hosted Tempo compiler built successfully!"
echo "Binary: build/self_hosted/tempo_self_hosted"