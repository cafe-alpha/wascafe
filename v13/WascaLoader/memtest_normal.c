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

#include "memtest_normal.h"
#include "shared.h"

#include "../menu.h"



/***************************************************************************
 *                          Cartridge RAM Test                             *
 ***************************************************************************/


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

unsigned int my_CartRAMsize = 0;
unsigned char cart_id = 0xFF;

void extram_configure(int cs)
{
    unsigned long setReg, refReg;

    // 3. write 1 to initialize
    // is at the end of A-BUS Dummy area
    // !!! word format !!! (but what is meant?)
    *((unsigned short *)0x257efffe) = 1;
    
    // 4. set A-bus set and refresh registers
    setReg = refReg = 0;
    if(cs == 0)
    {
        // set cs0 for 32MBit
        setReg |= 1 << 29;  // After-READ precharge insert bit
        setReg |= 3 << 24;  // Burst cycle wait number
        setReg |= 3 << 20;  // Normal cycle wait number
    }
    else
    {
        // set cs1 for 16MBit ??
        setReg |= 1 << 28;  // external wait effective bit
        setReg |= 15 << 24; // Burst cycle wait number
        setReg |= 15 << 20; // Normal cycle wait number
    }
    *((unsigned long *)0x25fe00B0) = setReg;
    // A-Bus refresh register
    refReg |= 1 << 4;       // A-Bus refresh enable bit
    // I've just take this value
    // see SCU user manual for value range
    refReg |= 1;            // A-Bus refresh cycle wait
    *((unsigned long *)0x25fe00B8) = refReg;
}


/* The memory test parameters.
 * Theses are set on main module side and used here.
 * (Yeah, that's a dirty way to get things done ...)
 */
int ram_init_done = 0;
int access_32bit  = 1;
int pre_fill_ff   = 1;
int pre_fill_00   = 1;


/*
 * When verifying all the data in the memory cart, about 3~4 seconds
 * are required, so in order to speed-up memory cart verify process, 
 * only some data in the memory cart are checked.
 */
//#define VERIFY_INCR 17
#define VERIFY_INCR 1



void memtest_normal(int cs, int do_mem_config)
{
    int i, row;

    /* Clear normal (excluding status) display. */
    clear_normal_display();

    row = 5;

    // cartridge initialization: as in ST-TECH-47.pdf
    // 1. verify cartridge id
    // 5A -  8 Mb (1MB)
    // 5C - 32 Mb (4MB)
    //! is last byte of A-BUS CS1 area
    cart_id = *((unsigned char *)0x24ffffff);
    if(cart_id == 0x5a)
    {
        my_CartRAMsize = 1*1024*1024;
        conio_printf(2, row++, COLOR_WHITE, "1 MB RAM cart detected!");
    }
    else if(cart_id == 0x5c)
    {
        my_CartRAMsize = 4*1024*1024;
        conio_printf(2, row++, COLOR_WHITE, "4 MB RAM cart detected!");
    }
    else
    {
#if 0
        my_CartRAMsize = 0x0;

        // 2. prompt proper connection
        conio_printf(2, row++, COLOR_WHITE, "cart_id:0x%02X", cart_id);
        row++;
        conio_printf(2, row++, COLOR_WHITE, "cartridge is not  ");
        conio_printf(2, row++, COLOR_WHITE, "inserted properly.");
        conio_printf(2, row++, COLOR_WHITE, "");
        conio_printf(2, row++, COLOR_WHITE, "Please turn off   ");
        conio_printf(2, row++, COLOR_WHITE, "power and reinsert");
        conio_printf(2, row++, COLOR_WHITE, "the extended RAM  ");
        conio_printf(2, row++, COLOR_WHITE, "cartridge.        ");

        wait_for_start(NULL/*msg*/);
        cart_id = 0xFF; // Error, no cart
        continue;
#else
        my_CartRAMsize = 4*1024*1024;
        conio_printf(2, row++, COLOR_WHITE, "Nothing detected : trying 4 MB");
#endif
    }

    if(do_mem_config)
    {
        extram_configure(cs);
    }

    unsigned long *DRAM0, *DRAM1, DRAMsize, ok;
    // 5. verify data before proceeding
    // cache-through DRAM0 and DRAM1
    DRAMsize = my_CartRAMsize >> 3; // byte length per cart -> longword length per chip
    DRAM0 = (unsigned long *)0x22400000;
    DRAM1 = (unsigned long *)0x22600000;

    if(pre_fill_ff)
    {
        memset(DRAM0, 0xFF, my_CartRAMsize/2);
        memset(DRAM1, 0xFF, my_CartRAMsize/2);
    }

    if(pre_fill_00)
    {
        memset(DRAM0, 0x00, my_CartRAMsize/2);
        memset(DRAM1, 0x00, my_CartRAMsize/2);
    }


    int verify_error_cnt = 0;
#define VERIFY_ERROR_MAX 16

    // write
    for(i = 0; i < DRAMsize; i+=VERIFY_INCR)
    {

        if(access_32bit)
        {
            *(DRAM0 + i) = i;
            *(DRAM1 + i) = DRAMsize - 1 - i;
        }
        else
        {
            ((unsigned short*)(DRAM0 + i))[0] = (unsigned short)i;
            ((unsigned short*)(DRAM0 + i))[1] = (unsigned short)i;

            ((unsigned short*)(DRAM1 + i))[0] = (unsigned short)(DRAMsize - 1 - i);
            ((unsigned short*)(DRAM1 + i))[1] = (unsigned short)(DRAMsize - 1 - i);
        }
    }    
    // read
    ok = 1;
    for(i = 0; i < DRAMsize; i+=VERIFY_INCR)
    {
        if(access_32bit)
        {
            if(*(DRAM0 + i) != i)
            {
                if(verify_error_cnt < VERIFY_ERROR_MAX)
                {
                    conio_printf(2, row++, COLOR_YELLOW, "%08X: %08X != %08X", DRAM0 + i, *(DRAM0 + i), i);
                    verify_error_cnt++;
                }
                ok = 0;
            }    

            if(*(DRAM1 + i) != (DRAMsize - 1 - i))
            {
                if(verify_error_cnt < VERIFY_ERROR_MAX)
                {
                    conio_printf(2, row++, COLOR_YELLOW, "%08X: %08X != %08X", DRAM1 + i, *(DRAM1 + i), DRAMsize - 1 - i);
                    verify_error_cnt++;
                }
                ok = 0;
            }    
        }
        else
        {
            if(((unsigned short*)(DRAM0 + i))[0] != (unsigned short)i)
            {
                if(verify_error_cnt < VERIFY_ERROR_MAX)
                {
                    conio_printf(2, row++, COLOR_YELLOW, "%08X+0: %04X != %04X", DRAM0 + i, ((unsigned short*)(DRAM0 + i))[0], (unsigned short)i);
                    verify_error_cnt++;
                }
                ok = 0;
            }    
            if(((unsigned short*)(DRAM0 + i))[1] != (unsigned short)i)
            {
                if(verify_error_cnt < VERIFY_ERROR_MAX)
                {
                    conio_printf(2, row++, COLOR_YELLOW, "%08X+1: %04X != %04X", DRAM0 + i, ((unsigned short*)(DRAM0 + i))[1], (unsigned short)i);
                    verify_error_cnt++;
                }
                ok = 0;
            }    

            if(((unsigned short*)(DRAM1 + i))[0] != (unsigned short)(DRAMsize - 1 - i))
            {
                if(verify_error_cnt < VERIFY_ERROR_MAX)
                {
                    conio_printf(2, row++, COLOR_YELLOW, "%08X+0: %04X != %04X", DRAM1 + i, ((unsigned short*)(DRAM1 + i))[0], (unsigned short)(DRAMsize - 1 - i));
                    verify_error_cnt++;
                }
                ok = 0;
            }    
            if(((unsigned short*)(DRAM1 + i))[1] != (unsigned short)(DRAMsize - 1 - i))
            {
                if(verify_error_cnt < VERIFY_ERROR_MAX)
                {
                    conio_printf(2, row++, COLOR_YELLOW, "%08X+1: %04X != %04X", DRAM1 + i, ((unsigned short*)(DRAM1 + i))[1], (unsigned short)(DRAMsize - 1 - i));
                    verify_error_cnt++;
                }
                ok = 0;
            }    
        }
    } 

    if(ok == 0)
    {
        conio_printf(2, row++, COLOR_ERROR, "verifying RAM cart FAILED!");

        wait_for_start(NULL/*msg*/);
        cart_id = 0xFE; // Error, verify failed
        return;
    }

    conio_printf(2, row++, COLOR_WHITE, "verifying RAM cart OK!");
    conio_printf(2, row++, COLOR_LIME , "ID=%02X, sz=%08X (%3d KB)", cart_id, my_CartRAMsize, my_CartRAMsize>>10);

    wait_for_start(NULL/*msg*/);
}

