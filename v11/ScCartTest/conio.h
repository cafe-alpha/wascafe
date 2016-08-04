/**
 *  Cartridge Test Utility
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/pskai/
 *
 *  See LICENSE file for details.
**/

#ifndef _CONIO_H_
#define _CONIO_H_

#include <stdio.h>
#include <stdarg.h>

/* General usage macros */
#define COLOR_RGB16(r,g,b) (((r)&0x1F)|((g)&0x1F)<<5|((b)&0x1F)<<10|0x8000)
#define COLOR_RGB24(r,g,b) (COLOR_RGB16(r>>3,g>>3,b>>3))

/* Colors from iapetus library. */
#define COLOR_TRANSPARENT 0
#define COLOR_BLACK   16
#define COLOR_BLUE    17
#define COLOR_GREEN   18
#define COLOR_TEAL    19
#define COLOR_RED     20
#define COLOR_PURPLE  21
#define COLOR_MAROON  22
#define COLOR_SILVER  23
#define COLOR_GRAY    24
#define COLOR_AQUA    25
#define COLOR_LIME    26
#define COLOR_OLIVE   27
#define COLOR_FUCHSIA 28
#define COLOR_PINK    29
#define COLOR_YELLOW  30
#define COLOR_WHITE   31

/* For all the "printf" functions, use this macro to define background and foreground colors.
 * Not using this macro will define the background color to 0 (= transparent color).
 */
#define COLOR_BG_FG(B,F) ((B)<<8|(F))

/* Function prototypes */
void conio_init(void);
void conio_set_color_to_palette(unsigned short col, unsigned char id);

void conio_printchar(short x, short y, unsigned short color, char v);
void conio_printstr(short x, short y, unsigned short color, char *s);
void conio_printf(short x, short y, unsigned short color, char* msg, ...);

#endif /* _CONIO_H_ */
