#include "de10_7seg.h"

#include <altera_avalon_pio_regs.h>


void h7seg_all_clear(void)
{
    IOWR(HEX0_BASE, 0, 0xFF);
    IOWR(HEX1_BASE, 0, 0xFF);
    IOWR(HEX2_BASE, 0, 0xFF);
    IOWR(HEX3_BASE, 0, 0xFF);
    IOWR(HEX4_BASE, 0, 0xFF);
    IOWR(HEX5_BASE, 0, 0xFF);

    IOWR(HEXDOT_BASE, 0, 0xFF);
}

// 7-Seg bit IDs:
//  0
// 5  1
//  6
// 4  2
//  3
const unsigned char _hex_lut[16] = 
{
    ~((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(0<<6)), // 0
    ~((0<<0)|(1<<1)|(1<<2)|(0<<3)|(0<<4)|(0<<5)|(0<<6)), // 1
    ~((1<<0)|(1<<1)|(0<<2)|(1<<3)|(1<<4)|(0<<5)|(1<<6)), // 2
    ~((1<<0)|(1<<1)|(1<<2)|(1<<3)|(0<<4)|(0<<5)|(1<<6)), // 3
    ~((0<<0)|(1<<1)|(1<<2)|(0<<3)|(0<<4)|(1<<5)|(1<<6)), // 4
    ~((1<<0)|(0<<1)|(1<<2)|(1<<3)|(0<<4)|(1<<5)|(1<<6)), // 5
    ~((1<<0)|(0<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)), // 6
    ~((1<<0)|(1<<1)|(1<<2)|(0<<3)|(0<<4)|(0<<5)|(0<<6)), // 7
    ~((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)), // 8
    ~((1<<0)|(1<<1)|(1<<2)|(1<<3)|(0<<4)|(1<<5)|(1<<6)), // 9
    ~((1<<0)|(1<<1)|(1<<2)|(0<<3)|(1<<4)|(1<<5)|(1<<6)), // A
    ~((0<<0)|(0<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)), // B
    ~((1<<0)|(0<<1)|(0<<2)|(1<<3)|(1<<4)|(1<<5)|(0<<6)), // C
    ~((0<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(0<<5)|(1<<6)), // D
    ~((1<<0)|(0<<1)|(0<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)), // E
    ~((1<<0)|(0<<1)|(0<<2)|(0<<3)|(1<<4)|(1<<5)|(1<<6)), // F
};

void h7seg_all_dots_clear(void)
{
    IOWR(HEXDOT_BASE, 0, 0xFF);
}

void h7seg_dot_out(unsigned char id, unsigned char val)
{
    unsigned char tmp = IORD(HEXDOT_BASE, 0);

    if(val == 0)
    {
        tmp |= 1 << id;
    }
    else
    {
        tmp &= ~(1 << id);
    }

    IOWR(HEXDOT_BASE, 0, tmp);
}

void h7seg_u4_out(unsigned char id, unsigned char val)
{
    unsigned long target_addr = HEX0_BASE + (id * (HEX1_BASE - HEX0_BASE));

    /* Clear display when value is irregular. */
    unsigned char hex_val = 0xFF;
    if(val <= 0x0F)
    {
        hex_val = _hex_lut[val];
    }
    
    IOWR(target_addr, 0, hex_val);
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

    /* Parameter sanitization. */
    if(digits_count > 6)
    {
        digits_count = 6;
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
        h7seg_u4_out(id, (unsigned char)(val % 10));

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

