# AtomicOS v0.9.0 Release Notes

## Major Features

### Inter-Process Communication (IPC)
- Implemented message-passing system for microkernel architecture
- Channel-based communication with permissions
- Static memory allocation with bounded message queues
- System calls: SYS_IPC_CREATE, SYS_IPC_SEND, SYS_IPC_RECEIVE, SYS_IPC_CLOSE, SYS_IPC_STATS
- Maximum 16 channels with 8 messages per channel
- Deterministic message delivery with WCET bounds

### Basic Filesystem Support
- Simple flat filesystem with static allocation
- Fixed-size blocks (512 bytes) with inode-based tracking
- Support for up to 64 files and 1024 blocks
- File operations: create, open, close, read, write, delete
- System calls: SYS_FS_OPEN, SYS_FS_CLOSE, SYS_FS_READ, SYS_FS_WRITE, SYS_FS_CREATE, SYS_FS_DELETE
- No dynamic memory allocation - all structures statically allocated

### Build System Improvements
- Reverted to Tempo v3 compiler due to v4 stability issues
- Added compilation support for IPC and filesystem modules
- Improved error handling in build process

## Technical Details

### IPC Implementation
- Message size limit: 256 bytes
- Channel permissions: read, write, owner
- Process isolation with channel-based security
- Zero-copy design where possible

### Filesystem Design
- Block size: 512 bytes
- Maximum file size: 64KB (128 blocks)
- Direct block pointers only (no indirect blocks)
- Simple path resolution (root directory only)
- File permissions: read, write, execute

## Bug Fixes
- Fixed interrupt_core.tempo syntax for v3 compiler compatibility
- Resolved type definition conflicts in kernel headers
- Fixed signed/unsigned comparison warnings

## Known Issues
- Tempo v4 compiler has parsing issues and is not ready for use
- Some example files need syntax updates for current compiler
- Filesystem currently supports only root directory (no subdirectories)

## Future Work
- Extended filesystem with directory support
- Improved IPC with shared memory regions
- Network stack for distributed IPC
- Enhanced security policies for IPC channels

---
*Ignacio Peña - AtomicOS Project*