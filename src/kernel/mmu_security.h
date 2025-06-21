/*
 * AtomicOS Security Headers
 * W^X Enforcement and Memory Protection
 */

#ifndef MMU_SECURITY_H
#define MMU_SECURITY_H

typedef unsigned int uint32_t;

// Enhanced page flags
#define PAGE_PRESENT     0x001
#define PAGE_WRITABLE    0x002
#define PAGE_USER        0x004
#define PAGE_EXECUTABLE  0x008
#define PAGE_NO_EXEC     0x010

// Security violation types
#define WX_VIOLATION     1
#define PRIV_VIOLATION   2
#define ACCESS_VIOLATION 3

typedef struct {
    uint32_t violation_type;
    uint32_t page_addr;
    uint32_t attempted_flags;
    uint32_t timestamp;
} security_violation_t;

typedef struct {
    uint32_t wx_violations_blocked;
    uint32_t privilege_violations;
    uint32_t total_violations;
    uint32_t pages_protected;
} security_metrics_t;

// Forward declarations from mmu.c
typedef struct page_directory page_directory_t;

// Security functions
int check_wx_violation(uint32_t flags);
int map_page_secure(page_directory_t* dir, uint32_t virt, uint32_t phys, uint32_t flags);
int map_code_page(page_directory_t* dir, uint32_t virt, uint32_t phys);
int map_data_page(page_directory_t* dir, uint32_t virt, uint32_t phys);
int map_readonly_page(page_directory_t* dir, uint32_t virt, uint32_t phys);
void log_security_violation(security_violation_t* violation);
security_metrics_t* get_security_metrics();
void init_security();
void test_wx_enforcement();

#endif // MMU_SECURITY_H