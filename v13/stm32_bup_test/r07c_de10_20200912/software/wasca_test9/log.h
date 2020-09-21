#ifndef _WL_LOG_H_
#define _WL_LOG_H_

#include <sys/alt_stdio.h>
#include <string.h>
#include <system.h>
#include <altera_avalon_pio_regs.h>

#include <fcntl.h>
#include <unistd.h>

/* For wl_logset_param_t structure definition. */
#include "wasca_defs.h"


extern log_infos_t* _log_inf_ptr;


/* Initialize log internals. */
void log_init(void);


#if LOG_ENABLE == 1

/* Append specified data to logs circular buffer. */
void log_cbwrite(unsigned char* data, unsigned long datalen);

/* Log output function. */
void logout_internal(int level, const char* fmt, ... );
#   define logout(_LVL_, ...) logout_internal(_LVL_, __VA_ARGS__)

void logflush_internal(void);
#   define logflush() logflush_internal()

#else
    /* Remove log output. */
#   define logout(_LVL_, ...)
#   define logflush()
#endif


#endif // _WL_LOG_H_
