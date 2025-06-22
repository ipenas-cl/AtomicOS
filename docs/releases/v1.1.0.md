# AtomicOS v1.1.0 Release Notes

## Release Date: 2025-06-22

## What's New

### Tempo Compiler v1.1.0 Enhancements

This release focuses on enhancing the Tempo compiler with critical features for systems programming:

#### 🆕 New Features

1. **Hexadecimal Number Support**
   - Added support for hex literals with `0x` prefix
   - Examples: `0xDEADBEEF`, `0xFF`, `0x1234`
   - Properly converts to decimal in generated assembly

2. **Inline Assembly Support**
   - New `asm { }` block syntax for embedding assembly code
   - Example:
     ```tempo
     asm {
         "mov eax, 0x1234"
         "add eax, ebx"
     };
     ```

3. **Security Annotations**
   - Added `@constant_time` annotation for cryptographic functions
   - Enables compile-time security analysis
   - Example:
     ```tempo
     function secure_compare(a: int32, b: int32) -> int32 @constant_time {
         // Implementation
     }
     ```

#### 🐛 Bug Fixes

- Fixed hex number tokenization where initial '0' was being dropped
- Updated all example files to use modern `->` return type syntax
- Fixed missing semicolons in multiple example files
- Corrected function syntax parsing issues

#### 📝 Updated Examples

All Tempo example files have been updated to use the modern syntax:
- `demo_ideal_os.tempo` - Complete security demonstration
- `deterministic_core.tempo` - Core OS functions
- `realtime_scheduler.tempo` - Real-time scheduling
- And many more...

### Version Information

- **AtomicOS**: v1.1.0
- **Tempo Compiler**: v1.1.0
- **Architecture**: i386

### Compatibility

This release maintains backward compatibility with v1.0.0 while adding new features.

### Known Issues

- Some complex example files may still require syntax updates
- Struct field offset calculation pending implementation
- Module import/export system in development

---

**Security First, Stability Second, Performance Third**