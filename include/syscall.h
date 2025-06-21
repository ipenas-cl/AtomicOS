// AtomicOS System Call Interface
// Deterministic syscalls with WCET guarantees

#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

// System call numbers (deterministic ordering)
typedef enum {
    // Process management
    SYS_EXIT        = 0,    // Terminate process
    SYS_FORK        = 1,    // Create new process
    SYS_EXEC        = 2,    // Execute program
    SYS_WAIT        = 3,    // Wait for child process
    SYS_GETPID      = 4,    // Get process ID
    SYS_KILL        = 5,    // Send signal to process
    
    // Memory management
    SYS_BRK         = 10,   // Set data segment size
    SYS_MMAP        = 11,   // Map memory
    SYS_MUNMAP      = 12,   // Unmap memory
    SYS_MPROTECT    = 13,   // Set memory protection
    
    // File system
    SYS_OPEN        = 20,   // Open file
    SYS_CLOSE       = 21,   // Close file
    SYS_READ        = 22,   // Read from file
    SYS_WRITE       = 23,   // Write to file
    SYS_LSEEK       = 24,   // Seek in file
    SYS_STAT        = 25,   // Get file status
    
    // Real-time operations
    SYS_RT_CREATE   = 30,   // Create RT task
    SYS_RT_DELETE   = 31,   // Delete RT task
    SYS_RT_YIELD    = 32,   // Yield to scheduler
    SYS_RT_SLEEP    = 33,   // Sleep for microseconds
    SYS_RT_GETTIME  = 34,   // Get system time
    SYS_RT_SETPRIO  = 35,   // Set RT priority
    
    // Security operations
    SYS_SETUID      = 40,   // Set user ID
    SYS_GETUID      = 41,   // Get user ID
    SYS_SETSEC      = 42,   // Set security level
    SYS_GETSEC      = 43,   // Get security level
    SYS_PLEDGE      = 44,   // Restrict capabilities
    
    // Inter-process communication
    SYS_PIPE        = 50,   // Create pipe
    SYS_MSGQUEUE    = 51,   // Create message queue
    SYS_MSGSEND     = 52,   // Send message
    SYS_MSGRECV     = 53,   // Receive message
    SYS_SHMGET      = 54,   // Get shared memory
    SYS_SHMATT      = 55,   // Attach shared memory
    
    // Device operations
    SYS_IOCTL       = 60,   // Device control
    SYS_INPORT      = 61,   // Read from I/O port
    SYS_OUTPORT     = 62,   // Write to I/O port
    
    SYS_MAX         = 63    // Maximum syscall number
} syscall_number_t;

// System call error codes (avoid conflicts with types.h)
typedef enum {
    E_SUCCESS    = 0,    // Success
    E_PERM       = 1,    // Operation not permitted
    E_NOENT      = 2,    // No such file or directory
    E_INTR       = 3,    // Interrupted system call
    E_IO         = 4,    // I/O error
    E_NOMEM      = 5,    // Out of memory
    E_ACCES      = 6,    // Permission denied
    E_FAULT      = 7,    // Bad address
    E_BUSY       = 8,    // Device or resource busy
    E_INVAL      = 9,    // Invalid argument
    E_NOSYS      = 10,   // Function not implemented
    E_DEADLINE   = 11,   // Deadline missed (RT)
    E_SECURITY   = 12,   // Security violation
    E_WCET       = 13,   // WCET violation
} syscall_error_t;

// System call result
typedef struct {
    int32_t value;      // Return value or error code
    uint32_t wcet;      // Actual WCET consumed
} syscall_result_t;

// WCET bounds for each system call (in CPU cycles)
static const uint32_t syscall_wcet_bounds[SYS_MAX + 1] = {
    [SYS_EXIT]      = 1000,     // Process cleanup
    [SYS_FORK]      = 5000,     // Process creation
    [SYS_EXEC]      = 10000,    // Program loading
    [SYS_WAIT]      = 500,      // Check child status
    [SYS_GETPID]    = 100,      // Simple read
    [SYS_KILL]      = 500,      // Signal delivery
    
    [SYS_BRK]       = 2000,     // Memory adjustment
    [SYS_MMAP]      = 3000,     // Memory mapping
    [SYS_MUNMAP]    = 2000,     // Memory unmapping
    [SYS_MPROTECT]  = 1000,     // Protection change
    
    [SYS_OPEN]      = 3000,     // File open
    [SYS_CLOSE]     = 500,      // File close
    [SYS_READ]      = 2000,     // Read operation
    [SYS_WRITE]     = 2000,     // Write operation
    [SYS_LSEEK]     = 200,      // Seek operation
    [SYS_STAT]      = 1000,     // File info
    
    [SYS_RT_CREATE] = 2000,     // RT task creation
    [SYS_RT_DELETE] = 1000,     // RT task deletion
    [SYS_RT_YIELD]  = 300,      // Scheduler yield
    [SYS_RT_SLEEP]  = 400,      // Sleep setup
    [SYS_RT_GETTIME]= 100,      // Time read
    [SYS_RT_SETPRIO]= 200,      // Priority change
    
    // ... other syscalls
};

// System call parameter structure (max 6 parameters)
typedef struct {
    uint32_t arg0;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    uint32_t arg4;
    uint32_t arg5;
} syscall_params_t;

// System call handler function type
typedef syscall_result_t (*syscall_handler_t)(syscall_params_t* params);

// System call table entry
typedef struct {
    syscall_handler_t handler;      // Handler function
    uint32_t wcet_bound;           // WCET bound
    uint8_t min_security_level;    // Minimum security level required
    uint8_t param_count;           // Number of parameters
    bool interruptible;            // Can be interrupted
    bool realtime_safe;            // Safe for RT tasks
} syscall_entry_t;

// Function prototypes
void syscall_init(void);
syscall_result_t syscall_dispatch(uint32_t number, syscall_params_t* params);
bool syscall_validate_params(uint32_t number, syscall_params_t* params);
uint32_t syscall_get_wcet_bound(uint32_t number);

// User-space syscall wrappers (inline assembly)
#ifdef __GNUC__
static inline int32_t syscall0(uint32_t number) {
    int32_t result;
    __asm__ __volatile__(
        "int $0x80"
        : "=a"(result)
        : "a"(number)
        : "memory"
    );
    return result;
}

static inline int32_t syscall1(uint32_t number, uint32_t arg0) {
    int32_t result;
    __asm__ __volatile__(
        "int $0x80"
        : "=a"(result)
        : "a"(number), "b"(arg0)
        : "memory"
    );
    return result;
}

static inline int32_t syscall2(uint32_t number, uint32_t arg0, uint32_t arg1) {
    int32_t result;
    __asm__ __volatile__(
        "int $0x80"
        : "=a"(result)
        : "a"(number), "b"(arg0), "c"(arg1)
        : "memory"
    );
    return result;
}

static inline int32_t syscall3(uint32_t number, uint32_t arg0, uint32_t arg1, uint32_t arg2) {
    int32_t result;
    __asm__ __volatile__(
        "int $0x80"
        : "=a"(result)
        : "a"(number), "b"(arg0), "c"(arg1), "d"(arg2)
        : "memory"
    );
    return result;
}
#else
// Non-GCC compilers: provide function declarations only
int32_t syscall0(uint32_t number);
int32_t syscall1(uint32_t number, uint32_t arg0);
int32_t syscall2(uint32_t number, uint32_t arg0, uint32_t arg1);
int32_t syscall3(uint32_t number, uint32_t arg0, uint32_t arg1, uint32_t arg2);
#endif

// High-level wrappers
#define exit(code)          syscall1(SYS_EXIT, (code))
#define getpid()            syscall0(SYS_GETPID)
#define rt_yield()          syscall0(SYS_RT_YIELD)
#define rt_gettime()        syscall0(SYS_RT_GETTIME)

#endif // SYSCALL_H