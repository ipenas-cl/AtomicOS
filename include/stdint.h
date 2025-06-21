// Basic stdint.h for AtomicOS
#ifndef _STDINT_H
#define _STDINT_H

// Exact-width integer types
typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef short               int16_t;
typedef unsigned short      uint16_t;
typedef int                 int32_t;
typedef unsigned int        uint32_t;
typedef long long           int64_t;
typedef unsigned long long  uint64_t;

// Pointer-sized types
typedef int32_t             intptr_t;
typedef uint32_t            uintptr_t;

// Limits
#define INT8_MAX    127
#define INT8_MIN    (-128)
#define UINT8_MAX   255

#define INT16_MAX   32767
#define INT16_MIN   (-32768)
#define UINT16_MAX  65535

#define INT32_MAX   2147483647
#define INT32_MIN   (-2147483648)
#define UINT32_MAX  4294967295U

#define SIZE_MAX    UINT32_MAX

// size_t
typedef uint32_t size_t;

// NULL
#ifndef NULL
#define NULL ((void*)0)
#endif

#endif