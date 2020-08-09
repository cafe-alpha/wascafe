#include "link.h"

#include <unistd.h>
#include <fcntl.h>

#include "crc.h"
#include "wasca_defs.h"

/* Declaration of link send/receive buffer.
 *
 * Since communication protocol just consists in answering
 * after being asked, there's no need to separate send
 * and receive buffers.
 */
unsigned char _link_rxtx_buffer[WL_PKT_MAXLEN];


void link_init(void)
{
    /* We probably don't want to stop everything in order
     * to read debug datagrams from PC, so set UART read
     * access to non-blocking access.
     */
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

    /*
     * Setup baudrate by setting UART core divisor register.
     *
     * From datasheet :
     *  https://www.altera.com/content/dam/altera-www/global/en_US/pdfs/literature/hb/nios2/n2cpu_nii51010.pdf
     *  Page 8-4
     *
     * Divisor Formula:
     *                clock_frequency 
     * divisor = int(----------------- + 0.5)
     *                   baud_rate
     * 
     * Example with f = 116 MHz, br = 115200 bps
     *    115905533/115200 + 0.5 = 1006.624418
     *    -> Set divisor to 1007 = 0x03EF
     *    -> With UART mapped from address 0x0005_1000 :
     *          [LOG]| Address: 0x00051010, Len:4
     *          [LOG]| Data: 0xEF 03 00 00
     * Other values :
     *  - 460800  bps -> divisor = 252 = 0xFC
     *  - 921600  bps -> divisor = 126 = 0x7E
     *  - 1500000 bps -> divisor =  78 = 0x4E
     *
     * Baud rate Formula:
     *                divisor + 1
     * baud_rate = -----------------
     *              clock_frequency 
     *
     * Table 8-6: UART Core Register Map
     *  Offset  Register     R/W    Description/Register Bits
     *          Name                15:13     12 11 10  9  8  7  6  5  4  3  2  1  0
     *    0     rxdata       RO     Reserved               Receive Data
     *    1     txdata       WO     Reserved               Transmit Data
     *    2     status       RW     Reserved  eop   dcts   e     trdy  toe   brk   p
     *                                           cts          rrdy  tmt   roe   fe
     *    3     control      RW     Reserved  ieop  idcts  ie    itrdy itoe  ibrk  ipe
     *                                           rts    trbk  irrdy itmt  iroe  ife
     *    4     divisor      RW     Baud Rate Divisor
     *    5     endof-packet RW     Reserved                End-of-Packet Value
     *
     *
     * UART related #definitions, in BSP folder -> system.h :
     *  - UART_0_BASE 0x51000
     *  - UART_0_BAUD 115200
     *  - UART_0_DATA_BITS 8
     *  - UART_0_FIXED_BAUD 0
     *  - UART_0_FREQ 116000000
     */
//#if UART_0_FREQ == 58000000
//    /* In the case frequency is set to half speed = 58 MHz.
//     * ... not sure that this lets enough time to process IRQ
//     *     for UART, so it's probably safer to divide baud rate
//     *     by two (= 460800 bps) on PC side.
//     */
//    ((unsigned long*)UART_0_BASE)[4] = 0x0000003F; //  921600 bps
//#else
#if UART_0_FREQ == 116000000
    /* Precise setting for historic "around 116 MHz" case. */
    //((unsigned long*)UART_0_BASE)[4] = 0x0000004E; // 1500000 bps
    ((unsigned long*)UART_0_BASE)[4] = 0x0000007E; //  921600 bps
    //((unsigned long*)UART_0_BASE)[4] = 0x000003EF; //  115200 bps
#else
    /* Setting for unusual inut clock and/or PLL settings. */
    unsigned long freq_tmp = (2 * UART_0_FREQ) / 921600;
    ((unsigned long*)UART_0_BASE)[4] = freq_tmp / 2;
#endif
//#endif

    // struct termios termios;
    // ioctl(STDIN_FILENO, TIOCMGET, &termios);
    // termios.c_ispeed = 14400;
    // termios.c_ospeed = termios.c_ispeed;
    // ioctl(STDIN_FILENO, TIOCMSET, &termios);
}


void link_send(wl_sndrcv_hdr_t* hdr, unsigned long data_len)
{
    unsigned char* data_ptr = ((unsigned char*)hdr) + sizeof(wl_sndrcv_hdr_t);

    /* Setup base header contents. */
    hdr->magic = WL_HDR_MAGIC_CA;
    hdr->len   = (unsigned short)(sizeof(wl_sndrcv_hdr_t) + data_len + sizeof(wl_crc_t));

    /* Compute and set CRC value. */
    wl_crc_t crc8 = crc_init();
    crc8 = crc_update(crc8, (unsigned char*)hdr, sizeof(wl_sndrcv_hdr_t) + data_len);
    crc8 = crc_finalize(crc8);
    data_ptr[data_len] = crc8;

    // /* Transfer start : turn on LED #2. */
    // IOWR(LEDS_BASE, 0, 0x02);

    /* Start asynchronous transfer. */
    link_snd_start(hdr->len);

    // /* Transfer (ansynchronous) end : turn off LED #2. */
    // IOWR(LEDS_BASE, 0, 0x00);
}


