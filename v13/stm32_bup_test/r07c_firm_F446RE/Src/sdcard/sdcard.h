/**
 *  SatCom Library
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/satcom/
 *
 *  See LICENSE file for details.
**/
/**
 *  SD card module for Sega Saturn.
 *
 *  Links:
 *   - Easy to understand HW and SW ressources :
 *      http://matty.99k.org/sd-card-parallel-port/
 *   - About HW (well explained) :
 *      http://elm-chan.org/docs/mmc/mmc_e.html
 *      http://gandalf.arubi.uni-kl.de/avr_projects/arm_projects/arm_memcards/index.html
 *   - C source code for SD card SPI access :
 *      http://www.koders.com/c/fid2ED5A1007B1CC397DCC985205254C754392EC716.aspx
 *   - FAT32-related ressources (old debug versions of this library were made from here) :
 *      http://www.pjrc.com/tech/8051/ide/fat32.html
 *      http://code.google.com/p/thinfat32/
 *      http://www.dharmanitech.com/
**/

#ifndef _SDCARD_INCLUDE_H_
#define _SDCARD_INCLUDE_H_

#include <stdarg.h>

/* FAT32 related definitions. */
#include "fat_io_lib/fat_global.h"



/**
 * Note: Please use the following folder/file naming rules :
 *   1. Please name from the root directory.
 *   2. Please use slash ('/') folder delimiter.
 *   3. Please use UPPER CASE maximum 8 characters directory names.
 *   4. Please use UPPER CASE 8.3 file names.
 *  -> Typical file name: "/FOLDER1/FOLDER2/FILE.EXT"
 *   5. Maximum SC_PATHNAME_MAXLEN (96) characters per full file path, 
 *      including terminating null character.
 *  -> Due to 8.3 limitation, maximum path length is more than 10 sub
 *     folders, so shouldn't be a problem in practice.
 *  * Theses rules applies to all file device : PC, CD-ROM, SD card, etc.
**/


/**
 * Maximum length for file name, 
 * including terminating null character.
 *
 * This size is used in many structures
 * (example : scdf_file_entry_t), 
 * in several projects
 * (example : satcom_lib, vmem, custom_yabause), 
 * so changing it is not recommended.
**/
#define SC_FILENAME_MAXLEN 27

/**
 * Maximum length for file full path, 
 * including terminating null character.
**/
#define SC_PATHNAME_MAXLEN 96


/**
 * Devices IDs.
**/
#define SC_FILE_SDCARD 0


/**
 *------------------------------------------------------------------
 * SD card library version.
 *------------------------------------------------------------------
**/
#define SDC_VERSION_MAJOR 0xFF
#define SDC_VERSION_MINOR 0x02



/**
 *------------------------------------------------------------------
 * Error codes returned by each functions.
 *------------------------------------------------------------------
**/
typedef long sdc_ret_t;
#define SDC_NOTINIT     0
#define SDC_OK          1
#define SDC_FAILURE     2
#define SDC_NOHARDWARE  3
#define SDC_NOCARD      4
#define SDC_FATERROR    5
#define SDC_INITERROR   6
#define SDC_NOFILE      7
#define SDC_TIMEOUT     8
#define SDC_OUTOFMEM    9
#define SDC_WRONGPARAM 10



/**
 *------------------------------------------------------------------
 * Defines & structures.
 *------------------------------------------------------------------
**/

// GCC have alternative #pragma pack(N) syntax and old gcc version not
// support pack(push,N), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack(1)
#else
#pragma pack(push,1)
#endif

typedef struct _sdcard_stats_t
{
    /*------------------------------------------*/
    /* Some stats about SD card currently used. */

    /* CID register - CMD10. */
    unsigned char cid[16];

    /* CSD register - CMD9. */
    unsigned char csd[16];

    /* Version numbers. */
    unsigned char version_major;
    unsigned char version_minor;

    /* CSD register - CMD9. */
    unsigned char sdhc;      /* 0:SD, 1:SDHC, 2-3:unsupported type. */
#define SDC_FILEFORMAT_PARTITIONTABLE 0
#define SDC_FILEFORMAT_FLOPPYLIKE     1
#define SDC_FILEFORMAT_UNIVERSAL      2
#define SDC_FILEFORMAT_UNKNOWN        3
    unsigned char file_format;
    unsigned long user_size; /* In MB. */
} sdcard_stats_t __attribute__((aligned(4)));


/**
 * Structure : sdcard_io_t
 * Stores SD card I/O data.
**/
typedef struct _sdcard_io_t
{
    /*----------------------*/
    /* SD card SPI signals. */
    unsigned char csl;  // SD chip selection   : Sat -> SD
    unsigned char din;  // SD data input       : Sat -> SD
    unsigned char clk;  // SD clock            : Sat -> SD
    //unsigned char dout; // SD data output      : Sat <- SD
    unsigned char reserved[1]; // Unused, for 4 byte alignment
} sdcard_io_t __attribute__((aligned(4)));



/**
 * Structure : sdcard_t
 * Purpose : structure in RAM containing global data for use by SD card controller.
 * 
 * Note: BUP_Init function receives pointers to 2 buffers in RAM :
 *  unsigned long	BackUpLibrary[4096]; 16KB, 4 bytes aligned
 *  unsigned long	BackUpRamWork[2048];  8KB, 4 bytes aligned
 * So sdcard_t should be designed to get a size less than 16KB.
**/
#define SDC_CMDBUFFER_LEN 24
typedef struct _sdcard_t
{
    /* I/O status. */
    sdcard_io_t s;

    /* SD card related statistics. */
    sdcard_stats_t stats;

    /* Packet-related informations. */
    unsigned char packet_timeout;
    unsigned char ccs;
    unsigned char sd_ver2;
    unsigned char mmc;

    /* Packet send/receive duration, in micro seconds. */
    unsigned long duration_usec;

    /*
     * Buffer to receive status and register data.
     * Note: 4 bytes alignment is required.
     * Note: register data can be found from the pointer below :
     *       stat_reg_buffer + status_len
     */
    unsigned char stat_reg_buffer[SDC_CMDBUFFER_LEN];
    unsigned char status_len;

    /* 0: SD card removed, 1: SD card inserted. */
    unsigned char inserted;

    /* 0: don't save log messages to SD card, 1: save log messages to SD card. */
    unsigned char log_sdcard;

    /* Set to 1 when everything from SPI to FAT library is initialized correctly. */
    unsigned char init_ok;

    /* SD card init status :
     *  - Value returned by sdc_init function (positive values).
     *  - Value returned by fl_attach_media function (negative values).
     */
    sdc_ret_t init_status;

    /* FAT32 stuff. */
    FL_GLOBAL fl_global;
} sdcard_t __attribute__((aligned(4)));


// GCC have alternative #pragma pack() syntax and old gcc version not
// support pack(pop), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack()
#else
#pragma pack(pop)
#endif

/**
 * sc_get_sdcard_buff
 *
 * Get address of SD card library workram.
**/
sdcard_t* sc_get_sdcard_buff(void);



/* Definition of scdf_file_entry_t used when listing folder contents. */
//#include "../debugger/scd_common.h"


// GCC have alternative #pragma pack(N) syntax and old gcc version not
// support pack(push,N), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack(1)
#else
#pragma pack(push,1)
#endif

/*
 * File entry record used when listing files in folder.
 *
 * Among others, it is used in custom_yabause, so
 * changing this structure is not recommended.
 */
typedef struct _scdf_file_entry_t
{
    /* Entry flags. */
#define FILEENTRY_FLAG_ENABLED     0x01
#define FILEENTRY_FLAG_FOLDER      0x02
#define FILEENTRY_FLAG_NAMETOOLONG 0x04
    /* Internal usage flags. */
#define FILEENTRY_FLAG_COMPRESSED  0x20
#define FILEENTRY_FLAG_LASTENTRY   0x40
#define FILEENTRY_FLAG_FILTERED    0x80
    unsigned char flags;

    /* File name+null character. */
    char filename[SC_FILENAME_MAXLEN];

    /* File index in this folder (start from zero). */
    unsigned short file_index;

    /* Index after file name sorting (start from zero). */
    unsigned short sort_index;
} scdf_file_entry_t;
// GCC have alternative #pragma pack() syntax and old gcc version not
// support pack(pop), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack()
#else
#pragma pack(pop)
#endif






/**
 * Extra function : turn OFF/ON LED on SD card reader board.
 *
 * Return SDC_OK when successed.
**/
/* First parameter : LED color. */
#define SDC_LED_RED   0
#define SDC_LED_GREEN 1
/* Second parameter : LED state. */
#define SDC_LED_OFF 0
#define SDC_LED_ON  1





/**
 *------------------------------------------------------------------
 * Logging/debugging-related define switchs.
 *------------------------------------------------------------------
**/
#define SDC_LOGOUT_USE 0

void sdc_logout_internal(char* string, ...);

#if SDC_LOGOUT_USE == 1
#   define sdc_logout(_STR_, ...) sdc_logout_internal(_STR_, __VA_ARGS__)
#else
#   define sdc_logout(_STR_, ...)
#endif




//----------------------------------------------------------------------
// Char manipulation functions.
// Used for filename sorting (scf_filelist function) in sc_saturn.c.
// 
//----------------------------------------------------------------------
char sc_toupper(char c);
char sc_tolower(char c);


/*------------------------------------------------------------------------*/
/*- File name/path related functions. ------------------------------------*/
/**
 * scf_compare_extension
 *
 * Compare filename's extension to extension given in parameter.
 * Comparision is case insensitive.
 *
 * Returns 1 when extension are the same, zero else.
 *
 * Example : scf_compare_extension("/FOLDER/FILE.BIN", ".bin") returns 1;
**/
int scf_compare_extension(char* str, char* ext);


/**
 * scf_get_filename
 *
 * Extract file name from specified path.
 *
 * Examples :
 *  "/FOLDER/SUBFLDR/FILE.EXT" -> "FILE.EXT"
**/
char* scf_get_filename(char* path);


/**
 * scf_parent_path
 *
 * Extract parent path from specified path.
 *
 * Examples :
 *  "/FOLDER/SUBFLDR/FILE.EXT" -> "/FOLDER/SUBFLDR"
 *  "/FOLDER/SUBFLDR"          -> "/FOLDER"
 *  "/FOLDER"                  -> ""
**/
void scf_parent_path(char* path);


/*------------------------------------------------------------------------*/
/*- File I/O functions. --------------------------------------------------*/



/**
 * sc_get_sdcard_buff
 *
 * Get address of SD card library workram.
**/
sdcard_t* sc_get_sdcard_buff(void);



/**
 * scf_init
 *
 * Initialize file access stuff.
**/
void scf_init(void);



/**
 * scf_size
 *
 * Return specified file's size.
 * In the case the file doesn't exists, return 0
**/
unsigned long scf_size(unsigned char dev_id, char* filename);


/**
 * scf_read
 *
 * Read `size' bytes from specified file's `offset'th byte to `ptr'.
 *
 * Return the number of bytes actually read.
 * In the case the file doesn't exists, or read operation failed, return 0
 *
 * Note1: To increase performances when reading from CD-ROM, it is recommended
 *        to use offset values that are multiple of 2048 bytes.
 *        If it is not the case, 2 open->read->close requests are performed (instead of 1).
 *        Moreover, up to 2047 "useless" bytes may be read from CD when offset is not a multiple of 2048 bytes.
 *
 * Note2: To increase performances when reading from CD-ROM, it is recommended
 *        to use files in root folder only.
 *
 * Note3: If `size' is higher than file's size (=fsz), fsz bytes are actually read, and fsz is returned.
 *        So you can use the following code :
 *        char buffer[1234];
 *        unsigned long file_len = sc_fread(SC_FILE_SDCARD, filename, 0, sizeof(buffer), (void*)buffer);
**/
unsigned long scf_read(unsigned char dev_id, char* filename, unsigned long offset, unsigned long size, void* ptr);


/**
 * scf_write
 *
 * Write `size' bytes from `ptr' to specified file.
 * In the case specified file already exists, its contents is erased.
 *
 * Return the number of bytes actually written.
 * In the case write operation failed, return 0
 *
 * Note: When trying to write to a file in the CD-ROM, 0 is returned.
**/
unsigned long scf_write(void* ptr, unsigned long size, unsigned char dev_id, char* filename);


/**
 * scf_append
 *
 * Append `size' bytes from `ptr' to specified file.
 * In the case specified file doesn't exists, file is created, and data written to it.
 *
 * Return the number of bytes actually appended.
 * In the case append operation failed, return 0
 *
 * Note: When trying to append to a file in the CD-ROM, 0 is returned.
**/
unsigned long scf_append(void* ptr, unsigned long size, unsigned char dev_id, char* filename);

/**
 * scf_mkdir
 *
 * Make directory.
 *
 * Return 1 on success, 0 else.
 * Note : returns 1 even in the case folder already exists.
**/
unsigned long scf_mkdir(unsigned char dev_id, char* folder);

/**
 * scf_delete
 *
 * Remove file specified in path.
 *
 * Return 1 on success, 0 else.
**/
unsigned long scf_delete(unsigned char dev_id, char* path);

/**
 * scf_filelist
 *
 * List files/folders in specified path.
 *
 * Up to `list_count' files/folders entries are stored in `list_ptr' array.
 *
 * `list_offset' specifies the entry ID from where to start listing.
 * Example : In the case 42 files/folders are present in specified path, 
 * and `list_count' = 10, and `list_offset' = 20, 
 * files/folders from ID #20 to ID #29 are stored in `list_ptr' array.
 *
 * This allows to list entries little by little, even in the case there are 
 * many files/folders to list and/or few memory is available on Saturn
 * for storing listing results.
 *
 * When `list_sort' is set to 1, sort entries according to their file names.
 * (Entries themselves in `list_ptr' array are unsorted, 
 * you need to use `sort_index' in each entry in order to get the sorted list)
 *
 * `filter' allows to filter files according to their extensions.
 *  - When NULL, all files are listed.
 *  - All extensions are separated by commas.
 *  - Extension can't be larger than 3 characters.
 * Example filter = ".dat,.bin,.txt"
 *
 * Note : when `list_count' is lower than files/folder in specified path, 
 * or when `list_offset' is nonzero, entries are not sorted, 
 * even when `list_sort' is set to 1.
 *
 * Return the count of files/folders entries (including disabled entries)
 * stored in output buffer.
 *
 * Note : if files/folders count in the specified folder is larger than
 * 'list_count' parameter, then return value will be smaller than files/folder count.
 *
 * Note : the list and its entries count may include disabled entries, which
 * typically happens when file filtering is applied, or when reserved file or
 * folders are discarded (example : "." or ".." folder entries).
 * So in order to verify if a folder is empty or not, it is necessary to use
 * scf_filecount function on scf_filelist function's output parameter.
**/
unsigned long scf_filelist(scdf_file_entry_t* list_ptr, unsigned long list_count, unsigned long list_offset, unsigned char list_sort, char* filter, unsigned char dev_id, char* path);


/**
 * scf_filecount
 *
 * Return the count of enabled files/folders in specified search result data.
**/
unsigned long scf_filecount(scdf_file_entry_t* list_ptr, unsigned long list_count);



/* Provide user access to library internals.
 * That's just for debug and should be commented-out someday.
 */
#include "sdcard_packet.h"
#include "sdcard_fat.h"


#endif // _SDCARD_INCLUDE_H_
