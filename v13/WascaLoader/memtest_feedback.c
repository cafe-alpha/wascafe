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

#include "memtest_feedback.h"
#include "shared.h"

#include "../menu.h"


/* Display error log on screen.
 * Purpose : easily trace memory errors.
 */
int _feedback_error_id = 0;
int _feedback_error_scroll = 0;
void memtest_error_feedback_func(void* addr, unsigned long write_data, unsigned long read_data, int width, unsigned short code, unsigned long err_cnt)
{
    const int start_row = 13;
    const int row_count = 12;

    /* Display only first errors when scrolling is disabled. */
    if((!_feedback_error_scroll)
    && ((_feedback_error_id+1) > row_count))
    {
        return;
    }

    if((_feedback_error_id+1) > row_count)
    {
        int i;
        for(i=1; i<row_count; i++)
        {
            conio_line_scroll_up(start_row + i);
        }
    }

    int row = start_row + (_feedback_error_id >= row_count ? row_count-1 : _feedback_error_id);
    conio_printf(2, row, COLOR_YELLOW, "A:         WR:         RD:        ");

    conio_printf(4, row, COLOR_WHITE, "%08X", addr);

    if(width == 8)
    {
        conio_printf(16+6, row, COLOR_WHITE, "%02X", write_data);
        conio_printf(28+6, row, COLOR_WHITE, "%02X", read_data);
    }
    else if(width == 16)
    {
        conio_printf(16+4, row, COLOR_WHITE, "%04X", write_data);
        conio_printf(28+4, row, COLOR_WHITE, "%04X", read_data);
    }
    else if(width == 32)
    {
        conio_printf(16+0, row, COLOR_WHITE, "%08X", write_data);
        conio_printf(28+0, row, COLOR_WHITE, "%08X", read_data);
    }

    _feedback_error_id++;
}

/* Write something to CS1 when memory test error happened, and then display
 * error log to screen.
 *
 * The purpose of using a macro is to trigger as fast as possible so
 * that SignalTap can capture something relevant.
 */
#define MEMTEST_ERROR_FEEDBACK(_ADDR_, _WRITE_DATA_, _READ_DATA_, _WIDTH_, _CODE_, _ERR_CNT_) ((unsigned short*)0x24000000)[0] = (_CODE_) | ((_ERR_CNT_) & 0x00000FFF); memtest_error_feedback_func(_ADDR_, _WRITE_DATA_, _READ_DATA_, _WIDTH_, _CODE_, _ERR_CNT_)



int _feedback_test_menuid = 1;

int memtest_feedback(void)
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
     *  | CS0  [AA] [55] [RN] [08] [16] [32]
     *  | +01   OK   NG   NG  999  999  999
     * - Error count is maxed to 999
     */
#define CHECK_CHUNKSIZE  (1*1024*1024)
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
        /* Consider capacity of unknown expansion memory cart as 4MB. */
        my_CartRAMsize = 4*1024*1024;
    }

    int test_cnt = 0;

    while(1)
    {
        menu_reset_settings(mset);
        menu_set_callback_vblank(mset, background_vblank_clbk);

        menu_set_help(mset, "A/C: Confirm, B: Cancel");
        menu_set_title(mset, "Feedback Memory Test");
        menu_set_pos(mset, -1/*x0*/, 4*CONIO_CHAR_H/*y0*/, 21/*w*/, 0/*h*/, 1/*cols*/);
        menu_set_erase_on_exit(mset, 1);
        menu_set_features
        (
            mset, 
            MENU_FEATURES_TEXTCENTER
            | (MENU_FEATURES_CONDENSED_TITLE | MENU_FEATURES_CONDENSED_TEXT)
        );

        menu_reset_items(mitems);
        menu_add_item(mitems, "Test once only" , 1   /*tag*/);
        menu_add_item(mitems, "Test 5 times"   , 5   /*tag*/);
        menu_add_item(mitems, "Test 10 times"  , 10  /*tag*/);
        menu_add_item(mitems, "Test 100 times" , 100 /*tag*/);
        menu_add_item(mitems, "Test 500 times" , 500 /*tag*/);
        menu_add_item(mitems, "Test 5000 times", 5000/*tag*/);
#define TEST_COUNT_INFINITE 9000
        menu_add_item(mitems, "Infinite Test" , TEST_COUNT_INFINITE/*tag*/);

        clear_screen();
        _feedback_test_menuid = menu_start(mitems, mset, _feedback_test_menuid/*selected_tag*/);

        if(mset->exit_code == MENU_EXIT_CANCEL)
        {
            return 0;
        }

        test_cnt = _feedback_test_menuid;
        break;
    }

    /* Reset display of access error. */
    _feedback_error_id = 0;

    /* Scroll error display only during infinite test.
     *  - Fixed count test = it's important to remember first errors.
     *  - Infnite test = retro screen saver !
     */
    _feedback_error_scroll = (test_cnt == TEST_COUNT_INFINITE ? 1 : 0);

    unsigned long total_err_cnt = 0;
    int test_id;
    for(test_id=0; (test_cnt == TEST_COUNT_INFINITE) || (test_id < test_cnt); test_id++)
    {
        conio_printf(2, 5, COLOR_YELLOW, "CS0  [AA] [55] [RN] [08] [16] [32]");

        row = 6;
        int chunk_id;
        for(chunk_id=4; chunk_id<8; chunk_id++)
        {
            conio_printf(2, row, COLOR_WHITE, empty_line);
            conio_printf(2, row, COLOR_WHITE, "+%02u", chunk_id);

            int pass_id;
            for(pass_id=0; pass_id<6; pass_id++)
            {
                char count_str[16];
                if(test_cnt == TEST_COUNT_INFINITE)
                {
                    sprintf(count_str, "%08X", test_id+1);
                }
                else
                {
                    sprintf(count_str, "%04u/%04u", test_id+1, test_cnt);
                }
                conio_printf(2, 4, COLOR_LIME , "ID:%02X (%2u MB) T:%s E:%08X", cart_id, my_CartRAMsize>>20, count_str, total_err_cnt);

                /* Emergency exit with B button. */
                pad = pad_read();
                if(pad->new[0] & (PAD_B))
                {
                    wait_for_start("Test Ended. Start to exit.");
                    return 1;
                }

                unsigned long err_cnt    = 0;
                unsigned long ite_total  = 0;
                volatile unsigned char* chunk_ptr = (volatile unsigned char*)(0x22000000 + (chunk_id * CHECK_CHUNKSIZE));

                if((pass_id == 0) || (pass_id == 1))
                { /* AA/55 write test (32 bits width, continous). */
                    unsigned short feedback_code = ((pass_id == 0) ? 0xA000 : 0xB000);
                    unsigned long fill_val = ((pass_id == 0) ? 0xAA5555AA : 0x55AAAA55);
                    unsigned long* ptr;

                    ptr = (unsigned long*)chunk_ptr;
                    for(i=((CHECK_CHUNKSIZE / sizeof(unsigned long)) - 1); i; i--)
                    {
                        /* Write. */
                        *ptr = fill_val;

                        /* Verify. */
                        unsigned long read_val = *ptr;
                        if(read_val != fill_val)
                        {
                            MEMTEST_ERROR_FEEDBACK((void*)ptr, fill_val, read_val, 32, feedback_code, total_err_cnt + err_cnt);
                            err_cnt++;
                        }

                        ptr++;
                    }

                    ite_total = CHECK_CHUNKSIZE / sizeof(unsigned long);
                }
                else if(pass_id == 2)
                { /* Write random value, then read back and compare. */
#define REF_LEN ( 512*1024)
                    for(i=0; i<REF_LEN; i++)
                    {
                        unsigned char r = (unsigned char)rand();

                        /* Write. */
                        chunk_ptr[i] = r;

                        /* Verify. */
                        unsigned char read_val = chunk_ptr[i];
                        if(read_val != r)
                        {
                            MEMTEST_ERROR_FEEDBACK((void*)(chunk_ptr + i), r, read_val, 8, 0xC000, total_err_cnt + err_cnt);
                            err_cnt++;
                        }
                    }

                    ite_total = REF_LEN;
                }
                else if((pass_id == 3) || (pass_id == 4) || (pass_id == 5))
                {
                    unsigned short feedback_code = (0xD + (pass_id-3)) << 12;
                    int byte_cnt = 1 << (pass_id-3);
                    volatile unsigned char * c8  = (volatile unsigned char *)chunk_ptr;
                    volatile unsigned short* c16 = (volatile unsigned short*)chunk_ptr;
                    volatile unsigned long * c32 = (volatile unsigned long *)chunk_ptr;

                    /* Fill both SDRAM and reference with random data at random addresses. */
#define MEMTEST_OFFSETS_CNT (64*1024)
                    for(i=0; i<MEMTEST_OFFSETS_CNT; i++)
                    {
                        unsigned long memtest_offset = rand() % (LRAM_LEN / byte_cnt);
                        if(byte_cnt == 1)
                        {
                            unsigned char val = (unsigned char)rand();
                            c8[memtest_offset] = val;

                            unsigned char read_val = c8[memtest_offset];
                            if(read_val != val)
                            {
                                MEMTEST_ERROR_FEEDBACK((void*)(c8 + memtest_offset), val, read_val, 8, feedback_code, total_err_cnt + err_cnt);
                                err_cnt++;
                            }
                        }
                        else if(byte_cnt == 2)
                        {
                            unsigned short val = (unsigned short)rand();
                            c16[memtest_offset] = val;

                            unsigned short read_val = c16[memtest_offset];
                            if(read_val != val)
                            {
                                MEMTEST_ERROR_FEEDBACK((void*)(c16 + memtest_offset), val, read_val, 16, feedback_code, total_err_cnt + err_cnt);
                                err_cnt++;
                            }
                        }
                        else // if(byte_cnt == 2)
                        {
                            unsigned long val = (unsigned long)rand();
                            c32[memtest_offset] = val;

                            unsigned long read_val = c32[memtest_offset];
                            if(read_val != val)
                            {
                                MEMTEST_ERROR_FEEDBACK((void*)(c32 + memtest_offset), val, read_val, 32, feedback_code, total_err_cnt + err_cnt);
                                err_cnt++;
                            }
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

