#!/bin/bash
# Test AtomicOS in QEMU

echo "Testing AtomicOS v0.8.0 with integrated features..."
echo "====================================================="
echo ""
echo "Expected behavior:"
echo "1. Boot message showing AtomicOS v0.8.0"
echo "2. Logo display"
echo "3. Timer interrupt indicator (rotating character in bottom-right)"
echo "4. 'OK' indicator in green if all systems pass validation"
echo "5. Test process output on line 20"
echo "6. Interactive shell prompt"
echo ""
echo "Press Ctrl+A then X to exit QEMU"
echo ""
sleep 2

# Run QEMU with debugging options
qemu-system-i386 \
    -fda build/atomicos.img \
    -monitor stdio \
    -d int,cpu_reset \
    -D qemu_debug.log \
    -no-reboot \
    -no-shutdown