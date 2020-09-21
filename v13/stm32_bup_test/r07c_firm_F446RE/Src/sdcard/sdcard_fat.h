/**
 *  SatCom Library
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/satcom/
 *
 *  See LICENSE file for details.
**/
#ifndef _SDCARD_FAT_H_
#define _SDCARD_FAT_H_


/**
 *------------------------------------------------------------------
 * End-user functions. (FAT-level related)
 * Note: if you use FAT-level functions, you don't
 *       need to call Packet-level functions.
 *------------------------------------------------------------------
**/


/**
 * Detect SD card, read its internal parameters, then init FAT library.
 * Must be called when the user inserted a SD card in the slot.
 *
 * Return SDC_OK when init successed.
**/
sdc_ret_t sdc_fat_init(void);


/**
 * Return specified file's size.
 *
 * Return 0 when the specified file couldn't be opened.
**/
sdc_ret_t sdc_fat_file_size(char* filename, unsigned long* filesize);


/**
 * Read file contents on SD card.
 *
 * Return SDC_OK when successed.
**/
sdc_ret_t sdc_fat_read_file(char* filename, unsigned long offset, unsigned long length, unsigned char* buffer, unsigned long* read_length);

/**
 * Write file contents on SD card.
 *
 * Return SDC_OK when successed.
**/
sdc_ret_t sdc_fat_write_file(char* filename, unsigned long length, unsigned char* buffer, unsigned long* write_length);

/**
 * Append data to file.
 *
 * Return SDC_OK when successed.
**/
sdc_ret_t sdc_fat_append_file(char* filename, unsigned long length, unsigned char* buffer, unsigned long* write_length);

/**
 * Create specified directory.
 *
 * Return SDC_OK when successed.
 * Return SDC_OK even when directory already exist.
**/
sdc_ret_t sdc_fat_mkdir(char* directory);


/**
 * Get folder list.
**/
sdc_ret_t sdc_fat_filelist(scdf_file_entry_t* list_ptr, unsigned long list_count, unsigned long list_offset, char* folder_in, unsigned long* file_count_ptr);


/**
 * Remove specified file.
**/
sdc_ret_t sdc_fat_unlink(char* path);


/**
 *------------------------------------------------------------------
 * Internal-use functions. (FAT-level related)
 *------------------------------------------------------------------
**/


/**
 * Clear SD card and FAT32-related internal variables.
 * Must be called on program startup.
 *
 * Return SDC_OK.
**/
sdc_ret_t sdc_fat_reset(void);


/**
 * Init SD card and FAT32-related internal variables.
 * Must be called when the user inserted a SD card in the slot.
 *
 * If SD card was already init, this function does nothing, 
 * so if you want to re-init SD card stuff, you need to call
 * sdc_fat_reset function first.
 *
 * Return SDC_OK when init successed.
 *
 * Note: As this function is automatically called when user tries to
 *       access to SD card, it doesn't need to be called by user program..
**/
sdc_ret_t sdc_fat_init(void);


#endif // _SDCARD_FAT_H_
