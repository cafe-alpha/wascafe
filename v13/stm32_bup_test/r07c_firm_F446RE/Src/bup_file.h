#ifndef _WL_BUP_FILE_H_
#define _WL_BUP_FILE_H_

#include "main.h"
#include "stm32f4xx_hal.h"

#include "satcom_lib/wasca_link.h"

/*
 * ------------------------------------------
 * - Backup memory file access. -------------
 * ------------------------------------------
 */

/* Do backup memory file access according to SPI packet contents.
 * This must be done on main module side, away from interrupts
 * throwing log messages.
 *
 * Returns 1 when finished to process a SPI packet, zero else.
 */
void bup_file_process(wl_spi_pkt_t* pkt_rx, wl_spi_pkt_t* pkt_tx);

#endif // _WL_BUP_FILE_H_
