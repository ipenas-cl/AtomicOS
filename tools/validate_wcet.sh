#!/bin/bash
# WCET Validation Script
# Compiles Tempo programs and validates WCET estimates

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if tempo compiler exists
TEMPO_COMPILER="./build/tempo_compiler"
if [ ! -f "$TEMPO_COMPILER" ]; then
    echo -e "${RED}Error: Tempo compiler not found. Run 'make all' first.${NC}"
    exit 1
fi

# Function to compile and analyze
analyze_tempo_file() {
    local tempo_file=$1
    local base_name=$(basename "$tempo_file" .tempo)
    local asm_file="build/${base_name}.s"
    local report_file="build/${base_name}_wcet.txt"
    
    echo -e "${YELLOW}Analyzing: $tempo_file${NC}"
    
    # Compile with different optimization levels
    echo "  Compiling with -O0 (no optimization)..."
    $TEMPO_COMPILER "$tempo_file" "${asm_file}.O0" -O0
    
    echo "  Compiling with -O1 (size optimization)..."
    $TEMPO_COMPILER "$tempo_file" "${asm_file}.O1" -O1
    
    echo "  Compiling with -O2 (speed optimization)..."
    $TEMPO_COMPILER "$tempo_file" "${asm_file}.O2" -O2
    
    # Extract WCET from assembly files
    wcet_O0=$(grep "Realistic WCET:" "${asm_file}.O0" | awk '{print $4}')
    wcet_O1=$(grep "Realistic WCET:" "${asm_file}.O1" | awk '{print $4}')
    wcet_O2=$(grep "Realistic WCET:" "${asm_file}.O2" | awk '{print $4}')
    
    # Calculate improvements
    if [ -n "$wcet_O0" ] && [ -n "$wcet_O2" ]; then
        improvement=$(echo "scale=2; (($wcet_O0 - $wcet_O2) * 100) / $wcet_O0" | bc)
        echo -e "  ${GREEN}WCET Results:${NC}"
        echo "    -O0: $wcet_O0 cycles"
        echo "    -O1: $wcet_O1 cycles"
        echo "    -O2: $wcet_O2 cycles"
        echo -e "    ${GREEN}Improvement: ${improvement}%${NC}"
    fi
    
    # Count eliminated instructions
    lines_O0=$(wc -l < "${asm_file}.O0")
    lines_O2=$(wc -l < "${asm_file}.O2")
    lines_saved=$((lines_O0 - lines_O2))
    echo "  Code size: $lines_saved lines saved"
    
    echo ""
}

# Main validation
echo -e "${GREEN}=== Tempo WCET Validation Tool ===${NC}"
echo ""

# Test all example files
for tempo_file in examples/*.tempo; do
    if [ -f "$tempo_file" ]; then
        analyze_tempo_file "$tempo_file"
    fi
done

# Summary report
echo -e "${GREEN}=== Validation Summary ===${NC}"
echo "All Tempo files have been analyzed for WCET."
echo "Check build/ directory for detailed assembly output."
echo ""
echo "To validate against hardware:"
echo "  1. Compile test programs to executables"
echo "  2. Run with CPU performance counters"
echo "  3. Compare measured cycles to WCET estimates"

# Make script executable
chmod +x tools/validate_wcet.sh