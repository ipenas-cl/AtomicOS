#ifndef SECURITY_H
#define SECURITY_H

#include <stdint.h>

// Security subsystem initialization
void security_init(void);

// OpenBSD-inspired security features
int pledge(const char *promises, const char *execpromises);
int unveil(const char *path, const char *permissions);

// Memory protection
void enable_wx_protection(void);
void enable_aslr(void);

// Stack protection
void enable_stack_protection(void);

#endif