#include "memtest.h"

#include <altera_avalon_pio_regs.h>

#include "crc.h"
#include "link.h"
#include "log.h"
#include "perf_cntr.h"


void memory_test_init(void)
{
#if MEMTEST_ENABLE == 1
    wl_memtest_res_t* res = &(wasca_internals->memtest_res);

    memset(res, 0, sizeof(wl_memtest_res_t));

    /* Indicate memory test feature as available. */
    res->available = 1;
#endif // MEMTEST_ENABLE == 1
}




void do_memory_test(wl_memtest_params_t* memt)
{
#if MEMTEST_ENABLE == 1
    int i, j;
    wl_memtest_res_t* res = &(wasca_internals->memtest_res);

    /* Initialize memory test status :
     *  - Mark status as "testing".
     *  - Reset error count.
     *  - Backup copy test parameters in global buffer.
     */
    res->available = 0;
    res->err_cnt = 0;
    memcpy(&res->params, memt, sizeof(wl_memtest_params_t));

    /*
     * Red LED    : On when SDRAM error detected
     * Green LED  : Periodically blinks during SDRAM test
     * Orange LED : Change according to switch (push: on, release, off)
     */
    unsigned char red_led    = 0;
    unsigned char green_led  = 0;
    unsigned char orange_led = 0;

    unsigned long ite_cnt = 0;

    unsigned char * p8  = (unsigned char  *)memt->addr_stt;
    unsigned short* p16 = (unsigned short *)memt->addr_stt;
    unsigned long * p32 = (unsigned long  *)memt->addr_stt;

    { /* Do a "fast" test without anything extra. */
        unsigned long wr, rd;

        /* Fast memory test : perform access 4 bytes at once. */
        ite_cnt = memt->len / sizeof(unsigned long);

        /* Write memory area to test, 
         * and measure memory write time.
         */
        wr = 0x12345678;
        WASCA_PERF_START_MEASURE();
        for(i=ite_cnt; i; i--)
        {
            *p32++ = wr;
        }
        WASCA_PERF_STOP_MEASURE();
        alt_u64 write_time = WASCA_PERF_GET_TIME();
        if((write_time >> 32) == 0)
        {
            res->write_time = write_time & 0xFFFFFFFF;
        }
        else
        {
            /* Overflow. */
            res->write_time = 0xFFFFFFFF;
        }

        /* Read back memory area to test, 
         * and measure memory read time.
         */
        p32 = (unsigned long *)memt->addr_stt;
        WASCA_PERF_START_MEASURE();
        for(i=ite_cnt; i; i--)
        {
            rd = *p32++;
            if(rd != wr)
            {
                res->err_cnt++;
            }
        }
        WASCA_PERF_STOP_MEASURE();
        alt_u64 read_time = WASCA_PERF_GET_TIME();
        if((read_time >> 32) == 0)
        {
            res->read_time = read_time & 0xFFFFFFFF;
        }
        else
        {
            /* Overflow. */
            res->read_time = 0xFFFFFFFF;
        }



        /* Do the same as above, but with
         * both 55/AA read and write combined.
         */
        p32 = (unsigned long *)memt->addr_stt;
        WASCA_PERF_START_MEASURE();
        for(i=ite_cnt; i; i--)
        {
            wr = 0x55555555; *p32 = wr; rd = *p32;
            if(rd != wr)
            {
                res->err_cnt++;
            }
            wr = 0xAAAAAAAA; *p32 = wr; rd = *p32;
            if(rd != wr)
            {
                res->err_cnt++;
            }
            p32++;
        }
    }

    { /* Do a "full featured" test. */
        int u8_access  = (memt->params & WL_MEMTEST_8B   ? 1 : 0);
        int u16_access = (memt->params & WL_MEMTEST_16B  ? 1 : 0);
        int u32_access = (memt->params & WL_MEMTEST_32B  ? 1 : 0);
        int test_55AA  = (memt->params & WL_MEMTEST_55AA ? 1 : 0);
        int test_incr  = (memt->params & WL_MEMTEST_INCR ? 1 : 0);

        WASCA_PERF_START_MEASURE();

        for(i=memt->len; i; i-=4)
        {
            if(u8_access)
            {
                for(j=4; j; j--)
                {
                    unsigned char wr, rd;
                    if(test_55AA)
                    {
                        wr = 0x55; *p8 = wr; rd = *p8;
                        if((rd != wr) && (res->err_cnt < 10))
                        {
                            logout(WL_LOG_ERROR, "[MEMTEST]0x55:0x%02X error at address 0x%08X", rd, (unsigned int)p8);
                            res->err_cnt++;
                        }
                        wr = 0xAA; *p8 = wr; rd = *p8;
                        if((rd != wr) && (res->err_cnt < 10))
                        {
                            logout(WL_LOG_ERROR, "[MEMTEST]0xAA:0x%02X error at address 0x%08X", rd, (unsigned int)p8);
                            res->err_cnt++;
                        }
                    }
                    if(test_incr)
                    {
                        wr = (unsigned char)((i+j) & 0xFF); *p8 = wr; rd = *p8;
                        if((rd != wr) && (res->err_cnt < 10))
                        {
                            logout(WL_LOG_ERROR, "[MEMTEST]incr[0x%02X:0x%02X] error at address 0x%08X", wr, rd, (unsigned int)p8);
                            res->err_cnt++;
                        }
                    }
                    p8++;
                }
            }

            if(u16_access)
            {
                for(j=2; j; j--)
                {
                    unsigned short wr, rd;
                    if(test_55AA)
                    {
                        wr = 0x5555; *p16 = wr; rd = *p16;
                        if((rd != wr) && (res->err_cnt < 10))
                        {
                            logout(WL_LOG_ERROR, "[MEMTEST]0x5555:0x%04X error at address 0x%08X", rd, (unsigned int)p16);
                            res->err_cnt++;
                        }
                        wr = 0xAAAA; *p16 = wr; rd = *p16;
                        if((rd != wr) && (res->err_cnt < 10))
                        {
                            logout(WL_LOG_ERROR, "[MEMTEST]0xAAAA:0x%04X error at address 0x%08X", rd, (unsigned int)p16);
                            res->err_cnt++;
                        }
                    }
                    if(test_incr)
                    {
                        wr = (unsigned short)((i+j) & 0xFFFF); *p16 = wr; rd = *p16;
                        if((rd != wr) && (res->err_cnt < 10))
                        {
                            logout(WL_LOG_ERROR, "[MEMTEST]incr[0x%04X:0x%04X] error at address 0x%08X", wr, rd, (unsigned int)p16);
                            res->err_cnt++;
                        }
                    }
                    p16++;
                }
            }

            if(u32_access)
            {
                unsigned long wr, rd;
                if(test_55AA)
                {
                    wr = 0x55555555; *p32 = wr; rd = *p32;
                    if((rd != wr) && (res->err_cnt < 10))
                    {
                        logout(WL_LOG_ERROR, "[MEMTEST]0x55555555:0x%08X error at address 0x%08X", rd, (unsigned int)p32);
                        res->err_cnt++;
                    }
                    wr = 0xAAAAAAAA; *p32 = wr; rd = *p32;
                    if((rd != wr) && (res->err_cnt < 10))
                    {
                        logout(WL_LOG_ERROR, "[MEMTEST]0xAAAAAAAA:0x%08X error at address 0x%08X", rd, (unsigned int)p32);
                        res->err_cnt++;
                    }
                }
                if(test_incr)
                {
                    wr = (unsigned long)(i+j); *p32 = wr; rd = *p32;
                    if((rd != wr) && (res->err_cnt < 10))
                    {
                        logout(WL_LOG_ERROR, "[MEMTEST]incr[0x%08X:0x%08X] error at address 0x%08X", wr, rd, (unsigned int)p32);
                        res->err_cnt++;
                    }
                }
                p32++;
            }

            if((i % (16*1024)) == 0)
            {
                green_led  = 1-green_led;
                red_led    = (res->err_cnt == 0 ? 0 : 1);
                orange_led = ((IORD(SWITCHES_BASE, 0) & 0x01) == 0 ? 1 : 0);

                unsigned char tmp = (red_led << 0) | (green_led << 1) | (orange_led << 2);
                IOWR(LEDS_BASE, 0, tmp);

                /* Performing link I/O here hangs NIOS CPU ?!
                 * That's probably because we are coming from this
                 * function when performing a test from SerialTerm.
                 */
                //link_do_io();
            }
        }
    }

    /* Store time elapsed during memory test.
     * It may be used for benchmark on PC side.
     */
    WASCA_PERF_STOP_MEASURE();
    alt_u64 misc_time = WASCA_PERF_GET_TIME();
    if((misc_time >> 32) == 0)
    {
        res->misc_time = misc_time & 0xFFFFFFFF;
    }
    else
    {
        /* Overflow. */
        res->misc_time = 0xFFFFFFFF;
    }

    res->cntr_clk = alt_get_cpu_freq();
    res->ite_cnt  = ite_cnt;
    wasca_perf_logout("MEMTEST", misc_time);

    logout(WL_LOG_DEBUGEASY, "[MEMTEST]Ended. Error count:%u, Addr:%08X, Size:%u KB, ite_cnt:%u, cntr_clk:%u"
        , res->err_cnt
        , memt->addr_stt
        , memt->len>>10
        , ite_cnt
        , res->cntr_clk);

    /* Mark memory test status as "finished". */
    res->available = 1;

#else // MEMTEST_ENABLE != 1

    /* Throw dummy result with memory test feature availability flag reset. */
    wl_memtest_res_t* res = &(wasca_internals->memtest_res);
    memset(res, 0, sizeof(wl_memtest_res_t));

#endif // MEMTEST_ENABLE != 1
}

