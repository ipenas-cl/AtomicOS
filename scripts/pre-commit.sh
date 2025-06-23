#!/bin/bash
# AtomicOS Pre-commit Hook
# Runs tests and checks before allowing commits

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}AtomicOS Pre-commit Checks${NC}"
echo "=========================="

# Check if we're in the right directory
if [ ! -f "Makefile" ] || [ ! -d "src" ]; then
    echo -e "${RED}Error: Not in AtomicOS root directory${NC}"
    exit 1
fi

# Function to run a check
run_check() {
    local name="$1"
    local command="$2"
    
    echo -ne "${YELLOW}Checking ${name}...${NC} "
    
    if eval "$command" >/dev/null 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        return 0
    else
        echo -e "${RED}FAIL${NC}"
        return 1
    fi
}

# Function to run a check with output
run_check_verbose() {
    local name="$1"
    local command="$2"
    
    echo -e "${YELLOW}Running ${name}...${NC}"
    
    if eval "$command"; then
        echo -e "${GREEN}✓ ${name} passed${NC}"
        return 0
    else
        echo -e "${RED}✗ ${name} failed${NC}"
        return 1
    fi
}

checks_passed=0
checks_failed=0

# Check 1: Code compilation
echo -e "\n${BLUE}1. Build System Checks${NC}"
if run_check "Clean build" "make clean && make all"; then
    ((checks_passed++))
else
    ((checks_failed++))
    echo -e "${RED}   Build failed - fix compilation errors before committing${NC}"
fi

# Check 2: Test suite
echo -e "\n${BLUE}2. Test Suite${NC}"
if run_check_verbose "Tempo compiler tests" "make test"; then
    ((checks_passed++))
else
    ((checks_failed++))
    echo -e "${RED}   Tests failed - fix failing tests before committing${NC}"
fi

# Check 3: Code quality checks
echo -e "\n${BLUE}3. Code Quality Checks${NC}"

# Check for external references
if run_check "External references" "! grep -r -i 'openbsd\|linux\|solaris\|aix' examples/ src/ tools/ --exclude-dir=build 2>/dev/null"; then
    ((checks_passed++))
else
    ((checks_failed++))
    echo -e "${RED}   Found external OS/company references - clean them before committing${NC}"
fi

# Check for proper file organization
if run_check "File organization" "[ -d src ] && [ -d examples ] && [ -d tools ] && [ -d tests ]"; then
    ((checks_passed++))
else
    ((checks_failed++))
    echo -e "${RED}   Directory structure incomplete${NC}"
fi

# Check for version consistency
current_version=$(grep "VERSION = " Makefile | cut -d' ' -f3)
if run_check "Version consistency" "grep -q 'v${current_version%.*}' tools/tempo_compiler_v3.c"; then
    ((checks_passed++))
else
    ((checks_failed++))
    echo -e "${RED}   Version inconsistency detected - run release script to fix${NC}"
fi

# Check 4: Documentation
echo -e "\n${BLUE}4. Documentation Checks${NC}"

if run_check "DEV_GUIDE.md exists" "[ -f DEV_GUIDE.md ]"; then
    ((checks_passed++))
else
    ((checks_failed++))
    echo -e "${RED}   DEV_GUIDE.md missing${NC}"
fi

if run_check "README.md exists" "[ -f README.md ]"; then
    ((checks_passed++))
else
    ((checks_failed++))
    echo -e "${RED}   README.md missing${NC}"
fi

# Check 5: Security checks
echo -e "\n${BLUE}5. Security Checks${NC}"

if run_check "No hardcoded secrets" "! grep -r -i 'password\|secret\|key.*=' src/ examples/ tools/ --exclude-dir=build 2>/dev/null"; then
    ((checks_passed++))
else
    ((checks_failed++))
    echo -e "${RED}   Potential hardcoded secrets found${NC}"
fi

# Summary
echo -e "\n${BLUE}Pre-commit Summary${NC}"
echo "==================="
echo -e "Checks passed: ${GREEN}${checks_passed}${NC}"
echo -e "Checks failed: ${RED}${checks_failed}${NC}"

if [ $checks_failed -eq 0 ]; then
    echo -e "\n${GREEN}✓ All checks passed! Commit allowed.${NC}"
    exit 0
else
    echo -e "\n${RED}✗ Some checks failed. Fix issues before committing.${NC}"
    echo -e "\nTo skip these checks (not recommended): git commit --no-verify"
    exit 1
fi