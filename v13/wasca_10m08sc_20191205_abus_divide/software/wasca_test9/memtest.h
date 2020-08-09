#ifndef _WL_MEMTEST_H_
#define _WL_MEMTEST_H_

#include <sys/alt_stdio.h>
#include <string.h>
#include <system.h>

#include "wasca_defs.h"

/*
 * ------------------------------------------
 * - Memory Test Module ---------------------
 * ------------------------------------------
 */
void memory_test_init(void);
void do_memory_test(wl_memtest_params_t* memt);


#endif // _WL_MEMTEST_H_
