/**
 *  Cartridge Test Utility
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/pskai/
 *
 *  See LICENSE file for details.
**/

#ifndef _VDP_H_
#define _VDP_H_

/* VDP registers definitions. */
#include "vdp2.h"


/* Global data */
extern volatile unsigned short *vdp1_vram;
extern volatile unsigned short *vdp1_fb;
extern volatile unsigned short *vdp2_vram;
extern volatile unsigned short *vdp2_cram;

/* Function prototypes */
void vdp_init(void);

#endif /* _VDP_H_ */

