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

#include "shared.h"


/* Definitions and declarations for each fields
 * A-Bus in configuration register.
 */
#define CFG_CS_CNT  2
#define CFG_FLD_CNT 8
typedef struct _cfgreg_fields_list_t
{
    char* abbr;
    char* desc;

    unsigned char shift;

    unsigned char max;
#define CFGREG_TYPE_RAW     0
#define CFGREG_TYPE_OFFON   1
#define CFGREG_TYPE_8B16B   2
    unsigned char type;

    unsigned char val;
} cfgreg_fields_list_t;

int _cfgreg_cs = 0;

cfgreg_fields_list_t _cfgreg_fields_list[CFG_CS_CNT][CFG_FLD_CNT] = 
{
{
    /* -------------- CS0 -------------- */
    {
        "A0PRD", 
        "Previous ReaD", 
        31, 1, CFGREG_TYPE_OFFON, 0x00
    },
    {
        "A0WPC", 
        "After Wr Pre-Chrg ins", 
        30, 1, CFGREG_TYPE_OFFON, 0x00
    },
    {
        "A0RPC", 
        "Read Pre-Charge insert", 
        29, 1, CFGREG_TYPE_OFFON, 0x00
    },
    {
        "A0EWT", 
        "External WaiT effective", 
        28, 1, CFGREG_TYPE_OFFON, 0x00
    },
    {
        "A0BW", 
        "Burst cycle Wait", 
        24, 15, CFGREG_TYPE_RAW, 0x00 // bit27-24
    },
    {
        "A0NW", 
        "Normal cycle Wait", 
        20, 15, CFGREG_TYPE_RAW, 0x00 // bit23-20
    },
    {
        "A0LN", 
        "Burst LeNgth", 
        18, 3, CFGREG_TYPE_RAW, 0x00 // bit19-18
    },
    {
        "A0SZ", 
        "Bus SiZe", 
        16, 1, CFGREG_TYPE_8B16B, 0x00// bit16
    },
},
{
    /* -------------- CS1 -------------- */
    {
        "A1PRD", 
        "Previous ReaD", 
        15, 1, CFGREG_TYPE_OFFON, 0x00
    },
    {
        "A1WPC", 
        "After Wr Pre-Chrg ins", 
        14, 1, CFGREG_TYPE_OFFON, 0x00
    },
    {
        "A1RPC", 
        "Read Pre-Charge insert", 
        13, 1, CFGREG_TYPE_OFFON, 0x00
    },
    {
        "A1EWT", 
        "External WaiT effective", 
        12, 1, CFGREG_TYPE_OFFON, 0x00
    },
    {
        "A1BW", 
        "Burst cycle Wait", 
        8, 15, CFGREG_TYPE_RAW, 0x00 // bit11-8
    },
    {
        "A1NW", 
        "Normal cycle Wait", 
        4, 15, CFGREG_TYPE_RAW, 0x00 // bit7-4
    },
    {
        "A1LN", 
        "Burst LeNgth", 
        2, 3, CFGREG_TYPE_RAW, 0x00 // bit3-2
    },
    {
        "A1SZ", 
        "Bus SiZe", 
        0, 1, CFGREG_TYPE_8B16B, 0x00// bit0
    },
}
};

int _cfgreg_field_id = 0;

/* The fields for AREF register. */
cfgreg_fields_list_t _aref_fields_list[2] = 
{
    {
        "ARFEN", 
        "ReFresh ENable", 
        4, 1, CFGREG_TYPE_OFFON, 0x00
    },
    {
        "ARWT", 
        "Refresh Wait Number", 
        0, 15, CFGREG_TYPE_RAW, 0x01
    },
};



/* Register work values. */
unsigned long _cart_bus_cfg_asr0 = 0;
unsigned long _cart_bus_cfg_asr1 = 0;
unsigned long _cart_bus_cfg_aref = 0;

/* Keep menu status from a function call to another. */
int _cart_bus_cfg_menusel = 100;


void cart_bus_keypress(pad_t* pad, int initial_release_wait, int stag)
{
    int cs, id;

    /* Update menu selected ID. */
    if(pad)
    {
        _cart_bus_cfg_menusel = stag;
    }

    /* Retrieve ID of field currently editing. */
    _cfgreg_field_id = (_cart_bus_cfg_menusel % 100);

    /* Modify current field value with left and right buttons. */
    if((pad)
    && (_cfgreg_field_id < (CFG_FLD_CNT+2)))
    {
        if(_cfgreg_field_id < CFG_FLD_CNT)
        {
            if(pad->delta[0] & PAD_LEFT)
            {
                if(_cfgreg_fields_list[_cfgreg_cs][_cfgreg_field_id].val != 0)
                {
                    _cfgreg_fields_list[_cfgreg_cs][_cfgreg_field_id].val--;
                }
                else if(_cfgreg_fields_list[_cfgreg_cs][_cfgreg_field_id].val == 0)
                {
                    _cfgreg_fields_list[_cfgreg_cs][_cfgreg_field_id].val = _cfgreg_fields_list[_cfgreg_cs][_cfgreg_field_id].max;
                }
            }
            else if(pad->delta[0] & PAD_RIGHT)
            {
                if(_cfgreg_fields_list[_cfgreg_cs][_cfgreg_field_id].val < _cfgreg_fields_list[_cfgreg_cs][_cfgreg_field_id].max)
                {
                    _cfgreg_fields_list[_cfgreg_cs][_cfgreg_field_id].val++;
                }
                else if(_cfgreg_fields_list[_cfgreg_cs][_cfgreg_field_id].val == _cfgreg_fields_list[_cfgreg_cs][_cfgreg_field_id].max)
                {
                    _cfgreg_fields_list[_cfgreg_cs][_cfgreg_field_id].val = 0;
                }
            }
        }
        else
        {
            /* Same stuff for AREF register. */
            if(pad->delta[0] & PAD_LEFT)
            {
                if(_aref_fields_list[_cfgreg_field_id-CFG_FLD_CNT].val != 0)
                {
                    _aref_fields_list[_cfgreg_field_id-CFG_FLD_CNT].val--;
                }
                else if(_aref_fields_list[_cfgreg_field_id-CFG_FLD_CNT].val == 0)
                {
                    _aref_fields_list[_cfgreg_field_id-CFG_FLD_CNT].val = _aref_fields_list[_cfgreg_field_id-CFG_FLD_CNT].max;
                }
            }
            else if(pad->delta[0] & PAD_RIGHT)
            {
                if(_aref_fields_list[_cfgreg_field_id-CFG_FLD_CNT].val < _aref_fields_list[_cfgreg_field_id-CFG_FLD_CNT].max)
                {
                    _aref_fields_list[_cfgreg_field_id-CFG_FLD_CNT].val++;
                }
                else if(_aref_fields_list[_cfgreg_field_id-CFG_FLD_CNT].val == _aref_fields_list[_cfgreg_field_id-CFG_FLD_CNT].max)
                {
                    _aref_fields_list[_cfgreg_field_id-CFG_FLD_CNT].val = 0;
                }
            }
        }
    }

    /* Update each configuration registers. */
    //#define SCU_BASE 0x25FE0000                                   INIT VAL
    //#define ASR0    (*(volatile unsigned long*)(SCU_BASE+0xb0))   00000001
    //#define ASR1    (*(volatile unsigned long*)(SCU_BASE+0xb4))   ????????
    //#define AREF    (*(volatile unsigned long*)(SCU_BASE+0xb8))   00000001
    _cart_bus_cfg_asr0 = 0;
    for(cs=0; cs<CFG_CS_CNT; cs++)
    {
        for(id=0; id<CFG_FLD_CNT; id++)
        {
            unsigned long val = _cfgreg_fields_list[cs][id].val;
            _cart_bus_cfg_asr0 |= val << _cfgreg_fields_list[cs][id].shift;
        }
    }
    _cart_bus_cfg_aref = 0;
    for(id=0; id<2; id++)
    {
        unsigned long val = _aref_fields_list[id].val;
        _cart_bus_cfg_aref |= val << _aref_fields_list[id].shift;
    }

    /* Display registers raw values. */
    int row = CFG_FLD_CNT+1;
    conio_font_printf(
        FONT_HEXA, FONT_CODEPAGE_JAPANESE, 
        (14*MENU_CHAR_W)+( 9*CONIO_CHAR_W), 4*CONIO_CHAR_H+MENU_CHAR_H*(row)+2, 
        COLOR_YELLOW, 
        "%s", "ASR0:");
    conio_font_printf(
        FONT_HEXA, FONT_CODEPAGE_JAPANESE, 
        (14*MENU_CHAR_W)+(14*CONIO_CHAR_W), 4*CONIO_CHAR_H+MENU_CHAR_H*(row)+2, 
        COLOR_WHITE, 
        "%08X", _cart_bus_cfg_asr0);

    row++;
    conio_font_printf(
        FONT_HEXA, FONT_CODEPAGE_JAPANESE, 
        (14*MENU_CHAR_W)+( 9*CONIO_CHAR_W), 4*CONIO_CHAR_H+MENU_CHAR_H*(row)+2, 
        COLOR_YELLOW, 
        "%s", "AREF:");
    conio_font_printf(
        FONT_HEXA, FONT_CODEPAGE_JAPANESE, 
        (14*MENU_CHAR_W)+(14*CONIO_CHAR_W), 4*CONIO_CHAR_H+MENU_CHAR_H*(row)+2, 
        COLOR_WHITE, 
        "%08X", _cart_bus_cfg_aref);

    /* Register fields values. */
    for(id=0; id<CFG_FLD_CNT; id++)
    {
        char sel[2];
        unsigned long val = _cfgreg_fields_list[_cfgreg_cs][id].val;
        unsigned char type = _cfgreg_fields_list[_cfgreg_cs][id].type;

        /* Show arrows character around digit currently modifying. */
        sel[0] = ' '; sel[1] = ' ';
        if(id == _cfgreg_field_id)
        {
            sel[0] = 26;
            sel[1] = 27;
        }

        /* Show field value. */
        switch(type)
        {
        default:
        case(CFGREG_TYPE_RAW):
            conio_font_printf(
                FONT_DEFAULT, FONT_CODEPAGE_JAPANESE, 
                14*MENU_CHAR_W, 4*CONIO_CHAR_H+MENU_CHAR_H*(1+id)+2, 
                COLOR_WHITE, 
                "%c%2u%c", 
                sel[0], val, sel[1]);
            break;

        case(CFGREG_TYPE_OFFON):
            conio_font_printf(
                FONT_DEFAULT, FONT_CODEPAGE_JAPANESE, 
                14*MENU_CHAR_W, 4*CONIO_CHAR_H+MENU_CHAR_H*(1+id)+2, 
                COLOR_WHITE, 
                "%c%s", 
                sel[0], (val ? "ON " : "OFF"));
            break;

        case(CFGREG_TYPE_8B16B):
            conio_font_printf(
                FONT_DEFAULT, FONT_CODEPAGE_JAPANESE, 
                14*MENU_CHAR_W, 4*CONIO_CHAR_H+MENU_CHAR_H*(1+id)+2, 
                COLOR_WHITE, 
                "%c%2u", 
                sel[0], (val ? 8 : 16));
            break;
        }

        /* Show field description. */
        conio_font_printf(
            FONT_OLDSCHOOL, FONT_CODEPAGE_JAPANESE, 
            18*MENU_CHAR_W, 4*CONIO_CHAR_H+MENU_CHAR_H*(1+id)+2, 
            COLOR_WHITE, 
            "%s", 
            _cfgreg_fields_list[_cfgreg_cs][id].desc);
    }

    /* Do stuff for AREF register too.
     * (That's basically the same stuff as above ...)
     */
    for(id=0; id<2; id++)
    {
        char sel[2];
        unsigned long val = _aref_fields_list[id].val;
        unsigned char type = _aref_fields_list[id].type;

        /* Show arrows character around digit currently modifying. */
        sel[0] = ' '; sel[1] = ' ';
        if((CFG_FLD_CNT+0+id) == _cfgreg_field_id)
        {
            sel[0] = 26;
            sel[1] = 27;
        }

        /* Show field value. */
        switch(type)
        {
        default:
        case(CFGREG_TYPE_RAW):
            conio_font_printf(
                FONT_DEFAULT, FONT_CODEPAGE_JAPANESE, 
                14*MENU_CHAR_W, 4*CONIO_CHAR_H+MENU_CHAR_H*(1+CFG_FLD_CNT+1+id)+2, 
                COLOR_WHITE, 
                "%c%2u%c", 
                sel[0], val, sel[1]);
            break;

        case(CFGREG_TYPE_OFFON):
            conio_font_printf(
                FONT_DEFAULT, FONT_CODEPAGE_JAPANESE, 
                14*MENU_CHAR_W, 4*CONIO_CHAR_H+MENU_CHAR_H*(1+CFG_FLD_CNT+1+id)+2, 
                COLOR_WHITE, 
                "%c%s", 
                sel[0], (val ? "ON " : "OFF"));
            break;

        case(CFGREG_TYPE_8B16B):
            conio_font_printf(
                FONT_DEFAULT, FONT_CODEPAGE_JAPANESE, 
                14*MENU_CHAR_W, 4*CONIO_CHAR_H+MENU_CHAR_H*(1+CFG_FLD_CNT+1+id)+2, 
                COLOR_WHITE, 
                "%c%2u", 
                sel[0], (val ? 8 : 16));
            break;
        }

        /* Don't show first field description, because this line
         * is used for register raw value.
         */
        if(id != 0)
        {
            conio_font_printf(
                FONT_OLDSCHOOL, FONT_CODEPAGE_JAPANESE, 
                18*MENU_CHAR_W, 4*CONIO_CHAR_H+MENU_CHAR_H*(1+CFG_FLD_CNT+1+id)+2, 
                COLOR_WHITE, 
                "%s", 
                _aref_fields_list[id].desc);
        }
    }


}


void cart_bus_config(void)
{
    menu_settings_t* mset   = &_menu_settings;
    menu_items_t   * mitems = &_menu_items;
    int i;

    /* Clear normal (excluding status) display. */
    clear_normal_display();

    /* Initialize display. */
    _cfgreg_field_id = _cart_bus_cfg_menusel % 100;
    cart_bus_keypress(NULL, 0, 0);

    while(1)
    {
        menu_reset_settings(mset);
        menu_set_callback_vblank(mset, menu_vblank_clbk);
        menu_set_callback_keypress(mset, cart_bus_keypress);

        char menu_title[32];
        sprintf(menu_title, "- CS%d -", _cfgreg_cs);
        menu_set_title(mset, menu_title);
        menu_set_help(mset, "A/C: Apply, B: Exit, L/R: Toggle CS");

        menu_set_pos(mset, 3*CONIO_CHAR_W/*x0*/, 4*CONIO_CHAR_H/*y0*/, 11/*w*/, 0/*h*/, 1/*cols*/);
        menu_set_erase_on_exit(mset, 0);

        menu_set_features(mset, MENU_FEATURES_CHANGEPAGE_SHOULDER | MENU_FEATURES_TEXTCENTER);

        menu_reset_items(mitems);
        for(i=0; i<CFG_FLD_CNT; i++)
        {
            menu_add_item(mitems, _cfgreg_fields_list[_cfgreg_cs][i].abbr, 100+i/*tag*/);
        }

        /* Add separator for other register. */
        menu_add_item(mitems, "-AREF REG.-", MENU_TAG_GREYOUT/*tag*/);
        menu_add_item(mitems, "ARFEN", 100+CFG_FLD_CNT + 0/*tag*/);
        menu_add_item(mitems, "ARWT ", 100+CFG_FLD_CNT + 1/*tag*/);

        _cart_bus_cfg_menusel = menu_start(mitems, mset, _cart_bus_cfg_menusel/*selected_tag*/);

        if(mset->exit_code == MENU_EXIT_CANCEL)
        {
            //clear_screen();
            return;
        }

        /* Update CS selection with shoulder buttons. */
        if(mset->exit_code == MENU_EXIT_NEXTPAGE_SHOULDER)
        {
            _cfgreg_cs = (_cfgreg_cs+1) % CFG_CS_CNT;
            cart_bus_keypress(NULL, 0, 0);
        }
        else if(mset->exit_code == MENU_EXIT_PREVPAGE_SHOULDER)
        {
            _cfgreg_cs = (_cfgreg_cs+(CFG_CS_CNT-1)) % CFG_CS_CNT;
            cart_bus_keypress(NULL, 0, 0);
        }
        else
        {
            /* Apply A-Bus configuration. */
            ASR0 = _cart_bus_cfg_asr0;
            AREF = _cart_bus_cfg_aref;
        }
    }
}


