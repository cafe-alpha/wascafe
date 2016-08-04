/**
 *  Cartridge Test Utility
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/pskai/
 *
 *  See LICENSE file for details.
**/

#include <stdio.h>
#include <string.h> /* For memset,memcpy */

#include "wasca.h"

#include "vdp2.h"
#include "smpc.h"

#include "vdp.h"
#include "conio.h"

#include "menu.h"

#include "shared.h"

#include "release.h"

#include "benchmark.h"
#include "hexedit.h"


/* Shared global data. */
char* empty_line = "                                     ";


/* Prompt user for Start button. */
void prompt_start(void)
{
    pad_t* pad;

    conio_printf(2, 26, COLOR_WHITE, "Press Start to continue");
    while (1)
    {
        pad = pad_read();
        if(pad->delta[0] & PAD_START)
        {
            break;
        }
        WaitForVBLANKIn();
        WaitForVBLANKOut();
    }
}


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

/*
 * When verifying all the data in the memory cart, about 3~4 seconds
 * are required, so in order to speed-up memory cart verify process, 
 * only some data in the memory cart are checked.
 */
//#define VERIFY_INCR 17
#define VERIFY_INCR 1
unsigned char my_RB_CartRAM_init(unsigned char cs)
{
    int row = 5;
    unsigned long setReg, refReg;
    unsigned char id;

    // cartridge initialization: as in ST-TECH-47.pdf
    // 1. verify cartridge id
    // 5A - 8 Mb
    // 5C - 32 Mb
    //! is last byte of A-BUS CS1 area
    id = *((unsigned char *)0x24ffffff);
    if(id == 0x5a)
    {
        my_CartRAMsize = 1*1024*1024;
        conio_printf(2, row++, COLOR_WHITE, "8 Mb RAM cart detected!");
    }
    else if(id == 0x5c)
    {
        my_CartRAMsize = 4*1024*1024;
        conio_printf(2, row++, COLOR_WHITE, "32 Mb RAM cart detected!");
    } else {
        my_CartRAMsize = 0x0;      

        // 2. prompt proper connection
        conio_printf(2, row++, COLOR_WHITE, "The extended RAM  ");
        conio_printf(2, row++, COLOR_WHITE, "cartridge is not  ");
        conio_printf(2, row++, COLOR_WHITE, "inserted properly.");
        conio_printf(2, row++, COLOR_WHITE, "");
        conio_printf(2, row++, COLOR_WHITE, "Please turn off   ");
        conio_printf(2, row++, COLOR_WHITE, "power and reinsert");
        conio_printf(2, row++, COLOR_WHITE, "the extended RAM  ");
        conio_printf(2, row++, COLOR_WHITE, "cartridge.        ");

        prompt_start();
        return 0xFF; // Error, no cart
    }

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

    unsigned long *DRAM0, *DRAM1, DRAMsize, i, ok;
    // 5. verify data before proceeding
    // cache-through DRAM0 and DRAM1
    DRAMsize = my_CartRAMsize >> 3; // byte length per cart -> longword length per chip
    DRAM0 = (unsigned long *)0x22400000;
    DRAM1 = (unsigned long *)0x22600000;
    // write
    for(i = 0; i < DRAMsize; i+=VERIFY_INCR) {
        *(DRAM0 + i) = i;
        *(DRAM1 + i) = DRAMsize - 1 - i;
    }    
    // read
    ok = 1;
    for(i = 0; i < DRAMsize; i+=VERIFY_INCR)
    {
        if(*(DRAM0 + i) != i)
        {
            //were_here("DRAM0 %i != %i", *(DRAM0 + i), i);
            ok = 0;
        }    
        if(*(DRAM1 + i) != (DRAMsize - 1 - i))
        {
            //were_here("DRAM1 %i != %i", *(DRAM1 + i), (DRAMsize - 1 - i));
            ok = 0;
        }    
    } 
       
    if(ok == 0)
    {
        conio_printf(2, row++, COLOR_RED, "verifying RAM cart FAILED!");

        prompt_start();
        return 0xFE; // Error, verify failed
    }

    conio_printf(2, row++, COLOR_WHITE, "verifying RAM cart OK!");
    conio_printf(2, row++, COLOR_LIME , "ID=%02X, sz=%08X (%3d KB)", id, my_CartRAMsize, my_CartRAMsize>>10);

    prompt_start();
    return id;
}



void wasca_mode_init(void)
{
    menu_settings_t* mset   = &_menu_settings;
    menu_items_t   * mitems = &_menu_items;

    int menu_sel = 0;
    unsigned short wasca_mode = WASCA_MODE_INIT;
    unsigned short progress;

    while(1)
    {
        /* Display main menu. */
        menu_reset_settings(mset);
        menu_reset_items(mitems);
        menu_set_title(mset, "Wasca Mode Init");
        menu_set_item(mitems,  0/*id*/, "        Init Ca",  0/*tag*/);
        menu_set_item(mitems,  1/*id*/, "rtridge        ",  0/*tag*/);

        menu_set_item(mitems,  2/*id*/, "      Exit to M",  1/*tag*/);
        menu_set_item(mitems,  3/*id*/, "ultiplayer     ",  1/*tag*/);

        menu_set_item(mitems,  4/*id*/, " Memory 0.5MiB ",  2/*tag*/);
        menu_set_item(mitems,  5/*id*/, " Memory 1MiB   ",  3/*tag*/);
        menu_set_item(mitems,  6/*id*/, " Memory 2MiB   ",  4/*tag*/);
        menu_set_item(mitems,  7/*id*/, " Memory 4MiB   ",  5/*tag*/);
        menu_set_item(mitems,  8/*id*/, " Ram 1MiB      ",  6/*tag*/);
        menu_set_item(mitems,  9/*id*/, " Ram 4MiB      ",  7/*tag*/);
        menu_set_item(mitems, 10/*id*/, " KoF95.bin     ",  8/*tag*/);
        menu_set_item(mitems, 11/*id*/, " Ultraman.bin  ",  9/*tag*/);
        menu_set_item(mitems, 12/*id*/, " Boot.bin      ", 10/*tag*/);

        menu_set_callback_vblank(mset, menu_vblank_clbk);
        menu_set_pos(mset, -1/*x0*/, MENU_TOP_ROW/*y0*/, 30/*w*/, 8/*h*/, 2/*cols*/);
        menu_set_erase_on_exit(mset, 1);

        conio_printf(2, 20, COLOR_WHITE, "wasca_mode = 0x%04X", wasca_mode);

        menu_sel = menu_start(mitems, mset, menu_sel/*selected_tag*/);

        /* Exit ? */
        if(mset->exit_code == MENU_EXIT_CANCEL)
        {
            return;
        }

        switch(menu_sel)
        {
        default:
        case( 0):
            REG_WASCA_SYS_MODE = wasca_mode;
            conio_printf(2, 21, COLOR_WHITE, "please wait");

            /* Wait init complete. */
            do
            {
                progress = REG_WASCA_SYS_PCNTR;
                conio_printf(14, 21, COLOR_WHITE, "%3d %% ...  ", progress);
            } while(progress < 0x0064);

            conio_printf(2, 22, COLOR_YELLOW, "Initialization completed");
            break;

        case( 1):
            (**(void(**)(void))0x600026C)();
            /* Tweak for emulators. */
            while(1){;}
            break;

        case( 2): wasca_mode = WASCA_MODE_MEM05; break;
        case( 3): wasca_mode = WASCA_MODE_MEM10; break;
        case( 4): wasca_mode = WASCA_MODE_MEM20; break;
        case( 5): wasca_mode = WASCA_MODE_MEM40; break;

        case( 6): wasca_mode = WASCA_RAM_1;      break;
        case( 7): wasca_mode = WASCA_RAM_2;      break;

        case( 8): wasca_mode = WASCA_ROM_KOF;    break;
        case( 9): wasca_mode = WASCA_ROM_ULTRA;  break;
        case(10): wasca_mode = WASCA_BOOT;       break;
        }
    }
}

void wasca_status(int wait)
{
    pad_t* pad;
    char buff[10];
    unsigned short tmp;
    int row = 24;

    conio_printf( 2, row, COLOR_YELLOW, "PCNTR");
    conio_printf( 7, row, COLOR_WHITE , ": %5d", REG_WASCA_SYS_PCNTR);
    conio_printf(20, row, COLOR_YELLOW, "STATUS");
    conio_printf(26, row, COLOR_WHITE , ": 0x%04X", REG_WASCA_SYS_STATUS);
    row++;

    tmp = REG_WASCA_SYS_HWVER;
    conio_printf( 2, row, COLOR_YELLOW, "HWVER");
    conio_printf( 7, row, COLOR_WHITE , ": v%d.%d  ", tmp >> 8, tmp&0x00FF);

    tmp = REG_WASCA_SYS_SWVER;
    conio_printf(20, row, COLOR_YELLOW, "SWVER");
    conio_printf(26, row, COLOR_WHITE , ": v%d.%d  ", tmp >> 8, tmp&0x00FF);
    row++;

    memset((void*)buff, 0, sizeof(buff));
    memcpy((void*)buff, (void*)REG_WASCA_SYS_SIG_PTR, 6);
    conio_printf( 2, row, COLOR_YELLOW, "SIGNATURE");
    conio_printf(11, row, COLOR_WHITE, ": %s ", buff);

    conio_printf(20, row, COLOR_YELLOW, "MODE");
    conio_printf(26, row, COLOR_WHITE , ": 0x%04X", REG_WASCA_SYS_MODE);
    row++;

    if(wait)
    {
        conio_printf( 2, row, COLOR_WHITE, "Press Start to continue");
        while (1)
        {
            pad = pad_read();
            if(pad->delta[0] & PAD_START)
            {
                break;
            }
            WaitForVBLANKIn();
            WaitForVBLANKOut();
        }
    }
}

int status_display = 0;
void menu_vblank_clbk(void)
{
    if(status_display)
    {
        wasca_status(0/*wait*/);
    }
}


int menu_selected = 0;

void _main(void)
{
    menu_settings_t* mset   = &_menu_settings;
    menu_items_t   * mitems = &_menu_items;

    vdp_init();
    conio_init();
    pad_init();

    TVMD = 0x8000;

    menu_selected = 3;

    while(1)
    {
        int i;

        conio_printf(2, 2, COLOR_BG_FG(COLOR_AQUA, COLOR_YELLOW), " Custom Cartridge Test Tool V.%d.%03d ", RELEASE_ID/1000, RELEASE_ID%1000);

        /* Display main menu. */
        menu_reset_settings(mset);
        menu_reset_items(mitems);
        menu_set_title(mset, "Please select action");
        menu_set_item(mitems, 0/*id*/, "   Init DRAM   ", 0/*tag*/);
        menu_set_item(mitems, 1/*id*/, "    Hex Edit   ", 1/*tag*/);

        menu_set_item(mitems, 2/*id*/, "   Rockin-B Ben", 2/*tag*/);
        menu_set_item(mitems, 3/*id*/, "chmark Tool    ", 2/*tag*/);

        menu_set_item(mitems, 4/*id*/, "     Wasca Init", 3/*tag*/);
        menu_set_item(mitems, 5/*id*/, "ialization     ", 3/*tag*/);

        menu_set_item(mitems, 6/*id*/, " Status-single ", 4/*tag*/);
        menu_set_item(mitems, 7/*id*/, " Status-cont.  ", 5/*tag*/);

        menu_set_item(mitems, 8/*id*/, "      Exit to M", 9/*tag*/);
        menu_set_item(mitems, 9/*id*/, "ultiplayer     ", 9/*tag*/);

        menu_set_callback_vblank(mset, menu_vblank_clbk);
        menu_set_pos(mset, -1/*x0*/, MENU_TOP_ROW/*y0*/, 30/*w*/, 8/*h*/, 2/*cols*/);
        menu_set_erase_on_exit(mset, 1);

        menu_selected = menu_start(mitems, mset, menu_selected/*selected_tag*/);

        /* Exit to multiplayer ? */
        if(((mset->exit_code == MENU_EXIT_OK) && (menu_selected == 9))
        ||  (mset->exit_code == MENU_EXIT_CANCEL))
        {
            (**(void(**)(void))0x600026C)();
            while(1) {;}
        }

        switch(menu_selected)
        {
        default:
        case(0):
            my_RB_CartRAM_init(0/*CS0*/);
            break;

        case(1):
            hexedit();
            break;

        case(2):
            benchmark();
            break;

        case(3):
            wasca_mode_init();
            break;

        case(4):
            wasca_status(1/*wait*/);
            break;
        case(5):
            status_display = status_display ? 0 : 1;
            break;
        }

        /* Clear display. */
        for(i=4; i<27; i++)
        {
            conio_printf(2, i, COLOR_WHITE, empty_line);
        }
    }
}
