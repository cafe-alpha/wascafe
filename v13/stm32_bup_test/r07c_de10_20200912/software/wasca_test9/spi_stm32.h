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

/* Read/write data from/to STM32 space.
 * 
 * Note : these functions exchange only one SPI packet, so it is
 *        necessary to call these functions several times when
 *        reading or writing data not fitting in one packet.
 * 
 * Returns 1 when no error happened, zero else.
 */
int spi_mem_read(unsigned long addr, unsigned long len, unsigned char* data);
int spi_mem_write(unsigned long addr, unsigned long len, unsigned char* data);

/* Backup memory access.
 * 
 * Note : these functions can process only one block (512 bytes)
 *        of backup memory data.
 * 
 * Returns 1 when no error happened, zero else.
 */
int spi_bup_open(unsigned long len_kb);
int spi_bup_read(unsigned long block, unsigned long len, unsigned char* data);
int spi_bup_write(unsigned long block, unsigned long len, unsigned char flush_flag, unsigned char* data);
int spi_bup_flush(void);
int spi_bup_close(void);


/* BUP file test bench.
 *
 * About BUP_TEST_BENCH_ENABLE :
 *  - 0 : disable the test bench
 *  - 1 : enable the test bench when a switch (# TBD) on DE10-lite board is set
 */
#define BUP_TEST_BENCH_ENABLE 1

#if BUP_TEST_BENCH_ENABLE == 1
void bup_test_bench(void);
#endif // BUP_TEST_BENCH_ENABLE == 1


/* Debug entry point, called when accessing WL_RDWR_DBG_SPACE. */
void spi_debug_ep(unsigned char* params);

/* Do SPI related tests. */
void spi_rxtx_test(void);


#endif // _WL_SPI_STM32_H_
