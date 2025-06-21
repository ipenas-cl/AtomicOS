// Basic stdbool.h for AtomicOS
#ifndef _STDBOOL_H
#define _STDBOOL_H

#ifndef __cplusplus

// Define bool as unsigned char for compatibility
typedef unsigned char bool;
#define true  1
#define false 0

#endif

#define __bool_true_false_are_defined 1

#endif