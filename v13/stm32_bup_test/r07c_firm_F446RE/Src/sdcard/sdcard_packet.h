/**
 *  SatCom Library
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/satcom/
 *
 *  See LICENSE file for details.
**/
#ifndef _SDCARD_PACKET_H_
#define _SDCARD_PACKET_H_


/**
 *------------------------------------------------------------------
 * End-user functions. (Packet-level related)
 * Note: don't need to call when using FAT-32 functions.
 *------------------------------------------------------------------
**/

/* Data block size : only 512 bytes supported. */
#define SDC_BLOCK_SIZE 512

/* Timeout value (in poll count) when waiting for data start bit. */
#define SDC_POLL_COUNTMAX 200000


//SD commands, many of these are not used here
#define SDC_GO_IDLE_STATE            0
#define SDC_SEND_OP_COND             1
#define SDC_SEND_IF_COND             8
#define SDC_SEND_CSD                 9
#define SDC_SEND_CID                 10
#define SDC_STOP_TRANSMISSION        12
#define SDC_SEND_STATUS              13
#define SDC_SET_BLOCK_LEN            16
#define SDC_READ_SINGLE_BLOCK        17
#define SDC_READ_MULTIPLE_BLOCKS     18
#define SDC_WRITE_SINGLE_BLOCK       24
#define SDC_WRITE_MULTIPLE_BLOCKS    25
#define SDC_ERASE_BLOCK_START_ADDR   32
#define SDC_ERASE_BLOCK_END_ADDR     33
#define SDC_ERASE_SELECTED_BLOCKS    38
#define SDC_SD_SEND_OP_COND          41   //ACMD
#define SDC_APP_CMD                  55
#define SDC_READ_OCR                 58
#define SDC_CRC_ON_OFF               59


/**
 * Verify if SD card was reinserted or not.
 *
 * Return values :
 *  0 : SD card wasn't reinserted.
 *  1 : SD card was reinserted, hence SPI & FAT library reinit is needed.
**/
int sdc_is_reinsert(void);


/**
 * Detect SD card, and read its internal parameters.
 * Must be called when the user inserted a SD card in the slot.
 *
 * Return SDC_OK when init successed.
**/
sdc_ret_t sdc_init(void);



/**
 * Send packet to SD card and read the response from SD card.
 * Return the first 4 bytes read from SD card.
 * (Full returned data is stored in `sdcard_t* _sdcard' stuff)
**/
unsigned char sdc_sendpacket(unsigned char cmd, unsigned long arg, unsigned char* buffer, unsigned long blocks_count);



/**
 * Read/write multiple blocks from/to SD card.
 *
 * Return 0 if no error, otherwise the response byte is returned.
**/
unsigned char sdc_read_multiple_blocks(unsigned long start_block, unsigned char* buffer, unsigned long blocks_count);
unsigned char sdc_write_multiple_blocks(unsigned long start_block, unsigned char* buffer, unsigned long blocks_count);


/**
 * Extra function : turn OFF/ON LED on SD card reader board.
 *
 * Return SDC_OK when successed.
**/
sdc_ret_t sdc_ledset(unsigned long led_color, unsigned long led_state);


/**
 *------------------------------------------------------------------
 * Internal-use functions. (Low level I/O related)
 *------------------------------------------------------------------
**/

/**
 * Get/Set signals from SD card.
**/
void sdc_output(void);


/**
 * Assert/Deassert SD's "Chip Select" line.
**/
void sdc_cs_assert(void);
void sdc_cs_deassert(void);

/**
 * Send byte array to SD card.
 * Send byte (8 bits) to SD card.
 * Send long (32 bits) to SD card.
**/
void sdc_sendbyte_array(unsigned char* ptr, unsigned short len);
void sdc_sendbyte(unsigned char dat);
void sdc_sendlong(unsigned long dat);
/**
 * Receive byte array from SD card.
 * Receive byte (8 bits) from SD card.
**/
void sdc_receivebyte_array(unsigned char* ptr, unsigned short len);
unsigned char sdc_receivebyte(void);


#endif // _SDCARD_PACKET_H_
