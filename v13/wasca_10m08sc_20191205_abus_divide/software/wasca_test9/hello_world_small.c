/* 
 * "Small Hello World" example. 
 * 
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example 
 * designs. It requires a STDOUT  device in your system's hardware. 
 *
 * The purpose of this example is to demonstrate the smallest possible Hello 
 * World application, using the Nios II HAL library.  The memory footprint
 * of this hosted application is ~332 bytes by default using the standard 
 * reference design.  For a more fully featured Hello World application
 * example, see the example titled "Hello World".
 *
 * The memory footprint of this example has been reduced by making the
 * following changes to the normal "Hello World" example.
 * Check in the Nios II Software Developers Manual for a more complete 
 * description.
 * 
 * In the SW Application project (small_hello_world):
 *
 *  - In the C/C++ Build page
 * 
 *    - Set the Optimization Level to -Os
 * 
 * In System Library project (small_hello_world_syslib):
 *  - In the C/C++ Build page
 * 
 *    - Set the Optimization Level to -Os
 * 
 *    - Define the preprocessor option ALT_NO_INSTRUCTION_EMULATION 
 *      This removes software exception handling, which means that you cannot 
 *      run code compiled for Nios II cpu with a hardware multiplier on a core 
 *      without a the multiply unit. Check the Nios II Software Developers 
 *      Manual for more details.
 *
 *  - In the System Library page:
 *    - Set Periodic system timer and Timestamp timer to none
 *      This prevents the automatic inclusion of the timer driver.
 *
 *    - Set Max file descriptors to 4
 *      This reduces the size of the file handle pool.
 *
 *    - Check Main function does not exit
 *    - Uncheck Clean exit (flush buffers)
 *      This removes the unneeded call to exit when main returns, since it
 *      won't.
 *
 *    - Check Don't use C++
 *      This builds without the C++ support code.
 *
 *    - Check Small C library
 *      This uses a reduced functionality C library, which lacks  
 *      support for buffering, file IO, floating point and getch(), etc. 
 *      Check the Nios II Software Developers Manual for a complete list.
 *
 *    - Check Reduced device drivers
 *      This uses reduced functionality drivers if they're available. For the
 *      standard design this means you get polled UART and JTAG UART drivers,
 *      no support for the LCD driver and you lose the ability to program 
 *      CFI compliant flash devices.
 *
 *    - Check Access device drivers directly
 *      This bypasses the device file system to access device drivers directly.
 *      This eliminates the space required for the device file system services.
 *      It also provides a HAL version of libc services that access the drivers
 *      directly, further reducing space. Only a limited number of libc
 *      functions are available in this configuration.
 *
 *    - Use ALT versions of stdio routines:
 *
 *           Function                  Description
 *        ===============  =====================================
 *        alt_printf       Only supports %s, %x, and %c ( < 1 Kbyte)
 *        alt_putstr       Smaller overhead than puts with direct drivers
 *                         Note this function doesn't add a newline.
 *        alt_putchar      Smaller overhead than putchar with direct drivers
 *        alt_getchar      Smaller overhead than getchar with direct drivers
 *
 */

#include <sys/alt_stdio.h>
#include <string.h>
#include <system.h>
#include <altera_avalon_pio_regs.h>


#include "wasca_defs.h"
#include "crc.h"
#include "link.h"
#include "log.h"
#include "m10_ver_infos.h"
#include "memtest.h"
#include "perf_cntr.h"
#include "spi_stm32.h"


/* Global pointer to waca internals
 * located in external SDRAM.
 *
 * It is used by several modules such as
 * log, UART, etc.
 */
wasca_internals_t _wasca_internals_buff;
wasca_internals_t* wasca_internals = NULL;





/*
 * ------------------------------------------
 * - UART: version request ------------------
 * ------------------------------------------
 */
void pkt_do_version(wl_sndrcv_hdr_t* hdr, unsigned char* pkt_data)
{
    memset(pkt_data, 0, sizeof(wl_verinfo_t));

    if(hdr->type == WL_PKT_VERSION_EXT)
    {
        char* dev_type = MAX10_DEV_TYPE;  // Example : "10M16SAE144C8G"
        char* prj_name = MAX10_PROJ_NAME; // Example : "brd_10m16sa"
        wl_verinfo_ext_t* ver = (wl_verinfo_ext_t*)pkt_data;

        strncpy(ver->dev_type, dev_type, sizeof(ver->dev_type) - 1);
        strncpy(ver->prj_name, prj_name, sizeof(ver->prj_name) - 1);
        ver->pl_date = MAX10_FWDT_VAL;         // Example : 20190828 (decimal, YYYYMMDD format)
        ver->pl_time = MAX10_FWTM_VAL - 10000; // Example : 11944 (decimal, 1HHMM format)
    }
    else
    {
        char* build_date = __DATE__;
        char* build_time = __TIME__;
        wl_verinfo_t* ver = (wl_verinfo_t*)pkt_data;

        /* Indicate type of board used : defined in wasca_defs.h . */
        ver->board_type = WASCA_BRD_TYPE;

        /* Write Nios build date and time. */
        strcpy(ver->build_date, build_date);
        strcpy(ver->build_time, build_time);

#if SPI_ENABLE == 1
        /* Write SPI frequency. */
        ver->stm32_spi_khz = SPI_STM32_TARGETCLOCK / 1000;

        /* If requested, retrieve STM32 firmware version. */
        if(hdr->type == WL_PKT_VERSION_ARM)
        {
            spi_get_version(&(ver->arm));
        }
#endif // SPI_ENABLE == 1

        /* Write OCRAM size. */
        ver->ocram_size = ONCHIP_MEMORY2_0_SPAN;
    }

    /* Send version informations UART.
     *
     * Note : this handles both cases of following structures :
     *  - wl_verinfo_t     : WL_PKT_VERSION, WL_PKT_VERSION_ARM
     *  - wl_verinfo_ext_t : WL_PKT_VERSION_EXT
     * And this doesn't causes problem because
     * both structures have the same size.
     */
    link_send(hdr, sizeof(wl_verinfo_t));
}

/*
 * ------------------------------------------
 * - UART: ping request ---------------------
 * ------------------------------------------
 */
void pkt_do_memtest(wl_sndrcv_hdr_t* hdr, unsigned char* pkt_data)
{
    unsigned short data_len = hdr->len - sizeof(wl_sndrcv_hdr_t) - sizeof(wl_crc_t);
    if(data_len == sizeof(wl_memtest_params_t))
    {
        wl_memtest_params_t* memt = (wl_memtest_params_t*)pkt_data;

        do_memory_test(memt);

        /* Send back parameters and test results to UART. */
        link_send(hdr, data_len);
    }
}

/*
 * ------------------------------------------
 * - UART: ping request ---------------------
 * ------------------------------------------
 */
void pkt_do_ping(wl_sndrcv_hdr_t* hdr, unsigned char* pkt_data)
{
    unsigned short data_len = hdr->len - sizeof(wl_sndrcv_hdr_t) - sizeof(wl_crc_t);
    int i;

    for(i=0; i<data_len; i++)
    {
        pkt_data[i] += 1;
    }

    /* Send back packet and data to UART. */
    link_send(hdr, data_len);
}

/*
 * ------------------------------------------
 * - UART: register read/write request ------
 * ------------------------------------------
 */
void pkt_do_reg_access(int read_access, wl_sndrcv_hdr_t* hdr, unsigned char* pkt_data)
{
    unsigned char* in_ptr;
    unsigned char* out_ptr;
    unsigned char  space;
    unsigned long  data_len;
    {
        /* Note : when reading data from PC, access parameters
         * in wl_reg_access_t structure are overwritten by read
         * data, so it is necessary to copy it here locally
         * before memcpy-ing data.
         */
        wl_reg_access_t* param = (wl_reg_access_t*)pkt_data;
        space = param->space;

        /* Translate Wasca internal space to MAX10 space. */
        if(space == WL_RDWR_INT_SPACE)
        {
            space = WL_RDWR_NIOS_SPACE;
            param->addr += (unsigned long)wasca_internals;
        }

        if(read_access)
        {
            in_ptr  = (unsigned char*)param->addr;
            out_ptr = (unsigned char*)param;
        }
        else
        {
            in_ptr   = ((unsigned char*)param) + sizeof(wl_reg_access_t);
            out_ptr  = (unsigned char*)param->addr;
        }

        data_len = param->len;
    }

    /* Verify access length validity. */
    if((sizeof(wl_sndrcv_hdr_t) + sizeof(wl_reg_access_t) + data_len) > WL_PKT_MAXLEN)
    {
        data_len = 0;
    }

    if(space == WL_RDWR_DBG_SPACE)
    { /* Debug space : do debug stuff. */
        logout(WL_LOG_DEBUGEASY, "DBG addr:0x%08X len:%u param:0x%02X 0x%02X 0x%02X 0x%02X"
            , out_ptr
            , data_len
            , in_ptr[0], in_ptr[1], in_ptr[2], in_ptr[3]);

        /* Send SPI debug packet(s). */
        spi_debug_ep(in_ptr);
    }
    else if(space == WL_RDWR_ARM_SPACE)
    { /* ARM space. */
        if(read_access)
        {
            spi_memread((unsigned long)in_ptr, data_len, out_ptr);
        }
        else
        {
            spi_memwrite((unsigned long)out_ptr, data_len, in_ptr);
        }
    }
    else if(space == WL_RDWR_NIOS_SPACE)
    { /* MAX10 space, including Wasca internals. */
        if(data_len == 2)
        {
            *((unsigned short*)out_ptr) = *((unsigned short*)in_ptr);
        }
        else if(data_len == 4)
        {
            *((unsigned long*)out_ptr) = *((unsigned long*)in_ptr);
        }
        else if(data_len > 0)
        {
            memcpy(out_ptr, in_ptr, data_len);
        }
    }
    else // if(space == WL_RDWR_ABUS_SPACE)
    { /* Saturn A-Bus space. */
        // TODO : use USB dev cart registers ?
    }

    /* Send back packet and data to UART. */
    unsigned long answer_len = (read_access ? data_len : 0);
    link_send(hdr, answer_len);
}



/*
 * ------------------------------------------
 * - UART: perform link I/O -----------------
 * ------------------------------------------
 */
void link_do_io(void)
{
    /* Initialize pointers to received data. */
    wl_sndrcv_hdr_t* pkt_hdr = (wl_sndrcv_hdr_t*)_link_rxtx_buffer;
    unsigned long pkt_rcv_len = 0;

    /* Verify if there's something received from UART. */
    pkt_rcv_len = link_rcv_cnt();
    if(pkt_rcv_len >= sizeof(wl_sndrcv_hdr_t))
    {
        /* Verify header validity :
         *  - Packets with incorrect magic byte are discarded
         *  - Packets larger than maximum size are discarded
         *  - Packets too small are discarded : must have at least header + CRC
         */
        int valid_header = 1;
        if((pkt_hdr->magic != WL_HDR_MAGIC_CA)
        || (pkt_hdr->len > WL_PKT_MAXLEN)
        || (pkt_hdr->len < (sizeof(wl_sndrcv_hdr_t)+sizeof(wl_crc_t))))
        {
            valid_header = 0;
        }

        /* When packet is completely received, process it. */
        if((valid_header)
        && (pkt_rcv_len >= pkt_hdr->len))
        {
            /* Ignore packets with incorrect CRC. */
            wl_crc_t crc8 = crc_init();
            crc8 = crc_update(crc8, _link_rxtx_buffer, pkt_hdr->len - 1);
            crc8 = crc_finalize(crc8);

            if(crc8 == _link_rxtx_buffer[pkt_hdr->len - 1])
            {
                unsigned char* pkt_data = _link_rxtx_buffer + sizeof(wl_sndrcv_hdr_t);

                //logout(WL_LOG_DEBUGEASY, "pkt received type:%x len:%x", pkt_hdr->type, pkt_hdr->len);

                if((pkt_hdr->type == WL_PKT_VERSION)
                || (pkt_hdr->type == WL_PKT_VERSION_ARM)
                || (pkt_hdr->type == WL_PKT_VERSION_EXT))
                {
                    /* Return version informations. */
                    pkt_do_version(pkt_hdr, pkt_data);
                }
                //else if(pkt_hdr->type == WL_PKT_LOGPOLL)
                //{
                //    /* Logs polling from PC. */
                //    pkt_do_logpoll(pkt_hdr, pkt_data);
                //}
                else if(pkt_hdr->type == WL_PKT_READ)
                {
                    /* Register/memory read. */
                    pkt_do_reg_access(1/*read_access*/, pkt_hdr, pkt_data);
                }
                else if(pkt_hdr->type == WL_PKT_WRITE)
                {
                    /* Register/memory write. */
                    pkt_do_reg_access(0/*read_access*/, pkt_hdr, pkt_data);
                }
                else if(pkt_hdr->type == WL_PKT_MEMTEST)
                {
                    /* Do memory test. */
                    pkt_do_memtest(pkt_hdr, pkt_data);
                }
                else if(pkt_hdr->type == WL_PKT_PING)
                {
                    /* Ping back received data. */
                    pkt_do_ping(pkt_hdr, pkt_data);
                }
            } /* CRC check. */

            /* Get ready to process next packet.
             * Note : it is probably needed to enable transfer after doing this ...
             */
            link_rcv_reset();

        } /* Packet received. */

        /* If header isn't correct, trash it. */
        if(!valid_header)
        {
            link_rcv_reset();
        }
    } /* Data received from UART. */
}






#define SOFTWARE_VERSION 0x0002

#define PCNTR_REG_OFFSET 0xF0
#define MODE_REG_OFFSET  0xF4
#define SWVER_REG_OFFSET 0xF8

//#define SH2FIRM_LEN 60416
#define SH2FIRM_LEN 60
extern const unsigned char rawData[SH2FIRM_LEN];

const char Power_Memory_Signature[16] = "BackUpRam Format";
const char Wasca_Sysarea_Signature[] =
{
    0x80, 0x00, 0x00, 0x00, 0x77, 0x61, 0x73, 0x63,
    0x61, 0x20, 0x73, 0x79, 0x73, 0x20, 0x20, 0x01,
    0xBE, 0xB6, 0xDE, 0xBB, 0xC0, 0xB0, 0xDD, 0x2C,
    0xBC, 0xDB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0E, 0x74, 0x7F, 0xFC, 0x7F, 0xFD, 0x7F, 0xFE,
    0x7F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};


const char hex_tbl[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

#if WASCA_BRD_TYPE == WL_DEVTYPE_WASCA
/* Dummy Saturn header, so that SH2 doesn't boots on cartridge. */
const char* _sega_rom_header = "DUMMYSATURN HDR (C)CAFE-ALPHA   PSKAI_F   V6.07420170423        JTUBKAEL        JW              PSEUDOSATURN_KAI(C) 2015-2016   By cafe-alpha   ppcenter.free.fr                Original Versionby CyberWarriorX                ";
#else
const char* _sega_rom_header = "SEGA SEGASATURN (C)CAFE-ALPHA   PSKAI_F   V6.07420170423        JTUBKAEL        JW              PSEUDOSATURN_KAI(C) 2015-2016   By cafe-alpha   ppcenter.free.fr                Original Versionby CyberWarriorX                ";
#endif

void initialize_ram(void)
{
    int i;
    char pattern_str[32];
    unsigned char* wr_ptr;
    unsigned long* wr4_ptr;

    /* Fill extra OCRAM. */
    strcpy(pattern_str, "Onchip RAM adr 0xXXXXXXXX --");
    wr_ptr = (unsigned char*)ONCHIP_MEMORY2_1_BASE;
    for(i=0; i<ONCHIP_MEMORY2_1_SPAN; i+=sizeof(pattern_str))
    {
IOWR(LEDS_BASE, 0, (1<<2) | (1<<1) | (0<<0));
        char* addr_fmt = pattern_str+17;
        *addr_fmt++ = hex_tbl[(i >> 28) & 0xF];
        *addr_fmt++ = hex_tbl[(i >> 24) & 0xF];
        *addr_fmt++ = hex_tbl[(i >> 20) & 0xF];
        *addr_fmt++ = hex_tbl[(i >> 16) & 0xF];
        *addr_fmt++ = hex_tbl[(i >> 12) & 0xF];
        *addr_fmt++ = hex_tbl[(i >>  8) & 0xF];
        *addr_fmt++ = hex_tbl[(i >>  4) & 0xF];
        *addr_fmt++ = hex_tbl[(i >>  0) & 0xF];
        memcpy(wr_ptr, pattern_str, sizeof(pattern_str));
        wr_ptr += sizeof(pattern_str);
IOWR(LEDS_BASE, 0, (1<<2) | (0<<1) | (0<<0));
    }


    /* Fill SDRAM.
     * Note : as filling whole SDRAM takes around two minutes to complete, 
     *        some parts are not filled.
     */
IOWR(LEDS_BASE, 0, (1<<2) | (1<<1) | (1<<0));
    strcpy(pattern_str, "External SDRAM adr 0xXXXXXXXX --");
    wr_ptr = (unsigned char*)EXTERNAL_SDRAM_CONTROLLER_BASE;
    wr4_ptr = (unsigned long*)wr_ptr;
#if 0
    memset(wr_ptr, 0xCA, EXTERNAL_SDRAM_CONTROLLER_SPAN);
#else
    /* Quick partial memset. */
    for(i=0; i<(EXTERNAL_SDRAM_CONTROLLER_SPAN / sizeof(unsigned long)); i+=16)
    {
        wr4_ptr[i] = 0x65666163;
    }
#endif
    for(i=0; i<(EXTERNAL_SDRAM_CONTROLLER_SPAN/*4*1024*/); /*nothing*/)
    {
IOWR(LEDS_BASE, 0, (1<<2) | (1<<1) | (1<<0));
        char* addr_fmt = pattern_str+21;
        *addr_fmt++ = hex_tbl[(i >> 28) & 0xF];
        *addr_fmt++ = hex_tbl[(i >> 24) & 0xF];
        *addr_fmt++ = hex_tbl[(i >> 20) & 0xF];
        *addr_fmt++ = hex_tbl[(i >> 16) & 0xF];
        *addr_fmt++ = hex_tbl[(i >> 12) & 0xF];
        *addr_fmt++ = hex_tbl[(i >>  8) & 0xF];
        *addr_fmt++ = hex_tbl[(i >>  4) & 0xF];
        *addr_fmt++ = hex_tbl[(i >>  0) & 0xF];
        memcpy(wr_ptr, pattern_str, sizeof(pattern_str));

        if(i < (128*1024))
        {
            i += sizeof(pattern_str);
            wr_ptr += sizeof(pattern_str);
        }
        else
        {
            i += 42*sizeof(pattern_str);
            wr_ptr += 42*sizeof(pattern_str);
        }
IOWR(LEDS_BASE, 0, (1<<2) | (0<<1) | (1<<0));
    }

    /* Add a dummy cartridge ROM header at the top of CS0, 
     * So that cartridge diagnostic feature is happy with it.
     * Note : do that on MAX 10 Board only as it would make
     *        a real SH2 trying to boot from cartridges and crash.
     */
    memcpy(wr4_ptr, _sega_rom_header, strlen(_sega_rom_header));
}


int main(void)
{
    int i,j;
    volatile int k,v;
    volatile unsigned char * p;
    volatile unsigned short * p16;
    unsigned short sMode;

    IOWR(LEDS_BASE, 0, (1<<2) | (1<<1) | (0<<0));

    // /* Wait around 0.5 seconds, just in case of ... */
    // int dummy = 0;
    // for(dummy = 0; dummy < 2000*1000; dummy++)
    // {
    //     /* Make orange LED half bright during that time. */
    //     if(dummy % 2)
    //     {
    //         IOWR(LEDS_BASE, 0, (1<<2) | (1<<1) | (0<<0));
    //     }
    //     else
    //     {
    //         IOWR(LEDS_BASE, 0, (0<<2) | (1<<1) | (0<<0));
    //     }
    // }

    /* Initialize location of Wasca internals.
     * (Not really needed since each modules
     * re-reset their internals just after)
     */
    wasca_internals = &_wasca_internals_buff;
    memset(wasca_internals, 0, sizeof(wasca_internals_t));

    /*------------------------------------------*/
    /* Initialize memory test module internals. */
    memory_test_init();

initialize_ram();
    /*--------------------------------------------*/
    /* Initialize log internals. */
    log_init();

    IOWR(LEDS_BASE, 0, (1<<2) | (0<<1) | (0<<0));

    /*--------------------------------------------*/
    /* Setup UART baudrate, and initialize other link-related internals. */
    link_init();

//#define UART_SEND_PERIOD 160000 // 1~2 messages per second
#define UART_SEND_PERIOD 16000 // around 10 messages per second
//#define UART_SEND_PERIOD 600 // super fast
    i = 0;
    j = 0;

    IOWR(LEDS_BASE, 0, (0<<2) | (0<<1) | (0<<0));

#if 0
    // Blink LEDs -->
    while(1)
    {
        unsigned char leds = IORD(SWITCHES_BASE, 0) & 0x01;

        if(i < ((8*UART_SEND_PERIOD) / 2))
        {
            leds |= 0x0E;
        }

        IOWR(LEDS_BASE, 0, leds);

        i++;
        if(i > (8*UART_SEND_PERIOD))
        {
            i = 0;
        }
    }
    // Blink LEDs <--
#endif


    while(1)
    {
        /* LED state :
         *  - LED #1 (TH)  : (if SPI not used, change according to SW1 state)
         *  - LED #2 (TH)  : (invert of LED #1)
         *  - LED #3 (SMD) : blink.
         */
        unsigned char sw_state = IORD(SWITCHES_BASE, 0) & 0x03;
#if SPI_ENABLE == 1
        unsigned char leds = sw_state;
#else // SPI_ENABLE != 1
        unsigned char leds = sw_state & 0x01;
        leds |= (1-leds) << 1;
#endif // SPI_ENABLE != 1

        if(i < (UART_SEND_PERIOD / 2))
        {
            leds |= 0x04;
        }

        IOWR(LEDS_BASE, 0, leds);


        if((i % 50) == 0)
        {
            /* Perform I/O check with UART. */
            link_do_io();
        }

#if SPI_ENABLE == 1
//unsigned char* spi_buff = _spi_rx_tx_buff;
//for(i=0; i<SPI_RXTX_LEN; i++)
//{
//    spi_buff[i] = (unsigned char)i;
//}

//WASCA_PERF_START_MEASURE();

if((sw_state & 0x01) == 0x00)
{
    /* Test SPI comunication when SW1 is closed.
     * Note : SPI should be disabled if STM32 is not connected.
     * Note : because SPI transfer takes long, link I/O check is not done
     *        as frenqently as usual, and log output below is done even less
     *        frequently too.
     */
    //while(1)
    {
        spi_rxtx_test();
    }
}
#endif // SPI_ENABLE == 1

#ifdef ENABLE_SPI
//WASCA_PERF_STOP_MEASURE();
//wasca_perf_logout("SPI", WASCA_PERF_GET_TIME());
#endif // ENABLE_SPI


        i++;
        if(i > UART_SEND_PERIOD)
        {
#if LOG_ENABLE == 1
            log_infos_t* logs = _log_inf_ptr;

            if((sw_state & 0x02) != 0x00)
            {
                /* Output something to UART only when SW2 is open. */
                logout(WL_LOG_DEBUGNORMAL, "tayst %x [sw:%02X wp:%x rp:%x bs:%x sa:%x ea:%x]", j, 
                    sw_state, 
                    logs->writeptr, logs->readptr, 
                    logs->buffer_size, logs->start_address, logs->end_address);
            }
#endif // LOG_ENABLE == 1

            i = 0;
            j++;
        }
    }


#if ABUS_SLAVE_ENABLE == 1
#if SDRAM_ENABLE == 1
    /* First things first - copy saturn bootcode into SDRAM. */
    p = (unsigned char *)EXTERNAL_SDRAM_CONTROLLER_BASE;
    for(i=0;i<SH2FIRM_LEN;i++)
    {
        //p[i] = rawData[i];
    }
    logout(WL_LOG_DEBUGNORMAL, "Bootloader copied! Dump :");
    for(i=0;i<256;i++)
    {
        logout(WL_LOG_DEBUGNORMAL, "%c ", p[i]);
        if(i%16 == 15)
        {
            logout(WL_LOG_DEBUGNORMAL, "");
        }
    }
    logout(WL_LOG_DEBUGNORMAL, "Dump done");
#endif // SDRAM_ENABLE == 1

    /* Write version. */
    p16 = (unsigned short *)ABUS_SLAVE_0_BASE;
    p16[SWVER_REG_OFFSET] = SOFTWARE_VERSION;

    /* Now wait until MODE register is non-zero, i.e. mode is set. */
    logout(WL_LOG_DEBUGNORMAL, "Waiting for MODE register to be set...");
    while(p16[MODE_REG_OFFSET] == 0)
    {
        for(k=0;k<1000000;k++);
        logout(WL_LOG_DEBUGNORMAL, ".");
    }
    sMode = p16[MODE_REG_OFFSET];
    logout(WL_LOG_DEBUGNORMAL, "Mode set to %x",sMode);

    /* Okay, now start a MODE-dependent preparation routine. */

    /* Lower octets have higher priority. */
    if((sMode & 0x000F) != 0)
    {
        logout(WL_LOG_DEBUGNORMAL, "Octet 0, value %x",(sMode & 0x000F));

        /* Lowest octet is active, it's a POWER MEMORY.
         * We should copy relevant image from SD card.
         * For now, we will only write header and emulate copying behavior.
         */
        p = (unsigned char *)EXTERNAL_SDRAM_CONTROLLER_BASE;

        switch (sMode & 0x000F )
        {
        case 0x1: /* 0.5 MB */
        case 0x2: /* 1 MB */
        case 0x3: /* 2 MB */
            logout(WL_LOG_DEBUGNORMAL, "Header 0-3 copy start");

            for(j=0;j<16;j++)
            {
                for(i=0;i<16;i++)
                {
                    p[j*16+i] = Power_Memory_Signature[i];
                    p[256+j*16+i] = 0;
                }
            }

            logout(WL_LOG_DEBUGNORMAL, "Header copied");
        break;

        case 0x04 : /* 4 MB */
            logout(WL_LOG_DEBUGNORMAL, "Header 4 copy start");
            for(j=0;j<32;j++)
            for(i=0;i<16;i++)
            {
                p[j*16+i] = Power_Memory_Signature[i];
                p[512+j*16+i] = 0;
            }

            logout(WL_LOG_DEBUGNORMAL, "Header copied");

            for(i=0;i<64;i++)
            {
                p[0x1FFF000 + i] = Wasca_Sysarea_Signature[i];
            }

            logout(WL_LOG_DEBUGNORMAL, "Sysarea stub copied");
        break;
        }

        /* Now emulate slow copy. */
        for(v=2;v<101;v++)
        {
            for(k=0;k<100000;k++) ;
            //logout(WL_LOG_DEBUGNORMAL, "Setting progress to %x",v);
            p16[PCNTR_REG_OFFSET] = v;
        }
    }
    else if((sMode & 0x00F0) != 0)
    {
        logout(WL_LOG_DEBUGNORMAL, "Octet 1, value %x",(sMode & 0x00F0));

        /* RAM expansion cart, clear bootrom's signature just in case. */
        p = (unsigned char *)EXTERNAL_SDRAM_CONTROLLER_BASE;

        for(i=0;i<256;i++)
        {
            p[i] = 0;
        }

        /* And that is all for RAM cart. */
        p16[PCNTR_REG_OFFSET] = 100;
    }
    else if((sMode & 0x0F00) != 0)
    {
        logout(WL_LOG_DEBUGNORMAL, "Octet 2, value %x",(sMode & 0x0F00));

        /* ROM
         * TODO : load corresponding rom from SD card.
         */
        p16[PCNTR_REG_OFFSET] = 100;
    }
    else
    {
        logout(WL_LOG_DEBUGNORMAL, "UNKNOWN MODE %x",sMode);
        p16[PCNTR_REG_OFFSET] = 100;
    }


    /* There should be a sync process, valid only for power memory. */
#endif // ABUS_SLAVE_ENABLE == 1



    /* Stop. */
    while(1){;}

    return 0;
}
