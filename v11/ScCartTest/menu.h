/**
 *  Cartridge Test Utility
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/pskai/
 *
 *  See LICENSE file for details.
**/

#ifndef _MENU_H_
#define _MENU_H_

#include <stdio.h>

/** Character size for font used in menu. **/
#define MENU_CHAR_W 9
#define MENU_CHAR_H 9


/** Tag/return values. **/
/* Disabled item : description not displayed, and can't be selected. */
#define MENU_TAG_DISABLED -1
/* Greyout item : description displayed in grey color, and can't be selected. */
#define MENU_TAG_GREYOUT  -2

/* Test if tag value is invalid. */
#define MENU_TAG_INVALID(_VAL_) ((_VAL_) < 0)
#define MENU_TAG_VALID(_VAL_) ((_VAL_) >= 0)


#define MENU_ITEMS_COUNT 90
typedef struct _menu_item_t {
    char* txt;
    int   txt_len;

    int tag;

    /* If nonzero, color is set to half grey, and prevent from selecting. */
    int disabled;

    /* 0: default color */
    unsigned short color;
} menu_item_t;
typedef struct _menu_items_t {
    menu_item_t it[MENU_ITEMS_COUNT/*rows * cols*/];
} menu_items_t;

/* Function called when menu display is refreshed.
 * First parameter is ID of item currently highlighted.
 */
typedef void (*Fct_menu_callback_redraw)(int);

/* Function called on each vblank. */
typedef void (*Fct_menu_callback_vblank)(void);

/* Function called when requesting menu string.
 * First parameter is string ID.
 * Second parameter is pointer to string.
 * Third parameter is string length, zero will uses autolength.
 */
typedef void (*Fct_menu_callback_getstr)(int, char**, int*);

/* Exit codes */
#define MENU_EXIT_OK        0
#define MENU_EXIT_CANCEL    1
#define MENU_EXIT_NEXTPAGE1 2
#define MENU_EXIT_PREVPAGE1 3
#define MENU_EXIT_NEXTPAGE2 4
#define MENU_EXIT_PREVPAGE2 5


typedef struct _menu_settings_t {
    /* Top-left position, in pixel unit. Negative value = center. */
    int x0, y0;

    /* Width, height (character unit). */
    /**
     * Note : one line is used to display title, so
     *        displayable items count is h-1.
     */
    int w, h;

    /* Number of columns. */
    int cols;

    /* Callback functions. */
    Fct_menu_callback_redraw callback_redraw;
    Fct_menu_callback_vblank callback_vblank;
    Fct_menu_callback_getstr callback_getstr;

    /* Title. */
    char* title;

    /* If non-zero, erase screen on exit. */
    int erase_screen;

    /* Features used or not. */
    unsigned short features;

    /* Menu border and title colors. */
    unsigned char  color_border;
    unsigned char  color_border_highlight;
    unsigned short color_title;

    //unsigned short color_notselect;
    /* Default colors for items. */
    unsigned short color_default;
    /* Background color for highlighted item. */
    unsigned short color_highlight;
    /* Colors for disabled items. */
    unsigned short color_greyout;

    /* Foreground color for items selected in list. */
    unsigned short color_selection;

    /* Exit code. (see MENU_EXIT_* definitions for possible values) */
    int exit_code;
} menu_settings_t;


/**
 * Note : there are 2 types of menus available : array and list.
 * Array allows complex shaped (= several columns, variable
 * dimensions, etc) menus but without scrolling, while lists 
 * allow scrolling, but only simple shapes.
**/


/**
 * menu_init
 * Initializes menu settings.
**/
void menu_reset_settings(menu_settings_t* s);

/**
 * menu_set_title
 * Set/modify menu title.
 * Please call this function before calling menu_start function.
**/
void menu_set_title(menu_settings_t* s, char* title);

/**
 * menu_set_erase_on_exit
 * Indicate to erase screen on exit or not.
 * Please call this function before calling menu_start function.
**/
void menu_set_erase_on_exit(menu_settings_t* s, int erase_screen);

/**
 * menu_set_features
 * Indicate to use extra features or not.
 * Please call this function before calling menu_start function.
**/
#define MENU_FEATURES_NONE       0x0000
#define MENU_FEATURES_CHANGEPAGE (1<<0)
#define MENU_FEATURES_TEXTCENTER (1<<1)
void menu_set_features(menu_settings_t* s, unsigned short f);

/**
 * menu_reset_items
 * Reset items array.
 * Please call this function before calling menu_set_item function.
 * For use with menu_start function only.
**/
void menu_reset_items(menu_items_t* m);

/**
 * menu_set_item
 * Reset items array.
 * Please call this function before calling menu_start function.
 * For use with menu_start function only.
**/
void menu_set_item(menu_items_t* m, int id, char* txt, int tag);

/**
 * menu_set_item_color
 * Set specified color in specified item in array.
 * For use with menu_start function only.
**/
void menu_set_item_color(menu_items_t* m, int id, unsigned short color);

/**
 * menu_disable_item
 * Mark specified item as disabled.
 * Please use this function after calling menu_set_item function.
 * For use with menu_start function only.
**/
void menu_disable_item(menu_items_t* m, int tag);

/**
 * menu_set_pos
 * Set menu position and size.
**/
void menu_set_pos(menu_settings_t* s, int x0, int y0, int w, int h, int cols);

/**
 * menu_set_callback
 * Set callback function.
 * Callback function is called when displaying/refreshing menu.
**/
void menu_set_callback_redraw(menu_settings_t* s, Fct_menu_callback_redraw fct);

/**
 * menu_set_vblank
 * Set callback function.
 * Callback function is called on each vblank.
**/
void menu_set_callback_vblank(menu_settings_t* s, Fct_menu_callback_vblank fct);

/**
 * menu_set_getstr
 * Set callback function.
 * Callback function is called when writing menu string.
 *
 * Note : it works only when list menu are used.
**/
void menu_set_callback_getstr(menu_settings_t* s, Fct_menu_callback_getstr fct);

/**
 * menu_start
 * Display menu array and allow user to select an item.
 * Return tag user selected.
**/
int menu_start(menu_items_t* m, menu_settings_t* s, int current_tag);

/**
 * menu_list_start
 * Display menu list and allow user to select an item.
 * Return ID of the item user selected in list.
**/
int menu_list_start(char* items[], int items_count, menu_settings_t* s, int current_id, int* menu_selected);

/**
 * menu_clear_display
 * Clear menu display.
**/
void menu_clear_display(menu_settings_t* s);


/**
 * Global variables, declared in menu.c
**/
extern menu_items_t    _menu_items;
extern menu_settings_t _menu_settings;


#endif /* _MENU_H_ */

