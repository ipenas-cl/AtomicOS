/*
 * SecureOS Type Definitions
 * Standard types for kernel development
 */

#ifndef TYPES_H
#define TYPES_H

// Exact-width integer types
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

// Size types
typedef uint32_t          size_t;
typedef int32_t           ssize_t;
typedef uint32_t          uintptr_t;
typedef int32_t           intptr_t;

// Boolean
#ifndef __cplusplus
#ifndef bool
typedef uint8_t           bool;
#define true              1
#define false             0
#endif
#endif

// NULL pointer
#define NULL              ((void*)0)

// Attributes
#define __packed          __attribute__((packed))
#define __aligned(x)      __attribute__((aligned(x)))
#define __noreturn        __attribute__((noreturn))
#define __always_inline   __attribute__((always_inline))
#define __noinline        __attribute__((noinline))
#define __pure            __attribute__((pure))
#define __const           __attribute__((const))
#define __hot             __attribute__((hot))
#define __cold            __attribute__((cold))

// Likely/unlikely for branch prediction
#define likely(x)         __builtin_expect(!!(x), 1)
#define unlikely(x)       __builtin_expect(!!(x), 0)

// Container macros
#define container_of(ptr, type, member) ({ \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); \
})

#define offsetof(type, member) ((size_t)&((type *)0)->member)

// Min/max without double evaluation
#define min(a, b) ({ \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    _a < _b ? _a : _b; \
})

#define max(a, b) ({ \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    _a > _b ? _a : _b; \
})

// Alignment macros
#define ALIGN_UP(x, align)   (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define IS_ALIGNED(x, align) (((x) & ((align) - 1)) == 0)

// Bit manipulation
#define BIT(n)            (1UL << (n))
#define BITS_PER_LONG     32
#define GENMASK(h, l)     (((1UL << ((h) - (l) + 1)) - 1) << (l))

// Cache line size (x86)
#define CACHE_LINE_SIZE   64

// Page size
#define PAGE_SIZE         4096
#define PAGE_SHIFT        12
#define PAGE_MASK         (~(PAGE_SIZE - 1))

// Error codes
#define EOK               0
#define ENOMEM           -1
#define EINVAL           -2
#define EFAULT           -3
#define EBUSY            -4
#define ENOSYS           -5
#define ETIMEDOUT        -6

// Function declarations
void kprint(const char* str);
void kprint_hex(uint32_t value);
void* kmalloc(size_t size);
void* kmalloc_aligned(size_t size, size_t alignment);
void handle_critical_fault(void* fault_info);

#endif /* TYPES_H */