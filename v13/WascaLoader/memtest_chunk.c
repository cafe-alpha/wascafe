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

#include "memtest_chunk.h"
#include "shared.h"

#include "../menu.h"


int _chunk_test_menuid = 1;
int _chunk_test_cs     = 0;

int memtest_chunk(void)
{
    pad_t* pad;
    menu_settings_t* mset   = &_menu_settings;
    menu_items_t   * mitems = &_menu_items;

    int i, row = 4;

    /* Chunk Test : test the following patterns :
     *  - Fill with 0xAA
     *  - Fill with 0x55
     *  - Fill with random values at random offsets and 8/16/32 access sizes
     *
     * Test only first half of CS0, so that registers (currently
     * located at the end of CS0) won't cause mess during testing.
     *
     * - Display pattern :
     *  | CS0  [AA] [55] [BI] [08] [16] [32]
     *  | +01   OK   NG   NG  999  999  999
     * - Error count is maxed to 999
     */
#define CHECK_CHUNKSIZE  (1*1024*1024)
#define CHECK_CHUNKCOUNT ((16*1024*1024) / CHECK_CHUNKSIZE)
#define LRAM_LEN (1024*1024)

    /* Clear normal (excluding status) display. */
    clear_normal_display();

    // cartridge initialization: as in ST-TECH-47.pdf
    // 1. verify cartridge id
    // 5A -  8 Mb (1MB)
    // 5C - 32 Mb (4MB)
    //! is last byte of A-BUS CS1 area
    cart_id = *((unsigned char *)0x24ffffff);
    if(cart_id == 0x5a)
    {
        my_CartRAMsize = 1*1024*1024;
    }
    else if(cart_id == 0x5c)
    {
        my_CartRAMsize = 4*1024*1024;
    }
    else
    {
        my_CartRAMsize = CHECK_CHUNKSIZE * CHECK_CHUNKCOUNT;
    }

    int test_cnt = 0;

    while(1)
    {
        menu_reset_settings(mset);
        menu_set_callback_vblank(mset, background_vblank_clbk);

        menu_set_help(mset, "A/C: Confirm, B: Cancel");
        menu_set_title(mset, "Memory Chunk Test");
        menu_set_pos(mset, -1/*x0*/, 4*CONIO_CHAR_H/*y0*/, 21/*w*/, 0/*h*/, 1/*cols*/);
        menu_set_erase_on_exit(mset, 1);
        menu_set_features(mset, MENU_FEATURES_TEXTCENTER);

        menu_reset_items(mitems);
        menu_add_item(mitems, "Test once only", 1  /*tag*/);
        menu_add_item(mitems, "Test 5 times"  , 5 /*tag*/);
        menu_add_item(mitems, "Test 10 times" , 10 /*tag*/);
        menu_add_item(mitems, "Test 100 times", 100/*tag*/);
        menu_add_item(mitems, "Test 500 times", 500/*tag*/);
        if(_chunk_test_cs == 0)
        {
            menu_add_item(mitems, "Test on : CS0", 999/*tag*/);
        }
        else
        {
            menu_add_item(mitems, "Test on : CS1", 999/*tag*/);
        }

        clear_screen();
        _chunk_test_menuid = menu_start(mitems, mset, _chunk_test_menuid/*selected_tag*/);

        if(mset->exit_code == MENU_EXIT_CANCEL)
        {
            return 0;
        }

        if(_chunk_test_menuid == 999)
        {
            /* Toggle CS0/CS1. */
            _chunk_test_cs = (_chunk_test_cs ? 0 : 1);
        }
        else
        {
            test_cnt = _chunk_test_menuid;
            break;
        }
    }

    unsigned long total_err_cnt = 0;
    int test_id;
    for(test_id=0; test_id<test_cnt; test_id++)
    {
        row = 4;
        if(_chunk_test_cs == 0)
        {
            conio_printf(2, row++, COLOR_LIME , "ID:%02X (%2u MB), t:%03u/%03u, e:%08X", cart_id, my_CartRAMsize>>20, test_id+1, test_cnt, total_err_cnt);
        }
        else
        {
            conio_printf(2, row++, COLOR_LIME , "CS1 TEST     , t:%03u/%03u, e:%08X", test_id+1, test_cnt, total_err_cnt);
        }

        conio_printf(2, row++, COLOR_YELLOW, "CS%u  [AA] [55] [BI] [08] [16] [32]", _chunk_test_cs);

        int chunk_id;
        for(chunk_id=0; chunk_id<CHECK_CHUNKCOUNT; chunk_id++)
        {
            conio_printf(2, row, COLOR_WHITE, empty_line);
            conio_printf(2, row, COLOR_WHITE, "+%02u", chunk_id);

            /* Skip unavailable memory areas when 4MB expansion RAM is detected.
             *
             * Don't care about the 1MB cart because it is arranged into 512KB
             * blocks, causing troubles here because tests are done 1MB per 1MB.
             */
            int skip_test = 0;
            if((_chunk_test_cs == 0) && (cart_id == 0x5c))
            {
                if((chunk_id < 4) || (chunk_id >= 8))
                {
                    skip_test = 1;
                }
            }
            if(skip_test)
            {
                conio_printf(8, row++, COLOR_WHITE, "--   --   --   --   --   -- ");
                continue;
            }

            int pass_id;
            for(pass_id=0; pass_id<6; pass_id++)
            {
                /* Emergency exit with B button. */
                pad = pad_read();
                if(pad->new[0] & (PAD_B))
                {
                    wait_for_start("Test Ended. Start to exit.");
                    return 1;
                }

                unsigned long err_cnt    = 0;
                unsigned long ite_total  = 0;
                unsigned char* lram_ptr  = (unsigned char*)0x00200000;
                unsigned char* chunk_ptr = (unsigned char*)((_chunk_test_cs == 0 ? 0x22000000 : 0x24000000) + (chunk_id * CHECK_CHUNKSIZE));

                if((pass_id == 0) || (pass_id == 1))
                { /* AA/55 write test (32 bits width, continous). */
                    unsigned long fill_val = ((pass_id == 0) ? 0xAA5555AA : 0x55AAAA55);
                    unsigned long* ptr;

                    ptr = (unsigned long*)chunk_ptr;
                    for(i=((CHECK_CHUNKSIZE / sizeof(unsigned long)) - 1); i; i--)
                    {
                        *ptr++ = fill_val;
                    }

                    ptr = (unsigned long*)chunk_ptr;
                    for(i=((CHECK_CHUNKSIZE / sizeof(unsigned long)) - 1); i; i--)
                    {
                        if(*ptr++ != fill_val)
                        {
                            err_cnt++;
                        }
                    }
                    ite_total = CHECK_CHUNKSIZE / sizeof(unsigned long);
                }
                else if(pass_id == 2)
                { /* Write BIOS, then read back and compare. */
#define REF_LEN ( 512*1024)
#define BI_TEST_USE_MEMCPY
                    unsigned char* ref_ptr = lram_ptr + REF_LEN;
                    for(i=0; i<REF_LEN; i++)
                    {
                        ref_ptr[i] = (unsigned char)rand();
                    }


#ifdef BI_TEST_USE_MEMCPY
                    memcpy(chunk_ptr, ref_ptr, REF_LEN);
#else
                    for(i=0; i<REF_LEN; i++)
                    {
                        chunk_ptr[i] = ref_ptr[i];
                    }
#endif

#ifdef BI_TEST_USE_MEMCPY
                    memcpy(lram_ptr, chunk_ptr, REF_LEN);
#else
                    for(i=0; i<REF_LEN; i++)
                    {
                        lram_ptr[i] = chunk_ptr[i];
                    }
#endif

                    for(i=0; i<REF_LEN; i++)
                    {
                        if(ref_ptr[i] != lram_ptr[i])
                        {
                            err_cnt++;
                        }
                    }
                    ite_total = REF_LEN;
                }
                else if((pass_id == 3) || (pass_id == 4) || (pass_id == 5))
                {
                    int byte_cnt = 1 << (pass_id-3);
                    unsigned char * l8  = (unsigned char *)lram_ptr;
                    unsigned short* l16 = (unsigned short*)lram_ptr;
                    unsigned long * l32 = (unsigned long *)lram_ptr;
                    unsigned char * c8  = (unsigned char *)chunk_ptr;
                    unsigned short* c16 = (unsigned short*)chunk_ptr;
                    unsigned long * c32 = (unsigned long *)chunk_ptr;

                    /* Copy reference data to LRAM. */
                    memcpy(lram_ptr , chunk_ptr, LRAM_LEN);

                    /* Fill both SDRAM and reference with random data at random addresses. */
#define MEMTEST_OFFSETS_CNT (4096)
                    for(i=0; i<MEMTEST_OFFSETS_CNT; i++)
                    {
                        unsigned long memtest_offset = rand() % (LRAM_LEN / byte_cnt);
                        if(byte_cnt == 1)
                        {
                            unsigned char val = (unsigned char)rand();
                            l8[memtest_offset] = val;
                            c8[memtest_offset] = val;
                        }
                        else if(byte_cnt == 2)
                        {
                            unsigned short val = (unsigned short)rand();
                            l16[memtest_offset] = val;
                            c16[memtest_offset] = val;
                        }
                        else // if(byte_cnt == 2)
                        {
                            unsigned long val = (unsigned long)rand();
                            l32[memtest_offset] = val;
                            c32[memtest_offset] = val;
                        }
                    }

                    /* Verify whole chunk with reference data. */
                    for(i=((LRAM_LEN / sizeof(unsigned long)) - 1); i; i--)
                    {
                        if(*c32++ != *l32++)
                        {
                            err_cnt++;
                        }
                    }
                    ite_total = LRAM_LEN / sizeof(unsigned long);
                }


                /* Update total error count.
                 * (And, Q&D avoid of error count overflow)
                 */
                if((total_err_cnt & 0x80000000) == 0)
                {
                    total_err_cnt += err_cnt;
                }

                int col = 7 + 5*pass_id;
                if(err_cnt == 0)
                {
                    conio_printf(col, row, COLOR_LIME, " OK");
                }
                else
                {
                    /* Display error ratio rather than error count.
                     * -> It's simpler to verify if error occured
                     *    at 10% rate or on every access.
                     */
                    unsigned long rate = 100 * err_cnt;
                    rate = rate / ite_total;
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

            } /* Test pass loop. */
            row++;
        } /* Chunk loop. */
    } /* Test count loop. */

    row++;
    if(total_err_cnt == 0)
    {
        conio_printf(2, row++, COLOR_LIME, "No error detected !");
    }
    else
    {
        conio_printf(2, row++, COLOR_ERROR, "Error count : 0x%08X", total_err_cnt);
    }
    wait_for_start(NULL/*msg*/);

    return 1;
}

