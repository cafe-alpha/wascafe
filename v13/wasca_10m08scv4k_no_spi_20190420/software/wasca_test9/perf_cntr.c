#include "perf_cntr.h"

#include <unistd.h>
#include <fcntl.h>

#include "wasca_defs.h"
#include "log.h"

void wasca_perf_logout(char* name, alt_u64 elapsed_time)
{
    alt_u64 total_clocks = elapsed_time;
    unsigned long clk_cnt_u32;

    unsigned long clkhz = alt_get_cpu_freq();
    unsigned long time_usec;

    clk_cnt_u32 = (unsigned long)total_clocks;

    total_clocks *= 1000*1000;
    time_usec = (unsigned long)(total_clocks / clkhz);

    logout(WL_LOG_IMPORTANT, "PERF[%s]:%u CLK (%u usec)", name, clk_cnt_u32, time_usec);
}

