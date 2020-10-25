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


#if MEMTEST_ENABLE == 1
#ifdef __ALTERA_AVALON_ONCHIP_MEMORY2_1
    /* Don't need to declare anything special here. */
#else
    /* Small buffer where to safely do memory testing
     * when extra OCRAM is not available.
     */
    unsigned char _memtest_ocram_buff[1024];
#endif
#endif // MEMTEST_ENABLE == 1

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

    /* Random access memory test. */
    if(memt->params & (WL_MEMTEST_R8B | WL_MEMTEST_R16B | WL_MEMTEST_R32B))
    {
        /* Initialize random number generator. */
        srand(memt->rand_seed);

        for(j=0; j<3; j++)
        {
            if(j == 0)
            {
                if((memt->params & WL_MEMTEST_R8B) == 0)
                {
                    continue;
                }
            }
            else if(j == 1)
            {
                if((memt->params & WL_MEMTEST_R16B) == 0)
                {
                    continue;
                }
            }
            else if(j == 2)
            {
                if((memt->params & WL_MEMTEST_R32B) == 0)
                {
                    continue;
                }
            }

            /* Setup reference test area, used for comparing random access.
             *
             * Extra OCRAM is used because (as of today) it is "just" used
             * as single usage use work memory. If it is not available, then
             * alternate solution is to use smaller area directly from stack.
             *
             * Note : rand function provided by stdlib returns number on
             *        16 bits only. A quick and dirty workaround can be
             *        done by calling rand function twice :
             *        ((rand() & 0xFFFF) << 16) | (rand() & 0xFFFF)
             */
#ifdef __ALTERA_AVALON_ONCHIP_MEMORY2_1
            unsigned char * l8  = (unsigned char*)ONCHIP_MEMORY2_1_BASE;
            unsigned long len   = ONCHIP_MEMORY2_1_SPAN;
#else
            unsigned char * l8  = _memtest_ocram_buff;
            unsigned long len   = sizeof(_memtest_ocram_buff);
#endif
            unsigned char * c8  = (unsigned char*)memt->addr_stt;

#define MEMTEST_OFFSETS_CNT (128)

            /* If test area larger than reference area, then split
             * reference area in two halves and use them on two
             * distant regions on test area.
             * This is to allow random memory access testing on different
             * rows even with limited resources.
             */
            unsigned long err_cnt = 0;
            if(memt->len > len)
            { /* Two tests on distant regions. */
                /* First region from start of test area, and
                 * second one from second half of test area.
                 */
                unsigned long offs12 = memt->len / 2;
                unsigned long len12  = len / 2;
                res->ra_length[0] = len12;
                res->ra_length[1] = len12;

                /* Copy reference data to OCRAM. */
                unsigned char* c32_stt = c8;
                unsigned char* l32_stt = l8;
                unsigned char* c32_end = c8 + offs12;
                unsigned char* l32_end = l8 + len12 ;
                for(i=0; i<len12; i++)
                {
                    l32_stt[i] = c32_stt[i];
                    l32_end[i] = c32_end[i];
                }

                /* Fill both SDRAM and reference buffer with random data at random addresses. */
                if(j == 0)
                {
                    for(i=0; i<MEMTEST_OFFSETS_CNT; i++)
                    {
                        /* Note : rand function is limited to 16 bits, but it's not
                         *        a problem here since reference area size is generally
                         *        smaller than 64KB.
                         */
                        unsigned long r = rand();
                        unsigned long memtest_offset = r % len12;
                        unsigned char val = (unsigned char)rand();

                        if(r & 0x00008000)
                        {
                            /* write on first half. */
                            l8[memtest_offset] = val;
                            c8[memtest_offset] = val;
                        }
                        else
                        {
                            /* write on second half. */
                            l8[len12  + memtest_offset] = val;
                            c8[offs12 + memtest_offset] = val;
                        }
                    }
                }
                else if(j == 1)
                {
                    unsigned short* l16 = (unsigned short*)l8;
                    unsigned short* c16 = (unsigned short*)c8;
                    unsigned long len16  = len12  / sizeof(unsigned short);
                    unsigned long offs16 = offs12 / sizeof(unsigned short);
                    for(i=0; i<MEMTEST_OFFSETS_CNT; i++)
                    {
                        unsigned long r = rand();
                        unsigned long memtest_offset = r % len16;

                        unsigned short val = (unsigned short)rand();
                        if(r & 0x00008000)
                        {
                            /* write on first half. */
                            l16[memtest_offset] = val;
                            c16[memtest_offset] = val;
                        }
                        else
                        {
                            /* write on second half. */
                            l16[len16  + memtest_offset] = val;
                            c16[offs16 + memtest_offset] = val;
                        }
                    }
                }
                else // if(j == 2)
                {
                    unsigned long* l32 = (unsigned long*)l8;
                    unsigned long* c32 = (unsigned long*)c8;
                    unsigned long len32  = len12  / sizeof(unsigned long);
                    unsigned long offs32 = offs12 / sizeof(unsigned long);
                    for(i=0; i<MEMTEST_OFFSETS_CNT; i++)
                    {
                        unsigned long r = rand();
                        unsigned long memtest_offset = r % len32;

                        unsigned long val = ((rand() & 0xFFFF) << 16) | (rand() & 0xFFFF);
                        if(r & 0x00008000)
                        {
                            /* write on first half. */
                            l32[memtest_offset] = val;
                            c32[memtest_offset] = val;
                        }
                        else
                        {
                            /* write on second half. */
                            l32[len32  + memtest_offset] = val;
                            c32[offs32 + memtest_offset] = val;
                        }
                    }
                }

                /* Verify whole chunk with reference data. */
                c32_stt = c8;
                l32_stt = l8;
                c32_end = c8 + offs12;
                l32_end = l8 + len12 ;
                for(i=0; i<len12; i++)
                {
                    if(c32_stt[i] != l32_stt[i])
                    {
                        err_cnt++;
                    }
                    if(c32_end[i] != l32_end[i])
                    {
                        err_cnt++;
                    }
                }
            }
            else
            { /* Normal test on one continous region. */
                len = memt->len;
                res->ra_length[0] = len;
                res->ra_length[1] = 0;

                /* Copy reference data to OCRAM. */
                //memcpy(l8 , c8, len);
                unsigned long* l32 = (unsigned long *)l8;
                unsigned long* c32 = (unsigned long *)c8;
                for(i=0; i<(len / sizeof(unsigned long)); i++)
                {
                    *l32++ = *c32++;
                }

                /* Fill both SDRAM and reference buffer with random data at random addresses. */
                if(j == 0)
                {
                    for(i=0; i<MEMTEST_OFFSETS_CNT; i++)
                    {
                        /* Note : rand function is limited to 16 bits, but it's not
                         *        a problem here since reference area size is generally
                         *        smaller than 64KB.
                         */
                        unsigned long memtest_offset = rand() % len;
                        unsigned char val = (unsigned char)rand();
                        l8[memtest_offset] = val;
                        c8[memtest_offset] = val;
                    }
                }
                else if(j == 1)
                {
                    unsigned short* l16 = (unsigned short*)l8;
                    unsigned short* c16 = (unsigned short*)c8;
                    unsigned long len16 = len / sizeof(unsigned short);
                    for(i=0; i<MEMTEST_OFFSETS_CNT; i++)
                    {
                        unsigned long memtest_offset = rand() % len16;

                        unsigned short val = (unsigned short)rand();
                        l16[memtest_offset] = val;
                        c16[memtest_offset] = val;
                    }
                }
                else // if(j == 2)
                {
                    unsigned long * l32 = (unsigned long*)l8;
                    unsigned long * c32 = (unsigned long*)c8;
                    unsigned long len32 = len / sizeof(unsigned long);
                    for(i=0; i<MEMTEST_OFFSETS_CNT; i++)
                    {
                        unsigned long memtest_offset = rand() % len32;

                        unsigned long val = ((rand() & 0xFFFF) << 16) | (rand() & 0xFFFF);
                        l32[memtest_offset] = val;
                        c32[memtest_offset] = val;
                    }
                }

                /* Verify whole chunk with reference data. */
                l32 = (unsigned long *)l8;
                c32 = (unsigned long *)c8;
                for(i=0; i<(len / sizeof(unsigned long)); i++)
                {
                    if(*c32++ != *l32++)
                    {
                        err_cnt++;
                    }
                }
            }

            /* Update error counts. */
            res->err_cnt += err_cnt;
            if(err_cnt > 255)
            {
                res->ra_err_cnt[j] = 255;
            }
            else
            {
                res->ra_err_cnt[j] = (unsigned char)err_cnt;
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

