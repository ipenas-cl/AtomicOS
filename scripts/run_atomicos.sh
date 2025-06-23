#!/bin/bash
#
# AtomicOS Run Script
# Run AtomicOS in QEMU emulator
#

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check if image exists
if [ ! -f "build/atomicos.img" ]; then
    echo -e "${GREEN}Building AtomicOS first...${NC}"
    make all || exit 1
fi

echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║         AtomicOS v1.0.0                ║${NC}"
echo -e "${BLUE}║   Deterministic Real-Time Security OS  ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
echo ""
echo -e "${GREEN}Starting AtomicOS in QEMU...${NC}"
echo "Press Ctrl+A then X to exit"
echo ""

# Run with serial output
qemu-system-i386 \
    -drive file=build/atomicos.img,format=raw,if=floppy \
    -nographic \
    -serial mon:stdio \
    -m 32M