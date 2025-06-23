#!/bin/bash
# Build script for self-hosted Tempo compiler on macOS

set -e

echo "Building self-hosted Tempo compiler for macOS..."

# Detect architecture
ARCH=$(uname -m)
echo "Detected architecture: $ARCH"

# Create build directory
mkdir -p ../../build/self_hosted

# First, ensure the C-based compiler is built
echo "Building C-based Tempo compiler..."
if [ ! -f "../../build/tempo_compiler" ]; then
    gcc -std=c99 -Wall -Wextra -O2 -o ../../build/tempo_compiler ../../tools/tempo_compiler.c
fi

# Compile each module
echo "Compiling lexer module..."
../../build/tempo_compiler lexer.tempo ../../build/self_hosted/lexer.s

echo "Compiling AST module..."
../../build/tempo_compiler ast.tempo ../../build/self_hosted/ast.s

echo "Compiling parser module..."
../../build/tempo_compiler parser.tempo ../../build/self_hosted/parser.s

echo "Compiling macOS code generator..."
../../build/tempo_compiler codegen_macos.tempo ../../build/self_hosted/codegen_macos.s

echo "Compiling main compiler module..."
../../build/tempo_compiler compiler.tempo ../../build/self_hosted/compiler.s

# Assemble all modules based on architecture
echo "Assembling modules..."
if [ "$ARCH" = "x86_64" ]; then
    # Intel Mac
    for file in ../../build/self_hosted/*.s; do
        base=$(basename "$file" .s)
        as -arch x86_64 "$file" -o "../../build/self_hosted/${base}.o"
    done
    
    # Link into executable
    echo "Linking self-hosted compiler (x86_64)..."
    ld -arch x86_64 -macosx_version_min 10.15 -lSystem -o ../../build/self_hosted/tempo_self_hosted \
       ../../build/self_hosted/compiler.o \
       ../../build/self_hosted/parser.o \
       ../../build/self_hosted/ast.o \
       ../../build/self_hosted/lexer.o \
       ../../build/self_hosted/codegen_macos.o
else
    # Apple Silicon
    for file in ../../build/self_hosted/*.s; do
        base=$(basename "$file" .s)
        as -arch arm64 "$file" -o "../../build/self_hosted/${base}.o"
    done
    
    # Link into executable
    echo "Linking self-hosted compiler (arm64)..."
    ld -arch arm64 -platform_version macos 11.0 11.0 -lSystem -o ../../build/self_hosted/tempo_self_hosted \
       ../../build/self_hosted/compiler.o \
       ../../build/self_hosted/parser.o \
       ../../build/self_hosted/ast.o \
       ../../build/self_hosted/lexer.o \
       ../../build/self_hosted/codegen_macos.o
fi

# Make executable
chmod +x ../../build/self_hosted/tempo_self_hosted

echo "Self-hosted Tempo compiler built successfully!"
echo "Binary: build/self_hosted/tempo_self_hosted"
echo ""
echo "Usage:"
echo "  ./build/self_hosted/tempo_self_hosted input.tempo -o output.s"
echo ""
echo "Example:"
echo "  ./build/self_hosted/tempo_self_hosted examples/hello.tempo -o hello.s"
echo "  as -arch $ARCH hello.s -o hello.o"
echo "  ld -arch $ARCH -lSystem hello.o -o hello"
echo "  ./hello"