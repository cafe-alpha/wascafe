#include "de10_7seg.h"


#if DE10_TARGET == 1

#include <altera_avalon_pio_regs.h>


/* Reverse 7-segment display.
 *  - 0 : normal display
 *  - 1 : upside down display : dots are at the top of the digits.
 */
#define H7SEG_REVERSE 1


/* Macro to mix each 7-seg bits together. */
#if H7SEG_REVERSE == 0 /* Normal DE10-lite board orientation. */
    //   0
    //  5  1
    //   6
    //  4  2
    //   3 .
#   define H7SEG_CONV(_SEG0_, _SEG1_, _SEG2_, _SEG3_, _SEG4_, _SEG5_, _SEG6_) (~(((_SEG0_)<<0) | ((_SEG1_)<<1) | ((_SEG2_)<<2) | ((_SEG3_)<<3) | ((_SEG4_)<<4) | ((_SEG5_)<<5) | ((_SEG6_)<<6)))
#else /* Upside down DE10-lite board orientation : dots are at the top of the digits. */
    // . 3
    //  2  4
    //   6
    //  1  5
    //   0
#   define H7SEG_CONV(_SEG0_, _SEG1_, _SEG2_, _SEG3_, _SEG4_, _SEG5_, _SEG6_) (~(((_SEG0_)<<3) | ((_SEG1_)<<4) | ((_SEG2_)<<5) | ((_SEG3_)<<0) | ((_SEG4_)<<1) | ((_SEG5_)<<2) | ((_SEG6_)<<6)))
#endif

const unsigned char _hex_lut[10 + 26] = 
{
    // https://en.wikichip.org/wiki/seven-segment_display
    //  0
    // 5  1
    //  6
    // 4  2
    //  3
    H7SEG_CONV(1, 1, 1, 1, 1, 1, 0), // 0
    H7SEG_CONV(0, 1, 1, 0, 0, 0, 0), // 1
    H7SEG_CONV(1, 1, 0, 1, 1, 0, 1), // 2
    H7SEG_CONV(1, 1, 1, 1, 0, 0, 1), // 3
    H7SEG_CONV(0, 1, 1, 0, 0, 1, 1), // 4
    H7SEG_CONV(1, 0, 1, 1, 0, 1, 1), // 5
    H7SEG_CONV(1, 0, 1, 1, 1, 1, 1), // 6
    H7SEG_CONV(1, 1, 1, 0, 0, 0, 0), // 7
    H7SEG_CONV(1, 1, 1, 1, 1, 1, 1), // 8
    H7SEG_CONV(1, 1, 1, 1, 0, 1, 1), // 9

    // https://en.wikichip.org/wiki/seven-segment_display/representing_letters
    H7SEG_CONV(1, 1, 1, 0, 1, 1, 1), // A
    //H7SEG_CONV(1, 1, 1, 1, 1, 0, 1), // a
    H7SEG_CONV(0, 0, 1, 1, 1, 1, 1), // b
    H7SEG_CONV(1, 0, 0, 1, 1, 1, 0), // C
    //H7SEG_CONV(0, 0, 0, 1, 1, 0, 1), // c
    H7SEG_CONV(0, 1, 1, 1, 1, 0, 1), // d
    H7SEG_CONV(1, 0, 0, 1, 1, 1, 1), // E
    H7SEG_CONV(1, 0, 0, 0, 1, 1, 1), // F
    H7SEG_CONV(1, 0, 1, 1, 1, 1, 0), // G
    H7SEG_CONV(0, 1, 1, 0, 1, 1, 1), // H
    //H7SEG_CONV(0, 0, 1, 0, 1, 1, 1), // h
    H7SEG_CONV(0, 0, 0, 0, 1, 1, 0), // I
    H7SEG_CONV(0, 1, 1, 1, 1, 0, 0), // J
    H7SEG_CONV(1, 0, 1, 0, 1, 1, 1), // (K unreadable)
    H7SEG_CONV(0, 0, 0, 1, 1, 1, 0), // L
    H7SEG_CONV(1, 1, 0, 1, 0, 1, 0), // (M unreadable)
    H7SEG_CONV(0, 0, 1, 0, 1, 0, 1), // n
    //H7SEG_CONV(1, 1, 1, 1, 1, 1, 0), // O
    H7SEG_CONV(0, 0, 1, 1, 1, 0, 1), // o
    H7SEG_CONV(1, 1, 0, 0, 1, 1, 1), // P
    H7SEG_CONV(1, 1, 1, 0, 0, 1, 1), // q
    H7SEG_CONV(0, 0, 0, 0, 1, 0, 1), // r
    H7SEG_CONV(1, 0, 1, 1, 0, 1, 1), // S
    H7SEG_CONV(0, 0, 0, 1, 1, 1, 1), // t
    H7SEG_CONV(0, 0, 1, 1, 1, 0, 0), // u
    H7SEG_CONV(0, 1, 1, 1, 1, 1, 0), // V
    H7SEG_CONV(0, 1, 1, 1, 1, 1, 1), // (W unreadable)
    H7SEG_CONV(1, 0, 0, 1, 0, 0, 1), // (X unreadable)
    H7SEG_CONV(0, 1, 1, 1, 0, 1, 1), // y
    H7SEG_CONV(1, 1, 0, 1, 1, 0, 1), // (Z same as 2)
};


/* A macro to convert digit ID to corresponding ID in memory. */
#if H7SEG_REVERSE == 0
#   define H7SEG_ID2ID(_ID_) (_ID_)
#else
#   define H7SEG_ID2ID(_ID_) (5 - (_ID_))
#endif


int _h7seg_enable_flag = 1;

void h7seg_enable(void)
{
    _h7seg_enable_flag = 1;
}

void h7seg_disable(void)
{
    _h7seg_enable_flag = 0;
}


void h7seg_all_clear(void)
{
    if(_h7seg_enable_flag)
    {
        IOWR(HEX0_BASE, 0, 0xFF);
        IOWR(HEX1_BASE, 0, 0xFF);
        IOWR(HEX2_BASE, 0, 0xFF);
        IOWR(HEX3_BASE, 0, 0xFF);
        IOWR(HEX4_BASE, 0, 0xFF);
        IOWR(HEX5_BASE, 0, 0xFF);
    }

    IOWR(HEXDOT_BASE, 0, 0xFF);
}


void h7seg_all_dots_clear(void)
{
    IOWR(HEXDOT_BASE, 0, 0xFF);
}

void h7seg_dot_out(unsigned char id, unsigned char val)
{
    unsigned char tmp = IORD(HEXDOT_BASE, 0);

    if(val == 0)
    {
        tmp |= 1 << H7SEG_ID2ID(id);
    }
    else
    {
        tmp &= ~(1 << H7SEG_ID2ID(id));
    }

    IOWR(HEXDOT_BASE, 0, tmp);
}

void h7seg_u4_out(unsigned char id, unsigned char val)
{
    if(_h7seg_enable_flag)
    {
        unsigned long target_addr = HEX0_BASE + (H7SEG_ID2ID(id) * (HEX1_BASE - HEX0_BASE));

        /* Clear display when value is irregular. */
        unsigned char hex_val = 0xFF;
        if(val <= 0x0F)
        {
            hex_val = _hex_lut[val];
        }
        
        IOWR(target_addr, 0, hex_val);
    }
}

void h7seg_u8_out(unsigned char id, unsigned char val)
{
    h7seg_u4_out(id+0, (val>>4) & 0x0F);
    h7seg_u4_out(id+1, (val>>0) & 0x0F);
}

void h7seg_u16_out(unsigned char id, unsigned short val)
{
    h7seg_u4_out(id+0, ((unsigned char)(val>>12)) & 0x0F);
    h7seg_u4_out(id+1, ((unsigned char)(val>> 8)) & 0x0F);
    h7seg_u4_out(id+2, ((unsigned char)(val>> 4)) & 0x0F);
    h7seg_u4_out(id+3, ((unsigned char)(val>> 0)) & 0x0F);
}



void h7seg_xdec_out(unsigned char stt_id, unsigned char digits_count, unsigned char dec_pos, unsigned long val)
{
    int i;

    /* Parameters sanitization. */
    if(stt_id > 5)
    {
        stt_id = 5;
    }
    if(digits_count == 0)
    {
        digits_count = 1;
    }
    if((stt_id + digits_count) > 6)
    {
        digits_count = 6 - stt_id;
    }

    /* Start display from lower digit. */
    unsigned char id = stt_id + digits_count-1;

    /* Display value as "999999" when it is larger than displayable maximum. */
    const unsigned long val_max_tbl[6] = {9, 99, 999, 9999, 99999, 999999};
    unsigned long val_max = val_max_tbl[digits_count-1];
    if(val > val_max)
    {
        val = val_max;
    }

    for(i=0; i<digits_count; i++)
    {
        unsigned char tmp = (unsigned char)(val % 10);

        if((i != 0) && (val == 0))
        {
            /* Show upper zero digits as blank.
             * Example : 103 on 4 digits is displayed as " 103".
             */
            tmp = 0xFF;
        }

        h7seg_u4_out(id, tmp);

        /* Go to upper digit. */
        val = val / 10;
        id--;
    }

    /* Show the dot at specified position. */
    if(dec_pos != 0)
    {
        id = stt_id + digits_count-1;

        for(i=0; i<digits_count; i++)
        {
            h7seg_dot_out(id, (i == dec_pos ? 1 : 0));
            id--;
        }
    }
}



void h7seg_char_out(unsigned char id, char c)
{
    if(_h7seg_enable_flag)
    {
        unsigned long target_addr = HEX0_BASE + (H7SEG_ID2ID(id) * (HEX1_BASE - HEX0_BASE));
        unsigned char tmp = 0xFF;

        if((c >= 'a') && (c <= 'z'))
        {
            tmp = _hex_lut[10 + (c - 'a')];
        }
        else if((c >= 'A') && (c <= 'Z'))
        {
            tmp = _hex_lut[10 + (c - 'A')];
        }
        else if((c >= '0') && (c <= '9'))
        {
            tmp = _hex_lut[c - '0'];
        }

        IOWR(target_addr, 0, tmp);
    }
}


void h7seg_str_out(unsigned char stt_id, char* str)
{
    int i;

    /* Parameter sanitization. */
    if(stt_id > 5)
    {
        stt_id = 5;
    }

    for(i=0; i<(6-stt_id); i++)
    {
        char c = str[i];

        if(c == '\0')
        {
            break;
        }

        h7seg_char_out(stt_id+i, c);
    }
}


void h7seg_show_build_date(void)
{
    // const char *months_list[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    char buff1[32] = {'\0'};
    strcpy(buff1, __DATE__);
    buff1[6] = '\0';
    buff1[11] = '\0';

    // char* year = buff1 + 7;

    // int month = 0;
    // int i;
    // for(i = 0; i < 12; i++)
    // {
    //     if (memcmp(buff1, months_list[i], 3) == 0)
    //     {
    //         month = i + 1;
    //         break;
    //     }
    // }
    char* day = buff1 + 4;

    char buff2[32] = {'\0'};
    strcpy(buff2, __TIME__);
    buff2[2] = '\0';
    buff2[5] = '\0';
    buff2[8] = '\0';

    char* hour   = buff2 + 0;
    char* minute = buff2 + 3;
    //char* second = buff2 + 6;

    /* Show build time stamp in DDHHm format, where :
     *  - DD is current day, in decimal format
     *  - MM is current hour, in decimal format
     *  - mm is current minute, in decimal format
     */
    if(day[0] == ' ')
    {
        day[0] = '0';
    }
    if(hour[0] == ' ')
    {
        hour[0] = '0';
    }
    if(minute[0] == ' ')
    {
        minute[0] = '0';
    }

    h7seg_str_out(0/*stt_id*/, day);
    h7seg_str_out(2/*stt_id*/, hour);
    h7seg_str_out(4/*stt_id*/, minute);
}


#include "perf_cntr.h"
#include "log.h"

void h7seg_contrast(unsigned char contrast_stt, unsigned char contrast_end, unsigned long duration_msec)
{
    /* Keep status of digits and dots in memory. */
    unsigned char h7seg_status[6+1];

    h7seg_status[0] = IORD(HEX0_BASE  , 0);
    h7seg_status[1] = IORD(HEX1_BASE  , 0);
    h7seg_status[2] = IORD(HEX2_BASE  , 0);
    h7seg_status[3] = IORD(HEX3_BASE  , 0);
    h7seg_status[4] = IORD(HEX4_BASE  , 0);
    h7seg_status[5] = IORD(HEX5_BASE  , 0);
    h7seg_status[6] = IORD(HEXDOT_BASE, 0);

    /* Sanitize parameters. */
    if(contrast_stt > 100)
    {
        contrast_stt = 100;
    }
    if(contrast_end > 100)
    {
        contrast_end = 100;
    }
    if(duration_msec == 0)
    {
        duration_msec = 1;
    }

    /* Prepare elapsed time measurement. */
    unsigned long clkhz = alt_get_cpu_freq();
    WASCA_PERF_START_MEASURE();
    unsigned long time_msec = 0;
logout(WL_LOG_DEBUGNORMAL, "time_msec#0:%u", time_msec);
logflush();

    while(1)
    {
        /* Setup display on and off counts. */
        int current_contrast = time_msec * (contrast_end - contrast_stt);
        current_contrast = current_contrast / duration_msec;
        current_contrast += contrast_stt;

        /* Multiply factor for one PWM period.
         * The total (off + on) count the 7-segs are written
         * is equal to this constant multiplied by 100.
         */
#define H7SEG_PWM_FACTOR 20

        /* Number of times display is turned off and on until
         * checking for elapsed time.
         * The longer the smoother the display, but the less
         * accurate elapsed time measurement is.
         */
#define H7SEG_REFRESH_CNT 100

        int disp_on  = current_contrast * H7SEG_PWM_FACTOR;
        int disp_off = (100 * H7SEG_PWM_FACTOR) - current_contrast;

        int refresh_count = 0;

        while(1)
        {
            int i;

            /* Clear display for a while. */
            for(i=disp_off; i; i--)
            {
                IOWR(HEX0_BASE  , 0, 0xFF);
                IOWR(HEX1_BASE  , 0, 0xFF);
                IOWR(HEX2_BASE  , 0, 0xFF);
                IOWR(HEX3_BASE  , 0, 0xFF);
                IOWR(HEX4_BASE  , 0, 0xFF);
                IOWR(HEX5_BASE  , 0, 0xFF);
                IOWR(HEXDOT_BASE, 0, 0xFF);
            }

            /* Display ON for another while. */
            for(i=disp_on; i; i--)
            {
                IOWR(HEX0_BASE  , 0, h7seg_status[0]);
                IOWR(HEX1_BASE  , 0, h7seg_status[1]);
                IOWR(HEX2_BASE  , 0, h7seg_status[2]);
                IOWR(HEX3_BASE  , 0, h7seg_status[3]);
                IOWR(HEX4_BASE  , 0, h7seg_status[4]);
                IOWR(HEX5_BASE  , 0, h7seg_status[5]);
                IOWR(HEXDOT_BASE, 0, h7seg_status[6]);
            }

            refresh_count++;
            if(refresh_count > H7SEG_REFRESH_CNT)
            {
                break;
            }
        }

        /* Verify elapsed time. */
        alt_u64 total_clocks = WASCA_PERF_GET_TIME();
        total_clocks *= 1000;
        time_msec = (unsigned long)(total_clocks / clkhz);
logout(WL_LOG_DEBUGNORMAL, "time_msec:%u", time_msec);
logflush();

        if(time_msec >= duration_msec)
        {
            break;
        }
    }

    /* Restore back display. */
    IOWR(HEX0_BASE  , 0, h7seg_status[0]);
    IOWR(HEX1_BASE  , 0, h7seg_status[1]);
    IOWR(HEX2_BASE  , 0, h7seg_status[2]);
    IOWR(HEX3_BASE  , 0, h7seg_status[3]);
    IOWR(HEX4_BASE  , 0, h7seg_status[4]);
    IOWR(HEX5_BASE  , 0, h7seg_status[5]);
    IOWR(HEXDOT_BASE, 0, h7seg_status[6]);
}

#endif // DE10_TARGET == 1
