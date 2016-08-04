/**
 *  Cartridge Test Utility
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/pskai/
 *
 *  See LICENSE file for details.
**/

#ifndef _SHARED_H_
#define _SHARED_H_

/* Empty line, used to clear display. */
extern char* empty_line;

/* Menu top row. */
#define MENU_TOP_ROW (4*MENU_CHAR_H)

/* Prompt user for Start button. */
void prompt_start(void);

/* RAM init function. */
unsigned char my_RB_CartRAM_init(unsigned char cs);

/* Status display when menu is displaying. */
void menu_vblank_clbk(void);

#endif /* _SMPC_H_ */

