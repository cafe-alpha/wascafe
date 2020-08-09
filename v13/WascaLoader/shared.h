/**
 *  Cartridge Test Utility
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/pskai/
 *
 *  See LICENSE file for details.
**/

#ifndef _SHARED_H_
#define _SHARED_H_

// dirty fix for using rand() function.
int rand (void);

/* Menu top row. */
#define MENU_TOP_ROW (4*MENU_CHAR_H)

/**
 * Indicates if CD-ROM drive can be used or not.
 *  0 : application loaded from firmware, CD-ROM unit unavailable.
 *  1 : application loaded from CD-ROM.
**/
extern int cdrom_available;

/* Minimalistic V-Blank call back, for use in menus. */
void background_vblank_clbk(void);

/**
 * wait_for_start.
 * Prompt user to push start button.
 * If msg parameter is set to NULL, 
 * defaut prompt message is displayed.
 *
 * Returns 1 if A/C/Start button pushed, zero else.
**/
int wait_for_start(char* msg);

/* RAM init function. */
unsigned char cartram_init_test(int cs);

/* Wasca status display flag. */
extern int _wasca_status_display;

/* Status display when menu is displaying. */
void menu_vblank_clbk(void);

/* Clear normal display (excluding status) area. */
void clear_normal_display(void);

/* Clear screen. */
void clear_screen(void);

#endif /* _SHARED_H_ */

