#include <stdio.h>
#include "bup_file.h"

#include "fatfs.h"
#include "log.h"

FIL _BupFile;
unsigned long _BupSize = 0;


/* Number of blocks (512 bytes) to keep in memory
 * before writing to SD card.
 * 
 * Write conditions :
 *  - Backup file close is requested.
 *  - Cache full.
 *  - No write activity after a given time interval.
 */
#define BUP_CACHE_BLOCK_SIZE 512
#define BUP_CACHE_BLOCK_CNT    8

/* Interval of time (msec unit) to wait until flushing
 * backup memory data after no write request from MAX 10.
 */
#define BUP_FLUSH_INTERVAL 500




/* Information for each blocks pending in cache. */
typedef struct _bup_cache_info_t
{
    /* Write block ID. */
    unsigned long block_id;
} bup_cache_info_t;

bup_cache_info_t _bup_cache_info[BUP_CACHE_BLOCK_CNT];
unsigned long _bup_cache_cnt = 0;
unsigned char _bup_cache_data[BUP_CACHE_BLOCK_CNT * BUP_CACHE_BLOCK_SIZE];

unsigned long _bup_prev_append_tick = 0;

int _bup_error_flag = 0;


/* Function to update backup status on LED. */
void bup_led_status_set(int turn_on)
{
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, (turn_on ? GPIO_PIN_SET : GPIO_PIN_RESET));
}


void bup_cache_flush(void)
{
    if((_BupSize == 0) || (_bup_cache_cnt == 0))
    {
        return;
    }

    int i, j;
    for(i=0; i<_bup_cache_cnt; /*nothing*/)
    {
        unsigned long block_id  = _bup_cache_info[i].block_id;
        unsigned long block_cnt = 1;

        /* Gather consecutive blocks in a single write call. */
        for(j=i+1; j<_bup_cache_cnt; j++)
        {
            if(_bup_cache_info[j].block_id != (block_id + block_cnt))
            {
                break;
            }

            block_cnt++;
        }

        /* Write the backup memory block(s). */
        unsigned long offset  = block_id  * BUP_CACHE_BLOCK_SIZE;
        unsigned long datalen = block_cnt * BUP_CACHE_BLOCK_SIZE;
        if((offset + datalen) > _BupSize)
        {
            /* Indicate that an error happened.
             * (Not really needed because that's already checked beforehand)
             */
            _bup_error_flag = 1;
        }
        else
        {
            f_lseek(&_BupFile, offset);
            UINT bytes_written = 0;
            int f_ret = f_write(&_BupFile, _bup_cache_data + (i*BUP_CACHE_BLOCK_SIZE), datalen, &bytes_written);
            if(f_ret != FR_OK)
            {
                /* Indicate that an error happened. */
                _bup_error_flag = 1;
            }
        }

        /* Jump to next block in cache. */
        i+=block_cnt;
    }

    /* Indicate that cache is now empty. */
    _bup_cache_cnt = 0;
}


void bup_cache_append(unsigned long block_id, unsigned char* ptr)
{
    if(_BupSize == 0)
    {
        return;
    }

    /* Let's indicate that we are processing backup data. */
    bup_led_status_set(1/*turn_on*/);

    /* If cache is full, let's empty it now. */
    if(_bup_cache_cnt >= (BUP_CACHE_BLOCK_CNT))
    {
        bup_cache_flush();
        return;
    }

    /* Append block data to cache. */
    _bup_cache_info[_bup_cache_cnt].block_id = block_id;
    memcpy(_bup_cache_data + (_bup_cache_cnt*BUP_CACHE_BLOCK_SIZE), ptr, BUP_CACHE_BLOCK_SIZE);
    _bup_cache_cnt++;

    /* Remember the last moment there was write activity from MAX 10. */
    _bup_prev_append_tick = HAL_GetTick();
}






void bup_init(void)
{
    memset(_bup_cache_info, 0, sizeof(_bup_cache_info));
    memset(_bup_cache_data, 0, sizeof(_bup_cache_data));
    _bup_cache_cnt = 0;

    _bup_prev_append_tick = HAL_GetTick();

    /* Make error flag persistent. */
    //_bup_error_flag = 0;

    bup_led_status_set(0/*turn_on*/);
}


void bup_periodic_check(void)
{
    if(_bup_error_flag)
    {
        /* In case of error, make LED constantly blinking. */
        bup_led_status_set((HAL_GetTick() % 1000) > 500 ? 1 : 0);
    }

    if((_BupSize == 0) || (_bup_cache_cnt == 0))
    {
        return;
    }

    unsigned long tick = HAL_GetTick();
    unsigned long diff;
    if(tick >= _bup_prev_append_tick)
    {
        diff = tick - _bup_prev_append_tick;
    }
    else
    {
        /* This happens once every 49.7 days. */
        diff = (0xFFFFFFFF - _bup_prev_append_tick) + tick;
    }

    if(diff >= BUP_FLUSH_INTERVAL)
    {
        bup_cache_flush();

        /* Synchronize any pending write to SD card, 
         * so that it's safe to turn off the Saturn now.
         */
        f_sync(&_BupFile);

        /* As all backup data is written to SD card and that
         * additional data wasn't received for a while,
         * let's indicate that backup data process finished.
         */
        bup_led_status_set(0/*turn_on*/);
    }

    _bup_prev_append_tick = HAL_GetTick();
}


void bup_file_process(wl_spi_pkt_t* pkt_rx, wl_spi_pkt_t* pkt_tx)
{
    int f_ret;

    /* Copy parameters to output buffer, and only use output buffer from now. */
    wl_spicomm_memacc_t* params = (wl_spicomm_memacc_t*)pkt_tx->params;
    memcpy(params, pkt_rx->params, WL_SPI_PARAMS_LEN*sizeof(unsigned char));

    if(pkt_rx->cmn.command == WL_SPICMD_BUPOPEN)
    {
        /* If already open, don't forget to close the previous file. */
        if(_BupSize != 0)
        {
            bup_cache_flush();
            f_close(&_BupFile);

            _BupSize = 0;
        }

        /* Setup and sanitize size of save data file.
         * Also, prepare save data file name according to its size.
         */
        char file_name[64];
        if((params->len % 1024) == 0)
        {
            /* 1/2/4/etc MB mode. */
            _BupSize = params->len * 1024;
            sprintf(file_name, "/BUP_%02u.BIN", (unsigned int)(params->len / 1024));
        }
        else // if(params->len == 512)
        {
            /* 0.5MB mode. */
            _BupSize = 512 * 1024;
            strcat(file_name, "/BUP_05M.BIN");
        }
        // termout(WL_LOG_DEBUGNORMAL, "[WL_SPICMD_BUPOPEN]file_name:\"%s\"", file_name);

        f_ret = f_open(&_BupFile, file_name, FA_READ | FA_WRITE);

        int create_new = 0;
        if(f_ret != FR_OK)
        {
            /* Try to re-open by force. */
            f_ret = f_open(&_BupFile, file_name, FA_READ | FA_WRITE | FA_CREATE_ALWAYS);
            create_new = 1;
        }

        if(f_ret != FR_OK)
        {
            /* Kindly indicate that something wrong happened. */
            params->len = 0;
            _bup_error_flag = 1;
        }
        else
        {
            /* Sanitize file size and pre-allocate its data if necessary. */
            if(create_new)
            {
                unsigned char empty_block[BUP_CACHE_BLOCK_SIZE];
                unsigned long offset;

                memset(empty_block, 0xFF, sizeof(empty_block));

                for(offset=0; offset<_BupSize; offset+=sizeof(empty_block))
                {
                    UINT bytes_written = 0;
                    f_write(&_BupFile, empty_block, sizeof(empty_block), &bytes_written);
                }
            }

            f_lseek(&_BupFile, _BupSize);
            f_truncate(&_BupFile);
        }
    }
    else if(pkt_rx->cmn.command == WL_SPICMD_BUPREAD)
    {
        /* Sanitize access length : only one block (512 bytes)
         * access possible for now.
         */
        params->len = BUP_CACHE_BLOCK_SIZE;

        /* Ensure that access is done within file boundaries. */
        unsigned long offset = params->addr * BUP_CACHE_BLOCK_SIZE;
        if((offset + params->len) > _BupSize)
        {
            /* Return FFh data and zero length in case of wrong access. */
            memset(pkt_tx->data, 0xFF, params->len);
            params->len = 0;
            _bup_error_flag = 1;
        }
        else
        {
            f_lseek(&_BupFile, offset);
            UINT bytes_read = 0;
            f_ret = f_read(&_BupFile, pkt_tx->data, params->len, &bytes_read);
            if(f_ret != FR_OK)
            {
                /* Return FFh data and zero length in case of wrong access. */
                memset(pkt_tx->data, 0xFF, params->len);
                params->len = 0;
                _bup_error_flag = 1;
            }
        }
    }
    else if(pkt_rx->cmn.command == WL_SPICMD_BUPWRITE)
    {
        /* Sanitize access length : only one block (512 bytes)
         * access possible for now.
         */
        params->len = BUP_CACHE_BLOCK_SIZE;

        /* Ensure that access is done within file boundaries. */
        unsigned long block_id = params->addr;
        unsigned long offset = block_id * BUP_CACHE_BLOCK_SIZE;
        if((offset + params->len) > _BupSize)
        {
            /* Return zero length in case of wrong access. */
            params->len = 0;
            _bup_error_flag = 1;
        }
        else
        {
            bup_cache_append(block_id, pkt_rx->data);
        }
    }
    else if(pkt_rx->cmn.command == WL_SPICMD_BUPCLOSE)
    {
        if(_BupSize != 0)
        {
            /* Flush data pending in write cache. */
            bup_cache_flush();
            f_close(&_BupFile);

            /* On success, return size of the file we just closed. */
            params->len = (unsigned short)(_BupSize / 1024);

            _BupSize = 0;
        }
        else
        {
            /* Return zero size when there was nothing to close. */
            params->len = 0;
            _bup_error_flag = 1;
        }
    }
}
