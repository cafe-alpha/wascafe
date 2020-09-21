#include <string.h>

#include "spi_stm32.h"

#include "de10_7seg.h"
#include "log.h"
#include "perf_cntr.h"


/* Uncomment the line below to enable SPI related log output */
//#define SPI_DEBUG_LOGS


#if SPI_ENABLE == 1

/* SPI status and control bits */
#define STAT_TMT_MSK  (0x20)
#define STAT_TRDY_MSK (0x40)
#define STAT_RRDY_MSK (0x80)
#define CTRL_SSO_MSK  (0x400)


/* Send specified data over SPI to STM32. */
void spi_send(unsigned short* ptr, unsigned long count)
{
    int i;
    for(i=count; i; i--)
    {
        /* Transmit word. */
        while(!(IORD(SPI_STM32_BASE, 2) & STAT_TRDY_MSK));
        IOWR(SPI_STM32_BASE, 1, *ptr++);

        /* Wait until the last word is transmitted. */
        while(!(IORD(SPI_STM32_BASE, 2) & STAT_TMT_MSK)){}

        /* Ignore the received data. */
        //while(!(IORD(SPI_STM32_BASE, 2) & STAT_RRDY_MSK));
        ////rx_data = IORD(SPI_STM32_BASE, 0);
    }
}

/* Receive specified length of data over SPI from STM32. */
void spi_receive(unsigned short* ptr, unsigned long count)
{
    /* Receive back the data size specified in request header.
     * -> Allows long request from MAX10, with short ACK from STM32.
     *
     * When size is not specified and/or set to zero, full packet
     * is expected to be received from STM32.
     */
    if(count == 0)
    {
        count = sizeof(wl_spi_pkt_t) / sizeof(unsigned short);
    }

    int i;
    for(i=count; i; i--)
    {
        /* Transmit dummy word. */
        //while(!(IORD(SPI_STM32_BASE, 2) & STAT_TRDY_MSK));
        IOWR(SPI_STM32_BASE, 1, 0x0000);

        /* Wait until the last word is transmitted. */
        while(!(IORD(SPI_STM32_BASE, 2) & STAT_TMT_MSK)){}

        /* Read the received data. */
        while(!(IORD(SPI_STM32_BASE, 2) & STAT_RRDY_MSK));
        *ptr++ = IORD(SPI_STM32_BASE, 0);
    }
}


#endif // SPI_ENABLE == 1




void spi_get_version(wl_spicomm_version_t* ver)
{
#if SPI_ENABLE == 1

    /* First, send transmission header. */
    wl_spi_pkt_t pkt_dat;
    wl_spi_pkt_t* pkt = &pkt_dat;

    /* Setup request parameters. */
    memset(pkt, 0, sizeof(wl_spi_pkt_t));
    pkt->cmn.magic_ca = 0xCA;
    pkt->cmn.magic_fe = 0xFE;
    pkt->cmn.command  = WL_SPICMD_VERSION;
    pkt->cmn.pkt_len  = 0; // TODO
    pkt->cmn.rsp_len  = sizeof(wl_spi_pkt_t); // TODO


    /* Wait for STM32 to be ready until sending transmission header. */
    while(IORD(SPI_SYNC_BASE, 0) == 1);

    spi_send((unsigned short*)pkt, sizeof(wl_spi_pkt_t) / sizeof(unsigned short));

    /* Wait for STM32 to be ready until receiving response. */
    while(IORD(SPI_SYNC_BASE, 0) == 0);

    spi_receive((unsigned short*)pkt, pkt->cmn.rsp_len / sizeof(unsigned short));

    /* Don't forget to copy version
     * informations to output buffer.
     */
    memcpy(ver, pkt->data, sizeof(wl_spicomm_version_t));

#ifdef SPI_DEBUG_LOGS
    if(log_output)
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
#endif //SPI_DEBUG_LOGS
#endif // SPI_ENABLE == 1
}



/* Generic STM32-via memory access function.
 *
 * About each parameters :
 *  - pkt      : pointer to buffer for SPI packet
 *  - command  : which command to use. Example memory read, memory write etc
 *  - addr     : start address
 *  - len      : data length
 *  - flag     : extra flag
 *  - rsp_len  : response (answer) length, including start header
 *  - snd_data : send data buffer
 *  - rcv_data : receive data buffer
 * 
 * Returns 1 when no error happened, zero else.
 */
int spi_generic_access
(
    unsigned char command, 
    unsigned long addr, unsigned long len, 
    unsigned char flag, 
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

    /* First, send transmission header. */
    wl_spi_pkt_t pkt_dat;
    wl_spi_pkt_t* pkt = &pkt_dat;

    memset(pkt, 0, sizeof(wl_spi_pkt_t));
    pkt->cmn.magic_ca = 0xCA;
    pkt->cmn.magic_fe = 0xFE;
    pkt->cmn.command  = command;
    pkt->cmn.pkt_len  = 0; // TODO
    pkt->cmn.rsp_len  = rsp_len;

    /* Setup memory read parameters. */
    wl_spicomm_memacc_t* params = (wl_spicomm_memacc_t*)pkt->params;
    params->addr = addr;
    params->len  = len;
    params->flag = flag;

    if(snd_data)
    {
        memcpy(pkt->data, snd_data, len);
    }

    /* Wait for STM32 to be ready until sending transmission header. */
    //while(IORD(SWITCHES_BASE, 0) >= (1<<2));
    while(IORD(SPI_SYNC_BASE, 0) == 1);

    spi_send((unsigned short*)pkt, sizeof(wl_spi_pkt_t) / sizeof(unsigned short));

    /* Wait for STM32 to be ready until receiving response. */
    //while(IORD(SWITCHES_BASE, 0) < (1<<2));
    while(IORD(SPI_SYNC_BASE, 0) == 0);

    spi_receive((unsigned short*)pkt, pkt->cmn.rsp_len / sizeof(unsigned short));

    /* Don't forget to copy read data to output buffer. */
    if(rcv_data)
    {
        memcpy(rcv_data, pkt->data, len);
    }

#ifdef SPI_DEBUG_LOGS
    if(log_output)
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
#endif //SPI_DEBUG_LOGS
#endif // SPI_ENABLE == 1

    /* On STM32 side, error is always specified by zero-ing the
     * data length parameter, and vice-versa
     * So simply check this parameter to know if an error happened or not.
     */
    return (params->len == 0 ? 0 : 1);
}



/* The functions to do raw memory read/write. */
int spi_mem_read(unsigned long addr, unsigned long len, unsigned char* data)
{
    spi_generic_access(WL_SPICMD_MEMREAD, addr, len, 0/*flag*/, WL_SPI_CUSTOM_RSPLEN(WL_SPI_DATA_LEN)/*rsp_len*/, NULL/*snd_data*/, data/*rcv_data*/);

    return 1;
}
int spi_mem_write(unsigned long addr, unsigned long len, unsigned char* data)
{
    spi_generic_access(WL_SPICMD_MEMWRITE, addr, len, 0/*flag*/, WL_SPI_CUSTOM_RSPLEN(0)/*rsp_len*/, data/*snd_data*/, NULL/*rcv_data*/);

    return 1;
}

/* The functions to do backup memory read/write/etc. */
int spi_bup_open(unsigned long len_kb)
{
    return spi_generic_access(WL_SPICMD_BUPOPEN, 0/*block*/, len_kb, 0/*flag*/, WL_SPI_PARAMS_RSPLEN/*rsp_len*/, NULL, NULL);
}
int spi_bup_read(unsigned long block, unsigned long len, unsigned char* data)
{
    return spi_generic_access(WL_SPICMD_BUPREAD, block, len, 0/*flag*/, WL_SPI_FULL_RSPLEN/*rsp_len*/, NULL/*snd_data*/, data/*rcv_data*/);
}
int spi_bup_write(unsigned long block, unsigned long len, unsigned char flush_flag, unsigned char* data)
{
    return spi_generic_access(WL_SPICMD_BUPWRITE, block, len, flush_flag, WL_SPI_PARAMS_RSPLEN/*rsp_len*/, data/*snd_data*/, NULL/*rcv_data*/);
}
int spi_bup_flush(void)
{
    return spi_generic_access(WL_SPICMD_BUPFLUSH, 0/*block*/, 0/*len*/, 0/*flag*/, WL_SPI_PARAMS_RSPLEN/*rsp_len*/, NULL, NULL);
}
int spi_bup_close(void)
{
    return spi_generic_access(WL_SPICMD_BUPCLOSE, 0/*block*/, 0/*len*/, 0/*flag*/, WL_SPI_PARAMS_RSPLEN/*rsp_len*/, NULL, NULL);
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
    unsigned char dummy_block[512];
    //unsigned long block_id = 0; // First block
    unsigned long block_id = (_bup_test_cnt * (_bup_test_cnt+1)) % 1000;
    for(i=0; i<sizeof(dummy_block); i++)
    {
        dummy_block[i] = (unsigned char)((i*i) ^ _bup_test_cnt);
    }
    //sprintf((char*)dummy_block, "Block #%u, write count: %u", (unsigned int)(block_id + 1), (unsigned int)_bup_test_cnt);

    WASCA_PERF_START_MEASURE();
    spi_bup_write(block_id, sizeof(dummy_block), 1/*flush_flag*/, dummy_block);
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
    /* Flush written data.
     * (Should be done when all the blocks of a save are written)
     */
    spi_bup_flush();

    //logout(WL_LOG_DEBUGNORMAL, "bup_tb::FLUSH END.", 0);


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




/* Debug entry point, called when accessing WL_RDWR_DBG_SPACE. */
#define SPI_DEBUG_LOGS
void spi_debug_ep(unsigned char* params)
{
#if SPI_ENABLE == 1
    int log_output = (params[0] != 0 ? 1 : 0);

    /* SPI start : red LED OFF, green LED OFF. */
    IOWR(LEDS_BASE, 0, 0x00);

    /* First, send transmission header. */
    wl_spi_pkt_t pkt_dat;
    wl_spi_pkt_t* pkt = &pkt_dat;

    memset(pkt, 0, sizeof(wl_spi_pkt_t));
    pkt->cmn.magic_ca = 0xCA;
    pkt->cmn.magic_fe = 0xFE;
    pkt->cmn.pkt_len = params[0];
    pkt->cmn.rsp_len = sizeof(wl_spi_pkt_t);//params[3];
    memset(pkt->params, '\0', WL_SPI_PARAMS_LEN);
    memset(pkt->data  , '\0', WL_SPI_DATA_LEN  );
    strcpy((char*)pkt->data, "Hi, this is a test from MAX10.");
    pkt->data[WL_SPI_DATA_LEN-1] = '\0';

    /* internals ready : red LED ON, green LED OFF. */
    IOWR(LEDS_BASE, 0, 0x01);
    //IOWR(EXTRA_LEDS_BASE, 0, 0x01); // EXTRA DEBUG

    /* Wait for STM32 to be ready until sending transmission header. */
    while(IORD(SPI_SYNC_BASE, 0) == 1);

    /* SYNC OK : red LED ON, green LED ON. */
    IOWR(LEDS_BASE, 0, 0x03);
    //IOWR(EXTRA_LEDS_BASE, 0, 0x03); // EXTRA DEBUG

    /* Small wait : red LED OFF, green LED OFF. */
    IOWR(LEDS_BASE, 0, 0x00);

    spi_send((unsigned short*)pkt, sizeof(wl_spi_pkt_t) / sizeof(unsigned short));

    /* Transmit end : red LED ON, green LED OFF. */
    IOWR(LEDS_BASE, 0, 0x01);
    //IOWR(EXTRA_LEDS_BASE, 0, 0x07); // EXTRA DEBUG


    memset(pkt, 0xAA, sizeof(wl_spi_pkt_t)); // TMP

    /* Wait for STM32 to be ready until receiving response. */
    while(IORD(SPI_SYNC_BASE, 0) == 0);
    IOWR(LEDS_BASE, 0, 0x03);
    //IOWR(EXTRA_LEDS_BASE, 0, 0x0F); // EXTRA DEBUG

    IOWR(LEDS_BASE, 0, 0x00);
    spi_receive((unsigned short*)pkt, sizeof(wl_spi_pkt_t) / sizeof(unsigned short));
    IOWR(LEDS_BASE, 0, 0x01);
    pkt->params[WL_SPI_PARAMS_LEN-1] = '\0';


#ifdef SPI_DEBUG_LOGS
    if(log_output)
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
#endif //SPI_DEBUG_LOGS
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


