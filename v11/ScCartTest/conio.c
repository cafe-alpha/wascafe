/**
 *  Cartridge Test Utility
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/pskai/
 *
 *  See LICENSE file for details.
**/

#include "conio.h"
#include "vga_font.h"
//#include "vga_pal.h"

#include "vdp2.h"


void conio_init(void)
{
    int i, j, k;
    /* Transparent color */
    conio_set_color_to_palette(COLOR_RGB16(0,0,0), 0);
    /* White */
    for(i = 1; i < 16; i++)
        conio_set_color_to_palette(COLOR_RGB16(31,31,31), i);

    /* Set color from iapetus library. */
    conio_set_color_to_palette(COLOR_RGB16( 0, 0, 0), COLOR_BLACK  );
    conio_set_color_to_palette(COLOR_RGB16( 0, 0,21), COLOR_BLUE   );
    conio_set_color_to_palette(COLOR_RGB16( 0,21, 0), COLOR_GREEN  );
    conio_set_color_to_palette(COLOR_RGB16( 0,21,21), COLOR_TEAL   );
    conio_set_color_to_palette(COLOR_RGB16(21, 0, 0), COLOR_RED    );
    conio_set_color_to_palette(COLOR_RGB16(21, 0,21), COLOR_PURPLE );
    conio_set_color_to_palette(COLOR_RGB16(21,10, 0), COLOR_MAROON );
    conio_set_color_to_palette(COLOR_RGB16(21,21,21), COLOR_SILVER );
    conio_set_color_to_palette(COLOR_RGB16(10,10,10), COLOR_GRAY   );
    conio_set_color_to_palette(COLOR_RGB16(10,10,31), COLOR_AQUA   );
    conio_set_color_to_palette(COLOR_RGB16(10,31,10), COLOR_LIME   );
    conio_set_color_to_palette(COLOR_RGB16(10,31,31), COLOR_OLIVE  );
    conio_set_color_to_palette(COLOR_RGB16(31,10,10), COLOR_FUCHSIA);
    conio_set_color_to_palette(COLOR_RGB16(31,10,31), COLOR_PINK   );
    conio_set_color_to_palette(COLOR_RGB16(31,31,10), COLOR_YELLOW );
    conio_set_color_to_palette(COLOR_RGB16(31,31,31), COLOR_WHITE  );

    /* 6*6*6 colors. */
    for(i = 0; i < 6; i++)
    {
        for(j = 0; j < 6; j++)
        {
            for(k = 0; k < 6; k++)
            {
                conio_set_color_to_palette(
                    COLOR_RGB16(5*k+1, 5*j+1, 5*i+1),
                    6*6*k + 6*j + i+32);
            }
        }
    }

    RAMCTL = 0x0000;
}

void conio_set_color_to_palette(unsigned short col, unsigned char id)
{
    volatile unsigned short* vdp2_cram = (volatile unsigned short*)VDP2_CRAM;
    vdp2_cram[id] = col;
}



void conio_printchar(short x, short y, unsigned short color, char v)
{
    int r, c;
    volatile unsigned char *bmp = (volatile unsigned char *)VDP2_VRAM;
    unsigned char fg = (color >> 0) & 0xFF;
    unsigned char bg = (color >> 8) & 0xFF;

    for(r = 0; r < 8; r++)
    {
        unsigned char b = conio_font[v<<3|r];
        for(c = 0; c < 8; c++)
        {
            unsigned char d = (b & (0x80 >> c)) ? fg : bg;
            bmp[((y<<3|r)<<10/* 9-> half size */)+(x<<3|c)] = d;
        }            
    }
}

void conio_printstr(short x, short y, unsigned short color, char *s)
{
    int i;
    for(i = 0; s[i] != 0; i++)
        conio_printchar(x+i,y,color,s[i]);
}


void conio_printf(short x, short y, unsigned short color, char* msg, ...)
{
    char buff[50];
    va_list ap;
    va_start(ap,msg);
    vsprintf(buff, msg,ap);
    va_end(ap);
    conio_printstr(x, y, color, buff);
}

