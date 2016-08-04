/**
 *  Cartridge Test Utility
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/pskai/
 *
 *  See LICENSE file for details.
**/

#include <stdio.h>
#include <string.h> /* For memset,memcpy */

#include "menu.h"

#include "conio.h"
#include "smpc.h"
#include "vdp.h"

#define BORD_LEFT    1
#define BORD_RIGHT   2
#define BORD_TOP     4
#define BORD_BOTTOM  8


/**
 * Global variables, declared in menu.c
**/
menu_items_t    _menu_items;
menu_settings_t _menu_settings;



extern unsigned char conio_font[];
void menu_printchar(int x, int y, unsigned short color, char v, unsigned char border_flags, unsigned char border_color, unsigned char mesh, unsigned char color_mesh)
{
    int r, c;
    volatile unsigned char *bmp = (volatile unsigned char *)VDP2_VRAM;
    unsigned char fg = (color >> 0) & 0xFF;
    unsigned char bg = (color >> 8) & 0xFF;
    unsigned char d;

    for(r = 0; r < 8; r++)
    {
        unsigned char b;
        b = conio_font[v<<3|r];
        for(c = 0; c < 8; c++)
        {
            int foregnd = b & (0x80 >> c);
            if(mesh)
            {
                if(foregnd ? 1 : (y+r + x+c)%2)
                {
                    /* Mesh, but use normal color. */
                    d = foregnd ? fg : bg;
                }
                else
                {
                    /* Use mesh color. */
                    d = color_mesh;
                }
            }
            else
            {
                /* No mesh. */
                d = foregnd ? fg : bg;
            }
            bmp[((y + r+1)<<10) + (x + c+1)] = d;
        }            
    }

    /* Reset borders. */
    d = bg;
    for(c = 0; c < 9; c++)
    {
        r = 0;
        if(mesh)
        {
            d = ((y+r + x+c)%2 ? bg : color_mesh);
        }
        bmp[((y + r)<<10) + (x + c)] = d;
    }
    for(r = 0; r < 9; r++)
    {
        c = 0;
        if(mesh)
        {
            d = ((y+r + x+c)%2 ? bg : color_mesh);
        }
        bmp[((y + r)<<10) + (x + c)] = d;
    }
    /* If needed, display borders. */
    d = border_color;
    for(c = 0; c < 10; c++)
    {
        if(border_flags & BORD_TOP   ) { r = 0; bmp[((y + r)<<10) + (x + c)] = d; }
        if(border_flags & BORD_BOTTOM) { r = 9; bmp[((y + r)<<10) + (x + c)] = d; }
    }
    for(r = 0; r < 10; r++)
    {
        if(border_flags & BORD_LEFT  ) { c = 0; bmp[((y + r)<<10) + (x + c)] = d; }
        if(border_flags & BORD_RIGHT ) { c = 9; bmp[((y + r)<<10) + (x + c)] = d; }
    }
}

void menu_prinstr_mesh(int x, int y, int w, unsigned short color, unsigned char border_flags, unsigned char border_color, unsigned char mesh, unsigned char color_mesh, int center, char* str, int str_len)
{
    int n = str_len;
    int i;
    unsigned char bf;
    int col_offset;

    /* If needed, get string length. */
    if(n == 0)
    {
        n = strlen(str);
    }

    col_offset = 0;
    if(center)
    {
        col_offset = (w - n)/2;
        if(col_offset < 0) col_offset = 0;
    }

    for(i=0; i<w; i++)
    {
        char v = ' ';
        if(i >= col_offset)
        {
            v = ((i-col_offset)<n ? str[i-col_offset] : ' ');
        }
        if(i == 0)
        {
            bf = border_flags & (~BORD_RIGHT);
        }
        else if(i == (w-1))
        {
            bf = border_flags & (~BORD_LEFT);
        }
        else
        {
            bf = border_flags & (~BORD_LEFT) & (~BORD_RIGHT);
        }
        menu_printchar(x + i*MENU_CHAR_W, y, color, v, bf, border_color, mesh, color_mesh);
    }
}
void menu_prinstr(int x, int y, int w, unsigned short color, unsigned char border_flags, unsigned char border_color, char* str, int str_len)
{
    menu_prinstr_mesh(x, y, w, color, border_flags, border_color, 0/*mesh*/, COLOR_WHITE/*color_mesh*/, 0/*center*/, str, str_len);
}




void menu_reset_settings(menu_settings_t* s)
{
    memset((void*)s, 0, sizeof(menu_settings_t));

    /* Set default menu position and size. */
    s->x0   = -1;
    s->y0   = -1;
    s->w    = 30;
    s->h    =  9;
    s->cols =  1;

    /* Reset title. */
    s->title = NULL;

    /* Erase screen on exit. */
    s->erase_screen = 1;

    /* Disable callback functions and features. */
    s->callback_redraw = NULL;
    s->callback_vblank = NULL;
    s->features = MENU_FEATURES_NONE;

    /* Set typical colors for menu display. */
    s->color_border           = COLOR_BLUE; // only foreground.
    s->color_border_highlight = COLOR_RED ; // only foreground.
    s->color_title            = COLOR_BG_FG(COLOR_YELLOW, COLOR_RED  );
    s->color_default          = COLOR_BG_FG(COLOR_WHITE , COLOR_BLACK);
    s->color_highlight        = COLOR_BG_FG(COLOR_OLIVE , COLOR_BLACK); // only background.
    s->color_greyout          = COLOR_BG_FG(COLOR_SILVER, COLOR_WHITE);
    s->color_selection        = COLOR_BG_FG(COLOR_WHITE , COLOR_LIME );
}

void menu_set_title(menu_settings_t* s, char* title)
{
    s->title = title;
}

void menu_set_erase_on_exit(menu_settings_t* s, int erase_screen)
{
    s->erase_screen = erase_screen;
}

void menu_set_features(menu_settings_t* s, unsigned short f)
{
    s->features = f;
}

void menu_reset_items(menu_items_t* m)
{
    int i;

    memset((void*)m, 0, sizeof(menu_items_t));

    /* Reset all items. */
    for(i=0; i<MENU_ITEMS_COUNT; i++)
    {
        m->it[i].txt     = NULL;
        m->it[i].txt_len = 0;
        m->it[i].tag     = MENU_TAG_DISABLED;
        m->it[i].color   = 0; /* default. */
    }
}

void menu_set_item(menu_items_t* m, int id, char* txt, int tag)
{
    m->it[id].txt     = txt;
    m->it[id].txt_len = 0;
    m->it[id].tag     = tag;
}
void menu_set_item_color(menu_items_t* m, int id, unsigned short color)
{
    m->it[id].color = color;
}

void menu_disable_item(menu_items_t* m, int tag)
{
    int i;

    for(i=0; i<MENU_ITEMS_COUNT; i++)
    {
        if(m->it[i].tag == tag)
        {
            m->it[i].disabled = 1;
        }
    }
}

void menu_set_pos(menu_settings_t* s, int x0, int y0, int w, int h, int cols)
{
    s->x0   = x0  ;
    s->y0   = y0  ;
    s->w    = w   ;
    s->h    = h   ;
    s->cols = cols;
}


void menu_set_callback_redraw(menu_settings_t* s, Fct_menu_callback_redraw fct)
{
    s->callback_redraw = fct;
}


void menu_set_callback_vblank(menu_settings_t* s, Fct_menu_callback_vblank fct)
{
    s->callback_vblank = fct;
}


void menu_set_callback_getstr(menu_settings_t* s, Fct_menu_callback_getstr fct)
{
    s->callback_getstr = fct;
}


void menu_clear_display(menu_settings_t* s)
{
    int x, y;
    for(x=0; x<s->w; x++)
    {
        for(y=0; y<s->h; y++)
        {
            menu_printchar(s->x0 + MENU_CHAR_W*x, s->y0 + MENU_CHAR_H*y, COLOR_BG_FG(COLOR_TRANSPARENT, COLOR_TRANSPARENT), ' ', BORD_RIGHT|BORD_BOTTOM/*border_flags*/, /*border_color*/COLOR_TRANSPARENT, 0/*mesh*/, COLOR_TRANSPARENT/*color_mesh*/);
        }
    }
}


void menu_display_item(menu_settings_t* s, menu_items_t* m, int r, int c, int item_id, int select_item_tag, int items_count, int bord)
{
    unsigned short color = s->color_default;
    unsigned char color_border = s->color_border;
    unsigned char color_mesh = COLOR_BLACK;
    unsigned char mesh = 0;
    if(item_id < items_count)
    {
        /* If defined, use custom color. */
        color = (m->it[item_id].color != 0 ? m->it[item_id].color : s->color_default);

        if(m->it[item_id].tag == select_item_tag)
        {
            /* When item is highlighted, display its background
             * in a different color. */
            color_mesh = (color >> 8) & 0xFF;
            color = (s->color_highlight & 0xFF00) | (color & 0x00FF);
            mesh  = 1;
            /* When item is highlighted, display its border color
             * in a different color. */
            color_border = s->color_border_highlight;
        }

        /* Disabled item ? */
        if(m->it[item_id].disabled)
        {
            color_mesh = (s->color_greyout >> 8) & 0xFF;
            mesh  = 1;
        }

        /* Special case when the menu item is "greyout". */
        if(m->it[item_id].tag == MENU_TAG_GREYOUT)
        {
            color = s->color_greyout;
        }
    }
    unsigned char border_flags = bord;
    if(c ==            0 ) border_flags |= BORD_LEFT;
    if(c == (s->cols - 1)) border_flags |= BORD_RIGHT;
    if(r ==            0 ) border_flags |= BORD_TOP;
    if(r == (s->h -    2)) border_flags |= BORD_BOTTOM;
    char* str = "";
    int str_len = 0;
    if(m->it[item_id].tag != MENU_TAG_DISABLED)
    { /* Don't display disabled entries. */
        if(item_id < items_count)
        {
            str = m->it[item_id].txt;
            str_len = m->it[item_id].txt_len;
        }
    }

    int center = 0;
    if(s->features & MENU_FEATURES_TEXTCENTER)
    {
        center = 1;
    }

    menu_prinstr_mesh(s->x0 + MENU_CHAR_W*(c*(s->w/s->cols)), s->y0 + MENU_CHAR_W*(r+1), s->w/s->cols, color, border_flags, color_border, mesh, color_mesh, center, str, str_len);
}
void menu_display(menu_settings_t* s, menu_items_t* m, int select_item_tag, int items_count, int* cursor_col, int* cursor_row)
{
    int i, c, d;
    int bord;

    /* If needed, compute top-left positions for center-justfied menus. */
    if(s->x0 < 0)
    {
        s->x0 = (320 - s->w * MENU_CHAR_W) / 2;
    }
    if(s->y0 < 0)
    {
        s->y0 = (224 - s->h * MENU_CHAR_H) / 2;
    }

    /* Clear, then display title area. */
    menu_prinstr(s->x0, s->y0, s->w, s->color_title, BORD_LEFT | BORD_RIGHT | BORD_TOP | BORD_BOTTOM, s->color_border, "", 0);
    if(s->title)
    {
        /* Display title in center of menu. */
        int n = strlen(s->title);
        int offs = (s->w - n) / 2;
        menu_prinstr(s->x0 + offs*MENU_CHAR_W, s->y0, s->w - offs, s->color_title, BORD_RIGHT | BORD_TOP | BORD_BOTTOM, s->color_border, s->title, 0);
    }

    *cursor_col = 0;
    *cursor_row = 0;

    /* Draw menu twice : whole menu, then selected items only.
     * This is in order to draw border of highlighted item correctly.
     */
    for(d=0; d<2; d++)
    {
        for(c=s->cols-1; c>=0; c--)
        {
            for(i=(s->h-1)-1; i>=0; i--)
            {
                int sel = (m->it[i*s->cols + c].tag == select_item_tag ? 1 : 0);

                if(((d==0) && (!sel)) || ((d==1) && (sel)))
                {
                    bord = 0;
                    if(c!=0)
                    {
                        bord |= (m->it[i*s->cols + (c-1)].tag != m->it[i*s->cols + c].tag ? BORD_LEFT : 0);
                    }
                    if(i!=0)
                    {
                        bord |= (m->it[(i-1)*s->cols + c].tag != m->it[i*s->cols + c].tag ? BORD_TOP  : 0);
                    }

                    if(sel)
                    {
                        bord |= (m->it[i*s->cols + (c+1)].tag != m->it[i*s->cols + c].tag ? BORD_RIGHT  : 0);
                        bord |= (m->it[(i+1)*s->cols + c].tag != m->it[i*s->cols + c].tag ? BORD_BOTTOM : 0);
                    }

                    menu_display_item(s, m, i/*r*/, c/*cols*/, i*s->cols + c/*item_id*/, select_item_tag, items_count, bord);
                    if(sel)
                    {
                        *cursor_col = c;
                        *cursor_row = i;
                    }
                }
            }
        }
    }

    /* If declared, call end-user's callback */
    if(s->callback_redraw)
    {
        s->callback_redraw(select_item_tag);
    }
}

/* menu_list_start internal function. */
int list_get_disp_offset(int sel_id, int items_count, menu_settings_t* s)
{
    /* Compute display offset : basically, 
     * at the middle of list display area. */
    int disp_offset = sel_id - ((s->h-1)/2);
    /* Case of end of list. */
    if((disp_offset + (s->h-1)) > items_count)
    {
        disp_offset = items_count - (s->h-1);
    }
    /* Case of begining of list. */
    if(disp_offset < 0)
    {
        disp_offset = 0;
    }

    return disp_offset;
}
/* menu_list_start internal function. */
menu_items_t _list_items;
void list_fill_items(char** items, int* menu_selected, int items_count, menu_settings_t* s, int disp_offset)
{
    int i;

    /* Fill display parameters. */
    for(i=0; i<(s->h-1); i++)
    {
        if((disp_offset + i) < items_count)
        {
            if(s->callback_getstr)
            {
                s->callback_getstr(disp_offset+i, &(_list_items.it[i].txt), &(_list_items.it[i].txt_len));
            }
            else
            {
                _list_items.it[i].txt = items[disp_offset + i];
                _list_items.it[i].txt_len = 0;
            }
            _list_items.it[i].tag = disp_offset + i;
        }
        else
        {
            _list_items.it[i].txt = "";
            _list_items.it[i].txt_len = 0;
            _list_items.it[i].tag = MENU_TAG_DISABLED;
        }
        /* Set item color. */
        if(menu_selected)
        {
            _list_items.it[i].color = menu_selected[disp_offset + i] ? s->color_selection : s->color_default;
        }
        else
        {
            /* Don't forget the case selection feature is not used. */
            _list_items.it[i].color = s->color_default;
        }
    }
}

int menu_list_start(char* items[], int items_count, menu_settings_t* s, int current_id, int* menu_selected)
{
    int sel_id = current_id;
    int cursor_col, cursor_row;
    pad_t* pad;
    int disp_offset;
    int sel_use = (menu_selected ? 1 : 0);
    int refresh_display = 0;

    s->exit_code = MENU_EXIT_OK;

    /* Force column count to 1. Only list can be
     * displayed with this function.
     */
    s->cols = 1;

    /* Compute initial value of display offset, etc. */
    if(MENU_TAG_INVALID(sel_id))
    {
        sel_id = 0;
    }
    if(sel_id < 0) sel_id = 0;
    if(sel_id > (items_count-1)) sel_id = items_count-1;

    /* Display menu on startup. */
    refresh_display = 1;

    /* Wait until user releases pad buttons. */
    while(1)
    {
        /* Wait for V-Blank. */
        WaitForVBLANKIn();
        WaitForVBLANKOut();

        /* If declared, call end-user's callback */
        if(s->callback_vblank)
        {
            s->callback_vblank();
        }

        /* Poll joypad */
        pad = pad_read();

        if(((pad->new[0] & PAD_A    ) == 0)
        && ((pad->new[0] & PAD_B    ) == 0)
        && ((pad->new[0] & PAD_C    ) == 0)
        && ((pad->new[0] & PAD_START) == 0))
        {
            break;
        }

    }

    while(1)
    {
        /* Wait for V-Blank. */
        WaitForVBLANKIn();
        WaitForVBLANKOut();

        /* If declared, call end-user's callback */
        if(s->callback_vblank)
        {
            s->callback_vblank();
        }

        if(refresh_display)
        {
            disp_offset = list_get_disp_offset(sel_id, items_count, s);
            list_fill_items(items, menu_selected, items_count, s, disp_offset);
            menu_display(s, &_list_items, sel_id, items_count, &cursor_col, &cursor_row);

            /* Reset display flag. */
            refresh_display = 0;
        }

        /* Poll joypad */
        pad = pad_read();

        /* Quit program ? */
        if((pad->new[0] & PAD_A) && (pad->new[0] & PAD_B) && (pad->new[0] & PAD_C) && (pad->new[0] & PAD_START))
        { /* Exit to multiplayer. */
            /* Exit to multiplayer. */
            (**(void(**)(void))0x600026C)();
        }

        if((pad->delta[0] & PAD_UP   ) || (pad->delta[0] & PAD_DOWN )
        || (pad->delta[0] & PAD_C    )
        || (pad->delta[0] & PAD_X    )
        || (pad->delta[0] & PAD_Y    )
        || (pad->delta[0] & PAD_Z    ))
        {
            int new_id = sel_id;

            if((pad->delta[0] & PAD_UP) && (new_id >= 0))
            {
                new_id--;
                if(new_id < 0)
                {
                    new_id = items_count-1;
                }
            }
            else if((pad->delta[0] & PAD_DOWN) && (new_id >= 0))
            {
                new_id++;
                if(new_id >= items_count)
                {
                    new_id = 0;
                }
            }
            /* Verify if we need to update display or not. */
            if(new_id != sel_id)
            {
                sel_id = new_id;
                refresh_display = 1;
            }

            if(sel_use)
            {
                if(((pad->delta[0] & PAD_C))
                || ((pad->delta[0] & PAD_X))
                || ((pad->delta[0] & PAD_Y))
                || ((pad->delta[0] & PAD_Z)))
                {
                    menu_selected[sel_id] = 1-menu_selected[sel_id];
                    refresh_display = 1;
                }
            }
        }

        if(((pad->new[0] & PAD_A      ) == 0)
        && ((pad->new[0] & PAD_B      ) == 0)
        //&& ((pad->new[0] & PAD_C      ) == 0)
        && ((pad->new[0] & PAD_START  ) == 0)
        && ((pad->new[0] & PAD_LEFT   ) == 0)
        && ((pad->new[0] & PAD_RIGHT  ) == 0)
        && ((pad->new[0] & PAD_LBUTTON) == 0)
        && ((pad->new[0] & PAD_RBUTTON) == 0))
        {
            if((pad->old[0] & PAD_A) /*|| (pad->old[0] & PAD_C)*/ || (pad->old[0] & PAD_START))
            { /* Confirm. */
                if((sel_use) && (sel_id >= 0))
                {
                    /* Mark this item as selected. */
                    menu_selected[sel_id] = 1;
                }
                s->exit_code = MENU_EXIT_OK;
                break;
            }
            if(pad->old[0] & PAD_B)
            { /* Cancel. */
                sel_id = current_id;
                s->exit_code = MENU_EXIT_CANCEL;
                break;
            }

            if(s->features & MENU_FEATURES_CHANGEPAGE)
            {
                if((pad->old[0] & PAD_LEFT) || (pad->old[0] & PAD_RIGHT))
                { /* Change page#1. */
                    s->exit_code = (pad->old[0] & PAD_LEFT ? MENU_EXIT_PREVPAGE1 : MENU_EXIT_NEXTPAGE1);
                    break;
                }
                if((pad->old[0] & PAD_LBUTTON) || (pad->old[0] & PAD_RBUTTON))
                { /* Change page#2. */
                    s->exit_code = (pad->old[0] & PAD_LBUTTON ? MENU_EXIT_PREVPAGE2 : MENU_EXIT_NEXTPAGE2);
                    break;
                }
            }
        }
    }

    if((s->erase_screen)
    && (s->exit_code != MENU_EXIT_PREVPAGE1)
    && (s->exit_code != MENU_EXIT_NEXTPAGE1)
    && (s->exit_code != MENU_EXIT_PREVPAGE2)
    && (s->exit_code != MENU_EXIT_NEXTPAGE2))
    {
        menu_clear_display(s);
    }

    return sel_id;
}



int menu_start(menu_items_t* m, menu_settings_t* s, int current_tag)
{
    int i, items_count;
    int first_tag;
    int stag = current_tag;
    int stag_candidate;
    int cursor_col, cursor_row;
    pad_t* pad;

    s->exit_code = MENU_EXIT_OK;

    items_count = s->cols * (s->h-1);

    /* Mark blank entries as disabled. */
    first_tag = 0;
    for(i=items_count-1; i>=0; i--)
    {
        if(m->it[i].txt == NULL)
        {
            m->it[i].tag = MENU_TAG_DISABLED;
        }

        /* Look for the first valid tag. */
        if((MENU_TAG_VALID(m->it[i].tag))
        && (!(m->it[i].disabled)))
        {
            first_tag = m->it[i].tag;
        }
    }

    /* Avoid invalid initial tag selection. */
    if(MENU_TAG_INVALID(current_tag))
    {
        stag = first_tag;
    }
    /* Avoid disabled initial tag selection. */
    for(i=items_count-1; i>=0; i--)
    {
        if((m->it[i].tag == current_tag)
        && (m->it[i].disabled))
        {
            stag = first_tag;
        }
    }

    //if(s->erase_screen) menu_clear_display(p);
    menu_display(s, m, stag, items_count, &cursor_col, &cursor_row);

    /* Wait until user releases pad buttons. */
    while(1)
    {
        /* Wait for V-Blank. */
        WaitForVBLANKIn();
        WaitForVBLANKOut();

        /* If declared, call end-user's callback */
        if(s->callback_vblank)
        {
            s->callback_vblank();
        }

        /* Poll joypad */
        pad = pad_read();

        if(((pad->new[0] & PAD_A    ) == 0)
        && ((pad->new[0] & PAD_B    ) == 0)
        && ((pad->new[0] & PAD_C    ) == 0)
        && ((pad->new[0] & PAD_START) == 0))
        {
            break;
        }

    }


    while(1)
    {
        /* Wait for V-Blank. */
        WaitForVBLANKIn();
        WaitForVBLANKOut();

        /* If declared, call end-user's callback */
        if(s->callback_vblank)
        {
            s->callback_vblank();
        }

        /* Poll joypad */
        pad = pad_read();

        /* Quit program ? */
        if((pad->new[0] & PAD_A) && (pad->new[0] & PAD_B) && (pad->new[0] & PAD_C) && (pad->new[0] & PAD_START))
        { /* Exit to multiplayer. */
            /* Exit to multiplayer. */
            (**(void(**)(void))0x600026C)();
        }

        if((pad->delta[0] & PAD_UP  ) || (pad->delta[0] & PAD_DOWN )
        || (pad->delta[0] & PAD_LEFT) || (pad->delta[0] & PAD_RIGHT))
        {
            int tag_max, tag_min;
            int id_start, id_end, id_pitch;
            int f = 1;

            if((pad->delta[0] & PAD_UP) || (pad->delta[0] & PAD_DOWN))
            {
                id_start =        0 * s->cols + cursor_col;
                id_end   = (s->h-1) * s->cols + cursor_col;
                id_pitch = s->cols;
            }
            else
            {
                id_start = cursor_row * s->cols + 0;
                id_end   = cursor_row * s->cols + s->cols;
                id_pitch = 1;
            }

            /* Get tag boundaries. */
            tag_max = m->it[0].tag;
            tag_min = m->it[0].tag;
            for(i=id_start; i<id_end; i+=id_pitch)
            {
                if(MENU_TAG_VALID(m->it[i].tag)
                && (!(m->it[i].disabled)))
                {
                    if(f)
                    {
                        tag_max = m->it[i].tag;
                        tag_min = m->it[i].tag;
                        f=0;
                    }
                    if(m->it[i].tag > tag_max) { tag_max = m->it[i].tag; }
                    if(m->it[i].tag < tag_min) { tag_min = m->it[i].tag; }
                }
            }

            /* Look for the nearest tag. */
            stag_candidate = ((pad->delta[0] & PAD_UP) || (pad->delta[0] & PAD_LEFT) ? tag_min : tag_max);
            for(i=id_start; i<id_end; i+=id_pitch)
            {
                if(MENU_TAG_VALID(m->it[i].tag)
                && (!(m->it[i].disabled)))
                {
                    if((pad->delta[0] & PAD_DOWN) || (pad->delta[0] & PAD_RIGHT))
                    {
                        if((m->it[i].tag > stag) && (m->it[i].tag < stag_candidate))
                        {
                            stag_candidate = m->it[i].tag;
                        }
                    }
                    if((pad->delta[0] & PAD_UP) || (pad->delta[0] & PAD_LEFT))
                    {
                        if((m->it[i].tag < stag) && (m->it[i].tag > stag_candidate))
                        {
                            stag_candidate = m->it[i].tag;
                        }
                    }
                }
            }
            /* No change ? If so go to the opposite tag or the next page. */
            int perform_refresh = 1;
            if(stag_candidate == stag)
            {
                stag_candidate = ((pad->delta[0] & PAD_UP) || (pad->delta[0] & PAD_LEFT) ? tag_max : tag_min);
            }
            /* If needed, refresh display. */
            if((perform_refresh) && (stag_candidate != stag))
            {
                stag = stag_candidate;
                menu_display(s, m, stag, items_count, &cursor_col, &cursor_row);
            }
        }

        if(((pad->new[0] & PAD_A      ) == 0)
        && ((pad->new[0] & PAD_B      ) == 0)
        && ((pad->new[0] & PAD_C      ) == 0)
        && ((pad->new[0] & PAD_START  ) == 0))
        {
            if((pad->old[0] & PAD_A) || (pad->old[0] & PAD_C) || (pad->old[0] & PAD_START))
            { /* Confirm. */
                s->exit_code = MENU_EXIT_OK;
                break;
            }
            if(pad->old[0] & PAD_B)
            { /* Cancel. */
                stag = current_tag;
                s->exit_code = MENU_EXIT_CANCEL;
                break;
            }
        }
    }

    if(s->erase_screen)
    {
        menu_clear_display(s);
    }
    return stag;
}
