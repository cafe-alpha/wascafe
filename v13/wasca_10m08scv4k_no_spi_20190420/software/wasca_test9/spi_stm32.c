#include "spi_stm32.h"

#include "log.h"
#include "perf_cntr.h"


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


void spi_small_wait(void)
{return;
int count = 5;
    /* Wait a little bit, just in case of ... */
    int dummy = 0;
    for(dummy = 0; dummy < count; dummy++)
    {
        /* Make orange LED half bright during that time. */
        if(dummy % 2)
        {
            IOWR(LEDS_BASE, 0, (1<<2) | (1<<1) | (0<<0));
        }
        else
        {
            IOWR(LEDS_BASE, 0, (0<<2) | (0<<1) | (1<<0));
        }
    }
}

#endif // SPI_ENABLE == 1



#if SPI_ENABLE == 1

#define SPI_RXTX_LEN 256
unsigned short _spi_rx_tx_buff[SPI_RXTX_LEN / sizeof(unsigned short)];
unsigned char _stm_answer[12];
unsigned long _spi_exch_cnt = 0;
unsigned long _spi_err_cnt = 0;

int _spi_test_ctrl = 0xFF;
int _spi_test_send_data = 0xFF;
#endif // SPI_ENABLE == 1



//    if(space == WL_RDWR_DBG_SPACE)
//    { /* Debug space : do debug stuff. */
//        logout(WL_LOG_DEBUGEASY, "DBG addr:0x%08X len:%u val:0x%02X 0x%02X 0x%02X 0x%02X", out_ptr, data_len, in_ptr[0], in_ptr[1], in_ptr[2], in_ptr[3]);
//
//#if SPI_ENABLE == 1
//        _spi_test_ctrl      = in_ptr[0];
//        _spi_test_send_data = in_ptr[1];
//#endif // SPI_ENABLE == 1
//    }

/* Debug entry point, called when accessing WL_RDWR_DBG_SPACE. */
#define SPI_DEBUG_LOGS
void spi_debug_ep(unsigned char* params)
{
#if SPI_ENABLE == 1
    int log_output = (params[0] != 0 ? 1 : 0);
//if(params[1] == 1)
//{
//    /* Send header only. */
//    unsigned short* pkt = _spi_rx_tx_buff;
//    memset(pkt, 0, params[0]);
//    strcpy(pkt, "Hello, MAX10 speaking.");
//    spi_send(pkt, params[0]);
//
//    return;
//}
//if(params[1] == 2)
//{
//    /* Send specified length of dummy data. */
//    unsigned short* pkt = _spi_rx_tx_buff;
//    memset(pkt, 0, params[0]);
//    strcpy(pkt, "Hello, MAX10 speaking.");
//    spi_send(pkt, params[0]);
//
//    return;
//}

    /* SPI start : red LED OFF, green LED OFF. */
    IOWR(LEDS_BASE, 0, 0x00);

    /* First, send transmission header. */
    wl_spi_trans_hdr_t hdr_dat;
    wl_spi_trans_hdr_t* hdr = &hdr_dat;

    memset(hdr, 0, sizeof(wl_spi_trans_hdr_t));
    hdr->magic_ca = 0xCA;
    hdr->magic_fe = 0xFE;
    hdr->pkt_len = params[0];
    hdr->rsp_len = params[3];
    memset(hdr->params, '\0', WL_SPI_PARAMS_LEN);
    strcpy((char*)hdr->params, "Pouet, this is a test from MAX10.");
    hdr->params[WL_SPI_PARAMS_LEN-1] = '\0';
    hdr->end_bytes[0] = 0x41;
    hdr->end_bytes[1] = 0x42;
    hdr->end_bytes[2] = 0x43;
    hdr->end_bytes[3] = 0x44;

    /* internals ready : red LED ON, green LED OFF. */
    IOWR(LEDS_BASE, 0, 0x01);

    /* Wait for STM32 to be ready until sending transmission header. */
    while(IORD(SWITCHES_BASE, 0) >= (1<<2));

    /* SYNC OK : red LED ON, green LED ON. */
    IOWR(LEDS_BASE, 0, 0x03);

    //spi_small_wait(); // TMP

    /* Small wait : red LED OFF, green LED OFF. */
    IOWR(LEDS_BASE, 0, 0x00);

    spi_send((unsigned short*)hdr, sizeof(wl_spi_trans_hdr_t) / sizeof(unsigned short));

    /* Transmit end : red LED ON, green LED OFF. */
    IOWR(LEDS_BASE, 0, 0x01);


    memset(hdr, 0xAA, sizeof(wl_spi_trans_hdr_t)); // TMP

    /* Wait for STM32 to be ready until receiving response. */
    while(IORD(SWITCHES_BASE, 0) < (1<<2));
    IOWR(LEDS_BASE, 0, 0x03);
    //spi_small_wait(); // TMP
    IOWR(LEDS_BASE, 0, 0x00);
    spi_receive((unsigned short*)hdr, sizeof(wl_spi_trans_hdr_t) / sizeof(unsigned short));
    IOWR(LEDS_BASE, 0, 0x01);
    hdr->params[WL_SPI_PARAMS_LEN-1] = '\0';


#ifdef SPI_DEBUG_LOGS
    if(log_output)
    {
        /* Output received data to log. */
        logout(WL_LOG_DEBUGEASY, "[SPI] HDR[%02X %02X] END[%02x%02X%02X%02X] PARAMS[%s]"
            , hdr->magic_ca
            , hdr->magic_fe
            //, hdr->pkt_len
            //, hdr->rsp_len
            , hdr->end_bytes[0]
            , hdr->end_bytes[1]
            , hdr->end_bytes[2]
            , hdr->end_bytes[3]
            , hdr->params);
        int i;
        for(i=0; i<8; i++)
        {
            unsigned char c = ((unsigned char*)hdr)[i];
            logout(WL_LOG_DEBUGNORMAL, "[SPI]| Rcv[%3d]:0x%02X (%c)", i, c, c);
        }
    }
#endif //SPI_DEBUG_LOGS
#endif // SPI_ENABLE == 1
}




void spi_rxtx_test(void)
{
#if SPI_ENABLE == 1
    unsigned char params[4];
    params[0] = 0x00; /* Log output flag. */
    params[1] = 0x10;
    params[2] = 0x00;
    params[3] = 0x40;
    spi_debug_ep(params);
return;


//     /* Allow to enable/disable SPI from SerialTerm. */
//     if(_spi_test_ctrl == 0xFF)
//     {
//         unsigned char tx_data, rx_data;
// 
//         unsigned char snd_buff[8];
//         int i;
//         for(i=0; i<sizeof(snd_buff); i++)
//         {
//             snd_buff[i] = 'A' + i;
//         }
// 
// 
//         /* Synchronize with STM32. */
//         rx_data = 0x00;
//         do
//         {
//             /* Transmit byte. */
//             tx_data = 0x01;//_spi_test_send_data;
// //tx_data = 0x31;
// //while(1)
// {
//             while(!(IORD(SPI_STM32_BASE, 2) & STAT_TRDY_MSK));
//             IOWR(SPI_STM32_BASE, 1, tx_data);
// }
// 
//             /* Read the received data. */
//             while(!(IORD(SPI_STM32_BASE, 2) & STAT_RRDY_MSK));
//             rx_data = IORD(SPI_STM32_BASE, 0);
// 
//             /* Wait until the last byte is transmitted: */
//             while(!(IORD(SPI_STM32_BASE, 2) & STAT_TMT_MSK))
//             {
// #ifdef SPI_USE_WAIT_CNTR
//                 wait_cntrs[cntr_id]++;
// #endif // SPI_USE_WAIT_CNTR
//             }
// 
//             logout(WL_LOG_DEBUGEASY, "TAYST Tx:%02X Rx:%02X", tx_data, rx_data);
//             logflush();
//         } while(rx_data != 0x02);
// 
// 
//         /* Send packet to STM32. */
//         for(i=0; i<sizeof(snd_buff); i++)
//         {
//             /* Transmit byte. */
//             tx_data = snd_buff[i];
//             while(!(IORD(SPI_STM32_BASE, 2) & STAT_TRDY_MSK));
//             IOWR(SPI_STM32_BASE, 1, tx_data);
// 
//             /* Read the received data. */
//             while(!(IORD(SPI_STM32_BASE, 2) & STAT_RRDY_MSK));
//             rx_data = IORD(SPI_STM32_BASE, 0);
// 
//             /* Wait until the last byte is transmitted: */
//             while(!(IORD(SPI_STM32_BASE, 2) & STAT_TMT_MSK))
//             {
// #ifdef SPI_USE_WAIT_CNTR
//                 wait_cntrs[cntr_id]++;
// #endif // SPI_USE_WAIT_CNTR
//             }
//         }
// 
//         logout(WL_LOG_DEBUGEASY, "Data send. Size:%u bytes", sizeof(snd_buff));
//         logflush();
// 
// 
// 
//         _spi_test_ctrl = 0;
//     }
//     if(_spi_test_ctrl < 100)
//     {
//         if(_spi_test_ctrl <= 0)
//         {
//             return;
//         }
//         _spi_test_ctrl--;
//     }
// 
// 
// 
// //#define SPI_USE_WAIT_CNTR
//     alt_u64 perf_cntrs[10];
//     memset(perf_cntrs, 0, sizeof(perf_cntrs));
// 
// #ifdef SPI_USE_WAIT_CNTR
//     int wait_cntrs[sizeof(perf_cntrs) / sizeof(alt_u64)];
//     memset(wait_cntrs, 0, sizeof(wait_cntrs));
// #endif // SPI_USE_WAIT_CNTR
// 
//     int cntr_id = 0;
// 
//     int count;
// #define COUNT_OVER (10*1000)
// 
//     int i;
//     /* Barebones SPI tx routine.
//      *
//      * Strategy:
//      * Write the slave select bit to pick an SS_n.
//      * * * * *(Writing all 3 as a unit, will change later.)
//      * Force SS_n
//      * Wait until ready then tx
//      * Wait until ready then rx and ignore data
//      * Wait until last bit is transferred
//      * Deassert SS_n
//      * Deselect slave
//      *
//      * Set your spi device for uC mode, not DSP since SSn drops before SCLK starts.
//      * 
//      */
//     int ctrlbyte;
//     unsigned char* spi_buff = _spi_rx_tx_buff;
// 
// WASCA_PERF_START_MEASURE();
// 
//     memset(spi_buff, 0, SPI_RXTX_LEN*sizeof(unsigned char));
//     strcpy((char*)spi_buff, "Hello, MAX10 speaking, [");
//     _stm_answer[sizeof(_stm_answer)-1] = '\0';
//     _stm_answer[sizeof(_stm_answer)-2] = '\0';
//     strcat((char*)spi_buff, (char*)_stm_answer);
//     strcat((char*)spi_buff, "] blah bla bla bla");
// 
// WASCA_PERF_STOP_MEASURE();
// perf_cntrs[cntr_id++] = WASCA_PERF_GET_TIME();
// 
// 
// 
// WASCA_PERF_START_MEASURE();
// 
//     /* select all 3 DACs */
//     IOWR(SPI_STM32_BASE, 5, 0x7); 
// 
// 
//     /* Assert SS_n */
//     ctrlbyte = IORD(SPI_STM32_BASE, 3);
//     IOWR(SPI_STM32_BASE, 3, (ctrlbyte | CTRL_SSO_MSK));
// 
//     /* Synchronization with STM32. */
//     count = 0;
//     while(1)
//     {
//         /* Transmit "0x01" byte. */
//         unsigned char tx_data = 0x01;
//         while(!(IORD(SPI_STM32_BASE, 2) & STAT_TRDY_MSK));
//         IOWR(SPI_STM32_BASE, 1, tx_data);
// 
//         /* Read the received data. */
//         unsigned char rx_data;
//         while(!(IORD(SPI_STM32_BASE, 2) & STAT_RRDY_MSK));
//         rx_data = IORD(SPI_STM32_BASE, 0);
// 
//         /* Wait until the last byte is transmitted: */
//         while(!(IORD(SPI_STM32_BASE, 2) & STAT_TMT_MSK))
//         {
// #ifdef SPI_USE_WAIT_CNTR
//             wait_cntrs[cntr_id]++;
// #endif // SPI_USE_WAIT_CNTR
//         }
// 
//         //logout(WL_LOG_DEBUGEASY, "Stage#1 Tx:%02X Rx:%02X", tx_data, rx_data);
//         //logflush();
// 
//         /* If received data is 0x01, then STM32 is ready for answering. */
//         if(rx_data == 0x01)
//         {
//             break;
//         }
// 
//         if(count++ > COUNT_OVER)
//         {
//             logout(WL_LOG_DEBUGEASY, "Stage#1 Tx:%02X Rx:%02X. Count over !", tx_data, rx_data);
//             logflush();
//             _spi_err_cnt++;
//             return;
//         }
//     }
// 
// WASCA_PERF_STOP_MEASURE();
// perf_cntrs[cntr_id++] = WASCA_PERF_GET_TIME();
// 
// WASCA_PERF_START_MEASURE();
// 
//     /* Send message from MAX10 to STM32. */
//     spi_buff = _spi_rx_tx_buff;
//     for(i=(SPI_RXTX_LEN-1); i; i--)
//     {
//         /* Transmit a byte: */
//         unsigned char tx_data = *spi_buff;
//         while(!(IORD(SPI_STM32_BASE, 2) & STAT_TRDY_MSK));
//         IOWR(SPI_STM32_BASE, 1, tx_data);
// 
//         /* Read the received data: */
//         unsigned char rx_data;
//         while(!(IORD(SPI_STM32_BASE, 2) & STAT_RRDY_MSK));
//         rx_data = IORD(SPI_STM32_BASE, 0);
//         *spi_buff++ = rx_data;
// 
//         /* Wait until the last byte is transmitted: */
//         while(!(IORD(SPI_STM32_BASE, 2) & STAT_TMT_MSK))
//         {
// #ifdef SPI_USE_WAIT_CNTR
//             wait_cntrs[cntr_id]++;
// #endif // SPI_USE_WAIT_CNTR
//         }
//     }
// 
//     /* Release SS_n: */
//     IOWR(SPI_STM32_BASE, 3, IORD(SPI_STM32_BASE, 3) & ~CTRL_SSO_MSK);
// 
// WASCA_PERF_STOP_MEASURE();
// perf_cntrs[cntr_id++] = WASCA_PERF_GET_TIME();
// 
// WASCA_PERF_START_MEASURE();
// 
//     /* Synchronization with STM32 answer. */
//     count = 0;
//     while(1)
//     {
//         /* Assert SS_n */
//         ctrlbyte = IORD(SPI_STM32_BASE, 3);
//         IOWR(SPI_STM32_BASE, 3, (ctrlbyte | CTRL_SSO_MSK));
// 
//         /* Transmit "0x11" byte. */
//         unsigned char tx_data = 0x11;
//         while(!(IORD(SPI_STM32_BASE, 2) & STAT_TRDY_MSK));
//         IOWR(SPI_STM32_BASE, 1, tx_data);
// 
//         /* Read the received data. */
//         unsigned char rx_data;
//         while(!(IORD(SPI_STM32_BASE, 2) & STAT_RRDY_MSK));
//         rx_data = IORD(SPI_STM32_BASE, 0);
// 
//         /* Wait until the last byte is transmitted: */
//         while(!(IORD(SPI_STM32_BASE, 2) & STAT_TMT_MSK))
//         {
// #ifdef SPI_USE_WAIT_CNTR
//             wait_cntrs[cntr_id]++;
// #endif // SPI_USE_WAIT_CNTR
//         }
// 
//         //logout(WL_LOG_DEBUGEASY, "Stage#2 Tx:%02X Rx:%02X", tx_data, rx_data);
//         //logflush();
// 
//         /* If received data is 0x11, then STM32 is ready for answering. */
//         if(rx_data == 0x11)
//         {
//             break;
//         }
// 
//         if(count++ > COUNT_OVER)
//         {
//             logout(WL_LOG_DEBUGEASY, "Stage#2 Tx:%02X Rx:%02X. Count over !", tx_data, rx_data);
//             logflush();
//             _spi_err_cnt++;
//             return;
//         }
// 
//         /* Release SS_n: */
//         IOWR(SPI_STM32_BASE, 3, IORD(SPI_STM32_BASE, 3) & ~CTRL_SSO_MSK);
//     }
// 
// WASCA_PERF_STOP_MEASURE();
// perf_cntrs[cntr_id++] = WASCA_PERF_GET_TIME();
// 
// WASCA_PERF_START_MEASURE();
// 
//     /* Receive from STM32 to MAX10. */
//     spi_buff = _spi_rx_tx_buff;
//     int start_received = 0;
//     for(i=(SPI_RXTX_LEN-1); i; i--)
//     {
//         /* Transmit a dummy byte. */
//         unsigned char tx_data = 0x31;
//         while(!(IORD(SPI_STM32_BASE, 2) & STAT_TRDY_MSK));
//         IOWR(SPI_STM32_BASE, 1, tx_data);
// 
//         /* Read the received data, and store it. */
//         unsigned char rx_data;
//         while(!(IORD(SPI_STM32_BASE, 2) & STAT_RRDY_MSK));
//         rx_data = IORD(SPI_STM32_BASE, 0);
// 
//         /* Wait for end of synchronization bytes. */
//         if(rx_data != 0x11)
//         {
//             start_received = 1;
//         }
//         if(start_received)
//         {
//             *spi_buff++ = rx_data;
//         }
// 
//         /* Wait until the last byte is transmitted: */
//         while(!(IORD(SPI_STM32_BASE, 2) & STAT_TMT_MSK))
//         {
// #ifdef SPI_USE_WAIT_CNTR
//             wait_cntrs[cntr_id]++;
// #endif // SPI_USE_WAIT_CNTR
//         }
//     }
// 
//     /* Release SS_n: */
//     IOWR(SPI_STM32_BASE, 3, IORD(SPI_STM32_BASE, 3) & ~CTRL_SSO_MSK);
// 
//     /* Deselect all DACs: */
//     IOWR(SPI_STM32_BASE, 5, 0x0);
// 
// WASCA_PERF_STOP_MEASURE();
// perf_cntrs[cntr_id++] = WASCA_PERF_GET_TIME();
// 
//     if((_spi_exch_cnt % 100) == 0)
//     {
//         _spi_rx_tx_buff[40] = '\0';
//         logout(WL_LOG_DEBUGEASY, "[SPI]cnt:%7u, err:%3u, answer:\"%s\"", _spi_exch_cnt, _spi_err_cnt, _spi_rx_tx_buff); //logflush();
//         memcpy(_stm_answer, _spi_rx_tx_buff+7, sizeof(_stm_answer));
// 
//         for(i=0; i<cntr_id; i++)
//         {
//             wasca_perf_logout("|SPI", perf_cntrs[i]);
//         }
// 
// #ifdef SPI_USE_WAIT_CNTR
//         for(i=0; i<cntr_id; i++)
//         {
//             logout(WL_LOG_DEBUGEASY, "|wait[%u]:%4u", i, wait_cntrs[i]);
//         }
// #endif // SPI_USE_WAIT_CNTR
//     }
// 
//     _spi_exch_cnt++;
#endif // SPI_ENABLE == 1
}


