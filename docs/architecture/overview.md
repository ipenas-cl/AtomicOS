# AtomicOS Architecture Overview

## System Architecture

AtomicOS is a deterministic real-time operating system designed with security as the primary concern. It implements a unique architecture that guarantees predictable timing behavior while maintaining strong security properties.

```
┌─────────────────────────────────────────────────────────┐
│                    User Space (Level 0-1)               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │
│  │   Guest     │  │    User     │  │    User     │    │
│  │  Process    │  │  Process    │  │  Process    │    │
│  └─────────────┘  └─────────────┘  └─────────────┘    │
├─────────────────────────────────────────────────────────┤
│                  System Space (Level 2)                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │
│  │   System    │  │     IPC     │  │ File System │    │
│  │  Services   │  │   Manager   │  │   Service   │    │
│  └─────────────┘  └─────────────┘  └─────────────┘    │
├─────────────────────────────────────────────────────────┤
│                  Kernel Space (Level 3)                  │
│  ┌─────────────────────────────────────────────────┐    │
│  │          Deterministic Microkernel               │    │
│  │  ┌─────────┐ ┌──────────┐ ┌──────────────┐    │    │
│  │  │Scheduler│ │  Memory  │ │   Security   │    │    │
│  │  │ (RT-EDF)│ │ Manager  │ │   Monitor    │    │    │
│  │  └─────────┘ └──────────┘ └──────────────┘    │    │
│  └─────────────────────────────────────────────────┘    │
├─────────────────────────────────────────────────────────┤
│                    Hardware Abstraction                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌────────┐ │
│  │   CPU    │  │  Memory  │  │   I/O    │  │ Timer  │ │
│  │ (i386)   │  │   (MMU)  │  │  Ports   │  │ (PIT)  │ │
│  └──────────┘  └──────────┘  └──────────┘  └────────┘ │
└─────────────────────────────────────────────────────────┘
```

## Core Components

### 1. Bootloader
- **Location**: `src/bootloader/boot.asm`
- **Function**: Initial system bootstrap
- **Features**:
  - Loads kernel into memory at 0x100000
  - Sets up initial GDT
  - Switches to protected mode
  - Transfers control to kernel

### 2. Microkernel
- **Location**: `src/kernel/kernel.asm`
- **Philosophy**: Minimal trusted computing base
- **Components**:
  - Interrupt handling (IDT)
  - Process management
  - Memory management
  - Security enforcement
  - Real-time scheduling

### 3. Security Modules (Tempo)
All security-critical components are written in Tempo for verification:

- **Deterministic Core** (`deterministic_core.tempo`)
  - Random number generation (deterministic PRNG)
  - Time management
  - Core security functions

- **Memory Protection** (`guard_pages.tempo`, `wx_protection.tempo`)
  - Guard page allocation
  - W^X (Write XOR Execute) enforcement
  - Memory isolation

- **Stack Protection** (`ssp_functions.tempo`)
  - Stack canary generation
  - Stack overflow detection
  - Return address validation

## Memory Layout

```
0x00000000 - 0x000003FF: Real mode IVT (unused in PM)
0x00000400 - 0x000004FF: BIOS data area
0x00000500 - 0x00007BFF: Free low memory
0x00007C00 - 0x00007DFF: Bootloader
0x00007E00 - 0x0009FFFF: Free
0x000A0000 - 0x000BFFFF: VGA memory
0x000C0000 - 0x000FFFFF: BIOS ROM
0x00100000 - 0x001FFFFF: Kernel code/data
0x00200000 - 0x005FFFFF: Process memory
0x00600000 - 0x00FFFFFF: Dynamic allocation pool
```

### Memory Protection

1. **Segmentation**: Flat memory model with privilege levels
2. **Guard Pages**: Empty pages between memory regions
3. **W^X Policy**: Pages are either writable OR executable
4. **Static Allocation**: No heap, fixed memory pools

## Process Model

### Process Structure
```c
struct Process {
    uint32_t pid;
    uint32_t state;      // READY, RUNNING, BLOCKED
    uint32_t priority;   // 0-255 (RT priorities)
    uint32_t security_level; // 0-3
    Context context;     // CPU state
    uint32_t deadline;   // For RT tasks
    uint32_t period;     // For periodic tasks
    uint32_t wcet;       // Worst-case execution time
}
```

### States
- **READY**: Waiting to run
- **RUNNING**: Currently executing
- **BLOCKED**: Waiting for resource
- **SUSPENDED**: Temporarily stopped
- **ZOMBIE**: Terminated, awaiting cleanup

## Security Architecture

### Four-Level Security Model

1. **Level 3 (Kernel)**
   - Full hardware access
   - Memory management
   - Security policy enforcement
   - Interrupt handling

2. **Level 2 (System)**
   - System services
   - Device drivers (user-mode)
   - Resource managers
   - IPC coordination

3. **Level 1 (User)**
   - Normal applications
   - Limited system calls
   - Memory protection enforced
   - No direct hardware access

4. **Level 0 (Guest)**
   - Untrusted code
   - Minimal privileges
   - Sandboxed execution
   - Heavily monitored

### Security Features

#### W^X Protection
```tempo
function enforce_wx(page: int32) -> bool {
    let flags = get_page_flags(page);
    
    // Page cannot be both writable and executable
    if (flags & WRITE) && (flags & EXEC) {
        return false;
    }
    
    return true;
}
```

#### Stack-Smashing Protection
```tempo
function check_canary() -> bool @constant_time {
    let stored = stack_canary;
    let current = read_canary();
    
    return secure_compare(&stored, &current, 4);
}
```

#### KASLR (Kernel ASLR)
- Deterministic randomization using secure PRNG
- Layout determined at boot
- Prevents address prediction attacks

## Real-Time Scheduling

### Scheduling Algorithms

1. **EDF (Earliest Deadline First)**
   - Dynamic priority based on deadlines
   - Optimal for single processor
   - Supports sporadic and periodic tasks

2. **RMS (Rate Monotonic Scheduling)**
   - Static priority based on periods
   - Simpler implementation
   - Predictable behavior

### Real-Time Guarantees

```tempo
struct RTTask {
    period: int32,      // Task period in microseconds
    wcet: int32,        // Worst-case execution time
    deadline: int32,    // Relative deadline
    priority: int32     // Computed priority
}

function schedulability_test(tasks: ptr<RTTask, N>) -> bool {
    let utilization = 0;
    
    // Liu & Layland bound for RMS
    let bound = N * (pow(2, 1/N) - 1);
    
    for i in 0..N {
        utilization += tasks[i].wcet / tasks[i].period;
    }
    
    return utilization <= bound;
}
```

## Inter-Process Communication

### Message Passing
- Synchronous and asynchronous modes
- Bounded message queues
- Priority inheritance to prevent inversion
- Type-safe message formats

### Channels
```tempo
struct Channel {
    sender: int32,
    receiver: int32,
    buffer: [64]Message,
    head: int32,
    tail: int32,
    size: int32
}
```

### Shared Memory
- Explicitly requested and controlled
- Read-only sharing by default
- Copy-on-write for efficiency
- Security level restrictions

## Device Driver Model

### User-Mode Drivers
- Drivers run in user space (Level 2)
- Hardware access through IPC
- Fault isolation
- Hot-swappable

### Driver Interface
```tempo
interface Driver {
    function init() -> int32;
    function read(buffer: ptr<int8>, size: int32) -> int32;
    function write(buffer: ptr<int8>, size: int32) -> int32;
    function ioctl(cmd: int32, arg: ptr<void>) -> int32;
}
```

## File System

### Design Principles
- Deterministic allocation
- Fixed-size blocks (512 bytes)
- No fragmentation
- Predictable access times

### Structure
```tempo
struct FileSystem {
    superblock: Superblock,
    inode_table: [1024]Inode,
    data_blocks: [65536]Block,
    allocation_map: [2048]int32
}
```

## Interrupt Handling

### Interrupt Descriptor Table
- 256 interrupt vectors
- CPU exceptions (0-31)
- Hardware IRQs (32-47)
- Software interrupts (48-255)

### Interrupt Processing
1. Save context
2. Switch to kernel stack
3. Handle interrupt
4. Update WCET tracking
5. Check real-time deadlines
6. Restore context
7. Return from interrupt

## Boot Process

### Stage 1: Bootloader
1. Load kernel from disk
2. Set up GDT
3. Enable A20 line
4. Switch to protected mode
5. Jump to kernel

### Stage 2: Kernel Init
1. Set up IDT
2. Initialize memory manager
3. Start timer
4. Initialize security modules
5. Create init process

### Stage 3: System Services
1. Start system services
2. Initialize drivers
3. Mount filesystem
4. Start user processes

## Performance Characteristics

### Deterministic Operations
- Memory allocation: O(1)
- Context switch: ~1000 cycles
- System call: ~500 cycles
- Interrupt latency: <100 cycles

### Memory Overhead
- Kernel: ~64KB
- Per-process: ~4KB
- Security modules: ~32KB
- Total minimum: ~128KB

## Future Enhancements

### Planned Features
1. SMP (Symmetric Multiprocessing)
2. Network stack (deterministic)
3. Graphical subsystem
4. Extended filesystem
5. Hardware crypto acceleration

### Research Areas
- Formal verification of kernel
- Zero-knowledge proofs for security
- Quantum-resistant cryptography
- Machine learning detection (bounded)

---
*Architecture designed for: Security First, Stability Second, Performance Third*