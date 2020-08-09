/**
 *  Cartridge Test Utility
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/pskai/
 *
 *  See LICENSE file for details.
**/

#ifndef _MEMTEST_NORMAL_H_
#define _MEMTEST_NORMAL_H_

#include <stdio.h>


#define CARTRAM_ID_8M       0x5a
#define CARTRAM_ID_32M      0x5c

#define CartRAM0    ((void *)0x02400000)
#define CartRAM1    ((void *)0x02600000)

//  RB_initRAMCart(unsigned char cs)
//  - checks for present RAM cart, inits and verifies it
//
//  inputs:
//      cs          -   select whether to set CS0 or CS1 space 
//                      (both works, don't know where the difference is)   
//
//  returns:
//      cartridge id    -   on success
//                          (8Mb: 0x5a or 32Mb: 0x5c)
//      0xFF            -   no cart
//      0xFE            -   data verify failed 

/* Globals below are shared with other tests.
 * (It's not really relevant to put that in global scope, 
 * but let's keep small things as-is as long as it's working)
 */
extern unsigned int my_CartRAMsize;
extern unsigned char cart_id;

/* Configure A Bus to handle external RAM on specified (0 or 1) CS. */
void extram_configure(int cs);



/* The memory test parameters.
 * Theses are set on main module side and used here.
 * (Yeah, that's a dirty way to get things done ...)
 */
extern int ram_init_done;
extern int access_32bit;
extern int pre_fill_ff;
extern int pre_fill_00;


void memtest_normal(int cs, int do_mem_config);


#endif /* _MEMTEST_NORMAL_H_ */

