/*
 * W^X Enforcement for AtomicOS
 * Ensures pages cannot be both writable and executable
 * Implemented in C, called from Tempo security modules
 */

#include "mmu.h"

// Enhanced page flags with security
#define PAGE_PRESENT     0x001
#define PAGE_WRITABLE    0x002
#define PAGE_USER        0x004
#define PAGE_EXECUTABLE  0x008  // NEW: Explicit execution permission
#define PAGE_NO_EXEC     0x010  // NEW: No-execute bit simulation
#define PAGE_ACCESSED    0x020
#define PAGE_DIRTY       0x040

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

// Security metrics
typedef struct {
    uint32_t wx_violations_blocked;
    uint32_t privilege_violations;
    uint32_t total_violations;
    uint32_t pages_protected;
} security_metrics_t;

static security_metrics_t security_stats = {0};

// External print function
extern void kprint(const char* msg);

/*
 * Core W^X enforcement function
 * Returns: 0 if safe, -1 if violation detected
 */
int check_wx_violation(uint32_t flags) {
    // Critical check: page cannot be both writable AND executable
    if ((flags & PAGE_WRITABLE) && (flags & PAGE_EXECUTABLE)) {
        return -1; // W^X violation detected
    }
    return 0; // Safe
}

/*
 * Enhanced page mapping with W^X enforcement
 * Replaces the standard map_page function
 */
int map_page_secure(page_directory_t* dir, uint32_t virt, uint32_t phys, uint32_t flags) {
    // W^X enforcement check
    if (check_wx_violation(flags) != 0) {
        // Log violation
        security_violation_t violation = {
            .violation_type = WX_VIOLATION,
            .page_addr = virt,
            .attempted_flags = flags,
            .timestamp = 0 // Would get real timestamp in full system
        };
        
        log_security_violation(&violation);
        
        // Block the mapping
        kprint("[SECURITY] W^X violation blocked!\n");
        return -1;
    }
    
    // If safe, proceed with normal mapping
    map_page(dir, virt, phys, flags);
    security_stats.pages_protected++;
    return 0;
}

/*
 * Log security violations
 */
void log_security_violation(security_violation_t* violation) {
    security_stats.total_violations++;
    
    switch (violation->violation_type) {
        case WX_VIOLATION:
            security_stats.wx_violations_blocked++;
            kprint("[SECURITY] W^X violation: attempt to create W+X page\n");
            break;
        case PRIV_VIOLATION:
            security_stats.privilege_violations++;
            kprint("[SECURITY] Privilege violation detected\n");
            break;
        default:
            kprint("[SECURITY] Unknown violation type\n");
            break;
    }
}

/*
 * Create executable-only page (for code)
 */
int map_code_page(page_directory_t* dir, uint32_t virt, uint32_t phys) {
    uint32_t flags = PAGE_PRESENT | PAGE_EXECUTABLE; // Read + Execute only
    return map_page_secure(dir, virt, phys, flags);
}

/*
 * Create writable page (for data)
 */
int map_data_page(page_directory_t* dir, uint32_t virt, uint32_t phys) {
    uint32_t flags = PAGE_PRESENT | PAGE_WRITABLE; // Read + Write only
    return map_page_secure(dir, virt, phys, flags);
}

/*
 * Create read-only page (for constants)
 */
int map_readonly_page(page_directory_t* dir, uint32_t virt, uint32_t phys) {
    uint32_t flags = PAGE_PRESENT; // Read only
    return map_page_secure(dir, virt, phys, flags);
}

/*
 * Validate existing page permissions
 */
int validate_page_permissions(page_directory_t* dir, uint32_t virt) {
    uint32_t dir_idx = virt >> 22;
    uint32_t table_idx = (virt >> 12) & 0x3FF;
    
    page_dir_entry_t* dir_entry = &dir->entries[dir_idx];
    if (!dir_entry->present) {
        return 0; // Not mapped, no violation
    }
    
    page_table_t* table = (page_table_t*)(dir_entry->frame << 12);
    page_table_entry_t* page = &table->entries[table_idx];
    
    if (!page->present) {
        return 0; // Not mapped, no violation
    }
    
    // Reconstruct flags and check
    uint32_t flags = PAGE_PRESENT;
    if (page->writable) flags |= PAGE_WRITABLE;
    // Note: We'd need to store executable bit in available bits
    
    return check_wx_violation(flags);
}

/*
 * Get security statistics
 */
security_metrics_t* get_security_metrics() {
    return &security_stats;
}

/*
 * Initialize security subsystem
 */
void init_security() {
    security_stats.wx_violations_blocked = 0;
    security_stats.privilege_violations = 0;
    security_stats.total_violations = 0;
    security_stats.pages_protected = 0;
    
    kprint("[SECURITY] W^X enforcement initialized\n");
}

/*
 * Security test function
 */
void test_wx_enforcement() {
    kprint("[SECURITY] Testing W^X enforcement...\n");
    
    // Test 1: Try to create W+X page (should fail)
    uint32_t bad_flags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_EXECUTABLE;
    int result = check_wx_violation(bad_flags);
    if (result == -1) {
        kprint("[SECURITY] Test 1 PASS: W+X violation detected\n");
    } else {
        kprint("[SECURITY] Test 1 FAIL: W+X violation not detected\n");
    }
    
    // Test 2: Try to create W page (should succeed)
    uint32_t write_flags = PAGE_PRESENT | PAGE_WRITABLE;
    result = check_wx_violation(write_flags);
    if (result == 0) {
        kprint("[SECURITY] Test 2 PASS: W page allowed\n");
    } else {
        kprint("[SECURITY] Test 2 FAIL: W page blocked incorrectly\n");
    }
    
    // Test 3: Try to create X page (should succeed)
    uint32_t exec_flags = PAGE_PRESENT | PAGE_EXECUTABLE;
    result = check_wx_violation(exec_flags);
    if (result == 0) {
        kprint("[SECURITY] Test 3 PASS: X page allowed\n");
    } else {
        kprint("[SECURITY] Test 3 FAIL: X page blocked incorrectly\n");
    }
    
    kprint("[SECURITY] W^X enforcement tests completed\n");
}