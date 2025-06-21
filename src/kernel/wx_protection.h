/*
 * W^X Protection Header for AtomicOS
 */

#ifndef WX_PROTECTION_H
#define WX_PROTECTION_H

#include <stdint.h>

// W^X permission validation
int wx_validate_permissions(uint32_t permissions);

// Permission setting functions
uint32_t wx_set_read_only(uint32_t permissions);
uint32_t wx_set_read_write(uint32_t permissions);
uint32_t wx_set_read_exec(uint32_t permissions);

// Memory protection transitions
int wx_make_writable(uint32_t addr, uint32_t size);
int wx_make_executable(uint32_t addr, uint32_t size);

// Validation functions
int wx_validate_address_range(uint32_t addr, uint32_t size);
int wx_check_violation(uint32_t addr, uint32_t size, uint32_t requested_perms);

// Utility functions
const char* wx_get_permissions_string(uint32_t permissions);
void wx_init(void);

#endif // WX_PROTECTION_H