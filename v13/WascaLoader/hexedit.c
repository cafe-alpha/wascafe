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

#include "wasca.h"

#include "../conio.h"
#include "../menu.h"
#include "../vdp.h"

#include "cart_bus_config.h"

#include "shared.h"

/* Hexview header start row. */
#define HEADER_DISPLAY_Y0 (4)

/* Hexviewer header/footer heights. */
#define HEXHEADER_HEIGHT (2)
#define HEXFOOTER_HEIGHT (2)

/* Display 16 rows, 8 bytes per rows. */
#define HEXDISPLAY_WIDTH  ( 8)
//#define HEXDISPLAY_HEIGHT (16)
#define HEXDISPLAY_HEIGHT ( 8)
#define HEXPAGE_SIZE (HEXDISPLAY_WIDTH * HEXDISPLAY_HEIGHT)

/* Menu start row (pixel unit). */
#define MENU_DISPLAY_Y0 (MENU_TOP_ROW)

void hexedit_clear(void)
{
    int i;
    /* Clear hexedit display. */
    for(i=0; i<(HEXHEADER_HEIGHT+HEXDISPLAY_HEIGHT+HEXFOOTER_HEIGHT); i++)
    {
        conio_printf(2, HEADER_DISPLAY_Y0+i, COLOR_TRANSPARENT, empty_line);
        //conio_printf(2, HEADER_DISPLAY_Y0+i, COLOR_WHITE, "*%2d*%s", i, empty_line);
    }
}



/**
 * hexedit_page_memtest_reset
 *
 * Reset page memtest internals.
**/
int _page_memtest_errcnt[3] = {0, 0, 0};
int _page_memtest_errfound = 0;
unsigned long _page_memtest_cnt = 0;
void hexedit_page_memtest_reset(void)
{
    int row = 20;
    int dw_id;

    /* Clear internals and display of previous results. */
    memset(_page_memtest_errcnt, 0, sizeof(_page_memtest_errcnt));
    _page_memtest_errfound = 0;
    _page_memtest_cnt = 0;
    for(dw_id=0; dw_id<4; dw_id++)
    {
        conio_printf(2, row++, COLOR_TRANSPARENT, empty_line);
    }
}


/**
 * hexedit_page_memtest_do
 *
 * Test memory currently displaying, and show test results
 * under Hexedit area.
**/
int _hexedit_page_memtest_enable = 0;
unsigned char _memtest_ref_buff[HEXPAGE_SIZE];  // Test reference data
unsigned char _memtest_work_buff[HEXPAGE_SIZE]; // Read back data
int _memtest_access_done[HEXPAGE_SIZE]; // Indicate if accessed or not
#define MEMTEST_RANDOM_ACC_CNT 20
void hexedit_page_memtest_do(unsigned char* hex_buff, unsigned char* hex_ptr)
{
    int row = 20;
    int i;
    int dw_id, tp_id;
    char tmp[16];

    /* Memory test contents :
     *  - Test memory for each 1/2/4 bytes size.
     *  - Test 00/FF/A5/5A values write and read back
     *  - Test incremental values write and read back
     *  - Test write and read back random values not in consecutive address
     *  - Test write and read back hexedit buffer
     */
    int data_width = 1;
    for(dw_id=0; dw_id<3; dw_id++)
    {
        unsigned char* ref_buff = _memtest_ref_buff;
        int* acc_done = _memtest_access_done;
        for(tp_id=0; /*tp_id<100*/; tp_id++)
        {
            /* Fill test reference buffer. */
            int random_access = 0;
            int incremental_access = 0;
            if(tp_id == 0)
            {
                memset(ref_buff, 0x00, HEXPAGE_SIZE);
            }
            else if(tp_id == 1)
            {
                memset(ref_buff, 0xFF, HEXPAGE_SIZE);
            }
            else if(tp_id == 2)
            {
                /* Write A5 and 5A merged in a single test pattern. */
                for(i=0; i<HEXPAGE_SIZE; i+=8)
                {
                    ref_buff[i+0] = 0xA5;
                    ref_buff[i+1] = 0x5A;
                    ref_buff[i+2] = 0xA5;
                    ref_buff[i+3] = 0x5A;
                    ref_buff[i+4] = 0xA5;
                    ref_buff[i+5] = 0x5A;
                    ref_buff[i+6] = 0x5A;
                    ref_buff[i+7] = 0xA5;
                }
            }
            else if(tp_id == 3)
            {
                incremental_access = 1;
                for(i=0; i<HEXPAGE_SIZE; i++)
                {
                    ref_buff[i] = (unsigned char)i;
                }
            }
            else if(tp_id == 4)
            {
                random_access = 1;
                for(i=0; i<HEXPAGE_SIZE; i++)
                {
                    ref_buff[i] = (unsigned char)rand();
                }
            }
            else if(tp_id == 5)
            {
                memcpy(ref_buff, hex_buff, HEXPAGE_SIZE);
            }
            else
            {
                /* Finished last test pattern. */
                break;
            }

            /* Initialize offset access table. */
            if(random_access)
            {
                memset(acc_done, 0, (HEXPAGE_SIZE/data_width)*sizeof(int));
            }

            /* Write test data, then read back and verify. */
            if(data_width == 1)
            {
                unsigned char* src_u8 = ref_buff;
                unsigned char* dst_u8 = hex_ptr;
                unsigned char* wrk_u8 = _memtest_work_buff;
                if(random_access)
                {
                    /* Do several random writes. */
                    for(i=0; i<MEMTEST_RANDOM_ACC_CNT; i++)
                    {
                        unsigned long offset = rand() % (HEXPAGE_SIZE/data_width);
                        dst_u8[offset] = src_u8[offset];
                        acc_done[offset] = 1;
                    }
                    /* Write remaining address. */
                    for(i=0; i<(HEXPAGE_SIZE/data_width); i++)
                    {
                        if(!acc_done[i])
                        {
                            dst_u8[i] = src_u8[i];
                        }
                    }

                    /* Do several random reads. */
                    memset(acc_done, 0, (HEXPAGE_SIZE/data_width)*sizeof(int));
                    for(i=0; i<MEMTEST_RANDOM_ACC_CNT; i++)
                    {
                        unsigned long offset = rand() % (HEXPAGE_SIZE/data_width);
                        wrk_u8[offset] = dst_u8[offset];
                        acc_done[offset] = 1;
                    }
                    /* Read remaining address. */
                    for(i=0; i<(HEXPAGE_SIZE/data_width); i++)
                    {
                        if(!acc_done[i])
                        {
                            wrk_u8[i] = dst_u8[i];
                        }
                    }
                }
                else
                {
                    /* Write. */
                    for(i=HEXPAGE_SIZE/data_width; i; i--)
                    {
                        *dst_u8++ = *src_u8++;
                    }

                    /* Read back. */
                    dst_u8 = hex_ptr;
                    for(i=HEXPAGE_SIZE/data_width; i; i--)
                    {
                        *wrk_u8++ = *dst_u8++;
                    }
                }
            }
            else if(data_width == 2)
            {
                unsigned short* src_u16 = (unsigned short*)ref_buff;
                unsigned short* dst_u16 = (unsigned short*)hex_ptr;
                unsigned short* wrk_u16 = (unsigned short*)_memtest_work_buff;
                if(random_access)
                {
                    /* Do several random writes. */
                    for(i=0; i<MEMTEST_RANDOM_ACC_CNT; i++)
                    {
                        unsigned long offset = rand() % (HEXPAGE_SIZE/data_width);
                        dst_u16[offset] = src_u16[offset];
                        acc_done[offset] = 1;
                    }
                    /* Write remaining address. */
                    for(i=0; i<(HEXPAGE_SIZE/data_width); i++)
                    {
                        if(!acc_done[i])
                        {
                            dst_u16[i] = src_u16[i];
                        }
                    }

                    /* Do several random reads. */
                    memset(acc_done, 0, (HEXPAGE_SIZE/data_width)*sizeof(int));
                    for(i=0; i<MEMTEST_RANDOM_ACC_CNT; i++)
                    {
                        unsigned long offset = rand() % (HEXPAGE_SIZE/data_width);
                        wrk_u16[offset] = dst_u16[offset];
                        acc_done[offset] = 1;
                    }
                    /* Read remaining address. */
                    for(i=0; i<(HEXPAGE_SIZE/data_width); i++)
                    {
                        if(!acc_done[i])
                        {
                            wrk_u16[i] = dst_u16[i];
                        }
                    }
                }
                else
                {
                    /* Write. */
                    for(i=HEXPAGE_SIZE/data_width; i; i--)
                    {
                        *dst_u16++ = *src_u16++;
                    }

                    /* Read back. */
                    dst_u16 = (unsigned short*)hex_ptr;
                    for(i=HEXPAGE_SIZE/data_width; i; i--)
                    {
                        *wrk_u16++ = *dst_u16++;
                    }
                }
            }
            else
            {
                unsigned long* src_u32 = (unsigned long*)ref_buff;
                unsigned long* dst_u32 = (unsigned long*)hex_ptr;
                unsigned long* wrk_u32 = (unsigned long*)_memtest_work_buff;
                if(random_access)
                {
                    /* Do several random writes. */
                    for(i=0; i<MEMTEST_RANDOM_ACC_CNT; i++)
                    {
                        unsigned long offset = rand() % (HEXPAGE_SIZE/data_width);
                        dst_u32[offset] = src_u32[offset];
                        acc_done[offset] = 1;
                    }
                    /* Write remaining address. */
                    for(i=0; i<(HEXPAGE_SIZE/data_width); i++)
                    {
                        if(!acc_done[i])
                        {
                            dst_u32[i] = src_u32[i];
                        }
                    }

                    /* Do several random reads. */
                    memset(acc_done, 0, (HEXPAGE_SIZE/data_width)*sizeof(int));
                    for(i=0; i<MEMTEST_RANDOM_ACC_CNT; i++)
                    {
                        unsigned long offset = rand() % (HEXPAGE_SIZE/data_width);
                        wrk_u32[offset] = dst_u32[offset];
                        acc_done[offset] = 1;
                    }
                    /* Read remaining address. */
                    for(i=0; i<(HEXPAGE_SIZE/data_width); i++)
                    {
                        if(!acc_done[i])
                        {
                            wrk_u32[i] = dst_u32[i];
                        }
                    }
                }
                else
                {
                    /* Write. */
                    for(i=HEXPAGE_SIZE/data_width; i; i--)
                    {
                        *dst_u32++ = *src_u32++;
                    }

                    /* Read back. */
                    dst_u32 = (unsigned long*)hex_ptr;
                    for(i=HEXPAGE_SIZE/data_width; i; i--)
                    {
                        *wrk_u32++ = *dst_u32++;
                    }
                }
            } /* Data width check. */

            /* Verify read back data with reference buffer. */
            int testpat_error = 0;
            if(memcmp(ref_buff, _memtest_work_buff, HEXPAGE_SIZE) != 0)
            {
                testpat_error = 1;
                _page_memtest_errfound = 1;
                _page_memtest_errcnt[dw_id]++;
            }

            /* Display results header. */
            if(dw_id == 0)
            {
                if(tp_id == 0)
                {
                    conio_printf( 8, row, COLOR_WHITE, "CNT");

                    /* Show count of tests done so far. */
                    conio_printf(30, row+0, COLOR_YELLOW, "PASS CNT");
                    conio_printf(30, row+1, COLOR_WHITE , "%08X", _page_memtest_cnt);
                }
                conio_printf(12+(3*tp_id)+0, row, COLOR_WHITE, (incremental_access ? "++" : "%02X"), ref_buff[0]);
            }

            /* Display status (OK/fail) for current data width. */
            conio_printf(12+(3*tp_id)+0, row+1, (!testpat_error ? COLOR_LIME : COLOR_ERROR), (!testpat_error ? "OK" : "NG"));

        } /* Test pattern loop. */

        /* Display total error count for current data width. */
        conio_printf( 2, row+1, COLOR_WHITE, "W:%2d", data_width*8);
        sprintf(tmp, "%3d", _page_memtest_errcnt[dw_id] > 999 ? 999 : _page_memtest_errcnt[dw_id]);
        conio_printf( 8, row+1, (_page_memtest_errcnt[dw_id] == 0 ? COLOR_LIME : COLOR_ERROR), "%s", tmp);

        data_width = data_width*2;
        row++;
    } /* Data width loop. */

    /* There's no interest to bump test count when error happeneded, 
     * so that it is incremented until a memory test error is detected.
     */
    if(!_page_memtest_errfound)
    {
        _page_memtest_cnt++;
    }
}



/**
 * hexedit_pat_data_fill
 *
 * Fill page with data at specified pattern.
**/
const char* _lorem_sample = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ac tincidunt odio. Duis et est eget justo egestas pellentesque ut nec nisi. Sed congue turpis ac mauris sodales semper. Duis posuere a dolor eu ultrices. Sed placerat congue mi, in aliquam dolor auctor ut. Aliquam vitae nibh tortor. Praesent vulputate laoreet mi ac sodales. Pellentesque eleifend convallis nisl, vitae mattis metus volutpat in. Nulla nisi magna, scelerisque non convallis ac, sodales at ipsum.";
const char* _cartrom_header = "SEGA SEGASATURN (C)CAFE-ALPHA   PSKAI_L   V6.28920190409        JTUBKAEL        JA              PSEUDOSATURN_KAI(C) 2015-2019   By cafe-alpha   ppcenter.free.fr                Initial Version by CyberWarriorX                ";
int _pat_data_fill_menu_selected = 0;
void hexedit_pat_data_fill(unsigned char* ptr, int data_width)
{
    int i, n;

    menu_settings_t* mset   = &_menu_settings;
    menu_items_t   * mitems = &_menu_items;
    int menu_selected;

    menu_reset_settings(mset);
    menu_reset_items(mitems);
    menu_set_title(mset, "Data Fill");

    menu_add_item(mitems, "Lorem (~400 chars)    ", 0/*tag*/);
    menu_add_item(mitems, "Cart ROM Header       ", 1/*tag*/);
    menu_add_item(mitems, "Increment (256 bytes) ", 2/*tag*/);


    menu_set_callback_vblank(mset, menu_vblank_clbk);
    menu_set_pos(mset, -1/*x0*/, MENU_TOP_ROW/*y0*/, 30/*w*/, 0/*h*/, 1/*cols*/);
    menu_set_erase_on_exit(mset, 1);

    menu_selected = _pat_data_fill_menu_selected;

    menu_selected = menu_start(mitems, mset, menu_selected/*selected_tag*/);
    if((mset->exit_code == MENU_EXIT_CANCEL)
    || (MENU_TAG_INVALID(menu_selected)    ))
    {
        return;
    }

    _pat_data_fill_menu_selected = menu_selected;
    switch(menu_selected)
    {
    default:
    case(0):
        n = strlen(_lorem_sample);
        for(i=0; i<n; i++)
        {
            ptr[i] = _lorem_sample[i];
        }
        break;

    case(1):
        n = strlen(_cartrom_header);
        for(i=0; i<n; i++)
        {
            ptr[i] = _cartrom_header[i];
        }
        break;

    case(2):
        for(i=0; i<256; i++)
        {
            ptr[i] = (unsigned char)i;
        }
        break;
    }
}


/**
 * hexedit_show_extmenu
 *
 * Return 1 when input successed, 0 when cancelled.
**/
int _extmenu_menu_selected = 0;
#define HEXEDIT_EXT_NO_ACTION 0
#define HEXEDIT_EXT_ABUS_CFG  1
#define HEXEDIT_EXT_BG_READ   2
#define HEXEDIT_EXT_BG_WRRD   3
#define HEXEDIT_EXT_PAT_FILL  4
int hexedit_show_extmenu(unsigned char* ptr, int data_width)
{
    int ret = HEXEDIT_EXT_NO_ACTION;
    menu_settings_t* mset   = &_menu_settings;
    menu_items_t   * mitems = &_menu_items;
    int menu_selected;

    menu_reset_settings(mset);
    menu_reset_items(mitems);
    menu_set_title(mset, "Hexedit Extended Menu");

    /* Memory test on current HexEdit page. */
    if(_hexedit_page_memtest_enable)
    {
        menu_add_item(mitems, "Page MemTest Disable  ", 0/*tag*/);
    }
    else
    {
        menu_add_item(mitems, "Page MemTest Enable   ", 0/*tag*/);
    }

    /* Enable/disable wasca status display. */
    if(_wasca_status_display)
    {
        menu_add_item(mitems, "Wasca Status Disable  ", 1/*tag*/);
    }
    else
    {
        menu_add_item(mitems, "Wasca Status Enable   ", 1/*tag*/);
    }

    menu_add_item(mitems, "A-Bus config. menu    ", 2/*tag*/);
    menu_add_item(mitems, "Bgnd data read        ", 3/*tag*/);
    menu_add_item(mitems, "Bgnd data write&read  ", 4/*tag*/);
    menu_add_item(mitems, "Pattern Data Fill     ", 5/*tag*/);


    menu_set_callback_vblank(mset, menu_vblank_clbk);
    menu_set_pos(mset, -1/*x0*/, MENU_TOP_ROW/*y0*/, 30/*w*/, 7/*h*/, 1/*cols*/);
    menu_set_erase_on_exit(mset, 1);

    menu_selected = _extmenu_menu_selected;

    menu_selected = menu_start(mitems, mset, menu_selected/*selected_tag*/);
    if((mset->exit_code == MENU_EXIT_CANCEL)
    || (MENU_TAG_INVALID(menu_selected)    ))
    {
        return ret;
    }

    _extmenu_menu_selected = menu_selected;
    switch(menu_selected)
    {
    default:
    case(0): // Hexedit current page memTest enable/disable
        if(_hexedit_page_memtest_enable)
        {
            _hexedit_page_memtest_enable = 0;
        }
        else
        {
            _hexedit_page_memtest_enable = 1;

            /* Don't forget to disable wasca status display, 
             * because memtest results are displayed on same area.
             */
            _wasca_status_display = 0;
        }

        /* Clear and reset previous memtest or wasca status results. */
        hexedit_page_memtest_reset();

        break;

    case(1): // Wasca status display enable/disable
        if(_wasca_status_display)
        {
            _wasca_status_display = 0;
        }
        else
        {
            _wasca_status_display = 1;
            /* Don't forget to disable page memtest display, 
             * because wasca status is displayed on same area.
             */
            _hexedit_page_memtest_enable = 0;
        }

        /* Clear and reset previous memtest or wasca status results. */
        hexedit_page_memtest_reset();

        break;

    case(2): // A-Bus configuration menu
        ret = HEXEDIT_EXT_ABUS_CFG;
        break;

    case(3): // Background data read
        ret = HEXEDIT_EXT_BG_READ;
        break;

    case(4): // Background data write&read
        ret = HEXEDIT_EXT_BG_WRRD;
        break;

    case(5): // Pattern data fill
        hexedit_pat_data_fill(ptr, data_width);
        ret = HEXEDIT_EXT_PAT_FILL;
        break;
    }

    return ret;
}






/**
 * hexedit_prompt_value
 *
 * Return 1 when input successed, 0 when cancelled.
**/
#define PROMPT_TYPE_DATA    0
#define PROMPT_TYPE_ADDRESS 1

/* Stuff for preset address. */
typedef struct _preset_addr_list_t
{
    unsigned long val;
    char* desc;
} preset_addr_list_t;

#define PRESET_ADDR_CNT 7
preset_addr_list_t preset_addr_list[PRESET_ADDR_CNT] = 
{
    {0x22000000, "CS0"}, 
    {0x24000000, "CS1"}, 
    {0x06004000, "HRAM"}, 
    {0x22400000, "Cart RAM #1"}, 
    {0x22600000, "Cart RAM #2"}, 
    {0x00200000, "LRAM"}, 
    {0x23FFFFE0, "DataTest FF00A55A"}, 
};

/* Still for preset address, but internal things. */
#define PRESET_ADDR_TAG 1000
char _preset_addr_str[6/*cols*/][PRESET_ADDR_CNT][6];


/* Similar stuff, but for value preset. */
#define PRESET_VAL_CNT 6
unsigned long preset_val_list[PRESET_VAL_CNT] = 
{
    0x00000000, 0xFFFFFFFF, 
    0xA5A5A5A5, 0x5A5A5A5A, 
    0x76543210, 0x12345678, 
};
#define PRESET_VAL_TAG 2000
char _preset_val_str[3/*cols*/][PRESET_VAL_CNT][6];


/* Restore previously selected item in prompt menu. */
int _prompt_menu_selected = 0;

int hexedit_prompt_value(unsigned long* value, int value_width, int prompt_type)
{
    menu_settings_t* mset   = &_menu_settings;
    menu_items_t   * mitems = &_menu_items;
    int menu_selected = _prompt_menu_selected;
    unsigned char adr_pos = 0;
    unsigned long v = *value;
    char tmp[64];
    int i, j;

    /* Prepare the preset value/address strings buffer. */
    if(prompt_type == PROMPT_TYPE_DATA)
    {
        for(i=0; i<PRESET_VAL_CNT; i++)
        {
            /* Show address and its description.
             * Don't forget to pad that string so that menu
             * can display unused items.
             */
            sprintf(tmp, " 0x%08X", (unsigned int)(preset_val_list[i]));
            for(j=strlen(tmp); j<((sizeof(tmp)/sizeof(char))-1); j++)
            {
                tmp[j] = ' ';
            }
            tmp[(sizeof(tmp)/sizeof(char))-1] = '\0';

            for(j=0; j<3/*cols*/; j++)
            {
                memcpy(_preset_val_str[j][i], tmp+j*5, 5*sizeof(char));
                _preset_val_str[j][i][5] = '\0';
            }
        }
    }
    else if(prompt_type == PROMPT_TYPE_ADDRESS)
    {
        for(i=0; i<PRESET_ADDR_CNT; i++)
        {
            /* Show address and its description.
             * Don't forget to pad that string so that menu
             * can display unused items.
             */
            sprintf(tmp, " 0x%08X:%s", 
                (unsigned int)(preset_addr_list[i].val), 
                preset_addr_list[i].desc);
            for(j=strlen(tmp); j<((sizeof(tmp)/sizeof(char))-1); j++)
            {
                tmp[j] = ' ';
            }
            tmp[(sizeof(tmp)/sizeof(char))-1] = '\0';

            for(j=0; j<6/*cols*/; j++)
            {
                memcpy(_preset_addr_str[j][i], tmp+j*5, 5*sizeof(char));
                _preset_addr_str[j][i][5] = '\0';
            }
        }
    }

    /* Sanitize previously selected item : avoid selecting
     * preset address when editing non-address things.
     */
    if(prompt_type == PROMPT_TYPE_DATA)
    {
        if((menu_selected >= PRESET_ADDR_TAG)
        && (menu_selected < (PRESET_ADDR_TAG+PRESET_ADDR_CNT)))
        {
            menu_selected = 0;
        }
    }
    if(prompt_type == PROMPT_TYPE_ADDRESS)
    {
        if((menu_selected >= PRESET_VAL_TAG)
        && (menu_selected < (PRESET_VAL_TAG+PRESET_VAL_CNT)))
        {
            menu_selected = 0;
        }
    }

    int value_row = 11;
    char* msg = "";
    if(prompt_type == PROMPT_TYPE_DATA)
    {
        msg = "Edit new value";
        value_row += (((PRESET_VAL_CNT/2) * MENU_CHAR_H) / CONIO_CHAR_H) + 1;
    }
    if(prompt_type == PROMPT_TYPE_ADDRESS)
    {
        msg = "Edit start address";
        value_row += ((PRESET_ADDR_CNT * MENU_CHAR_H) / CONIO_CHAR_H) + 1;
    }
    conio_printf(2, value_row, COLOR_TRANSPARENT, empty_line);

    while(1)
    {
        menu_reset_settings(mset);
        menu_reset_items(mitems);
        menu_set_title(mset, msg);

        /* Edit start address. */
        menu_add_item(mitems, "  0  ",   0/*tag*/);
        menu_add_item(mitems, "  1  ",   1/*tag*/);
        menu_add_item(mitems, "  2  ",   2/*tag*/);
        menu_add_item(mitems, "  3  ",   3/*tag*/);
        menu_add_item(mitems, "  4  ",   4/*tag*/);
        menu_add_item(mitems, " Go  ", 'G'/*tag*/);

        menu_add_item(mitems, "  5  ",   5/*tag*/);
        menu_add_item(mitems, "  6  ",   6/*tag*/);
        menu_add_item(mitems, "  7  ",   7/*tag*/);
        menu_add_item(mitems, "  8  ",   8/*tag*/);
        menu_add_item(mitems, "  9  ",   9/*tag*/);
        menu_add_item(mitems, "Exit ", 'X'/*tag*/);

        menu_add_item(mitems, "  A  ",  10/*tag*/);
        menu_add_item(mitems, "  B  ",  11/*tag*/);
        menu_add_item(mitems, "  C  ",  12/*tag*/);
        menu_add_item(mitems, "  D  ",  13/*tag*/);
        menu_add_item(mitems, "  E  ",  14/*tag*/);
        menu_add_item(mitems, "  F  ",  15/*tag*/);

        /* Append preset value or address. */
        if(prompt_type == PROMPT_TYPE_DATA)
        {
            for(i=0; i<PRESET_VAL_CNT; i++)
            {
                for(j=0; j<3; j++)
                {
                    menu_add_item(mitems, _preset_val_str[j][i], PRESET_VAL_TAG+i/*tag*/);
                }
            }
        }
        else if(prompt_type == PROMPT_TYPE_ADDRESS)
        {
            for(i=0; i<PRESET_ADDR_CNT; i++)
            {
                for(j=0; j<6; j++)
                {
                    menu_add_item(mitems, _preset_addr_str[j][i], PRESET_ADDR_TAG+i/*tag*/);
                }
            }
        }


        menu_set_callback_vblank(mset, menu_vblank_clbk);
        menu_set_pos(mset, -1/*x0*/, MENU_DISPLAY_Y0/*y0*/, 30/*w*/, 
            4+(prompt_type == PROMPT_TYPE_DATA ? PRESET_VAL_CNT/2 : PRESET_ADDR_CNT)/*h*/, 
            6/*cols*/);
        menu_set_erase_on_exit(mset, 1);
        menu_set_features(mset, MENU_FEATURES_CHANGEPAGE_SHOULDER);

#if 0
        /* Automatically move to digit currently editing.
         * (Not super convenient ...)
         */
        menu_selected = ((v >> ((value_width*2-1)*4 - adr_pos*4)) & 0xF);
#endif

        /* Show current value and highlight
         * digit currently modifying.
         */
        if(value_width == 1)
        {
            sprintf(tmp, "%02X", (unsigned char)v);
        }
        else if(value_width == 2)
        {
            sprintf(tmp, "%04X", (unsigned short)v);
        }
        else
        {
            sprintf(tmp, "%08X", (int)v);
        }

        conio_printf( 3, value_row, COLOR_LIME, " Value=0x", (unsigned char)v, tmp);
        for(i=0; i<(value_width*2); i++)
        {
            conio_printf(12+i, value_row, (i == adr_pos ? mset->color_highlight : COLOR_LIME), "%c", tmp[i]);
        }
        for(i=(12+value_width*2); i<36; i++)
        {
            conio_printf(i, value_row, COLOR_TRANSPARENT, " ");
        }

        menu_selected = menu_start(mitems, mset, menu_selected/*selected_tag*/);

        if((mset->exit_code == MENU_EXIT_CANCEL)
        || (MENU_TAG_INVALID(menu_selected)    )
        || (menu_selected == 'X'/*exit*/       ))
        {
            //menu_clear_display(p);
            *value = v;
            conio_printf(2, value_row, COLOR_TRANSPARENT, empty_line);
            return 0;
        }

        if(mset->exit_code == MENU_EXIT_OK)
        {
            if(menu_selected == 'G'/*GO*/)
            {
                *value = v;
                conio_printf(2, value_row, COLOR_TRANSPARENT, empty_line);

                return 1;
            }
            else if((menu_selected >= PRESET_ADDR_TAG)
                  && (menu_selected < (PRESET_ADDR_TAG+PRESET_ADDR_CNT)))
            {
                /* Use preset address. */
                *value = preset_addr_list[menu_selected-PRESET_ADDR_TAG].val;
                conio_printf(2, value_row, COLOR_TRANSPARENT, empty_line);

                _prompt_menu_selected = menu_selected;
                return 1;
            }
            else if((menu_selected >= PRESET_VAL_TAG)
                  && (menu_selected < (PRESET_VAL_TAG+PRESET_VAL_CNT)))
            {
                /* Use preset value. */
                *value = preset_val_list[menu_selected-PRESET_VAL_TAG];
                conio_printf(2, value_row, COLOR_TRANSPARENT, empty_line);

                _prompt_menu_selected = menu_selected;
                return 1;
            }
            else /* Edit value. */
            {
                v &= ~(0xF << ((value_width*2-1)*4 - adr_pos*4));
                v |= menu_selected << ((value_width*2-1)*4 - adr_pos*4);
                adr_pos = (adr_pos+1)%(2*value_width);

                _prompt_menu_selected = menu_selected;
            }
        }
        else if(mset->exit_code == MENU_EXIT_PREVPAGE_SHOULDER)
        {
            /* Select address' previous digit. */
            adr_pos = (adr_pos+(2*value_width - 1)) % (2*value_width);
        }
        else if(mset->exit_code == MENU_EXIT_NEXTPAGE_SHOULDER)
        {
            /* Select address' next digit. */
            adr_pos = (adr_pos+1) % (2*value_width);
        }
    }
}

unsigned char _hexedit_buff[HEXPAGE_SIZE];
void hexedit_start(unsigned char* ptr)
{
    unsigned long i, j;
    char tmp[16];

    /* Clear and reset previous memtest results. */
    hexedit_page_memtest_reset();

    hexedit_clear();

#define REFRESH_DISPLAY    0x01
#define REFRESH_CURRVAL    0x02
#define REFRESH_CURRVAL_WR 0x04
#define REFRESH_BUFFER     0x08
    unsigned char refresh = REFRESH_BUFFER | REFRESH_DISPLAY;

    pad_t* pad;
    unsigned long disp_offset = 0;
    unsigned long curs_offset = 0;
    int data_width = 2; // 1, 2 or 4
#define ACTION_SELECT_READ  1
#define ACTION_ALL_READ     2
#define ACTION_WRITE_READ   3
    unsigned char action = 0;

    while(1)
    {
        unsigned char* buff = _hexedit_buff;

        /* Tweak for hexedit page memtest : always reload hexedit
         * buffer so that memtest can be done constantly.
         */
        if(_hexedit_page_memtest_enable)
        {
            refresh |= REFRESH_DISPLAY | REFRESH_BUFFER;
        }

        if(refresh)
        {
            int row = HEADER_DISPLAY_Y0;

            if(refresh & REFRESH_BUFFER)
            {
                /* Read whole buffer from memory. */
                memset((void*)buff, 0x42, HEXPAGE_SIZE);

                if(data_width == 1)
                {
                    unsigned char* dst_u8 = buff;
                    unsigned char* src_u8 = ptr+disp_offset;
                    for(i=HEXPAGE_SIZE/data_width; i; i--)
                    {
                        *dst_u8++ = *src_u8++;
                    }
                }
                else if(data_width == 2)
                {
                    unsigned short* dst_u16 = (unsigned short*)buff;
                    unsigned short* src_u16 = (unsigned short*)(ptr+disp_offset);
                    for(i=HEXPAGE_SIZE/data_width; i; i--)
                    {
                        *dst_u16++ = *src_u16++;
                    }
                }
                else
                {
                    unsigned long* dst_u32 = (unsigned long*)buff;
                    unsigned long* src_u32 = (unsigned long*)(ptr+disp_offset);
                    for(i=HEXPAGE_SIZE/data_width; i; i--)
                    {
                        *dst_u32++ = *src_u32++;
                    }
                }

                /* If required, do and show results of hexedit page memtest. */
                if(_hexedit_page_memtest_enable)
                {
                    hexedit_page_memtest_do(buff, ptr+disp_offset);
                }
            }

            /* Read only current value from memory. */
            if(refresh & (REFRESH_CURRVAL | REFRESH_CURRVAL_WR))
            {
                if(data_width == 1)
                {
                    unsigned char* dst_u8 = buff+curs_offset;
                    unsigned char* src_u8 = ptr+disp_offset+curs_offset;
                    if(refresh & REFRESH_CURRVAL_WR)
                    {
                        *src_u8 = *dst_u8;
                    }
                    if(refresh & REFRESH_CURRVAL)
                    {
                        *dst_u8 = *src_u8;
                    }
                }
                else if(data_width == 2)
                {
                    unsigned short* dst_u16 = (unsigned short*)(buff+curs_offset);
                    unsigned short* src_u16 = (unsigned short*)(ptr+disp_offset+curs_offset);
                    if(refresh & REFRESH_CURRVAL_WR)
                    {
                        *src_u16 = *dst_u16;
                    }
                    if(refresh & REFRESH_CURRVAL)
                    {
                        *dst_u16 = *src_u16;
                    }
                }
                else
                {
                    unsigned long* dst_u32 = (unsigned long*)(buff+curs_offset);
                    unsigned long* src_u32 = (unsigned long*)(ptr+disp_offset+curs_offset);
                    if(refresh & REFRESH_CURRVAL_WR)
                    {
                        *src_u32 = *dst_u32;
                    }
                    if(refresh & REFRESH_CURRVAL)
                    {
                        *dst_u32 = *src_u32;
                    }
                }
            }

            /* Refresh display. */
            if(refresh & REFRESH_DISPLAY)
            {
                conio_printf(2, row++, COLOR_BG_FG(COLOR_GRAY, COLOR_LIME), " Cursor Address  = 0x%08X       ", ptr + disp_offset+curs_offset);
                conio_printf(2, row++, COLOR_BG_FG(COLOR_GRAY, COLOR_LIME), " Relative Offset = 0x%08X       ", disp_offset+curs_offset);

                /* Display page data. */
                unsigned char* c = buff;
                unsigned long o = 0;

                for(j=0; j<(HEXDISPLAY_HEIGHT+1); j++)
                {
                    conio_printf( 2, row + j, COLOR_BG_FG(COLOR_GRAY, COLOR_TRANSPARENT), " ");
                    conio_printf(37, row + j, COLOR_BG_FG(COLOR_GRAY, COLOR_TRANSPARENT), " ");
                }

                for(j=0; j<HEXDISPLAY_HEIGHT; j++)
                {
                    for(i=0; i<HEXDISPLAY_WIDTH; i+=data_width)
                    {
                        unsigned short color = COLOR_WHITE;
                        if(o == curs_offset)
                        {
                            color = COLOR_BG_FG(COLOR_AQUA, COLOR_WHITE);
                        }

                        if(data_width == 1)
                        {
                            sprintf(tmp, "%02X", *c);
                            conio_font_printf(FONT_HEXA, FONT_CODEPAGE_JAPANESE, (3 + 3*i)*CONIO_CHAR_W, (row + j)*CONIO_CHAR_H, color, tmp);
                            conio_printf(3 + 3*i+2, row + j, COLOR_TRANSPARENT, " ");

                            conio_printf(29 + 1*i+0, row + j, color, "%c", char2pchar(*c));
                        }
                        else if(data_width == 2)
                        {
                            unsigned short dat_u16 = *((unsigned short*)c);
                            sprintf(tmp, "%04X", dat_u16);
                            conio_font_printf(FONT_HEXA, FONT_CODEPAGE_JAPANESE, (3 + 3*i)*CONIO_CHAR_W, (row + j)*CONIO_CHAR_H, color, tmp);
                            conio_printf(3 + 3*i+4, row + j, COLOR_TRANSPARENT, "  ");

                            conio_printf(29 + 1*i+0, row + j, color, "%c", char2pchar(*c));
                            conio_printf(29 + 1*i+1, row + j, color, "%c", char2pchar(*(c+1)));
                        }
                        else
                        {
                            unsigned int dat_u32 = *((unsigned int*)c);
                            sprintf(tmp, "%08X", dat_u32);
                            conio_font_printf(FONT_HEXA, FONT_CODEPAGE_JAPANESE, (3 + 3*i)*CONIO_CHAR_W, (row + j)*CONIO_CHAR_H, color, tmp);
                            conio_printf(3 + 3*i+8, row + j, COLOR_TRANSPARENT, "    ");

                            conio_printf(29 + 1*i+0, row + j, color, "%c", char2pchar(*c));
                            conio_printf(29 + 1*i+1, row + j, color, "%c", char2pchar(*(c+1)));
                            conio_printf(29 + 1*i+2, row + j, color, "%c", char2pchar(*(c+2)));
                            conio_printf(29 + 1*i+3, row + j, color, "%c", char2pchar(*(c+3)));
                        }

                        c+=data_width; o+=data_width;
                    }
                }
                row+=HEXDISPLAY_HEIGHT;

                switch(action)
                {
                default:
                    conio_printf(3, row++, COLOR_TRANSPARENT, "                                  ");
                    break;
                case(ACTION_SELECT_READ):
                    conio_printf(3, row++, COLOR_YELLOW, "Background cursor read (R+Y:all)  ");
                    break;
                case(ACTION_ALL_READ):
                    conio_printf(3, row++, COLOR_YELLOW, "Background all read (R+Y:stop)    ");
                    break;
                case(ACTION_WRITE_READ):
                    conio_printf(3, row++, COLOR_YELLOW, "Background write-read (R+Z:stop)  ");
                    break;
                }
                conio_printf(2, row++, COLOR_BG_FG(COLOR_GRAY, COLOR_LIME), " Press L button to show help screen ");
            }

            refresh = 0;
        }

        pad = pad_read();

        /* Move cursor. */
        if(pad->delta[0] & PAD_RIGHT)
        {
            if((curs_offset+data_width) >= HEXPAGE_SIZE)
            {
                disp_offset += HEXDISPLAY_WIDTH;
                curs_offset -= HEXDISPLAY_WIDTH; curs_offset+=data_width;
                refresh |= REFRESH_DISPLAY | REFRESH_BUFFER;
            }
            else
            {
                curs_offset+=data_width;
                refresh |= REFRESH_DISPLAY | REFRESH_CURRVAL;
            }
        }
        if(pad->delta[0] & PAD_LEFT)
        {
            if(curs_offset == 0)
            {
                if(disp_offset >= HEXDISPLAY_WIDTH)
                {
                    disp_offset -= HEXDISPLAY_WIDTH;
                    curs_offset += HEXDISPLAY_WIDTH; curs_offset-=data_width;
                    refresh |= REFRESH_DISPLAY | REFRESH_BUFFER;
                }
            }
            else
            {
                curs_offset-=data_width;
                refresh |= REFRESH_DISPLAY | REFRESH_CURRVAL;
            }
        }
        if(pad->delta[0] & PAD_DOWN)
        {
            if((curs_offset+HEXDISPLAY_WIDTH) >= HEXPAGE_SIZE)
            {
                disp_offset += HEXDISPLAY_WIDTH;
                refresh |= REFRESH_DISPLAY | REFRESH_BUFFER;
            }
            else
            {
                curs_offset += HEXDISPLAY_WIDTH;
                refresh |= REFRESH_DISPLAY | REFRESH_CURRVAL;
            }
        }
        if(pad->delta[0] & PAD_UP)
        {
            if(curs_offset < HEXDISPLAY_WIDTH)
            {
                if(disp_offset >= HEXDISPLAY_WIDTH)
                {
                    disp_offset -= HEXDISPLAY_WIDTH;
                    refresh |= REFRESH_DISPLAY | REFRESH_BUFFER;
                }
            }
            else
            {
                curs_offset -= HEXDISPLAY_WIDTH;
                refresh |= REFRESH_DISPLAY | REFRESH_CURRVAL;
            }
        }

        /* Normal actions without pushing R shoulder button. */
        if((pad->new[0] & PAD_RBUTTON) == 0)
        {
            /* Change view width ? */
            if(pad->delta[0] & PAD_X)
            {
                if(data_width == 1)
                {
                    data_width = 2;
                    /* Avoid illegal data access. */
                    curs_offset &= ~0x01;
                }
                else if(data_width == 2)
                {
                    data_width = 4;
                    /* Avoid illegal data access. */
                    curs_offset &= ~0x03;
                }
                else
                {
                    data_width = 1;
                }
                refresh |= REFRESH_DISPLAY | REFRESH_BUFFER;
            }

            /* Re-read currently selected value (oneshot). */
            if(pad->delta[0] & PAD_Y)
            {
                refresh |= REFRESH_DISPLAY | REFRESH_CURRVAL;
            }

            /* Re-write currently selected value (oneshot). */
            if(pad->delta[0] & PAD_Z)
            {
                refresh |= REFRESH_DISPLAY | REFRESH_CURRVAL_WR;
            }

            if(pad->delta[0] & PAD_A)
            {
                hexedit_clear();

                int ext_action = hexedit_show_extmenu(ptr + disp_offset+curs_offset, data_width);

                if(ext_action == HEXEDIT_EXT_ABUS_CFG)
                { // A-Bus configuration menu
                    cart_bus_config();

                    /* Clear normal (excluding status) display. */
                    clear_normal_display();

                    /* Re-display hexedit. */
                    hexedit_clear();
                }
                if(ext_action == HEXEDIT_EXT_BG_READ)
                { // Background data read
                    /* Toggle "select read" -> "all read" -> "no action". */
                    if(action == ACTION_SELECT_READ)
                    {
                        action = ACTION_ALL_READ;
                    }
                    else if(action == ACTION_ALL_READ)
                    {
                        action = 0;
                    }
                    else
                    {
                        action = ACTION_SELECT_READ;
                    }
                }
                if(ext_action == HEXEDIT_EXT_BG_WRRD)
                { // Background data write&read
                    action = (action != ACTION_WRITE_READ ? ACTION_WRITE_READ : 0);
                }
                if(ext_action == HEXEDIT_EXT_PAT_FILL)
                {
                    refresh |= REFRESH_BUFFER;
                }

                refresh |= REFRESH_DISPLAY;
            }

            if(pad->delta[0] & PAD_C)
            {
                unsigned long val = 0;
                if(data_width == 1)
                {
                    unsigned char* ptr_u8 = buff+curs_offset;
                    val = *ptr_u8;
                }
                else if(data_width == 2)
                {
                    unsigned short* ptr_u16 = (unsigned short*)(buff+curs_offset);
                    val = *ptr_u16;
                }
                else
                {
                    unsigned long* ptr_u32 = (unsigned long*)(buff+curs_offset);
                    val = *ptr_u32;
                }

                if(hexedit_prompt_value(&val, data_width, PROMPT_TYPE_DATA))
                {
                    if(data_width == 1)
                    {
                        unsigned char* ptr_u8 = ptr+disp_offset+curs_offset;
                        *ptr_u8 = (unsigned char)val;
                    }
                    else if(data_width == 2)
                    {
                        unsigned short* ptr_u16 = (unsigned short*)(ptr+disp_offset+curs_offset);
                        *ptr_u16 = (unsigned short)val;
                    }
                    else
                    {
                        unsigned long* ptr_u32 = (unsigned long*)(ptr+disp_offset+curs_offset);
                        *ptr_u32 = val;
                    }
                    refresh |= REFRESH_DISPLAY | REFRESH_CURRVAL;
                }
                else
                {
                    /* Cancel data edition. */
                    refresh |= REFRESH_DISPLAY;
                }
            }
        }

        /* Extra actions with R shoulder button plus another button. */
        if(pad->new[0] & PAD_RBUTTON)
        {
            /* R+X : show A-Bus configuration menu. */
            if(pad->delta[0] & PAD_X)
            {
                cart_bus_config();

                /* Clear normal (excluding status) display. */
                clear_normal_display();

                /* Re-display hexedit. */
                hexedit_clear();
                refresh |= REFRESH_DISPLAY;
            }

            /* Constant data read/write/etc ? */
            if(pad->delta[0] & PAD_Y)
            {
                /* Toggle "select read" -> "all read" -> "no action". */
                if(action == ACTION_SELECT_READ)
                {
                    action = ACTION_ALL_READ;
                }
                else if(action == ACTION_ALL_READ)
                {
                    action = 0;
                }
                else
                {
                    action = ACTION_SELECT_READ;
                }
                refresh |= REFRESH_DISPLAY;
            }

            if(pad->delta[0] & PAD_Z)
            {
                action = (action != ACTION_WRITE_READ ? ACTION_WRITE_READ : 0);
                refresh |= REFRESH_DISPLAY;
            }
        }

        /* Show help. */
        if(pad->delta[0] & PAD_LBUTTON)
        {
            int row = HEADER_DISPLAY_Y0;

            hexedit_clear();

            conio_printf(2, row++, COLOR_YELLOW, "       HexEditor - Help screen      ");
            conio_printf(2, row++, COLOR_WHITE , " - Arrows : move cursor             ");
            conio_printf(2, row++, COLOR_WHITE , " - A : Extended Menu                ");
            conio_printf(2, row++, COLOR_WHITE , " - B : Exit                         ");
            conio_printf(2, row++, COLOR_WHITE , " - C : Change value                 ");
            conio_printf(2, row++, COLOR_WHITE , " - X : Change data width            ");
            conio_printf(2, row++, COLOR_WHITE , " - Y : Read data at cursor          ");
            conio_printf(2, row++, COLOR_WHITE , " - Z : Write data at cursor         ");
            conio_printf(2, row++, COLOR_WHITE , " - R+X : A-Bus configuration menu   ");
            conio_printf(2, row++, COLOR_WHITE , " - R+Y : Background data read       ");
            conio_printf(2, row++, COLOR_WHITE , " - R+Z : Background data write&read ");
            conio_printf(2, row++, COLOR_WHITE , empty_line);
            conio_printf(2, row++, COLOR_WHITE , "Press Start to continue.             ");

            while (1)
            {
                pad = pad_read();
                if(pad->delta[0] & PAD_START)
                {
                    break;
                }
                menu_vblank_clbk();
                WaitForVBLANKIn();
                WaitForVBLANKOut();
            }

            /* Clear normal (excluding status) display. */
            clear_normal_display();
            refresh |= REFRESH_DISPLAY;
        }

        if(action == ACTION_SELECT_READ)
        {
            /* Refresh only currently selected value. */
            refresh |= REFRESH_DISPLAY | REFRESH_CURRVAL;
        }
        else if(action == ACTION_ALL_READ)
        {
            /* Refresh all screen. */
            refresh |= REFRESH_DISPLAY | REFRESH_BUFFER;
        }
        else if(action == ACTION_WRITE_READ)
        {
            refresh |= REFRESH_DISPLAY | REFRESH_CURRVAL_WR | REFRESH_CURRVAL;
        }

        /* Exit ? */
        if(pad->delta[0] & PAD_B)
        {
            hexedit_clear();
            break;
        }

        menu_vblank_clbk();
        WaitForVBLANKIn();
        WaitForVBLANKOut();
    }
}


void hexedit(void)
{
    /* Init global stuff. */
    unsigned long mem_address = 0x22000000; // 0x06004000

    /* First, clear display area. */
    hexedit_clear();

    while(1)
    {
        if(hexedit_prompt_value(&mem_address, sizeof(mem_address), PROMPT_TYPE_ADDRESS) == 0)
        { /* Cancel. */
            hexedit_clear();
            return;
        }

        hexedit_start((unsigned char*)mem_address);
        hexedit_clear();
    }
}

