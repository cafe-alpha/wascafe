#ifndef __FAT_INTERNAL_H__
#define __FAT_INTERNAL_H__

// Printf output (directory listing / debug)
#include <stdarg.h>
void sdc_logout_internal(char* string, ...);
#define FAT_PRINTF(a) sdc_logout_internal a

#endif
