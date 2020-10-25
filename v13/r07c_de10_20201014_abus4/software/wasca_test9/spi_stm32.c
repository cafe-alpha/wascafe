#include <string.h>

#include "spi_stm32.h"

#include "crc.h"
#include "de10_7seg.h"
#include "log.h"
#include "perf_cntr.h"


/* SPI related log output flag.
 *  - 0 : no logs (normal mode)
 *  - 1 : output logs
 * 
 * Warning : SPI transfer becomes significantly
 *           slower when log output is enabled.
 */
#define SPI_DEBUG_LOGS 0




/* Definitions for block SPI core. */
#define BUFFSPI_BUFFER0_WRITE       0x0000
#define BUFFSPI_BUFFER1_WRITE       0x0800
#define BUFFSPI_BUFFER0_READ        0x1000
#define BUFFSPI_BUFFER1_READ        0x1800
#define BUFFSPI_REG_START           0x2000
#define BUFFSPI_REG_LENGTH          0x2001
#define BUFFSPI_REG_COUNTER         0x2002
#define BUFFSPI_REG_CS_MODE         0x2003
#define BUFFSPI_REG_DELAY           0x2004
#define BUFFSPI_REG_BUFFER_SELECT   0x2005
#define BUFFSPI_REG_MAGIC_DEAF      0x2006
#define BUFFSPI_REG_MAGIC_FACE      0x2007

/* Maximum word (16 bits) count for each (Tx0, Tx1, Rx0, Rx1) buffers.
 *
 * Memory usage by the core is then :
 *  BUFFSPI_MAXCNT * 4 (buffer count) * 2 (bytes per word)
 *
 * Simple speed bench with several buffer sizes :
 * - Chunk = 512 words -> 100% speed
 * - Chunk = 128 words ->  98% speed
 * - Chunk =  64 words ->  98% speed
 * - Chunk =  32 words ->  97% speed
 * - Chunk =  16 words ->  94% speed
 * - Chunk =   8 words ->  90% speed
 * -> let's use 128 words as maximum transmission size for block SPI core.
 */
//#define BUFFSPI_MAXCNT              512
#define BUFFSPI_MAXCNT              128

void spi_init(void)
{
#if SPI_ENABLE == 1
    volatile unsigned short * p16_spi = (unsigned short *)BUFFERED_SPI_0_BASE;

    /* Clear block SPI transmit and receive buffers.
     * (Not super necessary, but just in case of)
     */
    int i;
    for(i=0; i<BUFFSPI_MAXCNT; i++)
    {
        p16_spi[BUFFSPI_BUFFER0_WRITE + i] = 0;
        p16_spi[BUFFSPI_BUFFER1_WRITE + i] = 0;
        p16_spi[BUFFSPI_BUFFER0_READ  + i] = 0;
        p16_spi[BUFFSPI_BUFFER1_READ  + i] = 0;
    }

    p16_spi[BUFFSPI_REG_LENGTH       ] = sizeof(wl_spi_pkt_t) / sizeof(unsigned short); /* Number of words = 16 bit each. */
    p16_spi[BUFFSPI_REG_CS_MODE      ] =   1; /* CS blinking. */
    p16_spi[BUFFSPI_REG_DELAY        ] =  70; /* 70 SPI clocks @ 7.25 Mhz between each 16 bit. */
    p16_spi[BUFFSPI_REG_BUFFER_SELECT] =   0; /* Using buffer 0. */

#endif // SPI_ENABLE == 1
}



#if SPI_ENABLE == 1


/* Common function for SPI data send and receive.
 * 
 * Both "single" and "dual" functions are available :
 *  - Single : use only Rx0 and Tx0
 *  - Dual   : use only Rx0/Rx1 and Tx0/Tx1
 */
void spi_sendreceive_single(unsigned short* snd_ptr, unsigned short* rcv_ptr, unsigned long count)
{
    volatile unsigned short* p16_spi = (unsigned short *)BUFFERED_SPI_0_BASE;

    int pos;
    for(pos=0; pos<count; pos+=BUFFSPI_MAXCNT)
    {
        int len = BUFFSPI_MAXCNT;
        if((pos + BUFFSPI_MAXCNT) > count)
        {
            len = count - pos;
        }

        /* Setting up the core. */
        p16_spi[BUFFSPI_REG_LENGTH       ] = len; /* Number of words = 16 bit each. */
        p16_spi[BUFFSPI_REG_CS_MODE      ] =   1; /* CS blinking. */
        p16_spi[BUFFSPI_REG_DELAY        ] =   1; /* 1 SPI clock @ 7.25 Mhz between each 16 bit. */
        p16_spi[BUFFSPI_REG_BUFFER_SELECT] =   0; /* Using buffer 0. */

        /* Fill transmit buffer with input data. */
        int i;
        if(snd_ptr)
        {
            for(i=0; i<len; i++)
            {
                p16_spi[BUFFSPI_BUFFER0_WRITE + i] = *snd_ptr++;
            }
        }

        /* Fire spi transaction. */
        p16_spi[BUFFSPI_REG_START] = 1; /* Go go go! */


        /* Wait until complete. */
#if 0 // Official way of waiting (with transfer busy flag polling), which doesn't works here.
        while(p16_spi[BUFFSPI_REG_START] != 0);
#else // Alternate waiting with check of the count of words sent so far.
        volatile unsigned long wait_cnt = 0;
        while(1)
        {
            volatile unsigned short c1 = p16_spi[BUFFSPI_REG_COUNTER];
            volatile unsigned short c2 = p16_spi[BUFFSPI_REG_COUNTER];
            wait_cnt++;
            if((c1 >= len) && (c2 >= len))
            {
                break;
            }
        }
#if SPI_DEBUG_LOGS == 1
        logout(WL_LOG_DEBUGEASY, "wait_cnt[%07u]", wait_cnt);
#endif // SPI_DEBUG_LOGS == 1
#endif

        /* Read the received data. */
        if(rcv_ptr)
        {
#if SPI_DEBUG_LOGS == 1 // Extra log output to verify that everything is received
            for(i=0; i<5; i++)
            {
                logout(WL_LOG_DEBUGEASY, "B[%04X] C[%04u] D:%04X %04X %04X %04X", 
                    p16_spi[BUFFSPI_REG_START], 
                    p16_spi[BUFFSPI_REG_COUNTER], 
                    p16_spi[BUFFSPI_BUFFER0_READ +   0], 
                    p16_spi[BUFFSPI_BUFFER0_READ +  10], 
                    p16_spi[BUFFSPI_BUFFER0_READ + 100], 
                    p16_spi[BUFFSPI_BUFFER0_READ + len-1]);
            }
#endif // SPI_DEBUG_LOGS == 1

            for(i=0; i<len; i++)
            {
                *rcv_ptr++ = p16_spi[BUFFSPI_BUFFER0_READ + i];
            }
        }
    }
}


void spi_sendreceive_dual(unsigned short* snd_ptr, unsigned short* rcv_ptr, unsigned long count)
{
    volatile unsigned short* p16_spi = (unsigned short *)BUFFERED_SPI_0_BASE;

    /* Setting up core base registers. */
    p16_spi[BUFFSPI_REG_CS_MODE] = 1; /* CS blinking. */
    p16_spi[BUFFSPI_REG_DELAY  ] = 1; /* 1 SPI clock @ 7.25 Mhz between each 16 bit. */

    int next_len = (BUFFSPI_MAXCNT > count ? count : BUFFSPI_MAXCNT);
    int prev_len = 0;
    int spi_buffer = 0;
    volatile unsigned short* p16_wk;

    int i;
    int pos;
    for(pos=0; pos<count; pos+=BUFFSPI_MAXCNT)
    {
        int len = next_len;

        /* Prepare length of next block. */
        next_len = BUFFSPI_MAXCNT;
        if(((pos+len) + BUFFSPI_MAXCNT) > count)
        {
            next_len = count - (pos+len);
        }

        /* Setting up transfer length and buffer ID. */
        p16_spi[BUFFSPI_REG_LENGTH       ] = len; /* Number of words = 16 bit each. */
        p16_spi[BUFFSPI_REG_BUFFER_SELECT] = spi_buffer;

        if((snd_ptr) && (pos == 0))
        {
            /* Fill transmit buffer with first round of input data. */
            p16_wk = p16_spi + (spi_buffer == 0 ? BUFFSPI_BUFFER0_WRITE : BUFFSPI_BUFFER1_WRITE);
            for(i=0; i<len; i++)
            {
                *p16_wk++ = *snd_ptr++;
            }
        }

        /* Fire spi transaction. */
        p16_spi[BUFFSPI_REG_START] = 1; /* Go go go! */

        if((snd_ptr) && (next_len != 0))
        {
            /* Fill transmit buffer with next round of input data
             * while current one is being sent.
             */
            p16_wk = p16_spi + (spi_buffer == 0 ? BUFFSPI_BUFFER1_WRITE : BUFFSPI_BUFFER0_WRITE);
            for(i=0; i<next_len; i++)
            {
                *p16_wk++ = *snd_ptr++;
            }
        }

        if((rcv_ptr) && (prev_len != 0))
        {
            /* Read previous round of output data
             * while current one is being received.
             */
            p16_wk = p16_spi + (spi_buffer == 0 ? BUFFSPI_BUFFER1_READ : BUFFSPI_BUFFER0_READ);
            for(i=0; i<prev_len; i++)
            {
                *rcv_ptr++ = *p16_wk++;
            }
        }


        /* Wait until complete. */
#if 0 // Official way of waiting (with transfer busy flag polling) doesn't works here.
        while(p16_spi[BUFFSPI_REG_START] != 0);
#else // Alternate waiting with check of the count of words sent so far.
        volatile unsigned long wait_cnt = 0;
        while(1)
        {
            volatile unsigned short c1 = p16_spi[BUFFSPI_REG_COUNTER];
            volatile unsigned short c2 = p16_spi[BUFFSPI_REG_COUNTER];
            wait_cnt++;
            if((c1 >= len) && (c2 >= len))
            {
                break;
            }
        }
#if SPI_DEBUG_LOGS == 1
        logout(WL_LOG_DEBUGEASY, "wait_cnt[%07u]", wait_cnt);
#endif // SPI_DEBUG_LOGS == 1
#endif

        if((rcv_ptr) && (next_len == 0))
        {
            /* Read last round of received data. */
            p16_wk = p16_spi + (spi_buffer == 0 ? BUFFSPI_BUFFER0_READ : BUFFSPI_BUFFER1_READ);
            for(i=0; i<len; i++)
            {
                *rcv_ptr++ = *p16_wk++;
            }
        }

        /* Toggle SPI buffer. */
        spi_buffer = 1-spi_buffer;
        prev_len = len;
    }
}


/* Send specified data over SPI to STM32. */
#define spi_send(_PTR_, _COUNT_) spi_sendreceive_dual(_PTR_, NULL, _COUNT_)

/* Receive specified length of data over SPI from STM32. */
#define spi_receive(_PTR_, _COUNT_) spi_sendreceive_dual(NULL, _PTR_, _COUNT_)


/* Initialize SPI header contents. */
void spi_init_header(wl_spi_common_hdr_t* hdr)
{
    memset(hdr, 0, sizeof(wl_spi_common_hdr_t));
    hdr->magic_ca = 0xCA;
    hdr->magic_fe = 0xFE;
}

#endif // SPI_ENABLE == 1




void spi_get_version(wl_spicomm_version_t* ver)
{
#if SPI_ENABLE == 1

    wl_spi_pkt_t pkt_dat;
    wl_spi_pkt_t* pkt = &pkt_dat;

    /* Setup packet header. */
    spi_init_header(&pkt->cmn);
    pkt->cmn.command  = WL_SPICMD_VERSION;
    pkt->cmn.pkt_len  = 0; // TODO
    pkt->cmn.rsp_len  = sizeof(wl_spi_pkt_t); // TODO


    /* Wait for STM32 to be ready until being able to send packet. */
    while(IORD(SPI_SYNC_BASE, 0) == 1);

    spi_send((unsigned short*)pkt, sizeof(wl_spi_pkt_t) / sizeof(unsigned short));

    /* Wait for STM32 to be ready until receiving response. */
    while(IORD(SPI_SYNC_BASE, 0) == 0);

    spi_receive((unsigned short*)pkt, pkt->cmn.rsp_len / sizeof(unsigned short));

    /* Don't forget to copy version
     * informations to output buffer.
     */
    memcpy(ver, pkt->data, sizeof(wl_spicomm_version_t));

#if SPI_DEBUG_LOGS == 1
    //if(log_output)
    {
        /* Output received data to log. */
        logout(WL_LOG_DEBUGEASY, "[SPI] HDR[%02X %02X] PARAMS[%s]"
            , pkt->cmn.magic_ca
            , pkt->cmn.magic_fe
            //, pkt->cmn.pkt_len
            //, pkt->cmn.rsp_len
            , pkt->params);
        int i;
        for(i=0; i<8; i++)
        {
            unsigned char c = ((unsigned char*)pkt)[i];
            logout(WL_LOG_DEBUGNORMAL, "[SPI]| Rcv[%3d]:0x%02X (%c)", i, c, c);
        }
    }
#endif // SPI_DEBUG_LOGS == 1

#endif // SPI_ENABLE == 1
}



/**
 * spi_generic_access
 *
 * Generic STM32-via memory access function.
 * It supports raw memory access and backup memory access.
 *
 * About each parameters :
 *  - pkt      : pointer to buffer for SPI packet
 *  - command  : which command to use. Example memory read, memory write etc
 *  - addr     : start address
 *  - len      : data length
 *  - rsp_len  : response (answer) length, including start header
 *  - snd_data : send data buffer
 *  - rcv_data : receive data buffer
 * 
 * Returns 1 when no error happened, zero else.
**/
int spi_generic_access
(
    unsigned char command, 
    unsigned long addr, unsigned long len, 
    unsigned short rsp_len, 
    unsigned char* snd_data, 
    unsigned char* rcv_data
)
{
#if SPI_ENABLE == 1
rsp_len = sizeof(wl_spi_pkt_t); // TMP

    /* Clamp size to one SPI packet.
     * This should be improved to use as many packets as needed ...
     *
     * Note : this is now clamped on caller side, hence commented-out.
     */
    // if(len > WL_SPI_DATA_LEN)
    // {
    //     len = WL_SPI_DATA_LEN;
    // }

    wl_spi_pkt_t pkt_dat;
    wl_spi_pkt_t* pkt = &pkt_dat;

    /* Setup packet header. */
    spi_init_header(&pkt->cmn);
    pkt->cmn.command  = command;
    pkt->cmn.pkt_len  = 0; // TODO
    pkt->cmn.rsp_len  = rsp_len;

    /* Setup memory read parameters. */
    wl_spicomm_memacc_t* params = (wl_spicomm_memacc_t*)pkt->params;
    params->addr = addr;
    params->len  = len;

    if(snd_data)
    {
        memcpy(pkt->data, snd_data, len);
    }

    /* Wait for STM32 to be ready until being able to send packet. */
    while(IORD(SPI_SYNC_BASE, 0) == 1);

    spi_send((unsigned short*)pkt, sizeof(wl_spi_pkt_t) / sizeof(unsigned short));

    /* Wait for STM32 to be ready until receiving response. */
    while(IORD(SPI_SYNC_BASE, 0) == 0);

    spi_receive((unsigned short*)pkt, pkt->cmn.rsp_len / sizeof(unsigned short));

    /* Don't forget to copy read data to output buffer. */
    if(rcv_data)
    {
        memcpy(rcv_data, pkt->data, len);
    }

#if SPI_DEBUG_LOGS == 1
    //if(log_output)
    {
        /* Output received data to log. */
        logout(WL_LOG_DEBUGEASY, "[SPI] HDR[%02X %02X] PARAMS[%s] rsp_len[%u]"
            , pkt->cmn.magic_ca
            , pkt->cmn.magic_fe
            //, pkt->cmn.pkt_len
            , pkt->cmn.rsp_len
            , pkt->params);
        int i;
        for(i=0; i<8; i++)
        {
            unsigned char c = ((unsigned char*)pkt)[i];
            logout(WL_LOG_DEBUGNORMAL, "[SPI]| Rcv[%3d]:0x%02X (%c)", i, c, c);
        }
    }
#endif // SPI_DEBUG_LOGS == 1

    /* On STM32 side, error is always specified by zero-ing the
     * data length parameter, and vice-versa
     * So simply check this parameter to know if an error happened or not.
     */
    return (params->len == 0 ? 0 : 1);
#else
    return 0;
#endif // SPI_ENABLE != 1
}



/* The functions to do raw memory read/write. */
int spi_mem_read(unsigned long addr, unsigned long len, unsigned char* data)
{
    spi_generic_access(WL_SPICMD_MEMREAD, addr, len, WL_SPI_CUSTOM_RSPLEN(WL_SPI_DATA_LEN)/*rsp_len*/, NULL/*snd_data*/, data/*rcv_data*/);

    return 1;
}
int spi_mem_write(unsigned long addr, unsigned long len, unsigned char* data)
{
    spi_generic_access(WL_SPICMD_MEMWRITE, addr, len, WL_SPI_CUSTOM_RSPLEN(0)/*rsp_len*/, data/*snd_data*/, NULL/*rcv_data*/);

    return 1;
}

/* The functions to do backup memory read/write/etc. */
int spi_bup_open(unsigned long len_kb)
{
    return spi_generic_access(WL_SPICMD_BUPOPEN, 0/*block*/, len_kb, WL_SPI_PARAMS_RSPLEN/*rsp_len*/, NULL, NULL);
}
int spi_bup_read(unsigned long block, unsigned long len, unsigned char* data)
{
    return spi_generic_access(WL_SPICMD_BUPREAD, block, len, WL_SPI_FULL_RSPLEN/*rsp_len*/, NULL/*snd_data*/, data/*rcv_data*/);
}
int spi_bup_write(unsigned long block, unsigned long len, unsigned char* data)
{
    return spi_generic_access(WL_SPICMD_BUPWRITE, block, len, WL_SPI_PARAMS_RSPLEN/*rsp_len*/, data/*snd_data*/, NULL/*rcv_data*/);
}
int spi_bup_close(void)
{
    return spi_generic_access(WL_SPICMD_BUPCLOSE, 0/*block*/, 0/*len*/, WL_SPI_PARAMS_RSPLEN/*rsp_len*/, NULL, NULL);
}


/* Boot ROM access : retrieve informations. */
void spi_boot_getinfo(wl_spicomm_bootinfo_t* info, char* full_path)
{
    wl_spi_pkt_t pkt_dat;
    wl_spi_pkt_t* pkt = &pkt_dat;

    /* Setup packet header. */
    spi_init_header(&pkt->cmn);
    pkt->cmn.command  = WL_SPICMD_BOOTINFO;
    pkt->cmn.pkt_len  = 0; // TODO
    pkt->cmn.rsp_len  = sizeof(wl_spi_pkt_t);

    /* Wait for STM32 to be ready until being able to send packet. */
    while(IORD(SPI_SYNC_BASE, 0) == 1);

    spi_send((unsigned short*)pkt, sizeof(wl_spi_pkt_t) / sizeof(unsigned short));

    /* Wait for STM32 to be ready until receiving response. */
    while(IORD(SPI_SYNC_BASE, 0) == 0);

    spi_receive((unsigned short*)pkt, pkt->cmn.rsp_len / sizeof(unsigned short));

    /* Copy received data to output parameters. */
    memcpy(info, pkt->params, sizeof(wl_spicomm_bootinfo_t));

    if(full_path)
    {
        memcpy(full_path, pkt->data, WL_MAX_PATH);
        full_path[WL_MAX_PATH - 1] = '\0';
    }
}


/* Boot ROM access : read data. */
int spi_boot_readdata(unsigned long offset, unsigned long len, unsigned char* dst)
{
    wl_spi_pkt_t pkt_dat;
    wl_spi_pkt_t* pkt = &pkt_dat;

    /* Setup packet header. */
    spi_init_header(&pkt->cmn);
    pkt->cmn.command  = WL_SPICMD_BOOTREAD;
    pkt->cmn.pkt_len  = 0; // TODO
    pkt->cmn.rsp_len  = sizeof(wl_spi_pkt_t);

    /* Setup read parameters. */
    wl_spicomm_memacc_t* params = (wl_spicomm_memacc_t*)pkt->params;
    params->addr = offset;
    params->len  = len;

    /* Wait for STM32 to be ready until being able to send packet. */
    while(IORD(SPI_SYNC_BASE, 0) == 1);

    spi_send((unsigned short*)pkt, sizeof(wl_spi_pkt_t) / sizeof(unsigned short));

    /* Wait for STM32 to be ready until receiving response. */
    while(IORD(SPI_SYNC_BASE, 0) == 0);

    spi_receive((unsigned short*)pkt, pkt->cmn.rsp_len / sizeof(unsigned short));

    /* Copy back read data to destination buffer. */
    wl_spicomm_bootinfo_t* info = (wl_spicomm_bootinfo_t*)pkt->params;
    if(info->block_len != 0)
    {
        memcpy(dst, pkt->data, info->block_len);
    }

    return (info->status == WL_BOOTROM_END ? 1 : 0);
}




/*
 * ------------------------------------------
 * - BUP file test bench --------------------
 * ------------------------------------------
 */
#if BUP_TEST_BENCH_ENABLE == 1

unsigned long _bup_test_cnt = 0;
unsigned long _bup_write_usec_max = 0;
unsigned long _bup_read_usec_max  = 0;

void bup_test_bench(void)
{
    int i;

    /* Do test when second switch from 7-segs is up. */
    unsigned char sw_state = IORD(SWITCHES_BASE, 0) & 0x02;

    if(!sw_state)
    {
        _bup_test_cnt = 0;
        return;
    }

    /* Initialize 7-segs display during first test. */
    if(_bup_test_cnt == 0)
    {
        _bup_write_usec_max = 0;
        _bup_read_usec_max  = 0;
        h7seg_all_clear();
    }

    /* Last digit of 7-segs display : activity counter. */
    _bup_test_cnt++;
    unsigned char activity_cntr = (_bup_test_cnt / 10) & 0xF;
    h7seg_u4_out(5, activity_cntr);

    /* ------------------------------------------ */
    /* Open a backup file. */
    spi_bup_open(1024); // 1MB

    //logout(WL_LOG_DEBUGNORMAL, "bup_tb::OPEN END.", 0);

    /* ------------------------------------------ */
    /* Write one block of backup data. */
//TODO : bulk write several blocks, in a similar manner as BIOS frenetically writing a save data to cartridge
    unsigned char dummy_block[512];
    //unsigned long block_id = 0; // First block
    unsigned long block_id = (_bup_test_cnt * (_bup_test_cnt+1)) % 2000;
    for(i=0; i<sizeof(dummy_block); i++)
    {
        dummy_block[i] = (unsigned char)((i*i) ^ _bup_test_cnt);
    }
    //sprintf((char*)dummy_block, "Block #%u, write count: %u", (unsigned int)(block_id + 1), (unsigned int)_bup_test_cnt);

    WASCA_PERF_START_MEASURE();
    spi_bup_write(block_id, sizeof(dummy_block), dummy_block);
    WASCA_PERF_STOP_MEASURE();

    unsigned long write_usec = wasca_perf_get_usec(WASCA_PERF_GET_TIME());
    if(write_usec > _bup_write_usec_max)
    {
        _bup_write_usec_max = write_usec;
    }

    //logout(WL_LOG_DEBUGNORMAL, "bup_tb::WRITE END. Time:%2u.%03u ms. MAX:%2u.%03u ms. Block: %u", write_usec / 1000, write_usec % 1000, _bup_write_usec_max / 1000, _bup_write_usec_max % 1000, block_id);

    /* Display block write time on 7-segs first 4 digits, in 00.01 msec unit.
     * And, toggle display between real and and maximum values.
     * Example : 1.234 msec taken -> displays 01.23.
     */
    if(activity_cntr < 8)
    {
        h7seg_xdec_out(0/*stt_id*/, 4/*digits_count*/, 2/*dec_pos*/, write_usec / 10);
    }
    else
    {
        h7seg_xdec_out(0/*stt_id*/, 4/*digits_count*/, 2/*dec_pos*/, _bup_write_usec_max / 10);
    }



    /* ------------------------------------------ */
    /* Read back the block written just before. */
    unsigned char read_block[sizeof(dummy_block)];

    WASCA_PERF_START_MEASURE();
    spi_bup_read(block_id, sizeof(read_block), read_block);
    WASCA_PERF_STOP_MEASURE();

    unsigned long read_usec = wasca_perf_get_usec(WASCA_PERF_GET_TIME());
    if(read_usec > _bup_read_usec_max)
    {
        _bup_read_usec_max = read_usec;
    }

    int compare_result = memcmp(read_block, dummy_block, sizeof(dummy_block));

    /* Output read and write access measurement results. */
    logout
    (
        WL_LOG_DEBUGNORMAL, 
        "bup_tb::Wr:%2u.%03u/%2u.%03u ms. Rd:%2u.%03u/%2u.%03u ms. Cmpr:%s. Blk:%4u, Cnt: %u", 
        write_usec          / 1000, write_usec          % 1000, 
        _bup_write_usec_max / 1000, _bup_write_usec_max % 1000, 
        read_usec           / 1000, read_usec           % 1000, 
        _bup_read_usec_max  / 1000, _bup_read_usec_max  % 1000, 
        (compare_result != 0 ? "NG" : "OK"), 
        block_id, 
        _bup_test_cnt
    );

    /* Fifth digit of 7-segs display : displays "E" when compare error happened. */
    if(compare_result != 0)
    {
        h7seg_u4_out(4, 0xE);
    }
}
#endif // BUP_TEST_BENCH_ENABLE == 1





unsigned char char2pchar(unsigned char c)
{
    /* Display differently null and FF characters.
     * It helps to spot them in screen full of hexa codes.
     */
    if(c == 0x00) return '~';
    if(c == 0xFF) return 'X';

    /* Non-displayable characters. */
    if(c < ' ') return '.';
    if(c > 127) return '?';

    return c;
}

unsigned long _spi_dbg_transfer_count = 0;
unsigned long _spi_crc_fail_count     = 0;

/* Debug entry point, called when accessing WL_RDWR_DBG_SPACE. */
void spi_debug_ep(unsigned char* params)
{
#if SPI_ENABLE == 1
#if SPI_DEBUG_LOGS == 1
    int log_output = (params[0] != 0 ? 1 : 0);
#endif // SPI_DEBUG_LOGS == 1

    /* First, send transmission header. */
    wl_spi_pkt_t pkt_dat;
    wl_spi_pkt_t* pkt = &pkt_dat;
    unsigned short len = sizeof(wl_spi_pkt_t) / sizeof(unsigned short);

    spi_init_header(&pkt->cmn);
    pkt->cmn.command  = WL_SPICMD_DUMMY;
    pkt->cmn.pkt_len  = sizeof(wl_spi_pkt_t);//params[0];
    pkt->cmn.rsp_len  = sizeof(wl_spi_pkt_t);//params[3];
    memset(pkt->params, '\0', WL_SPI_PARAMS_LEN);
    //memset(pkt->data  , '\0', WL_SPI_DATA_LEN  );

#define SPI_TEST_WITH_CRC32 1

    int use_crc = 0;
#if SPI_TEST_WITH_CRC32 == 1
    use_crc = IORD(SWITCHES_BASE, 0) & 0x08;

    if(use_crc)
    {
        /* Fill data region of the packet with pseudo-random values.
         * Note : this slow downs the SPI transfer measurement.
         */
        int i;
        for(i=0; i<WL_SPI_DATA_LEN; i++)
        {
            pkt->data[i] = (((_spi_dbg_transfer_count + 17) * (i + 53)) >> 12) & 0xFF;
        }
    }
#endif // SPI_TEST_WITH_CRC32 == 1

    strcpy((char*)pkt->data, "Hi, this is a test from MAX10.");
    //pkt->data[WL_SPI_DATA_LEN-1] = '\0';

#if SPI_TEST_WITH_CRC32 == 1
    unsigned long* ptr = (unsigned long*)pkt->params;

    if(use_crc)
    {
        /* Compute CRC of data region of the packet, and put it
         * into start of parameters region.
         *
         * Note about parameters :
         *  - In the first 4 bytes, set 1 to indicate STM32 to use CRC.
         *  - In the next 4 bytes, set CRC32 of data region.
         */
        ptr[0] = 1;
        ptr[1] = crc32_calc(pkt->data, WL_SPI_DATA_LEN);
    }
#endif // SPI_TEST_WITH_CRC32 == 1


    /* Wait for STM32 to be ready until being able to send packet. */
    while(IORD(SPI_SYNC_BASE, 0) == 1);

    spi_send((unsigned short*)pkt, len);

    /* Wait for STM32 to be ready until receiving response. */
    while(IORD(SPI_SYNC_BASE, 0) == 0);

    spi_receive((unsigned short*)pkt, pkt->cmn.rsp_len / sizeof(unsigned short));


#if SPI_TEST_WITH_CRC32 == 1
    if(use_crc)
    {
        /* In parameters region of the packet, STM32 returns :
         *  - In the first 4 bytes, "2" to indicate that CRC check and computation was done on STM32 side.
         *  - CRC32 check result of packet received from MAX 10 (0: mismatch, else: OK) in the next 4 bytes.
         *  - CRC32 of data region of the packet in the next 4 bytes.
         */
        if(ptr[0] == 2)
        {
            if(ptr[1] == 0)
            {
                /* CRC mismatch in data from MAX 10. */
                _spi_crc_fail_count++;
            }

            if(ptr[2] != crc32_calc(pkt->data, WL_SPI_DATA_LEN))
            {
                /* CRC mismatch in data from STM32. */
                _spi_crc_fail_count++;
            }
        }
        else
        {
            _spi_crc_fail_count = 98;
        }
    }
#endif // SPI_TEST_WITH_CRC32 == 1


    /* Show transfer speed on 7 segs. */
    if((_spi_dbg_transfer_count % 77) == 0)
    {
        /* Enable access to 7-segments display only here
         * so that it is not overwritten by log information from other modules.
         */
        h7seg_enable();

        unsigned long disp_val = _spi_dbg_transfer_count;

        if(use_crc)
        {
            /* First two digits : CRC errors count. */
            h7seg_xdec_out(0/*stt_id*/, 2/*digits_count*/, 0/*dec_pos*/, _spi_crc_fail_count);

            /* Last four digits : packet exchange count. */
            h7seg_xdec_out(2/*stt_id*/, 4/*digits_count*/, 0/*dec_pos*/, disp_val % 10000);
        }
        else
        {
            /* All 6 digits : packet exchange count. */
            h7seg_xdec_out(0/*stt_id*/, 6/*digits_count*/, 0/*dec_pos*/, disp_val % 1000000);
        }

        h7seg_disable();
    }


#if SPI_DEBUG_LOGS == 1
    //if(log_output)
    {
        /* Output received data to log. */
        unsigned long* ptr = (unsigned long*)pkt->params;
        pkt->params[WL_SPI_PARAMS_LEN-1] = '\0';

        logout(WL_LOG_DEBUGEASY, "[SPI] HDR[%02X %02X %4u] P[%08X %08X %08X] D[%c%c%c%c]"
            , pkt->cmn.magic_ca
            , pkt->cmn.magic_fe
            //, pkt->cmn.pkt_len
            , pkt->cmn.rsp_len
            , ptr[0], ptr[1], ptr[2]
            , char2pchar(pkt->data[0]), char2pchar(pkt->data[1]), char2pchar(pkt->data[2]), char2pchar(pkt->data[3])
            , char2pchar(pkt->data[4]), char2pchar(pkt->data[5]), char2pchar(pkt->data[6]), char2pchar(pkt->data[7]));
        // int i;
        // for(i=0; i<8; i++)
        // {
        //     unsigned char c = ((unsigned char*)pkt)[i];
        //     logout(WL_LOG_DEBUGNORMAL, "[SPI]| Rcv[%3d]:0x%02X (%c)", i, c, c);
        // }
    }
#endif // SPI_DEBUG_LOGS == 1

    _spi_dbg_transfer_count++;

#endif // SPI_ENABLE == 1
}


/* Standalone test, called when a switch on the board is closed. */
void spi_rxtx_test(void)
{
#if SPI_ENABLE == 1
    unsigned char params[4];
    params[0] = 0x00; /* Log output flag. */
    params[1] = 0x10;
    params[2] = 0x00;
    params[3] = 0x40;

    spi_debug_ep(params);
#endif // SPI_ENABLE == 1
}


