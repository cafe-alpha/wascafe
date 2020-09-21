#ifndef _DE10_7SEG_
#define _DE10_7SEG_

#include <sys/alt_stdio.h>
#include <string.h>
#include <system.h>

#include "wasca_defs.h"

/*
 * ------------------------------------------
 * - DE10-lite board's 7-segments display ---
 * ------------------------------------------
 */


/**
 * h7seg_all_clear
 * 
 * Clear all 7-segments displays, including dots.
 * 
 * Note : this should be called on NIOS startup.
**/
void h7seg_all_clear(void);


/**
 * h7seg_all_dots_clear
 * 
 * Clear all 6 dots.
**/
void h7seg_all_dots_clear(void);

/**
 * h7seg_dot_out
 * 
 * Set/clear specified dot.
**/
void h7seg_dot_out(unsigned char id, unsigned char val);


/**
 * h7seg_u4_out/h7seg_u8_out/h7seg_u16_out
 * 
 * Display specified hexadecimal value.
 * 
 * Parameters :
 *  - id  : start display ID (0~5)
 *  - val : value to display
 *
 * Note : display is cleared when value out of boundaries
 *        is provided to h7seg_u4_out.
**/
void h7seg_u4_out(unsigned char id, unsigned char val);
void h7seg_u8_out(unsigned char id, unsigned char val);
void h7seg_u16_out(unsigned char id, unsigned short val);

/* Extra macro to clear sepcified display. */
#define H7SEG_CLEAR(_ID_) h7seg_u4_out(_ID_, 0xFF)


/**
 * h7seg_xdec_out
 * 
 * Dispay decimal value on 7-segments display.
 * 
 * Parameters :
 *  - stt_id       : start display ID (0~5)
 *                   Example : 2->display from third display
 *  - digits_count : number of digits (1~6)
 *  - dec_pos      : decimal point position; not displayed when set to zero
 *                   Example : val=1234, dec_pos=3 -> displays "1.234"
 *  - val          : value to display, containing from integer and decimal parts
 * 
 * Notes :
 *  - If value have more digits than display than allowed count, then upper
 *    digits are not displayed.
 *    Example : val=12345, digits_count=3 -> displays "345"
**/
void h7seg_xdec_out(unsigned char stt_id, unsigned char digits_count, unsigned char dec_pos, unsigned long val);

/* Extra two and four digits display macros, for usage convenience. */
#define H7SEG_DEC2_OUT0(_VAL_) h7seg_xdec_out(0, 2, 0, _VAL_)
#define H7SEG_DEC2_OUT2(_VAL_) h7seg_xdec_out(2, 2, 0, _VAL_)
#define H7SEG_DEC2_OUT4(_VAL_) h7seg_xdec_out(4, 2, 0, _VAL_)

#define H7SEG_DEC4_OUT0(_VAL_) h7seg_xdec_out(0, 4, 0, _VAL_)
#define H7SEG_DEC4_OUT2(_VAL_) h7seg_xdec_out(2, 4, 0, _VAL_)


#endif // _DE10_7SEG_
