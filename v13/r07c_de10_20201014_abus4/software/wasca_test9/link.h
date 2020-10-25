#ifndef _WL_LINK_H_
#define _WL_LINK_H_

#include <sys/alt_stdio.h>
#include <string.h>
#include <system.h>
#include <altera_avalon_pio_regs.h>

/* UART related #includes. */
#include "sys/ioctl.h"
#include "altera_avalon_uart.h"
#include "sys/alt_driver.h"
#include "priv/alt_file.h"


/*
 * ------------------------------------------
 * - Link stuff -----------------------------
 * ------------------------------------------
 */

/* Declaration for stdin (PC via UART) access. */
ALT_DRIVER_READ_EXTERNS(ALT_STDIN_DEV);

/* Declaration of send/receive buffer. */
extern unsigned char _link_rxtx_buffer[];

/* Initialize link internals. */
void link_init(void);

/* Return received count (in bytes) in Rx buffer. */
#define link_rcv_cnt() altera_avalon_uart_rxcnt(&ALT_DRIVER_STATE(ALT_STDIN_DEV))

/* Discard all data in Rx buffer. */
#define link_rcv_reset() altera_avalon_uart_rx_reset(&ALT_DRIVER_STATE(ALT_STDIN_DEV))


/* Start anyschronous data transmission. */
#define link_snd_start(_DATA_LEN_) altera_avalon_uart_tx_start(&ALT_DRIVER_STATE(ALT_STDIN_DEV), _DATA_LEN_)


/*
 * Send header, data and CRC through UART.
 * Data must be set just after header.
 * Base of header contents are automatically set inside this function.
 * CRC is automatically set inside this function.
 */
void link_send(wl_sndrcv_hdr_t* hdr, unsigned long data_len);


/*
 * Perform I/O check with UART.
 * Implemented directly on main module side.
 */
void link_do_io(void);

#endif // _WL_LINK_H_
