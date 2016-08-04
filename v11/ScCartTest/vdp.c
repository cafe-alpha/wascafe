/**
 *  Cartridge Test Utility
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/pskai/
 *
 *  See LICENSE file for details.
**/

#include <stdio.h>

#include "vdp2.h"

/* Global data */
volatile unsigned short *vdp2_vram = (volatile unsigned short *)VDP2_VRAM;
volatile unsigned short *vdp2_cram = (volatile unsigned short *)VDP2_CRAM;
volatile unsigned short *vdp2_reg  = (volatile unsigned short *)VDP2_REGISTER_BASE;
void vdp_init(void)
{
    //void vdp2_init(void)
    int i;
    TVMD = 0x0000;
    RAMCTL = RAMCTL & (~0x3000);

    // Map Offset Register: Bitmap screen will be located at VRAM offset 0
    MPOFN = 0;

    // Character Control Register: 256 colors, enable NBG0 as a bitmap
    // |8 for 1024x256 bitmap
    CHCTLA = 0x0012|8;

    // Screen Scroll Value Registers: No scroll
    SCXIN0 = 0;
    SCXDN0 = 0;
    SCYIN0 = 0;

    // Screen Display Enable Register: Invalidate the transparency code for
    // NBG0 and display NBG0
    BGON = 0x0001;

    /* Clear VRAM */
    for(i = 0; i < 0x40000; i++)
        vdp2_vram[i] = 0x0000;

    /* Clear CRAM */
    for(i = 0; i < 0x0800; i++)
        vdp2_cram[i] = 0x0000;

}
