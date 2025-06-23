#!/bin/bash
# AtomicOS Release Management Script
# Version: 0.7.0

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get current version from Makefile
CURRENT_VERSION=$(grep "VERSION = " Makefile | cut -d' ' -f3)
echo -e "${BLUE}Current version: ${CURRENT_VERSION}${NC}"

# Function to validate version format
validate_version() {
    if [[ ! $1 =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        echo -e "${RED}Error: Version must be in format MAJOR.MINOR.PATCH (e.g., 0.7.1)${NC}"
        exit 1
    fi
}

# Function to update version in files
update_version() {
    local new_version=$1
    echo -e "${YELLOW}Updating version to ${new_version}...${NC}"
    
    # Update Makefile
    sed -i.bak "s/VERSION = .*/VERSION = ${new_version}/" Makefile
    
    # Update test runner
    sed -i.bak "s/Test Suite v[0-9]\+\.[0-9]\+\.[0-9]\+/Test Suite v${new_version}/" tests/tempo_test_runner.c
    
    # Update Tempo compiler
    sed -i.bak "s/Tempo v[0-9]\+\.[0-9]\+/Tempo v${new_version%.*}/" tools/tempo_compiler_v3.c
    
    # Remove backup files
    rm -f Makefile.bak tests/tempo_test_runner.c.bak tools/tempo_compiler_v3.c.bak
    
    echo -e "${GREEN}Version updated successfully${NC}"
}

# Function to run tests
run_tests() {
    echo -e "${YELLOW}Running test suite...${NC}"
    make clean
    make all
    make test
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}All tests passed!${NC}"
    else
        echo -e "${RED}Tests failed! Aborting release.${NC}"
        exit 1
    fi
}

# Function to create git tag
create_tag() {
    local version=$1
    local tag="v${version}"
    
    echo -e "${YELLOW}Creating git tag ${tag}...${NC}"
    
    # Check if tag already exists
    if git tag -l | grep -q "^${tag}$"; then
        echo -e "${RED}Tag ${tag} already exists!${NC}"
        exit 1
    fi
    
    # Create annotated tag
    git tag -a "${tag}" -m "AtomicOS Release ${version}

Features:
- Tempo v$(echo ${version} | cut -d. -f1-2) deterministic programming language
- Advanced security framework with capability restrictions
- Real-time scheduling with WCET analysis
- Comprehensive test suite for compiler validation
- Enhanced memory protection and filesystem integrity

Changelog:
- See CHANGELOG.md for detailed changes"
    
    echo -e "${GREEN}Tag ${tag} created successfully${NC}"
}

# Function to generate changelog entry
generate_changelog_entry() {
    local version=$1
    local date=$(date +"%Y-%m-%d")
    
    echo -e "${YELLOW}Generating changelog entry...${NC}"
    
    # Create or update CHANGELOG.md
    if [ ! -f CHANGELOG.md ]; then
        cat > CHANGELOG.md << EOF
# Changelog

All notable changes to AtomicOS will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

EOF
    fi
    
    # Add new entry at the top
    local temp_file=$(mktemp)
    cat > "$temp_file" << EOF
## [${version}] - ${date}

### Added
- Comprehensive test suite for Tempo compiler
- Advanced security framework with capability restrictions
- Real-time scheduling with WCET analysis
- Enhanced memory protection and filesystem integrity
- Automated release management system

### Changed
- Updated to Tempo v$(echo ${version} | cut -d. -f1-2)
- Improved compiler error handling and validation
- Enhanced deterministic execution guarantees

### Fixed
- Compiler stability and regression prevention
- Memory allocation determinism
- Security framework integration

EOF
    
    # Insert after header and before existing content
    tail -n +7 CHANGELOG.md >> "$temp_file"
    mv "$temp_file" CHANGELOG.md
    
    echo -e "${GREEN}Changelog updated${NC}"
}

# Function to show help
show_help() {
    echo "AtomicOS Release Management Script"
    echo ""
    echo "Usage: $0 [COMMAND] [VERSION]"
    echo ""
    echo "Commands:"
    echo "  bump [VERSION]    Update version and create release"
    echo "  patch             Increment patch version (0.7.0 -> 0.7.1)"
    echo "  minor             Increment minor version (0.7.0 -> 0.8.0)"
    echo "  major             Increment major version (0.7.0 -> 1.0.0)"
    echo "  test              Run test suite only"
    echo "  tag [VERSION]     Create git tag only"
    echo "  help              Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 bump 0.7.1     # Bump to specific version"
    echo "  $0 patch          # Increment patch (0.7.0 -> 0.7.1)"
    echo "  $0 minor          # Increment minor (0.7.0 -> 0.8.0)"
    echo "  $0 test           # Run tests only"
}

# Function to increment version
increment_version() {
    local version=$1
    local part=$2
    
    IFS='.' read -ra VERSION_PARTS <<< "$version"
    local major=${VERSION_PARTS[0]}
    local minor=${VERSION_PARTS[1]}
    local patch=${VERSION_PARTS[2]}
    
    case $part in
        "major")
            major=$((major + 1))
            minor=0
            patch=0
            ;;
        "minor")
            minor=$((minor + 1))
            patch=0
            ;;
        "patch")
            patch=$((patch + 1))
            ;;
    esac
    
    echo "${major}.${minor}.${patch}"
}

# Main script logic
case "$1" in
    "bump")
        if [ -z "$2" ]; then
            echo -e "${RED}Error: Version required for bump command${NC}"
            show_help
            exit 1
        fi
        validate_version "$2"
        run_tests
        update_version "$2"
        generate_changelog_entry "$2"
        create_tag "$2"
        echo -e "${GREEN}Release $2 created successfully!${NC}"
        echo -e "${YELLOW}Don't forget to push tags: git push --tags${NC}"
        ;;
    "patch"|"minor"|"major")
        new_version=$(increment_version "$CURRENT_VERSION" "$1")
        echo -e "${BLUE}Incrementing ${1} version: ${CURRENT_VERSION} -> ${new_version}${NC}"
        run_tests
        update_version "$new_version"
        generate_changelog_entry "$new_version"
        create_tag "$new_version"
        echo -e "${GREEN}Release ${new_version} created successfully!${NC}"
        echo -e "${YELLOW}Don't forget to push tags: git push --tags${NC}"
        ;;
    "test")
        run_tests
        ;;
    "tag")
        if [ -z "$2" ]; then
            echo -e "${RED}Error: Version required for tag command${NC}"
            show_help
            exit 1
        fi
        validate_version "$2"
        create_tag "$2"
        ;;
    "help"|"--help"|"-h")
        show_help
        ;;
    *)
        echo -e "${RED}Error: Unknown command '$1'${NC}"
        show_help
        exit 1
        ;;
esac