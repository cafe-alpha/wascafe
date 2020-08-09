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


/* Debug entry point, called when accessing WL_RDWR_DBG_SPACE. */
void spi_debug_ep(unsigned char* params);

/* Do SPI related tests. */
void spi_rxtx_test(void);


#endif // _WL_SPI_STM32_H_
