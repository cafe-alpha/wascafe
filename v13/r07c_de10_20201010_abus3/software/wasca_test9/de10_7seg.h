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

#if DE10_TARGET == 1


/**
 * h7seg_enable/h7seg_disable
 * 
 * Enable/disable access to 7-segments display.
 * (Access to dots is however available without consideration)
 * 
 * Purpose of these functions is to allow usage of 7-segments display
 * only for a portion of code to debug.
**/
void h7seg_enable(void);
void h7seg_disable(void);


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
#define h7seg_clear(_ID_) h7seg_u4_out(_ID_, 0xFF)


/**
 * h7seg_xdec_out
 * 
 * Display decimal value on 7-segments display.
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
#define h7seg_dec2_out0(_VAL_) h7seg_xdec_out(0, 2, 0, _VAL_)
#define h7seg_dec2_out2(_VAL_) h7seg_xdec_out(2, 2, 0, _VAL_)
#define h7seg_dec2_out4(_VAL_) h7seg_xdec_out(4, 2, 0, _VAL_)

#define h7seg_dec4_out0(_VAL_) h7seg_xdec_out(0, 4, 0, _VAL_)
#define h7seg_dec4_out2(_VAL_) h7seg_xdec_out(2, 4, 0, _VAL_)


/**
 * h7seg_char_out, h7seg_str_out
 * 
 * Display a single character or character string on 7-segments display.
 * 
 * Parameters :
 *  - id/stt_id    : start display ID (0~5)
 *                   Example : 2->display from third display
 *  - c            : character to display
 *  - str          : string to display, null terminated
**/
void h7seg_char_out(unsigned char id, char c);
void h7seg_str_out(unsigned char stt_id, char* str);


/**
 * h7seg_show_build_date
 * 
 * Display software build date, for use (for example) on startup to quickly
 * verify which version is running.
 * 
 * Build time stamp is displayed in MDDHHm format, where :
 *  - M is current month, in hexadecimal format
 *  - DD is current day, in decimal format
 *  - MM is current hour, in decimal format
 *  - m is current minute upper digit, in decimal format
**/
void h7seg_show_build_date(void);


/**
 * h7seg_contrast
 * 
 * Change contrast of 7-segments and dots during specified time interval.
 * verify which version is running.
 *
 * Note 1 : Contrast gradually changes during the time interval.
 * 
 * Note 2 : Contrast variation is done on NIOS side, hence this function
 *          is blocking during specified time interval.
 * 
 * Note 3 : 7-segments status before calling this function is contrasted.
 *          -> It is required to write to 7-segs before calling this function.
 * 
 * Parameters :
 *  - contrast_stt  : contrast on startup             (0:black ~ 100:red)
 *  - contrast_end  : contrast at the end of duration (0:black ~ 100:red)
 *  - duration_msec : time interval when contrast is tuned (milliseconds unit)
**/
void h7seg_contrast(unsigned char contrast_stt, unsigned char contrast_end, unsigned long duration_msec);


#else // DE10_TARGET != 1


/* Dummy declarations when DE10-lite board is not used. */
#define h7seg_all_clear()
#define h7seg_all_dots_clear()
#define h7seg_dot_out(_ID_, _VAL_)
#define h7seg_u4_out(_ID_, _VAL_)
#define h7seg_u8_out(_ID_, _VAL_)
#define h7seg_u16_out(_ID_, _VAL_)
#define h7seg_clear(_ID_)
#define h7seg_xdec_out(_STT_ID_, _DIGITS_COUNT_, _DEC_POS_, _VAL_)
#define h7seg_dec2_out0(_VAL_)
#define h7seg_dec2_out2(_VAL_)
#define h7seg_dec2_out4(_VAL_)
#define h7seg_dec4_out0(_VAL_)
#define h7seg_dec4_out2(_VAL_)
#define h7seg_char_out(_ID_, _C_)
#define h7seg_str_out(_STT_ID_, _STR_)
#define h7seg_show_build_date()
#define h7seg_contrast(_CONTRAST_STT_, _CONTRAST_END_, _DURATION_MSEC_)

#endif // DE10_TARGET != 1


#endif // _DE10_7SEG_
