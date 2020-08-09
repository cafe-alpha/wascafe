#ifndef _WL_SPI_STM32_H_
#define _WL_SPI_STM32_H_

#include <sys/alt_stdio.h>
#include <string.h>
#include <system.h>
#include <altera_avalon_pio_regs.h>

#include "wasca_defs.h"

/* SPI related #includes. */
// (nothing really special)


/*
 * ------------------------------------------
 * - SPI stuff ------------------------------
 * ------------------------------------------
 */

/* Retrieve STM32 firmware version. */
void spi_get_version(wl_spicomm_version_t* ver);

/* Read/write data from/to STM32 space. */
void spi_memread(unsigned long addr, unsigned long len, unsigned char* data);
void spi_memwrite(unsigned long addr, unsigned long len, unsigned char* data);

/* Debug entry point, called when accessing WL_RDWR_DBG_SPACE. */
void spi_debug_ep(unsigned char* params);

/* Do SPI related tests. */
void spi_rxtx_test(void);


#endif // _WL_SPI_STM32_H_
