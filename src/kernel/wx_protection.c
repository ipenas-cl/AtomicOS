/*
 * W^X (Write XOR Execute) Protection for AtomicOS
 * Based on OpenBSD's W^X enforcement
 */

#include <stdint.h>
#include "wx_protection.h"

// Page permission bits
#define PAGE_READ    0x01
#define PAGE_WRITE   0x02
#define PAGE_EXEC    0x04
#define PAGE_VALID   0x08

// Page size (4KB)
#define PAGE_SIZE    4096
#define PAGE_MASK    0xFFF

// W^X validation - returns 1 if valid, 0 if invalid
int wx_validate_permissions(uint32_t permissions) {
    uint32_t write_bit = permissions & PAGE_WRITE;
    uint32_t exec_bit = permissions & PAGE_EXEC;
    
    // W^X enforcement: cannot be both writable and executable
    if (write_bit && exec_bit) {
        return 0;  // Violation - both W and X set
    }
    
    return 1;  // Valid permissions
}

// Set page to read-only
uint32_t wx_set_read_only(uint32_t permissions) {
    return (permissions & (PAGE_READ | PAGE_VALID)) | PAGE_READ;
}

// Set page to read-write (removes execute)
uint32_t wx_set_read_write(uint32_t permissions) {
    uint32_t new_perms = permissions | PAGE_READ | PAGE_WRITE;
    return new_perms & ~PAGE_EXEC;  // Clear execute bit
}

// Set page to read-execute (removes write)
uint32_t wx_set_read_exec(uint32_t permissions) {
    uint32_t new_perms = permissions | PAGE_READ | PAGE_EXEC;
    return new_perms & ~PAGE_WRITE;  // Clear write bit
}

// Convert executable pages to writable (for JIT, etc.)
int wx_make_writable(uint32_t addr, uint32_t size) {
    // Align to page boundaries
    uint32_t start_page = addr & ~PAGE_MASK;
    uint32_t end_page = (addr + size + PAGE_MASK) & ~PAGE_MASK;
    
    // Validate address range
    if (!wx_validate_address_range(start_page, end_page - start_page)) {
        return 0;  // Invalid range
    }
    
    // In real implementation:
    // 1. Walk page tables
    // 2. Update PTE permissions to RW
    // 3. Flush TLB
    // 4. Ensure no execute permission
    
    return 1;  // Success
}

// Convert writable pages to executable (after code generation)
int wx_make_executable(uint32_t addr, uint32_t size) {
    // Align to page boundaries
    uint32_t start_page = addr & ~PAGE_MASK;
    uint32_t end_page = (addr + size + PAGE_MASK) & ~PAGE_MASK;
    
    // Validate address range
    if (!wx_validate_address_range(start_page, end_page - start_page)) {
        return 0;  // Invalid range
    }
    
    // In real implementation:
    // 1. Flush instruction cache
    // 2. Update PTE permissions to RX
    // 3. Remove write permission
    // 4. Flush TLB
    
    return 1;  // Success
}

// Validate address range for W^X operations
int wx_validate_address_range(uint32_t addr, uint32_t size) {
    uint32_t end_addr = addr + size;
    
    // Check for overflow
    if (end_addr < addr) {
        return 0;  // Overflow
    }
    
    // Check alignment
    if ((addr & PAGE_MASK) != 0) {
        return 0;  // Not page-aligned
    }
    
    // Check user space bounds (below kernel space)
    if (addr < 0x400000 || end_addr > 0xBF000000) {
        return 0;  // Outside user space
    }
    
    return 1;  // Valid range
}

// Get permission description for debugging
const char* wx_get_permissions_string(uint32_t permissions) {
    int read = permissions & PAGE_READ;
    int write = permissions & PAGE_WRITE;
    int exec = permissions & PAGE_EXEC;
    
    if (read && write && !exec) {
        return "RW-";  // Read-Write
    } else if (read && !write && exec) {
        return "R-X";  // Read-Execute
    } else if (read && !write && !exec) {
        return "R--";  // Read-Only
    } else if (!read && !write && !exec) {
        return "---";  // No access
    } else {
        return "INVALID";  // W^X violation or invalid combo
    }
}

// Initialize W^X enforcement
void wx_init(void) {
    // In real implementation:
    // 1. Set up page fault handlers
    // 2. Configure MMU for W^X enforcement
    // 3. Mark existing code sections as RX
    // 4. Mark data sections as RW
    
    // For now, just validate that we're enforcing W^X
}

// Check if address range violates W^X
int wx_check_violation(uint32_t addr, uint32_t size, uint32_t requested_perms) {
    if (!wx_validate_permissions(requested_perms)) {
        return 1;  // W^X violation
    }
    
    if (!wx_validate_address_range(addr, size)) {
        return 1;  // Invalid range
    }
    
    return 0;  // No violation
}