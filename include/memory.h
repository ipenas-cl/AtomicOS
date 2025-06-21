#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

// Page size (4KB)
#define PAGE_SIZE 4096
#define PAGE_ALIGN(addr) ((addr) & ~(PAGE_SIZE - 1))

// Page directory and table entries
#define PAGE_PRESENT    (1 << 0)
#define PAGE_WRITABLE   (1 << 1)
#define PAGE_USER       (1 << 2)
#define PAGE_NOCACHE    (1 << 4)

// Memory layout
#define KERNEL_BASE     0x100000  // 1MB
#define HEAP_START      0x400000  // 4MB
#define USER_BASE       0x40000000 // 1GB

// Page directory/table structures
typedef struct {
    uint32_t entries[1024];
} page_directory_t;

typedef struct {
    uint32_t entries[1024];
} page_table_t;

// Memory management functions
void memory_init(void);
void paging_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
void* kalloc_page(void);
void kfree_page(void* page);

// Virtual memory
uint32_t* get_page_directory(void);
void switch_page_directory(uint32_t* dir);
void map_page(uint32_t virtual, uint32_t physical, uint32_t flags);
void unmap_page(uint32_t virtual);

// Physical memory allocator
void* alloc_frame(void);
void free_frame(void* frame);

#endif