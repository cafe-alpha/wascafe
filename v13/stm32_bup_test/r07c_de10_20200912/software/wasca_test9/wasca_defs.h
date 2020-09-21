#ifndef _WASCA_DEFS_H_
#define _WASCA_DEFS_H_

#include <system.h>
#include <sys/alt_stdio.h>

/* Link related inclusion.
 *
 * The wasca_link.h header below is shared
 * between both PC and Nios sides, hence the #define
 * below is needed.
 */
#include "satcom_lib/wasca_link.h"


/* Log enable/disable switch :
 *  - 0 : disable all logs
 *  - 1 : enable logging
 */
#define LOG_ENABLE 1


/* Memtest enable/disable switch :
 *  - 0 : disable memtest
 *  - 1 : enable memtest
 *
 * Note : disabling memory test saves
 *        around 3KB on NIOS executable.
 */
#define MEMTEST_ENABLE 1


/* Indicate type of board used :
 * WL_DEVTYPE_GENERIC     0
 * WL_DEVTYPE_WASCA       1
 * WL_DEVTYPE_M10BRD      2
 * WL_DEVTYPE_SIM         3
 * WL_DEVTYPE_M10BRD_WARP 4
 * WL_DEVTYPE_SIM_WARP    5
 */
#define WASCA_BRD_TYPE (WL_DEVTYPE_M10BRD_WARP)


/**
 *------------------------------------------
 * Below are automatic switches definitions.
 * Theses adapt to changes mades into Qsys, 
 * so don't need to be manually modified.
 *------------------------------------------
**/

/* Saturn A-Bus enable/disable switch :
 *  - 0 : enable A-Bus slave
 *  - 1 : disable A-Bus slave
 */
#ifdef __ABUS_SLAVE
#   define ABUS_SLAVE_ENABLE 1
#else
#   define ABUS_SLAVE_ENABLE 0
#endif


/* SPI enable/disable switch :
 *  - 0 : disable SPI
 *  - 1 : enable SPI
 */
#ifdef __ALTERA_AVALON_SPI
#   define SPI_ENABLE 1
#else
#   define SPI_ENABLE 0
#endif


/* SDRAM enable/disable switch :
 *  - 0 : disable SDRAM
 *  - 1 : enable SDRAM
 */
#ifdef __ALTERA_AVALON_NEW_SDRAM_CONTROLLER
#   define SDRAM_ENABLE 1
#else
#   define SDRAM_ENABLE 0
#endif


#endif // _WASCA_DEFS_H_
