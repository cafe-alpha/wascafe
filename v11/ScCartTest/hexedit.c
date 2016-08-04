/**
 *  Cartridge Test Utility
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/pskai/
 *
 *  See LICENSE file for details.
**/

#include <stdio.h>

#include "wasca.h"

#include "conio.h"
#include "menu.h"
#include "smpc.h"
#include "vdp.h"

#include "shared.h"

/* Hexview header start row. */
#define HEADER_DISPLAY_Y0 (4)

/* Hexviewer header/footer heights. */
#define HEXHEADER_HEIGHT (2)
#define HEXFOOTER_HEIGHT (2)

/* Display 16 rows, 8 bytes per rows. */
#define HEXDISPLAY_WIDTH  ( 8)
#define HEXDISPLAY_HEIGHT (16)
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

/* Return 1 when input successed, 0 when cancelled. */
int hexedit_prompt_value(unsigned long* value, int value_width, char* msg)
{
    menu_settings_t* mset   = &_menu_settings;
    menu_items_t   * mitems = &_menu_items;
    int menu_selected;
    unsigned char adr_pos = 0;
    unsigned long v = *value;
    char tmp[16];

    while(1)
    {
        menu_reset_settings(mset);
        menu_reset_items(mitems);
        menu_set_title(mset, msg);

        /* Edit start address. */
        menu_set_item(mitems,  0/*id*/, "  0  ",   0/*tag*/);
        menu_set_item(mitems,  1/*id*/, "  1  ",   1/*tag*/);
        menu_set_item(mitems,  2/*id*/, "  2  ",   2/*tag*/);
        menu_set_item(mitems,  3/*id*/, "  3  ",   3/*tag*/);
        menu_set_item(mitems,  4/*id*/, "  4  ",   4/*tag*/);
        menu_set_item(mitems,  5/*id*/, " Go  ", 'G'/*tag*/);
        menu_set_item(mitems,  6/*id*/, "  5  ",   5/*tag*/);
        menu_set_item(mitems,  7/*id*/, "  6  ",   6/*tag*/);
        menu_set_item(mitems,  8/*id*/, "  7  ",   7/*tag*/);
        menu_set_item(mitems,  9/*id*/, "  8  ",   8/*tag*/);
        menu_set_item(mitems, 10/*id*/, "  9  ",   9/*tag*/);
        menu_set_item(mitems, 11/*id*/, "Exit ", 'X'/*tag*/);
        menu_set_item(mitems, 12/*id*/, "  A  ",  10/*tag*/);
        menu_set_item(mitems, 13/*id*/, "  B  ",  11/*tag*/);
        menu_set_item(mitems, 14/*id*/, "  C  ",  12/*tag*/);
        menu_set_item(mitems, 15/*id*/, "  D  ",  13/*tag*/);
        menu_set_item(mitems, 16/*id*/, "  E  ",  14/*tag*/);
        menu_set_item(mitems, 17/*id*/, "  F  ",  15/*tag*/);

        menu_set_callback_vblank(mset, menu_vblank_clbk);
        menu_set_pos(mset, -1/*x0*/, MENU_DISPLAY_Y0/*y0*/, 30/*w*/, 4/*h*/, 6/*cols*/);
        menu_set_erase_on_exit(mset, 1);

        menu_selected = ((v >> ((value_width*2-1)*4 - adr_pos*4)) & 0xF);
        if(value_width == 1)
        {
            sprintf(tmp, "%02X", (unsigned char)v);
            tmp[adr_pos] = '*';
            conio_printf(3, 11/*row*/, COLOR_LIME, " Value=0x%02X (0x%s)                ", (unsigned char)v, tmp);
        }
        else if(value_width == 2)
        {
            sprintf(tmp, "%04X", (unsigned short)v);
            tmp[adr_pos] = '*';
            conio_printf(3, 11/*row*/, COLOR_LIME, " Value=0x%04X (0x%s)            ", (unsigned short)v, tmp);
        }
        else
        {
            sprintf(tmp, "%08X", (int)v);
            tmp[adr_pos] = '*';
            conio_printf(3, 11/*row*/, COLOR_LIME, " Value=0x%08X (0x%s)    ", (int)v, tmp);
        }
        menu_selected = menu_start(mitems, mset, menu_selected/*selected_tag*/);

        if((mset->exit_code == MENU_EXIT_CANCEL)
        || (MENU_TAG_INVALID(menu_selected)    )
        || (menu_selected == 'X'/*exit*/       ))
        {
            //menu_clear_display(p);
            *value = v;
            conio_printf(2, 11/*row*/, COLOR_TRANSPARENT, empty_line);
            return 0;
        }

        if(menu_selected == 'G'/*GO*/)
        {
            *value = v;
            conio_printf(2, 11/*row*/, COLOR_TRANSPARENT, empty_line);
            return 1;
        }
        else /* Edit value. */
        {
            v &= ~(0xF << ((value_width*2-1)*4 - adr_pos*4));
            v |= menu_selected << ((value_width*2-1)*4 - adr_pos*4);
            adr_pos = (adr_pos+1)%(2*value_width);
        }
    }

}

unsigned char char2pchar(unsigned char c)
{
    /* Display differently null and FF characters.
     * It helps to spot them in screen full of hexa codes.
     */
    if(c == 0x00) return '~';
    if(c == 0xFF) return 'X';

    /* Non-displayable characters. */
    if(c < ' ') return '.';
    if(c > 127) return '?';

    return c;
}


void hexedit_start(unsigned char* ptr)
{
    unsigned long i, j;
    char tmp[16];

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
#define ACTION_READ  1
#define ACTION_WRRD  2
    unsigned char action = 0;

    while(1)
    {
        unsigned char buff[HEXPAGE_SIZE];

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
                conio_printf(2, row++, COLOR_BG_FG(COLOR_GRAY, COLOR_LIME), " Start address = 0x%08X         ", ptr);
                conio_printf(2, row++, COLOR_BG_FG(COLOR_GRAY, COLOR_LIME), " Cursor offset = 0x%08X         ", disp_offset+curs_offset);

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
                            conio_printf(3 + 3*i, row + j, color, tmp);
                            conio_printf(3 + 3*i+2, row + j, COLOR_TRANSPARENT, " ");

                            conio_printf(29 + 1*i+0, row + j, color, "%c", char2pchar(*c));
                        }
                        else if(data_width == 2)
                        {
                            unsigned short dat_u16 = *((unsigned short*)c);
                            sprintf(tmp, "%04X", dat_u16);
                            conio_printf(3 + 3*i, row + j, color, tmp);
                            conio_printf(3 + 3*i+4, row + j, COLOR_TRANSPARENT, "  ");

                            conio_printf(29 + 1*i+0, row + j, color, "%c", char2pchar(*c));
                            conio_printf(29 + 1*i+1, row + j, color, "%c", char2pchar(*(c+1)));
                        }
                        else
                        {
                            unsigned int dat_u32 = *((unsigned int*)c);
                            sprintf(tmp, "%08X", dat_u32);
                            conio_printf(3 + 3*i, row + j, color, tmp);
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
                case(ACTION_READ):
                    conio_printf(3, row++, COLOR_YELLOW, "Background read (Y:stop)          ");
                    break;
                case(ACTION_WRRD):
                    conio_printf(3, row++, COLOR_YELLOW, "Background write-read (Z:stop)    ");
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

        /* Constant data read/write/etc ? */
        if(pad->delta[0] & PAD_Y)
        {
            action = (action != ACTION_READ ? ACTION_READ : 0);
            refresh |= REFRESH_DISPLAY;
        }
        if(pad->delta[0] & PAD_Z)
        {
            action = (action != ACTION_WRRD ? ACTION_WRRD : 0);
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

            if(hexedit_prompt_value(&val, data_width, "Edit new value"))
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

        /* Show help. */
        if(pad->delta[0] & PAD_LBUTTON)
        {
            int row = HEADER_DISPLAY_Y0;

            hexedit_clear();

            conio_printf(2, row++, COLOR_YELLOW, "       HexEditor - Help screen       ");
            conio_printf(2, row++, COLOR_WHITE , " - Arrows : move cursor              ");
            conio_printf(2, row++, COLOR_WHITE , " - A : unused                        ");
            conio_printf(2, row++, COLOR_WHITE , " - B : Exit                          ");
            conio_printf(2, row++, COLOR_WHITE , " - C : Change value                  ");
            conio_printf(2, row++, COLOR_WHITE , " - X : Change data width             ");
            conio_printf(2, row++, COLOR_WHITE , " - Y : Background data read          ");
            conio_printf(2, row++, COLOR_WHITE , " - Z : Background data write & read  ");
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

            hexedit_clear();
            refresh |= REFRESH_DISPLAY;
        }

        if(action == ACTION_READ)
        {
            refresh |= REFRESH_DISPLAY | REFRESH_CURRVAL;
        }
        if(action == ACTION_WRRD)
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
    unsigned long mem_address = 0x06004000; // 0x22000000

    /* First, clear display area. */
    hexedit_clear();

    while(1)
    {
        if(hexedit_prompt_value(&mem_address, sizeof(mem_address), "Edit start address") == 0)
        { /* Cancel. */
            hexedit_clear();
            return;
        }

        hexedit_start((unsigned char*)mem_address);
        hexedit_clear();
    }
}

