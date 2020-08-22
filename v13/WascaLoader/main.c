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
#include "../debug_console.h"
#include "../menu.h"
#include "../soft_reset.h"
#include "../vdp.h"

#include "shared.h"

#include "../release.h"

#include "benchmark.h"
#include "cart_bus_config.h"
#include "hexedit.h"
#include "selftest.h"

#include "memtest_normal.h"
#include "memtest_chunk.h"
#include "memtest_feedback.h"
#include "memtest_region.h"


int _cdrom_available = 1;


/* Callback for V-Blank display.
 *
 * This is used to display starfield and background logo
 * in Pseudo Saturn Kai and Save Data Manager, but as theses
 * modules aren't used in this application, it is fine to
 * put this function only checking for connectivity.
 */
void background_vblank_clbk(void)
{
    sc_check();
}

/* Clear ... screen display. */
void clear_screen(void)
{
    int i;

    /* Clear lines 3~25. */
    for(i=3; i<26; i++)
    {
        conio_printf(2, i, COLOR_TRANSPARENT, empty_line);
    }
}


/* Prompt user for Start button. */
int wait_for_start(char* msg)
{
    pad_t* pad;

    conio_printf(2, 26, COLOR_TRANSPARENT, empty_line);
    if(msg)
    {
        conio_printf(2, 26, COLOR_WHITE, "%s", msg);
    }
    else
    {
        conio_printf(2, 26, COLOR_WHITE, "Press Start to continue");
    }

    while(1)
    {
        pad = pad_read();
        if(pad->delta[0] & (PAD_A|PAD_B|PAD_C|PAD_START))
        {
            break;
        }
        WaitForVBLANKIn();
        WaitForVBLANKOut();
    }

    conio_printf(2, 26, COLOR_TRANSPARENT, empty_line);

    return (pad->delta[0] & PAD_B ? 0 : 1);
}


//////////////////////////////////////////////////////////////////////////////



/***************************************************************************
 *                       Interrupt stuff - start                           *
 ***************************************************************************/
unsigned long	Timer0handlerCounter = 0;
unsigned long	Timer0handlerCompare = 0;

unsigned long	Timer1handlerCounter = 0;

unsigned long	HblankhandlerCounter = 0;

unsigned long	FramesDisplayedCounter = 0;

unsigned long	SpriteEndCounter = 0;
#define	DISPLAY_PIXEL_HEIGHT		240


void Sprhandler(void)
{
    /* Paranoia save of MACH/MACL */
#if 0
    asm(" \
        sts.l	macl,@-r15 \
        sts.l	mach,@-r15 \
    ");
#else
    asm("sts.l  macl,@-r15");
    asm("sts.l  mach,@-r15");
#endif

   /* Update the counter */
    SpriteEndCounter += 1;

    /* Restore the Paranoia save of MACH/MACL */
#if 0
    asm(" \
        lds.l	@r15+,mach \
        lds.l	@r15+,macl \
    ");
#else
    asm("lds.l  @r15+,mach");
    asm("lds.l  @r15+,macl");
#endif
}

void Timer0handler(void)
{
    /* Paranoia save of MACH/MACL */
#if 0
    asm(" \
        sts.l	macl,@-r15 \
        sts.l	mach,@-r15 \
    ");
#else
    asm("sts.l  macl,@-r15");
    asm("sts.l  mach,@-r15");
#endif

   /* Update the timer 0 counter */
    Timer0handlerCounter += 1;

   /* Force multiple Timer 0 interrupts during this VSCAN */
    Timer0handlerCompare += DISPLAY_PIXEL_HEIGHT / 5;
    if (Timer0handlerCompare >= DISPLAY_PIXEL_HEIGHT)
   {
        Timer0handlerCompare = DISPLAY_PIXEL_HEIGHT / 5;
   }

    TIM_T0_SET_CMP(Timer0handlerCompare);

    /* Restore the Paranoia save of MACH/MACL */
#if 0
    asm(" \
        lds.l	@r15+,mach \
        lds.l	@r15+,macl \
    ");
#else
    asm("lds.l  @r15+,mach");
    asm("lds.l  @r15+,macl");
#endif
}


void Timer1handler(void)
{
    /* Paranoia save of MACH/MACL */
    asm("sts.l  macl,@-r15");
    asm("sts.l  mach,@-r15");

    /* Update the timer 1 counter */
    Timer1handlerCounter += 1;
    TIM_T1_SET_DATA(1000);

    /* Restore the Paranoia save of MACH/MACL */
    asm("lds.l  @r15+,mach");
    asm("lds.l  @r15+,macl");
}


unsigned long	VblankInhandlerCounter = 0;
void VblankInhandler(void)
{
    /* Paranoia save of MACH/MACL */
    asm("sts.l  macl,@-r15");
    asm("sts.l  mach,@-r15");

    /* Update the counter */
    VblankInhandlerCounter += 1;

    /* Restore the Paranoia save of MACH/MACL */
    asm("lds.l  @r15+,mach");
    asm("lds.l  @r15+,macl");
}
unsigned long	VblankOuthandlerCounter = 0;
void VblankOuthandler(void)
{
    /* Paranoia save of MACH/MACL */
    asm("sts.l  macl,@-r15");
    asm("sts.l  mach,@-r15");

    /* Update the counter */
    VblankOuthandlerCounter += 1;

    /* Restore the Paranoia save of MACH/MACL */
    asm("lds.l  @r15+,mach");
    asm("lds.l  @r15+,macl");
}


void HblankInhandler(void)
{
    /* Paranoia save of MACH/MACL */
    asm("sts.l  macl,@-r15");
    asm("sts.l  mach,@-r15");

    /* Update the counter */
    HblankhandlerCounter += 1;

    /* Restore the Paranoia save of MACH/MACL */
    asm("lds.l  @r15+,mach");
    asm("lds.l  @r15+,macl");
}

int _interrupt_enable_flag = 0;
void interrupt_setup(int enable)
{
    // D:\dev\SaturnOrbit\SBL_601-\INTERUPT\TESTTIM0.C

    /* Fire up the Vblank handlers */
    //scd_logout("[INT][HBI] _INT_SetScuFunc(0x%02X, 0x%08X) ~~~", INT_SCU_VBLK_IN, VblankInhandler);
    INT_ChgMsk(INT_MSK_NULL,INT_MSK_VBLK_IN);
    _INT_SetScuFunc(INT_SCU_VBLK_IN, enable ? VblankInhandler : NULL);
    INT_ChgMsk(INT_MSK_VBLK_IN,INT_MSK_NULL);

    //scd_logout("[INT][HBO] _INT_SetScuFunc(0x%02X, 0x%08X) ~~~", INT_SCU_VBLK_OUT, VblankOuthandler);
    INT_ChgMsk(INT_MSK_NULL,INT_MSK_VBLK_OUT);
    _INT_SetScuFunc(INT_SCU_VBLK_OUT, enable ? VblankOuthandler : NULL);
    INT_ChgMsk(INT_MSK_VBLK_OUT,INT_MSK_NULL);


    /* Fire up the Hblank handler */
    //scd_logout("[INT][HBI] _INT_SetScuFunc(0x%02X, 0x%08X) ~~~", INT_SCU_HBLK_IN, HblankInhandler);
    INT_ChgMsk(INT_MSK_NULL,INT_MSK_HBLK_IN);
    _INT_SetScuFunc(INT_SCU_HBLK_IN, enable ? HblankInhandler : NULL);
    INT_ChgMsk(INT_MSK_HBLK_IN,INT_MSK_NULL);


    /* Fire up Timer 0 */
    //scd_logout("[INT][T0] _INT_SetScuFunc(0x%02X, 0x%08X) ~~~", INT_SCU_TIM0, Timer0handler);
    INT_ChgMsk(INT_MSK_NULL,INT_MSK_TIM0);
    _INT_SetScuFunc(INT_SCU_TIM0, enable ? Timer0handler : NULL);
    INT_ChgMsk(INT_MSK_TIM0,INT_MSK_NULL);

    /* Fire up Timer 1 */
    //scd_logout("[INT][T1] _INT_SetScuFunc(0x%02X, 0x%08X) ~~~", INT_SCU_TIM1, Timer1handler);
    INT_ChgMsk(INT_MSK_NULL,INT_MSK_TIM1);
    _INT_SetScuFunc(INT_SCU_TIM1, enable ? Timer1handler : NULL);
    INT_ChgMsk(INT_MSK_TIM1,INT_MSK_NULL);

    /* Fire up sprite draw end */
    //scd_logout("[INT][SPR] _INT_SetScuFunc(0x%02X, 0x%08X) ~~~", INT_SCU_SPR, Sprhandler);
    INT_ChgMsk(INT_MSK_NULL,INT_MSK_SPR);
    _INT_SetScuFunc(INT_SCU_SPR, enable ? Sprhandler : NULL);
    INT_ChgMsk(INT_MSK_SPR,INT_MSK_NULL);

    /* Fire up Timer 0 */
    Timer0handlerCompare = DISPLAY_PIXEL_HEIGHT / 5;
    TIM_T0_SET_CMP(Timer0handlerCompare);

    TIM_T1_SET_DATA(1000);
/*   TIM_T1_SET_MODE(TIM_TENB_ON | TIM_T1MD_CST_LINE); */

    TIM_T1_SET_MODE(TIM_TENB_ON);

    /* Enable interrupt status display. */
    _interrupt_enable_flag = enable;
}

void interrupt_show_status(void)
{
    /* If interrupt not enabled, don't show status. */
    if(!_interrupt_enable_flag)
    {
        return;
    }

    unsigned short frame_count = 0;
    unsigned long r = 0;
    if(frame_count != 0) r = (100*SpriteEndCounter) / frame_count;

    int row = 16;

    conio_printf(2, row++, COLOR_WHITE, "INT T0[%d] T1[%d] HB[%d]", Timer0handlerCounter, Timer1handlerCounter, HblankhandlerCounter);
    conio_printf(2, row++, COLOR_WHITE, "INT SPR[%4d] FC[%4d] r=%3d.%02d", SpriteEndCounter, frame_count, r/100, r%100);
    conio_printf(2, row++, COLOR_WHITE, "INT VBL I[%5d] O[%5d]", VblankInhandlerCounter, VblankOuthandlerCounter);

    unsigned short* ptr2;

    /* Cheat codes debug stuff : show default and alternate work RAM areas. */
    //ptr2 = (unsigned short*)0x060FF000;
    //conio_printf( 2, row++, COLOR_WHITE, "%08X: %04X %04X %04X %04X %04X", 
    //                        ptr2, ptr2[0], ptr2[1], ptr2[2], ptr2[3], ptr2[4]); ptr2+=5;
    //conio_printf( 2, row++, COLOR_WHITE, "        : %04X %04X %04X %04X %04X", 
    //                        ptr2[0], ptr2[1], ptr2[2], ptr2[3], ptr2[4]); ptr2+=5;
    ptr2 = (unsigned short*)0x06002800;
    conio_printf( 2, row++, COLOR_WHITE, "%08X: %04X %04X %04X %04X %04X", 
                            ptr2, ptr2[0], ptr2[1], ptr2[2], ptr2[3], ptr2[4]); ptr2+=5;
    //conio_printf( 2, row++, COLOR_WHITE, "        : %04X %04X %04X %04X %04X", 
    //                        ptr2[0], ptr2[1], ptr2[2], ptr2[3], ptr2[4]); ptr2+=5;

    /* Cheat codes debug stuff : interrupt vectors. */
    unsigned long* ptr4 = (unsigned long*)0x06000000;
    conio_printf( 2, row++, COLOR_YELLOW, "%08X %08X %08X %08X", ptr4[0], ptr4[1], ptr4[2], ptr4[3]); ptr4+=4;
    conio_printf( 2, row++, COLOR_YELLOW, "%08X %08X %08X %08X", ptr4[0], ptr4[1], ptr4[2], ptr4[3]); ptr4+=4;
    conio_printf( 2, row++, COLOR_YELLOW, "%08X %08X %08X %08X", ptr4[0], ptr4[1], ptr4[2], ptr4[3]); ptr4+=4;

    /* Cheat codes debug stuff : show region where cheat codes are applied. */
    unsigned char* ptr = (unsigned char*)0x060FE000;
    conio_printf( 2, row++, COLOR_WHITE, "%08X: %02X %02X %02X %02X %02X %02X %02X %02X", ptr, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]); ptr+=8;
    conio_printf( 2, row++, COLOR_WHITE, "%08X: %02X %02X %02X %02X %02X %02X %02X %02X", ptr, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]); ptr+=8;
    //conio_printf( 2, row++, COLOR_WHITE, "%08X: %02X %02X %02X %02X %02X %02X %02X %02X", ptr, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]); ptr+=8;

    /* Cheat codes debug stuff : show memory for "2D0 ARUKOTOWA SANDOAHRU - JPN" cheat code. */
    ptr2 = (unsigned short*)0x06055FEC; conio_printf( 2, row  , COLOR_YELLOW, "%08X: %04X", ptr2, *ptr2);
    ptr2 = (unsigned short*)0x06055FED; conio_printf(20, row++, COLOR_YELLOW, "%08X: %04X", ptr2, *ptr2);
    ptr2 = (unsigned short*)0x0603A166; conio_printf( 2, row  , COLOR_YELLOW, "%08X: %04X", ptr2, *ptr2);
    ptr2 = (unsigned short*)0x0603A16A; conio_printf(20, row++, COLOR_YELLOW, "%08X: %04X", ptr2, *ptr2);


}


void wasca_status(int wait)
{
    pad_t* pad;
    char buff[10];
    unsigned short tmp;
    int row = 20;

    conio_printf( 2, row, COLOR_YELLOW, "DATATEST");
    conio_printf(10, row, COLOR_WHITE , ": %04X %04X %04X %04X %u"
        , REG_WASCA_SYS_DATAT1
        , REG_WASCA_SYS_DATAT2
        , REG_WASCA_SYS_DATAT3
        , REG_WASCA_SYS_DATAT4
        , rand()%10);
    row++;

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


int _wasca_status_display = 0;
void menu_vblank_clbk(void)
{
    if(_wasca_status_display)
    {
        wasca_status(0/*wait*/);
    }
    else
    {
        /* Show interrupt status. */
        interrupt_show_status();
    }

    sc_check();
}


void clear_normal_display(void)
{
    int i;

    /* Clear display. */
    for(i=4; i<27; i++)
    {
        conio_printf(2, i, COLOR_WHITE, empty_line);
    }
}

int _cartram_menu_selected = 0;

unsigned char cartram_init_test(int cs)
{
    menu_settings_t* mset   = &_menu_settings;
    menu_items_t   * mitems = &_menu_items;

    while(1)
    {
        /* Clear normal (excluding status) display. */
        clear_normal_display();

        menu_reset_settings(mset);
        menu_reset_items(mitems);
        menu_set_title(mset, "CartRam Setup");
        menu_add_item(mitems, "Config & Test"   ,  0/*tag*/);
        menu_add_item(mitems, "MemTest Only"    ,  1/*tag*/);
        menu_add_item(mitems, "SIZE: 16 bits"   ,  2/*tag*/);
        menu_add_item(mitems, "SIZE: 32 bits"   ,  3/*tag*/);
        menu_add_item(mitems, "FFh pre-fill"    ,  4/*tag*/);
        menu_add_item(mitems, "00h pre-fill"    ,  5/*tag*/);
        menu_add_item(mitems, "- Region Test -" ,  6/*tag*/);
        menu_add_item(mitems, "- ChunkTest -"   ,  7/*tag*/);
        menu_add_item(mitems, "- FeedbackTest -",  8/*tag*/);

        menu_set_callback_vblank(mset, menu_vblank_clbk);
        menu_set_pos(mset, -1/*x0*/, MENU_TOP_ROW/*y0*/, 18/*w*/, 10/*h*/, 1/*cols*/);
        menu_set_erase_on_exit(mset, 1);

        menu_set_features
        (
            mset, 
            MENU_FEATURES_TEXTCENTER
            | (MENU_FEATURES_CONDENSED_TITLE | MENU_FEATURES_CONDENSED_TEXT)
        );

        /* Show settings. */
        int row = 18;
        conio_printf(2, row++, COLOR_WHITE, "Access size : %d bits", (access_32bit ? 32 : 16));
        conio_printf(2, row++, COLOR_WHITE, "FFh pre fill : %s", (pre_fill_ff ? "ON " : "OFF"));
        conio_printf(2, row++, COLOR_WHITE, "00h pre fill : %s", (pre_fill_00 ? "ON " : "OFF"));

        _cartram_menu_selected = menu_start(mitems, mset, _cartram_menu_selected/*selected_tag*/);

        if(((mset->exit_code == MENU_EXIT_OK) && (_cartram_menu_selected == 9))
        ||  (mset->exit_code == MENU_EXIT_CANCEL))
        {
            break;
        }

        int do_mem_config = 0;
        if((_cartram_menu_selected == 0)
        || (_cartram_menu_selected == 1))
        {
            if(_cartram_menu_selected == 0)
            {
                /* Indicate that the RAM will be initialized soon. */
                ram_init_done = 1;
                do_mem_config = 1;
            }
        }
        else if(_cartram_menu_selected == 2)
        {
            access_32bit = 0;
            continue;
        }
        else if(_cartram_menu_selected == 3)
        {
            access_32bit = 1;
            continue;
        }
        else if(_cartram_menu_selected == 4)
        {
            pre_fill_ff = 1 - pre_fill_ff;
            continue;
        }
        else if(_cartram_menu_selected == 5)
        {
            pre_fill_00 = 1 - pre_fill_00;
            continue;
        }
        else if(_cartram_menu_selected == 6)
        {
            /* Test only some specific memory regions. */
            int ret = 0;
            do
            {
                ret = memtest_region();
            } while(ret != 0);
            continue;
        }
        else if(_cartram_menu_selected == 7)
        {
            /* Test memory by 1MB chunks. */
            int ret = 0;
            do
            {
                ret = memtest_chunk();
            } while(ret != 0);
            continue;
        }
        else if(_cartram_menu_selected == 8)
        {
            /* Test CS0 memory by 1MB chunks, and feedback
             * with a write to CS1 when an error happens.
             */
            int ret = 0;
            do
            {
                ret = memtest_feedback();
            } while(ret != 0);
            continue;
        }

        /* "Normal" memory test, doing only simple things 
         * and limited to 1MB/4MB expansion RAM too.
         */
        memtest_normal(cs, do_mem_config);
    }

    return cart_id;
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
        menu_add_item(mitems, "        Init Ca",  0/*tag*/);
        menu_add_item(mitems, "rtridge        ",  0/*tag*/);

        menu_add_item(mitems, "      Exit to M",  1/*tag*/);
        menu_add_item(mitems, "ultiplayer     ",  1/*tag*/);

        menu_add_item(mitems, " Memory 0.5MiB ",  2/*tag*/);
        menu_add_item(mitems, " Memory 1MiB   ",  3/*tag*/);
        menu_add_item(mitems, " Memory 2MiB   ",  4/*tag*/);
        menu_add_item(mitems, " Memory 4MiB   ",  5/*tag*/);
        menu_add_item(mitems, " Ram 1MiB      ",  6/*tag*/);
        menu_add_item(mitems, " Ram 4MiB      ",  7/*tag*/);
        menu_add_item(mitems, " KoF95.bin     ",  8/*tag*/);
        menu_add_item(mitems, " Ultraman.bin  ",  9/*tag*/);
        menu_add_item(mitems, " Boot.bin      ", 10/*tag*/);

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


void pskai_load(int full_mode)
{
    int row = 16;
    unsigned long load_length, load_ret;

    unsigned char dev_id = SC_FILE_CDROM;
    char* file_path;


    /* Load the firmware ROM temporarily to HRAM, 
     * then copy to cartridge boot area.
     */
    void* tmp_adr = (void*)0x00200000;
    void* rom_adr = (void*)0x02000000;
    file_path = "/FIRM.BIN";
    load_length = (full_mode ? 1024*1024 : 256*1024);

    conio_printf(2, row++, COLOR_WHITE, "Loading cartridge ROM ...");
    load_ret = scf_read(dev_id, file_path, 0/*offset*/, load_length, tmp_adr);
    conio_printf(2, row++, COLOR_WHITE, "Loaded %u KB", load_ret>>10);

    if(load_ret == 0)
    {
        sc_sysres();
    }

    if(!full_mode)
    {
        /* When loading lite version of the ROM, 
         * modify IP header's product name from
         * "PSKAI_F" to "PSKAI_L".
         */
        ((unsigned char*)tmp_adr)[0x26] = 'L';
    }

    /* Verify that data copied to ROM is correct. */
    memcpy(rom_adr, tmp_adr, load_length);
    if(memcmp(rom_adr, tmp_adr, load_length) != 0)
    {
        conio_printf(2, row++, COLOR_ERROR, "ERROR: copy to ROM area failed !");
        wait_for_start(NULL/*msg*/);
        return;
    }


    /* Load the lowram executable. */
    void* ram_adr = (void*)0x00200000;
    file_path = "/LOWRAM.BIN";

    conio_printf(2, row++, COLOR_WHITE, "Fetching cartridge executable ...");
    load_ret = scf_size(dev_id, file_path);
    conio_printf(2, row++, COLOR_WHITE, "Loading : %u KB", load_ret>>10);

    if(load_ret == 0)
    {
        sc_sysres();
    }

    conio_printf(2, row++, COLOR_WHITE, "Loading cartridge executable ...");
    load_ret = scf_read(dev_id, file_path, 0/*offset*/, load_length, ram_adr);
    conio_printf(2, row++, COLOR_WHITE, "Loaded %u KB", load_ret>>10);

    if(load_ret == 0)
    {
        sc_sysres();
    }


    /* Execute Pseudo Saturn Kai executable. */
    soft_reset(SC_SOFTRES_EACH_VDPS);
    sc_execute(SC_SOFTRES_ALL, (unsigned long)ram_adr/*tmp*/, load_length, (unsigned long)ram_adr/*exec*/);
}


#include "../minipseudo/minipseudo_rc.h"
void do_minipseudo(void)
{
    unsigned long crc32;

    int valid_executable = 0;

    /* Copy then execute minimalistic Pseudo Saturn.
     * Before that, ensure about integrity of mini Pseudo Saturn.
     */
    if(rc_minipseudo_romcode_check())
    {
        unsigned char* minips_src = (unsigned char*)MINIPSEUDO_DATA_START;
        unsigned char* minips_dst = (unsigned char*)MINIPSEUDO_EXEC_START;
        unsigned long  minips_len = MINIPSEUDO_DATA_LEN;

        /* Verify if executable length makes sense or not. */
        if(minips_len <= MINIPSEUDO_MAX_LEN)
        {
            crc32 = crc32_calc(minips_src, minips_len);

            if(crc32 == MINIPSEUDO_DATA_CRC)
            {
                memcpy((void*)minips_dst, (void*)minips_src, MINIPSEUDO_DATA_LEN);

                /* Indicate to execute minimalistic Pseudo Saturn. */
                valid_executable = 1;
            }
        }
    }

    if(valid_executable)
    {
        unsigned int minipseudo_ep[RC_MINIPSEUDO_ENTRIES_COUNT];
        rc_minipseudo_get_entry_points(minipseudo_ep, RC_MINIPSEUDO_ENTRIES_COUNT);

        int (*minipseudo_ep_go)(unsigned long);
        minipseudo_ep_go = (int (*)(unsigned long))(minipseudo_ep[MINIPSEUDO_GO_EP_ID]);

        /*int ret = */minipseudo_ep_go(MINIPSEUDO_COPY | MINIPSEUDO_EXEC | MINIPSEUDO_DISPLAY);
    }
}


/**
 * satcom_init
 *
 * Initialize SatCom.
**/
unsigned long _sc_workram[SC_WORKRAM_U32SIZE];
void satcom_init(int logscreen_enable)
{
    sc_initcfg_t init_cfg;

    sc_initcfg_reset(&init_cfg, _sc_workram);
    init_cfg.flags        = _cdrom_available ? 0/*default*/ : SC_INITFLAG_NOCDROM/*default*/;
    init_cfg.soft_reset   = soft_reset;
    init_cfg.logfile_name = "/WASCALDR.LOG";
    if(logscreen_enable)
    {
        init_cfg.logscreen_callback = debug_console;
    }
    sc_init_cfg(&init_cfg);

    if(_cdrom_available)
    {
        conio_title_printf(2, 2, COLOR_BG_FG(COLOR_AQUA, COLOR_YELLOW), " ~ Wasca Firmware Loader - V%d.%03d ~ ", RELEASE_ID/1000, RELEASE_ID%1000);
    }
    else
    {
        conio_title_printf(2, 2, COLOR_BG_FG(COLOR_AQUA, COLOR_YELLOW), " ~ Wasca Test Utility  --  V%d.%03d ~ ", RELEASE_ID/1000, RELEASE_ID%1000);
    }
}

void _main(void)
{
    menu_settings_t* mset   = &_menu_settings;
    menu_items_t   * mitems = &_menu_items;
    pad_t* pad;

    /* 0:main menu, 1:sub menu. */
    int main_menu_page = 0;

    /* Check if was loaded from CD-ROM or not. */
    _cdrom_available = (memcmp((void*)(0x06002060), (void*)"WASCA BOOTLOADER", 16) == 0 ? 1 : 0);

    /* Init internals for soft reset routine. */
    soft_reset_init();

    /* Init iapetus library. */
    init_iapetus();

    /**
     * Wasca Loader startup shortcut keys.
     * X : Unused.
     * Y : Show and keep debug console displayed until key release.
     * Z : Unused.
     *
     * Note : this requires SMPC_USE_POLLING mode to be used, 
     *        which have issues regarding third party (wireless)
     *        pad support.
    **/
    pad_init();
    pad = pad_read();

    int logscreen_enable  = 0;
    if(pad->new[0] & PAD_Y)
    {
        logscreen_enable = 1;
    }

    vdp_init();
    conio_init();

    /* Init console. */
    cons_init();

    /* Initialize SatCom, and display application title. */
    satcom_init(logscreen_enable);

    /* Startup finished, clear and disable screen logs. */
    if(logscreen_enable)
    {
        cons_wait_and_disable();
    }

    int menu_selected = 0;

    while(1)
    {
        /* Display main menu. */
        menu_reset_settings(mset);

        menu_set_title(mset, "Wasca Setup Menu");
        menu_set_help(mset, "A/C: Enter, B: Exit, L/R: Toggle menus");

        menu_set_callback_vblank(mset, menu_vblank_clbk);
        menu_set_features(mset, MENU_FEATURES_CHANGEPAGE_SHOULDER);

        menu_set_pos(mset, -1/*x0*/, MENU_TOP_ROW/*y0*/, 30/*w*/, 10/*h*/, 2/*cols*/);
        menu_set_erase_on_exit(mset, 0);
        menu_reset_items(mitems);

//TODO : add self test

        if(main_menu_page == 0)
        { /* Main menu. */
            menu_add_item(mitems, "   Init DRAM   ",  0/*tag*/);
            menu_add_item(mitems, "    Hex Edit   ",  1/*tag*/);

            menu_add_item(mitems, "   Rockin-B Ben",  2/*tag*/);
            menu_add_item(mitems, "chmark Tool    ",  2/*tag*/);

            menu_add_item(mitems, "      Cartridge",  3/*tag*/);
            menu_add_item(mitems, " Self Test     ",  3/*tag*/);

            menu_add_item(mitems, " Cart Bus Cfg. ",  4/*tag*/);
            menu_add_item(mitems, "  Wasca Init   ",  5/*tag*/);

            menu_add_item(mitems, " Status-single ",  6/*tag*/);
            menu_add_item(mitems, " Status-cont.  ",  7/*tag*/);

            if(_cdrom_available)
            {
#if 0 // Allow full firmware loading
                menu_add_item(mitems, "  PSKAI  lite  ", 20/*tag*/);
                menu_add_item(mitems, "  PSKAI  full  ", 21/*tag*/);
#else // Lite firmware loading only
                menu_add_item(mitems, "    Start Pseud", 20/*tag*/);
                menu_add_item(mitems, "o Saturn Kai   ", 20/*tag*/);
#endif

                menu_add_item(mitems, "   Mini Pseudo ",  90/*tag*/);
                menu_add_item(mitems, "Saturn Loader  ",  90/*tag*/);
            }

            menu_add_item(mitems, " System Reset  ",  98/*tag*/);
            menu_add_item(mitems, "  Multiplayer  ",  99/*tag*/);
        }
        else
        { /* Sub menu. */
            menu_add_item(mitems, "       Initiali",  50/*tag*/);
            menu_add_item(mitems, "ze CDROM       ",  50/*tag*/);

            menu_add_item(mitems, " Interrupt ON  ",  57/*tag*/);
            menu_add_item(mitems, " Interrupt OFF ",  58/*tag*/);

            menu_add_item(mitems, "      Exit to M",  99/*tag*/);
            menu_add_item(mitems, "ultiplayer     ",  99/*tag*/);
        }


        menu_selected = menu_start(mitems, mset, menu_selected/*selected_tag*/);

        /* System reset ? */
        if(((mset->exit_code == MENU_EXIT_OK) && (menu_selected == 98))
        ||  (mset->exit_code == MENU_EXIT_CANCEL))
        {
            sc_sysres();
        }

        /* Exit to multiplayer ? */
        if(menu_selected == 99)
        {
            sc_exit();
        }

        if((mset->exit_code == MENU_EXIT_PREVPAGE_ARROW)
        || (mset->exit_code == MENU_EXIT_NEXTPAGE_ARROW)
        || (mset->exit_code == MENU_EXIT_PREVPAGE_SHOULDER)
        || (mset->exit_code == MENU_EXIT_NEXTPAGE_SHOULDER))
        {
            /* Toggle menu page. */
            main_menu_page = (main_menu_page ? 0 : 1);
        }
        else
        {
            /* Do action. */
            menu_clear_display(mset);

            switch(menu_selected)
            {
            default:
            case(0):
                cartram_init_test(0/*CS0*/);
                break;

            case(1):
                hexedit();
                break;

            case(2):
                benchmark();
                break;

            case(3):
                cart_selftest();
                break;

            case(4):
                cart_bus_config();
                break;

            case(5):
                wasca_mode_init();
                break;

            case(6):
                wasca_status(1/*wait*/);
                break;
            case(7):
                _wasca_status_display = _wasca_status_display ? 0 : 1;
                break;

            case(57):
                /* Enable interrupts. */
                interrupt_setup(1/*enable*/);
                break;
            case(58):
                /* Disable interrupts. */
                interrupt_setup(0/*enable*/);
                break;

            case(20):
                pskai_load(0/*full*/);
                break;
            case(21):
                pskai_load(1/*full*/);
                break;

            case(50):
                _cdrom_available = 1;
                satcom_init(logscreen_enable);
                break;

            case(90):
                do_minipseudo();
                break;
            }
        }

        /* Clear normal (excluding status) display. */
        clear_normal_display();
    }
}
