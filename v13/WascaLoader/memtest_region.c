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

/* Inclusion of some globals and constants. */
#include "memtest_normal.h"

#include "memtest_region.h"
#include "shared.h"

#include "../menu.h"

int _region_test_menuid = 1;
int _region_test_range = 0;
int _region_test_bits = 8;

int memtest_region(void)
{
    pad_t* pad;
    menu_settings_t* mset   = &_menu_settings;
    menu_items_t   * mitems = &_menu_items;

    int i, col, row = 4;

    int test_cnt = 0;

    while(1)
    {
        menu_reset_settings(mset);
        menu_set_callback_vblank(mset, background_vblank_clbk);

        menu_set_help(mset, "A/C: Confirm, B: Cancel");
        menu_set_title(mset, "Region Test Settings");
        menu_set_pos(mset, -1/*x0*/, 4*CONIO_CHAR_H/*y0*/, 21/*w*/, 0/*h*/, 1/*cols*/);
        menu_set_erase_on_exit(mset, 1);
        menu_set_features(mset, MENU_FEATURES_TEXTCENTER);

        menu_reset_items(mitems);
        menu_add_item(mitems, "Test once only" , 1   /*tag*/);
        menu_add_item(mitems, "Test 10 times"  , 10  /*tag*/);
        menu_add_item(mitems, "Test 100 times" , 100 /*tag*/);
        menu_add_item(mitems, "Test 500 times" , 500 /*tag*/);
        menu_add_item(mitems, "Test 5000 times", 5000/*tag*/);
        if(_region_test_bits == 8)
        {
            menu_add_item(mitems, "Access: 8 bits", 9998/*tag*/);
        }
        else if(_region_test_bits == 16)
        {
            menu_add_item(mitems, "Access: 16 bits", 9998/*tag*/);
        }
        else // if(_region_test_bits == 32)
        {
            menu_add_item(mitems, "Access: 32 bits", 9998/*tag*/);
        }

        if(_region_test_range == 0)
        {
            menu_add_item(mitems, "Test: All", 9999/*tag*/);
        }
        else if(_region_test_range == 1)
        {
            menu_add_item(mitems, "Test: regs only", 9999/*tag*/);
        }
        else if(_region_test_range == 2)
        {
            menu_add_item(mitems, "Test: OCRAM only", 9999/*tag*/);
        }
        else //if(_region_test_range == 3)
        {
            menu_add_item(mitems, "Test: SDRAM only", 9999/*tag*/);
        }

        clear_screen();
        _region_test_menuid = menu_start(mitems, mset, _region_test_menuid/*selected_tag*/);

        if(mset->exit_code == MENU_EXIT_CANCEL)
        {
            return 0;
        }

        if(_region_test_menuid == 9998)
        {
            if(_region_test_bits == 8)
            {
                _region_test_bits = 16;
            }
            else if(_region_test_bits == 16)
            {
                _region_test_bits = 32;
            }
            else // if(_region_test_bits == 32)
            {
                _region_test_bits = 8;
            }
        }
        else if(_region_test_menuid == 9999)
        {
            /* Toggle test range. */
            _region_test_range = (_region_test_range + 1) % 4;
        }
        else
        {
            test_cnt = _region_test_menuid;
            break;
        }
    }

    unsigned long total_err_cnt[16] = { 0 };
    unsigned long pass_cnt = 0;
    while(1)
    {
        int top_row = 4;
        row = top_row;

        conio_printf(2, row++, COLOR_YELLOW, "REGION  @%2ub  ITE COUNT ERR_TOTAL", _region_test_bits);

        int region_id;
        for(region_id=0; region_id<8; region_id++)
        {
            unsigned char* region_ptr = NULL;
            unsigned long region_len = 0;
            char* region_str = NULL;

            //conio_printf(2, row, COLOR_WHITE, empty_line);
            conio_printf(2, row, COLOR_WHITE, "                        ");
            if(region_id == 0)
            {
                /* Read/write registers. */
                region_ptr = (unsigned char*)0x23FFFFC0;
                region_len = 16;
                region_str = "R/W REGS";

                if((_region_test_range != 0) && (_region_test_range != 1))
                {
                    /* Test not requested. */
                    continue;
                }
            }
            else if(region_id == 1)
            {
                /* OCRAM. */
                region_ptr = (unsigned char*)0x23000000;
                region_len = 16;
                region_str = "OCRAM 16B";

                if((_region_test_range != 0) && (_region_test_range != 2))
                {
                    /* Test not requested. */
                    continue;
                }
            }
            else if(region_id == 2)
            {
                /* OCRAM. */
                region_ptr = (unsigned char*)0x23000000;
                region_len = 1*1024;
                region_str = "OCRAM 1KB";

                if((_region_test_range != 0) && (_region_test_range != 2))
                {
                    /* Test not requested. */
                    continue;
                }
            }
            else if(region_id == 3)
            {
                /* OCRAM. */
                region_ptr = (unsigned char*)0x23000000;
                region_len = 4*1024;
                region_str = "OCRAM 4KB";

                if((_region_test_range != 0) && (_region_test_range != 2))
                {
                    /* Test not requested. */
                    continue;
                }
            }
            else if(region_id == 4)
            {
                /* SDRAM. */
                region_ptr = (unsigned char*)0x22400000;
                region_len = 16;
                region_str = "SDRAM 16B";

                if((_region_test_range != 0) && (_region_test_range != 3))
                {
                    /* Test not requested. */
                    continue;
                }
            }
            else if(region_id == 5)
            {
                /* SDRAM. */
                region_ptr = (unsigned char*)0x22400000;
                region_len = 1*1024;
                region_str = "SDRAM 1KB";

                if((_region_test_range != 0) && (_region_test_range != 3))
                {
                    /* Test not requested. */
                    continue;
                }
            }
            else if(region_id == 6)
            {
                /* SDRAM. */
                region_ptr = (unsigned char*)0x22400000;
                region_len = 256*1024;
                region_str = "SDRAM 256KB";

                if((_region_test_range != 0) && (_region_test_range != 3))
                {
                    /* Test not requested. */
                    continue;
                }
            }
            else if(region_id == 7)
            {
                /* SDRAM. */
                region_ptr = (unsigned char*)0x22400000;
                region_len = 1*1024*1024;
                region_str = "SDRAM 1MB";

                if((_region_test_range != 0) && (_region_test_range != 3))
                {
                    /* Test not requested. */
                    continue;
                }
            }

            conio_printf(2, row, COLOR_WHITE, "%s", region_str);

            unsigned long err_cnt = 0;
            int test_id;
            for(test_id=0; test_id<test_cnt; test_id++)
            {
                /* Emergency exit with B button. */
                pad = pad_read();
                if(pad->new[0] & (PAD_B))
                {
                    wait_for_start("Test Ended. Start to exit.");
                    return 1;
                }

                unsigned char * l8  = (unsigned char*)0x00200000;
                unsigned char * c8  = region_ptr;
                unsigned long len   = region_len;

                /* Copy reference data to LRAM. */
                memcpy(l8 , c8, len);

                /* Fill both SDRAM and reference buffer with random data at random addresses. */
#define MEMTEST_OFFSETS_CNT (4096)
                if(_region_test_bits == 8)
                {
                    for(i=0; i<MEMTEST_OFFSETS_CNT; i++)
                    {
                        /* Note : rand function provided by stdlib returns number on
                         *        16 bits only. A quick and dirty workaround can be
                         *        done by calling rand function twice.
                         */
                        unsigned long memtest_offset = ((rand() & 0xFFFF) << 16) | (rand() & 0xFFFF);
                        memtest_offset = memtest_offset % len;

                        unsigned char val = (unsigned char)rand();
                        l8[memtest_offset] = val;
                        c8[memtest_offset] = val;
                    }
                }
                else if(_region_test_bits == 16)
                {
                    unsigned short* l16 = (unsigned short*)l8;
                    unsigned short* c16 = (unsigned short*)c8;
                    unsigned long len16 = len / sizeof(unsigned short);
                    for(i=0; i<MEMTEST_OFFSETS_CNT; i++)
                    {
                        unsigned long memtest_offset = ((rand() & 0xFFFF) << 16) | (rand() & 0xFFFF);
                        memtest_offset = memtest_offset % len16;

                        unsigned short val = (unsigned short)rand();
                        l16[memtest_offset] = val;
                        c16[memtest_offset] = val;
                    }
                }
                else // if(_region_test_bits == 32)
                {
                    unsigned long * l32 = (unsigned long*)l8;
                    unsigned long * c32 = (unsigned long*)c8;
                    unsigned long len32 = len / sizeof(unsigned long);
                    for(i=0; i<MEMTEST_OFFSETS_CNT; i++)
                    {
                        unsigned long memtest_offset = ((rand() & 0xFFFF) << 16) | (rand() & 0xFFFF);
                        memtest_offset = memtest_offset % len32;

                        unsigned long val = ((rand() & 0xFFFF) << 16) | (rand() & 0xFFFF);
                        l32[memtest_offset] = val;
                        c32[memtest_offset] = val;
                    }
                }



                /* Verify whole chunk with reference data. */
                unsigned long * l32 = (unsigned long *)l8;
                unsigned long * c32 = (unsigned long *)c8;
                for(i=0; i<(len / sizeof(unsigned long)); i++)
                {
                    if(*c32++ != *l32++)
                    {
                        err_cnt++;
                    }
                }

                /* Update total error count.
                 * (And, Q&D avoid of error count overflow)
                 */
                if((total_err_cnt[region_id] & 0x80000000) == 0)
                {
                    total_err_cnt[region_id] += err_cnt;
                }

                /* Show progression. */
                conio_printf(16, row, COLOR_WHITE, "%u", (100 * test_id) / test_cnt);

                /* Show error count for current pass. */
                col = 20;
                if(err_cnt == 0)
                {
                    conio_printf(col, row, COLOR_LIME, "OK ");
                }
                else
                {
                    /* Display error ratio rather than error count.
                     * -> It's simpler to verify if error occured
                     *    at 10% rate or on every access.
                     */
                    unsigned long rate = 100 * err_cnt;
                    rate = rate / (len / sizeof(unsigned long));
                    if(rate == 0)
                    {
                        rate = 1;
                    }
                    if(rate >= 100)
                    {
                        /* Highscore !!! */
                        conio_printf(col, row, COLOR_ERROR, "MAX");
                    }
                    else
                    {
                        conio_printf(col, row, (rate < 30 ? COLOR_YELLOW : COLOR_ERROR), "%02u%%", rate);
                    }
                }
            } /* Test count loop */

            /* Hide progression and display total error count. */
            conio_printf(16, row, COLOR_WHITE, "---");

            col = 26;
            if(total_err_cnt[region_id] == 0)
            {
                conio_printf(col, row, COLOR_LIME, "OK");
            }
            else
            {
                conio_printf(col, row, COLOR_ERROR , "%08X", total_err_cnt[region_id]);
            }

            row++;
        } /* Region loop */


        row++;
        pass_cnt += test_cnt;
        conio_printf(2, row, COLOR_YELLOW, "Pass Count: %u", pass_cnt);

        row++;
        unsigned long overall_count = 0;
        for(i=0; i<(sizeof(total_err_cnt) / sizeof(unsigned long)); i++)
        {
            overall_count += total_err_cnt[i];
        }

        if(overall_count == 0)
        {
            conio_printf(2, row, COLOR_LIME, "No error detected !");
        }
        else
        {
            conio_printf(2, row, COLOR_ERROR, "ErrorCnt: 0x%08X (%u)", overall_count, overall_count);
        }

    } /* while(1) */

    return 0;
}
