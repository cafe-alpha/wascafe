#include <stdio.h>
#include "bup_file.h"

#include "fatfs.h"
#include "log.h"

FIL _BupFile;
unsigned long _BupSize = 0;

void bup_file_process(wl_spi_pkt_t* pkt_rx, wl_spi_pkt_t* pkt_tx)
{
    int f_ret;

    /* Copy parameters to output buffer, and only use output buffer from now. */
    wl_spicomm_memacc_t* params = (wl_spicomm_memacc_t*)pkt_tx->params;
    memcpy(params, pkt_rx->params, WL_SPI_PARAMS_LEN*sizeof(unsigned char));

    if(pkt_rx->cmn.command == WL_SPICMD_BUPOPEN)
    {
        /* If already open, close the previous file. */
        if(_BupSize != 0)
        {
//termout(WL_LOG_DEBUGNORMAL, "[WL_SPICMD_BUPOPEN]close previous file ...");
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
//termout(WL_LOG_DEBUGNORMAL, "[WL_SPICMD_BUPOPEN]file_name:\"%s\"", file_name);

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
        }
        else
        {
            /* Sanitize file size and pre-allocate its data if necessary. */
            if(create_new)
            {
                unsigned char empty_block[512];
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
        params->len = 512;

        /* Ensure that access is done within file boundaries. */
        unsigned long offset = params->addr * 512;
        if((offset + params->len) > _BupSize)
        {
            /* Return FFh data and zero length in case of wrong access. */
            memset(pkt_tx->data, 0xFF, params->len);
            params->len = 0;
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
            }
        }
    }
    else if(pkt_rx->cmn.command == WL_SPICMD_BUPWRITE)
    {
        /* Sanitize access length : only one block (512 bytes)
         * access possible for now.
         */
        params->len = 512;

        /* Ensure that access is done within file boundaries. */
        unsigned long offset = params->addr * 512;
        if((offset + params->len) > _BupSize)
        {
            /* Return zero length in case of wrong access. */
            params->len = 0;
        }
        else
        {
            f_lseek(&_BupFile, offset);
            UINT bytes_written = 0;
            f_ret = f_write(&_BupFile, pkt_rx->data, params->len, &bytes_written);
            if(f_ret != FR_OK)
            {
                /* Return zero length in case of wrong access. */
                params->len = 0;
            }

            if(params->flag)
            {
                /* If requested, flush data to SD card right now. */
                f_sync(&_BupFile);
            }
        }
    }
    else if(pkt_rx->cmn.command == WL_SPICMD_BUPFLUSH)
    {
        if(_BupSize != 0)
        {
            f_sync(&_BupFile);
            params->len = (unsigned short)(_BupSize / 1024);
        }
        else
        {
            /* Return zero size when could not flush. */
            params->len = 0;
        }
    }
    else if(pkt_rx->cmn.command == WL_SPICMD_BUPCLOSE)
    {
        if(_BupSize != 0)
        {
            f_close(&_BupFile);

            /* On success, return size of the file we just closed. */
            params->len = (unsigned short)(_BupSize / 1024);

            _BupSize = 0;
        }
        else
        {
            /* Return zero size when there was nothing to close. */
            params->len = 0;
        }
    }
}
