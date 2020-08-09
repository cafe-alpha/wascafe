/**
 *  Cartridge Test Utility
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/pskai/
 *
 *  See LICENSE file for details.
**/

#include <stdio.h>
#include <string.h> // For memset,memcpy

#include <sc_saturn.h>

#include "selftest.h"
#include "shared.h"

#include "../menu.h"
#include "../soft_reset.h"

#include "../romfs_location.h"
#include "../bootstub/bootstub_rom_exec.h"
#include "../cheat_codes/cheat_codes_rom_exec.h"
#include "../vmem/vmem_rom_exec.h"



/* File entries, used when listing SD card root folder contents. */
#define FILES_MAXCOUNT 100
scdf_file_entry_t _file_entries[FILES_MAXCOUNT];

void cart_selftest(void)
{
    menu_settings_t* mset   = &_menu_settings;
    menu_items_t   * mitems = &_menu_items;

    int menu_selected = 1;

    /* Initialize menu internals. */
    menu_reset_settings(mset);
    menu_set_callback_vblank(mset, background_vblank_clbk);

    menu_set_help(mset, "A/C: Confirm, B: Cancel");
    menu_set_title(mset, "Cartridge Self Test");
    menu_set_pos(mset, -1/*x0*/, 4*CONIO_CHAR_H/*y0*/, 21/*w*/, 0/*h*/, 1/*cols*/);
    menu_set_erase_on_exit(mset, 1);
    menu_set_features(mset, MENU_FEATURES_TEXTCENTER);

    menu_reset_items(mitems);
    menu_add_item(mitems, "Test once only", 1  /*tag*/);
    menu_add_item(mitems, "Test 10 times" , 10 /*tag*/);
    menu_add_item(mitems, "Test 100 times", 100/*tag*/);
    menu_add_item(mitems, "Test 500 times", 500/*tag*/);

    menu_selected = menu_start(mitems, mset, menu_selected/*selected_tag*/);

    if(mset->exit_code == MENU_EXIT_CANCEL)
    {
        return;
    }

    //* Greyout logo in order to show selftest results.
    // *
    // * And, disable startfield display since it's not
    // * possible to regularly animate it during cartridge
    // * self test sequence.
    // */
    //sf_set_control(0/*sf_enable*/);
    //logo_set_pattern(LOGOPAT_FORCE_GREYOUT);
    background_vblank_clbk();

    /* Selftest : verify cartridge stuff
     *  1. Show SD card related informations
     *     (former SD card troubleshooting screen)
     *  2. Verify main executable CRC value
     *  3. Verify each ROM code CRC value
     *  4. Verify RomFs integrity.
     *  5. Verify extra RAM.
     *  6. Write test program from RAM to SD card, 
     *     read it back, and verify if data matches or not.
     *
     * At end of loop, execute SD card test program.
     */
    int test_cnt = menu_selected;
    int test_id;
    int errorcnt = 0;
    int row = 3;
    int sdcard_use = 1;

    /* RAM test related. */
    sc_cartram_t ram;

    /* SD card test related. */
    char* filename;
    unsigned long tmp;
    sdcard_t* sd = sc_get_sdcard_buff();
    sdc_ret_t ret;

    int i, j;

    /* ROM code test related. */
    #define TEST_ROMCODE_CNT 3
    sc_rom_code_header_t* romcode_list[TEST_ROMCODE_CNT] = 
    {
        (sc_rom_code_header_t*)(ROM_BOOTSTUB_EXEC_ADDRESS - sizeof(sc_rom_code_header_t)), 
        (sc_rom_code_header_t*)(ROM_CHEATS_START                                         ), 
        (sc_rom_code_header_t*)(ROM_VMEM_EXEC_ADDRESS     - sizeof(sc_rom_code_header_t))
    };


    /* Re-init SD card library.
     * Note : if error is detected here, then SD card won't be
     *        tested after that. This is in order to support
     *        test on Action Replay carts without flooding
     *        screen with errors.
     */
    sdc_fat_reset();
    sdc_fat_init();
    ret = sd->init_status;

    // conio_printf(2, row++, COLOR_YELLOW, "SD card driver version : %02X.%02X", sd->stats.version_major, sd->stats.version_minor);
    // conio_printf(2, row++, COLOR_WHITE , "");

    if(ret == SDC_OK)
    {
        sdcard_use = 1;
        conio_printf(2, row++, COLOR_LIME, "SD card init success");
        conio_printf(2, row++, COLOR_WHITE , "");
        conio_printf(2, row++, COLOR_WHITE , "");
    }
    else
    {
        sdcard_use = 0;
        conio_printf(2, row++, COLOR_ERROR, "SD card init failure !");
        conio_printf(2, row++, COLOR_ERROR, "ret = %d", ret);
        conio_printf(2, row++, COLOR_WHITE , "");
    }

    conio_printf(2, row++, COLOR_YELLOW, "CID register details:");
    conio_printf(2, row++, COLOR_WHITE , " | Mfg ID         MID=%d"                 , sd->stats.cid[ 0]);
    conio_printf(2, row++, COLOR_WHITE , " | OEM/App ID     OID=%c%c"               , char2pchar(sd->stats.cid[ 1]), char2pchar(sd->stats.cid[ 2]));
    conio_printf(2, row++, COLOR_WHITE , " | Product name   PNM=%c%c%c%c%c"         , char2pchar(sd->stats.cid[ 3]), char2pchar(sd->stats.cid[ 4]), char2pchar(sd->stats.cid[ 5]), char2pchar(sd->stats.cid[ 6]), char2pchar(sd->stats.cid[ 7])); 
    conio_printf(2, row++, COLOR_WHITE , " | Product rev    PRV=0x%02X (v%d.%d)"    , sd->stats.cid[ 8], sd->stats.cid[ 8]>>4, sd->stats.cid[ 8] & 0x0F);
    conio_printf(2, row++, COLOR_WHITE , " | Product serial PSN=0x%02X%02X%02X%02X" , sd->stats.cid[ 9], sd->stats.cid[10], sd->stats.cid[11], sd->stats.cid[12]);
    conio_printf(2, row++, COLOR_WHITE , " | Mfg date       MDT=0x%04X %4d/%2d"     , ((sd->stats.cid[13]&0x0F) << 8) | sd->stats.cid[14], 2000+(((sd->stats.cid[13]&0x0F) << 4) | sd->stats.cid[14]>>4), sd->stats.cid[14]&0x0F);
    conio_printf(2, row++, COLOR_WHITE , " | checksum*2+1   CRC=0x%02X"             , sd->stats.cid[15]);
#define CHAR_PER_LINE 8
    for(j=0; j<(sizeof(sd->stats.cid) / CHAR_PER_LINE); j++)
    {
        /* Display start address. */
        conio_printf(2, row, COLOR_YELLOW, "%03X:", j*CHAR_PER_LINE);

        for(i=0; i<CHAR_PER_LINE; i++)
        {
            unsigned char c = sd->stats.cid[j*CHAR_PER_LINE + i];
            conio_printf(2+4+i*3  , row, COLOR_WHITE, "%02X", c);
            conio_printf(2+4+8*3+i, row, COLOR_WHITE, "%c"  , char2pchar(c));
        }
        row++;
    }
#undef CHAR_PER_LINE
    conio_printf(2, row++, COLOR_WHITE , "");

    conio_printf(2, row++, COLOR_YELLOW, "CSD register details:");
    conio_printf(2, row++, COLOR_WHITE , " | sdhc        = %d (CCS=%d)", sd->stats.sdhc, sd->ccs);
    conio_printf(2, row++, COLOR_WHITE , " | file_format = %d", sd->stats.file_format);
    conio_printf(2, row++, COLOR_WHITE , " | user_size   = %d MB", sd->stats.user_size);
#define CHAR_PER_LINE 8
    for(j=0; j<(sizeof(sd->stats.csd) / CHAR_PER_LINE); j++)
    {
        /* Display start address. */
        conio_printf(2, row, COLOR_YELLOW, "%03X:", j*CHAR_PER_LINE);

        for(i=0; i<CHAR_PER_LINE; i++)
        {
            unsigned char c = sd->stats.csd[j*CHAR_PER_LINE + i];
            conio_printf(2+4+i*3  , row, COLOR_WHITE, "%02X", c);
            conio_printf(2+4+8*3+i, row, COLOR_WHITE, "%c"  , char2pchar(c));
        }
        row++;
    }
#undef CHAR_PER_LINE
    conio_printf(2, row++, COLOR_WHITE , "");

    if(!wait_for_start("Press Start to continue"))
    {
        clear_screen();
        return;
    }
    clear_screen(); row = 3;

    if(sdcard_use)
    {
        conio_printf(2, row++, COLOR_WHITE, "Sector #0 contents:");

        /* Display sector#0 contents. */
        unsigned char sector_data[512];
        memset(sector_data, 0xFE, sizeof(sector_data));
        sdc_read_multiple_blocks(0/*sector*/, sector_data, 1/*blocks_count*/);

        int offset_list[20] =
        {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 
            55, 56, 57, 58, 59, 60, 61, /* First partition data. */
            63          /* Signature (55 AA). */
        };
#define CHAR_PER_LINE 8
        for(j=0; j<sizeof(offset_list) / sizeof(int); j++)
        {
            /* Display start address. */
            conio_printf(2, row, COLOR_YELLOW, "%03X:", offset_list[j]*CHAR_PER_LINE);

            for(i=0; i<CHAR_PER_LINE; i++)
            {
                unsigned char c = sector_data[offset_list[j]*CHAR_PER_LINE+i];
                conio_printf(2+4+i*3  , row, COLOR_WHITE, "%02X", c);
                conio_printf(2+4+8*3+i, row, COLOR_WHITE, "%c"  , char2pchar(c));
            }
            row++;
        }
#undef CHAR_PER_LINE

        if(!wait_for_start(NULL))
        {
            clear_screen();
            return;
        }
        clear_screen();


        /* Try to list files in root folder. */
        row = 3;
        if(ret == SDC_OK)
        {
            unsigned long items_count=0, i;

            sdc_fat_reset();
            ret = sdc_fat_init();

            if(ret == SDC_OK)
            {
                //scd_logout("Listing files ...");
                items_count = scf_filelist(_file_entries, FILES_MAXCOUNT, 0/*list_offset*/, 1/*list_sort*/, NULL/*all files*/, SC_FILE_SDCARD, "/");
                //scd_logout("File count = %d", items_count);
            }

            conio_printf(2, row++, COLOR_YELLOW, "FAT library test:");
            conio_printf(2, row++, COLOR_WHITE , "%d files in root folder", items_count);

            for(i=0; i<items_count; i++)
            {
                unsigned short sort_index = _file_entries[i].sort_index;
                scd_logout("ITM[%2d][%2d]=\"%s\"", i, sort_index, _file_entries[sort_index].filename);
                if(i < 17)
                {
                    conio_printf(2, row++, COLOR_WHITE , " | [%2d]=%s", sort_index, _file_entries[sort_index].filename);
                }
                else if(i==17)
                {
                    conio_printf(2, row++, COLOR_WHITE , " ...");
                }
            }
        }
        else
        {
            conio_printf(2, row++, COLOR_YELLOW, "FAT library test:");
            conio_printf(2, row++, COLOR_ERROR, "SD/FAT library init failure !");
            conio_printf(2, row++, COLOR_ERROR, "ret = %d", ret);
        }

        if(!wait_for_start("Press start to self-test cartridge"))
        {
            clear_screen();
            return;
        }
        clear_screen();
    }


    /* Selftest stuff. */
    unsigned char* st_ptr  = (unsigned char*)0x00280000;
    unsigned long  st_size = 0;
    int dummy_selftest     = 1;

    /* Test loop. */
    clear_screen();
    for(test_id=0; test_id<test_cnt; test_id++)
    {
        row = 4;

        conio_printf( 2, row, COLOR_WHITE, "      -= Cartridge Self Test =-     ", test_id+1, test_cnt);
        row++;
        row++; conio_printf(2, row, COLOR_TRANSPARENT, empty_line);

        /* Verify main program CRC. */
        unsigned char* fw_main_ptr = (unsigned char*)(FC_FLASH_ADDRESS + FC_DATA_OFFSET);
        unsigned long  fw_main_len = ((unsigned long*)(FC_FLASH_ADDRESS + FC_PARAMS_OFFSET))[FC_COMPDATALEN_INDEX];
        unsigned long  fw_main_crc = ((unsigned long*)(FC_FLASH_ADDRESS + FC_PARAMS_OFFSET))[FC_COMPDATACRC_INDEX];

        conio_printf(2, row, COLOR_WHITE, "Main prgm ...");
        if(fw_main_len <= (256*1024))
        {
            unsigned long crc32 = crc32_calc((const void*)fw_main_ptr, fw_main_len);

            if(crc32 == fw_main_crc)
            {
                conio_printf(16, row, COLOR_LIME, "OK (%08X, %3d KB)", crc32, fw_main_len >> 10);
            }
            else
            {
                conio_printf(16, row, COLOR_ERROR, "Fail ! (crc=%08X)", crc32);
                errorcnt++;
            }
        }
        else
        {
            /* Don't do CRC check when firmware is larger than expected.
             * This may happen when flash ROM is corrupted, or when
             * running demo version of Pseudo Saturn Kai.
             * Note : in order to keep compatibility with Action Replay
             *        cartridges, I don't plan to make main program
             *        larger than 256KB.
             */
            conio_printf(16, row, COLOR_ERROR, "Fail ! (sz=%08X)", fw_main_len);
            errorcnt++;
        }
        row++; conio_printf(2, row, COLOR_TRANSPARENT, empty_line);


        /* Verify each ROM codes CRC. */
        for(i=0; i<TEST_ROMCODE_CNT; i++)
        {
            int rc_check;

            if(i == 0)
            {
                /* Bootstub ROM code, used in both lite and full firmwares. */
                rc_check = 1;
                conio_printf(2, row, COLOR_WHITE, "Bootstub  ...");
            }
            else if(i == 1)
            {
                /* Cheats ROM code, used in both lite and full firmwares. */
                rc_check = 1;
                if(rc_check)
                {
                    conio_printf(2, row, COLOR_WHITE, "Cheatcodes...");
                }
            }
            else //if(i == 2)
            {
                /* Vmem ROM code, used full firmware only.
                 * Let's consider it is not worth testing
                 * when SD card interface is not found.
                 */
                rc_check = (sdcard_use ? 1 : 0);
                if(rc_check)
                {
                    conio_printf(2, row, COLOR_WHITE, "Vmem      ...");
                }
            }
            
            if(rc_check)
            {
                sc_rom_code_header_t* hdr = romcode_list[i];

                if((hdr->magic_42 == 0x42)
                && (hdr->magic_53 == 0x53))
                {
                    /* Verify ROM code CRC. */
                    unsigned long crc32 = crc32_calc((const void*)((unsigned char*)(hdr)+sizeof(sc_rom_code_header_t)), hdr->usedlen);

                    if(crc32 == hdr->crc32)
                    {
                        if(hdr->usedlen < 10000)
                        {
                            /* When it's possible, display size in byte unit. */
                            conio_printf(16, row, COLOR_LIME, "OK (%08X, %4d B)", crc32, hdr->usedlen);
                        }
                        else
                        {
                            conio_printf(16, row, COLOR_LIME, "OK (%08X, %3d KB)", crc32, hdr->usedlen >> 10);
                        }
                    }
                    else
                    {
                        conio_printf(16, row, COLOR_ERROR, "Fail ! (%08X)", crc32);
                        errorcnt++;
                    }
                }
                else
                {
                    /* Don't do CRC check when ROM code header is incorrect.
                     * This may happen when flash ROM is corrupted, or when
                     * running demo version of Pseudo Saturn Kai.
                     */
                    conio_printf(16, row, COLOR_ERROR, "Not found ! (%02X %02X)", hdr->magic_42, hdr->magic_53);
                    errorcnt++;
                }

                row++; conio_printf(2, row, COLOR_TRANSPARENT, empty_line);
            }
        }

        /* Verify RomFs integrity.
         * Let's consider it is not worth testing
         * when SD card interface is not found.
         */
        if(sdcard_use)
        {
            conio_printf(2, row, COLOR_WHITE, "RomFs     ...");
            if(!sc_romfs_setdata((unsigned char*)PSKAI_ROMFS_ADDRESS))
            {
                conio_printf(16, row, COLOR_ERROR, "Mount Error !");
                errorcnt++;
            }
            else
            {
                conio_printf(16, row, COLOR_LIME, "Mount OK");
            }
            row++; conio_printf(2, row, COLOR_TRANSPARENT, empty_line);
        }

        /* Initialize and test RAM. */
        conio_printf(2, row, COLOR_WHITE, "Extra RAM ...");
        scm_cartram_detect(&ram, SCM_CARTRAM_FULL_VERIFY);
        if(ram.detected)
        {
            if(ram.verify_ok)
            {
                conio_printf(16, row, COLOR_LIME, "OK (%d KB)", ram.size>>10);
            }
            else
            {
                conio_printf(16, row, COLOR_ERROR, "%s", ram.status_str);
                errorcnt++;
            }
        }
        else
        {
            /* RAM not found. But that's not an error. */
            conio_printf(16, row, COLOR_WHITE, "Unavailable");
        }
        row++; conio_printf(2, row, COLOR_TRANSPARENT, empty_line);

        if(sdcard_use)
        {
            /* Re-init SD card library. */
            conio_printf(2, row, COLOR_WHITE, "SD card   ...");
            sdc_fat_reset();
            sdc_fat_init();
            ret = sd->init_status;

            if(ret == SDC_OK)
            {
                conio_printf(16, row, COLOR_LIME, "Init Success");
            }
            else
            {
                conio_printf(16, row, COLOR_ERROR, "Error ! (%d)", sd->init_status);
                errorcnt++;
            }
            row++; conio_printf(2, row, COLOR_TRANSPARENT, empty_line);

#define TEST_FOLDER "/SELFTEST"

            /* First, create test directory. */
            conio_printf(2, row, COLOR_WHITE, "|mkdir    ...");
            if(!scf_mkdir(SC_FILE_SDCARD, TEST_FOLDER))
            {
                conio_printf(16, row, COLOR_ERROR, "Error !");
                errorcnt++;
            }
            else
            {
                conio_printf(16, row, COLOR_LIME, "OK");
            }
            row++; conio_printf(2, row, COLOR_TRANSPARENT, empty_line);


            /* Read self-test program from RomFs. */
            char* selftest_path = "/PSKAI/SELFTEST.BIN";

            dummy_selftest = 0;
            st_size = scf_size(SC_FILE_ROMFS, selftest_path);
            if(st_size != 0)
            {
                st_size = scf_read(SC_FILE_ROMFS, selftest_path, 0/*offset*/, st_size, st_ptr);
            }

            if(st_size == 0)
            {
                /* Use oncode size and dummy pattern if couldn't
                 * load selftest from RomFs.
                 * This also may happen when using lite firmware
                 * while SD card can be accessed, typically after
                 * incorrectly flashing incorrect firmware, or falling
                 * into lite mode because RomFs intergrity check failed.
                 */
                dummy_selftest = 1;
                st_size = 17349;

                /* Use dummy test pattern for selftest data. */
                for(tmp=0; tmp<st_size; tmp++)
                {
                    /* That's not super entropy, but enough for testing SD card.
                     * Trust me, I'm an engineer !
                     */
                    st_ptr[tmp] = (unsigned char)((tmp * 17) ^ (tmp % 53));
                }
                strcpy((char*)st_ptr, "TEST");
            }

            /* Calculate seltest's CRC. This will be used when
             * compating with data read back from SD card.
             */
            unsigned long st_crc = crc32_calc((const void *)st_ptr, st_size);

            /* Delete selftest program, if already exist. */
            filename = TEST_FOLDER "/TEST.BIN";
            conio_printf(2, row, COLOR_WHITE, "|delete   ...");
            if(!scf_delete(SC_FILE_SDCARD, filename))
            {
                conio_printf(16, row, COLOR_ERROR, "Error !");
                errorcnt++;
            }
            else
            {
                conio_printf(16, row, COLOR_LIME, "OK");
            }
            row++; conio_printf(2, row, COLOR_TRANSPARENT, empty_line);

            /* Write selftest program to SD card. */
            conio_printf(2, row, COLOR_WHITE, "|write    ...");
            tmp = scf_write((void*)st_ptr, st_size, SC_FILE_SDCARD, filename);
            if(tmp != st_size)
            {
                conio_printf(16, row, COLOR_ERROR, "Error ! (%d)", tmp);
                errorcnt++;
            }
            else
            {
                conio_printf(16, row, COLOR_LIME, "OK (%d bytes)", tmp);
            }
            row++; conio_printf(2, row, COLOR_TRANSPARENT, empty_line);

            /* Verify file size. */
            conio_printf(2, row, COLOR_WHITE, "|size     ...");
            tmp = scf_size(SC_FILE_SDCARD, filename);
            if(tmp != st_size)
            {
                conio_printf(16, row, COLOR_ERROR, "Error ! (%d)", tmp);
                errorcnt++;
            }
            else
            {
                conio_printf(16, row, COLOR_LIME, "OK (%d bytes)", tmp);
            }
            row++; conio_printf(2, row, COLOR_TRANSPARENT, empty_line);

            /* Read back selftest program from SD card. */
            memset((void*)st_ptr, 0, st_size);
            conio_printf(2, row, COLOR_WHITE, "|read     ...");
            tmp = scf_read(SC_FILE_SDCARD, filename, 0/*offset*/, st_size, (void*)st_ptr);
            if(tmp != st_size)
            {
                conio_printf(16, row, COLOR_ERROR, "Error ! (%d)", tmp);
                errorcnt++;
            }
            else
            {
                if(crc32_calc((const void *)st_ptr, st_size) != st_crc)
                {
                    /* Could read something, but contents don't match ! */
                    conio_printf(16, row, COLOR_ERROR, "CRC Error !");
                    errorcnt++;
                }
                else
                {
                    conio_printf(16, row, COLOR_LIME, "OK (%d bytes)", tmp);
                }
            }
            row++; conio_printf(2, row, COLOR_TRANSPARENT, empty_line);
#undef TEST_FOLDER
        } // if(sdcard_use)


        /* Last line : display test ID and error count. */
        row++; conio_printf(2, row, COLOR_TRANSPARENT, empty_line);
        conio_printf( 2, row, COLOR_WHITE, "Test %3d / %3d, Error cnt : ", test_id+1, test_cnt);
        conio_printf(30, row, (errorcnt == 0 ? COLOR_LIME : COLOR_ERROR), "%d", errorcnt);
    } // test loop

    /* Display some message when error happened. */
    if(errorcnt != 0)
    {
        row++;
        conio_printf(2, row++, COLOR_WHITE, "Majority of self-test failures are");
        conio_printf(2, row++, COLOR_WHITE, "caused by bad cartridge connection.");
        conio_printf(2, row++, COLOR_WHITE, "Turn off Saturn, re-insert cartridge");
        conio_printf(2, row++, COLOR_WHITE, "and then retry cartridge self-test.");
    }


    if((sdcard_use) && (!dummy_selftest))
    {
        /* Prompt for start button push before executing test program. */
        if(!wait_for_start("Press Start to execute test program"))
        {
            clear_screen();
            return;
        }

        /* Re-read selftest program from SD card, and execute it. */
        soft_reset(SC_SOFTRES_EACH_VDPS);
        memset((void*)st_ptr, 0, st_size);
        tmp = scf_read(SC_FILE_SDCARD, filename, 0/*offset*/, st_size, (void*)st_ptr);
        if(tmp != st_size)
        {
            sc_sysres();
        }

        sc_execute(SC_SOFTRES_ALL, (unsigned long)st_ptr/*tmp*/, st_size, 0x06004000/*exec*/);
    }
    else
    {
        /* No SD card available, or self test program couldn't
         * be retrieved from RomFs, so just display end message.
         */
        wait_for_start("Press Start to go back");
        clear_screen();
    }
}

