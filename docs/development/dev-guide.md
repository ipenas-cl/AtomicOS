# AtomicOS Developer Guide

## Overview

AtomicOS is a deterministic real-time security operating system written in x86 assembly and C, with a custom programming language called Tempo for security modules. This guide covers development practices, building, testing, and contributing.

**Current Version**: v1.3.0

## Essential Commands

### Building and Running
```bash
make all        # Build complete OS image
make run        # Run OS in QEMU with curses display
make debug      # Run OS in QEMU with debugging enabled
make clean      # Clean build artifacts
make distclean  # Clean everything including generated files
```

### Testing and Quality Assurance
```bash
make test         # Run comprehensive test suite
make test-quick   # Run quick compilation tests only
make test-verbose # Run tests with detailed output
make security-check # Run security analysis
scripts/pre-commit.sh # Run all pre-commit checks
```

### Release Management
```bash
make version      # Show current version information
make release-patch # Increment patch version (e.g., 1.3.0 -> 1.3.1)
make release-minor # Increment minor version (e.g., 1.3.0 -> 1.4.0)
make release-major # Increment major version (e.g., 1.3.0 -> 2.0.0)
```

## Development Workflow

### 1. Setting Up Your Environment

```bash
# Clone the repository
git clone https://github.com/ipenas-cl/AtomicOS.git
cd AtomicOS

# Build the Tempo compiler
make build/tempo_compiler

# Build the entire OS
make all
```

### 2. Making Changes

1. Create a feature branch
2. Make your changes following the coding standards
3. Run tests: `make test`
4. Run pre-commit checks: `scripts/pre-commit.sh`
5. Commit your changes (see Git guidelines below)

### 3. Testing Your Changes

```bash
# Run the OS in QEMU
make run

# Debug with GDB
make debug
# In another terminal:
gdb build/kernel.elf
(gdb) target remote :1234
```

## Coding Standards

### Assembly Code
- Use NASM syntax
- Comment every non-trivial operation
- Include WCET annotations for timing-critical sections
- Follow the existing indentation style (4 spaces)

### C Code
- Follow C99 standard
- No dynamic memory allocation (static only)
- All operations must have bounded execution time
- Use the provided security primitives

### Tempo Code
- Use type annotations for all function parameters
- Include WCET bounds for loops
- Mark security-critical functions with appropriate annotations
- Follow the naming conventions in existing modules

## Architecture Guidelines

### Security Principles
1. **Security First**: All code must be secure by design
2. **No Dynamic Allocation**: Use static memory pools only
3. **Bounded Operations**: All loops must have provable bounds
4. **Constant-Time Crypto**: No timing side channels

### Real-Time Guarantees
- All operations must have WCET analysis
- Interrupt handlers must complete within bounded time
- No unbounded blocking operations
- Use the real-time scheduler appropriately

## Adding New Features

### Adding a Tempo Module
1. Create `.tempo` file in `examples/`
2. Add build target in Makefile if needed
3. Include in kernel.asm with `%include`
4. Test compilation: `./build/tempo_compiler your_module.tempo output.s`
5. Run full test suite

### Adding Kernel Functionality
1. Implement in appropriate file (assembly or C)
2. Add WCET annotations
3. Update relevant headers
4. Add tests
5. Update documentation

## Testing Guidelines

### Unit Tests
- Write tests for all new functionality
- Use the test framework in `tests/`
- Ensure tests are deterministic
- Include edge cases and error conditions

### Integration Tests
- Test interaction between components
- Verify security boundaries
- Check real-time constraints
- Test with various workloads

## Git Guidelines

### Commit Messages
- Use descriptive commit messages
- Focus on "why" not just "what"
- Reference issue numbers when applicable
- Keep commits atomic and focused

### IMPORTANT: Anonymity Rules
- **NEVER create commits** - Let users handle Git operations
- **NEVER use git config** to set name/email
- **NEVER add Co-Authored-By** mentioning AI/Claude
- **NO references to AI assistance** in commits or code

## Debugging Tips

### Common Issues
1. **Build Failures**: Check dependencies, run `make clean`
2. **Runtime Crashes**: Use `make debug` with GDB
3. **Timing Issues**: Verify WCET annotations
4. **Security Violations**: Check privilege levels

### Useful GDB Commands
```gdb
break kernel_main     # Set breakpoint
info registers       # Show CPU registers
x/10i $eip          # Disassemble at EIP
x/10x $esp          # Examine stack
```

## Performance Optimization

### Guidelines
1. Measure first, optimize second
2. Maintain WCET bounds
3. Profile with appropriate tools
4. Document any optimizations
5. Ensure security isn't compromised

### Common Optimizations
- Loop unrolling (with WCET updates)
- Inline assembly for critical paths
- Cache-friendly data structures
- Reduced instruction count

## Security Considerations

### Code Review Checklist
- [ ] No buffer overflows possible
- [ ] All inputs validated
- [ ] Privilege levels enforced
- [ ] No timing side channels
- [ ] Memory properly cleared
- [ ] Error codes don't leak info

### Security Testing
- Fuzz testing for inputs
- Timing analysis for crypto
- Privilege escalation attempts
- Memory safety verification

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run all tests
5. Submit a pull request

### Pull Request Requirements
- All tests must pass
- Code must follow style guidelines
- Documentation must be updated
- Security review completed
- WCET analysis provided

## Resources

- [Architecture Overview](../architecture/overview.md)
- [Tempo Language Reference](../tempo/README.md)
- [API Documentation](../api/kernel-api.md)
- [Security Model](../architecture/security-model.md)

## Support

For questions or issues:
- Check existing documentation
- Search closed issues
- Open a new issue with details
- Join community discussions

---
*Remember: Security First, Stability Second, Performance Third*