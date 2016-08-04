/**
 *  Cartridge Test Utility
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/pskai/
 *
 *  See LICENSE file for details.
**/

#include "smpc.h"

per_data_struct per[MAX_PERIPHERALS];
static unsigned short old_per_push[MAX_PERIPHERALS]; // fix me

/* Global variable containing status of each pads. */
pad_t _pad;

void per_handler(void);
void smpc_handler(void);


void pad_init(void)
{
    unsigned char temp;
    int count;
    pad_t* p = &_pad;

    /* Reset pad internals. */
    memset((void*)p, 0, sizeof(pad_t));
    pad_set_autorepeat(PAD_AUTOREPEAT_DEFAULT);

    /* Get port connection status. */
    DDR1    = 0x60; /* TH,TR out, TL,D3-D0 in */
    DDR2    = 0x60; /* TH,TR out, TL,D3-D0 in */
    IOSEL   = 0x03; /* Manual mode for both ports */
    EXLE    = 0x00;

    p->port_connected = 0x03;

    /** Test pad1 **/
    PDR1 = 0x60;
    for(count=0; count < 16; count++)
        __asm("nop");
    temp = PDR1;

    if((temp&0x1) != 0x00)
    { /* Pad1 is disconnected. */
        p->port_connected -= 0x01;
    }

    /** Test pad2 **/
    PDR2 = 0x60;
    for(count=0; count < 16; count++)
        __asm("nop");
    temp = PDR2;

    if((temp&0x1) != 0x00)
    { /* Pad2 is disconnected. */
        p->port_connected -= 0x02;
    }


    DDR1 = 0;
    EXLE = 0;
    IOSEL = 0; // Set both ports to SMPC control mode

    // Clear internal variables
    memset(old_per_push, 0, MAX_PERIPHERALS * sizeof(unsigned short));
    memset(per, 0, sizeof(per_data_struct) * MAX_PERIPHERALS);
}

void pad_shutdown(void)
{
    DDR1 = 0;
    EXLE = 0;
    IOSEL = 0;
}

void pad_set_autorepeat(int val)
{
    pad_t* p = &_pad;

    if(val < 0)
    {
        p->repeat_th = 0;
    }
    else if(val > 255)
    {
        p->repeat_th = 255;
    }
    else
    {
        p->repeat_th = (unsigned char)val;
    }
}

void pad_update_autorepeat(pad_t *p, int id)
{
    /* Check for emergency exit. */
    if((p->new[id] & (PAD_A | PAD_B | PAD_C | PAD_START)) == (PAD_A | PAD_B | PAD_C | PAD_START))
    {
        /* Exit to multiplayer. */
        (**(void(**)(void))0x600026C)();
    }

    /* Button auto push feature. */
    if(p->repeat_th != 0)
    {
        int i;
        /* ID -> shift table. */
        unsigned short button_shift_tbl[13] =
        {
            PAD_LBUTTON, 
            PAD_RBUTTON, 
            PAD_X      , 
            PAD_Y      , 
            PAD_Z      , 
            PAD_START  , 
            PAD_A      , 
            PAD_C      , 
            PAD_B      , 
            PAD_RIGHT  , 
            PAD_LEFT   , 
            PAD_DOWN   , 
            PAD_UP     
        };

        /* Count the number of times each button is consecutively hold. */
        for(i=0; i<13; i++)
        {
            if((p->new[id] & (button_shift_tbl[i])) != 0)
            {
                if(p->repeat_cnt[id][i] != 255)
                {
                    p->repeat_cnt[id][i]++;
                }
            }
            else
            {
                p->repeat_cnt[id][i] = 0;
            }
        }

        /* If hold enough, modify pad delta value. */
        for(i=0; i<13; i++)
        {
            if(p->repeat_cnt[id][i] >= p->repeat_th)
            {
                /* Mark this key as "pushed". */
                p->delta[id] |= button_shift_tbl[i];
            }
        }
    }
}

pad_t* pad_read(void)
{
    pad_t* p = &_pad;

    /* Waits for SF to be Reset. */
    smpc_wait_till_ready();

    /* Sets the Command Parameter in IREG. */
    SMPC_REG_IREG(0) = 0x00; // no intback status
    SMPC_REG_IREG(1) = 0x0A; // 15-byte mode, peripheral data returned, time optimized
    SMPC_REG_IREG(2) = 0xF0; // ???

    smpc_issue_command(SMPC_CMD_INTBACK);

    /* Wait SF Clear. */
    smpc_wait_till_ready();

    smpc_handler();

    return p;
}

void smpc_command (unsigned char cmd)
{
   smpc_wait_till_ready();
   smpc_issue_command(cmd);
}


void smpc_handler(void)
{
    /* Port Status:
    0x04 - Sega-tap is connected
    0x16 - Multi-tap is connected
    0x21-0x2F - Clock serial peripheral is connected
    0xF0 - Not Connected or Unknown Device
    0xF1 - Peripheral is directly connected

    Peripheral ID:
    0x02 - Digital Device Standard Format
    0x13 - Racing Device Standard Format
    0x15 - Analog Device Standard Format
    0x23 - Pointing Device Standard Format
    0x23 - Shooting Device Standard Format
    0x34 - Keyboard Device Standard Format
    0xE1 - Mega Drive 3-Button Pad
    0xE2 - Mega Drive 6-Button Pad
    0xE3 - Saturn Mouse
    0xFF - Not Connected

    Special Notes:

    If a peripheral is disconnected from a port, SMPC only returns 1 byte for
    that port(which is the port status 0xF0), at the next OREG is the port
    status of the next port.

    e.g. If Port 1 has nothing connected, and Port 2 has a controller
        connected:

    OREG0 = 0xF0
    OREG1 = 0xF1
    OREG2 = 0x02
    etc.
    */

    pad_t* p = &_pad;
    unsigned short per_data;
    unsigned char port_con;
    unsigned char per_id;
    unsigned char oreg_counter=0;
    unsigned char per_counter=0;
    int i, i2;

    // See what is actually connected to Port 1/2
    for (i2 = 0; i2 < 2; i2++)
    {
        port_con = SMPC_REG_OREG(oreg_counter);
        oreg_counter++;

        if ((port_con & 0xF0) != 0x20)
        {
            for (i = 0; i < (port_con & 0x0F); i++)
            {
                per[i+per_counter].id = per_id = SMPC_REG_OREG(oreg_counter);
                oreg_counter++;

                //            if (perid == 0x02)
                //            {
                //               oldperpush[i+percounter] = per[i+percounter].butpush;
                //               perdata = (SMPC_REG_OREG(oregcounter) << 8) | SMPC_REG_OREG(oregcounter+1);
                //               oregcounter += 2;
                //            }
                //            else
                //               oregcounter += (perid & 0xF);
                switch (per_id >> 4)
                {
                case 0x0: // Standard Pad type
                case 0x1: // Analog type(Racing wheel/Analog pad)
                case 0x3: // Keyboard type
                {
                    old_per_push[i+per_counter] = per[i+per_counter].but_push;
                    per_data = (SMPC_REG_OREG(oreg_counter) << 8) | SMPC_REG_OREG(oreg_counter+1);
                    oreg_counter += 2;

                    switch (per_id & 0xF)
                    {
                    case 0x3: // Racing
                        oreg_counter += 1;
                        break;
                    case 0x4: // Keyboard
                        ((keyboarddata_struct *)per)[i+per_counter].kbd_type = per_data & 0x7;
                        per_data |= 0x7;
                        ((keyboarddata_struct *)per)[i+per_counter].flags = SMPC_REG_OREG(oreg_counter);
                        ((keyboarddata_struct *)per)[i+per_counter].key = SMPC_REG_OREG(oreg_counter+1);
                        if (((keyboarddata_struct *)per)[i+per_counter].flags != 0x6)
                        {
                            if (((keyboarddata_struct *)per)[i+per_counter].key == KEY_LEFTSHIFT
                            ||  ((keyboarddata_struct *)per)[i+per_counter].key == KEY_RIGHTSHIFT)
                            {
                                ((keyboarddata_struct *)per)[i+per_counter].extra_state[0] = (((keyboarddata_struct *)per)[i+per_counter].flags >> 3) & 0x1;
                            }
                        }
                        oreg_counter += 2;
                        break;
                    case 0x5: // Analog Pad
                        oreg_counter += 3;
                        break;
                    default:
                        break;
                    }

                    per[i+per_counter].but_push = per_data ^ 0xFFFF;
                    per[i+per_counter].but_push_once = (old_per_push[i+per_counter] ^ per[i+per_counter].but_push) & per[i+per_counter].but_push;

                    break;
                }
                case 0x2: // Pointer type(Mouse/Gun)
                case 0xE: // Other type(Mega Drive 3/6 button pads/Shuttle Mouse)
                {
                    switch (per_id & 0xF)
                    {
                    case 0x2: // Mega Drive 6-button Pad
                    case 0x1: // Mega Drive 3-button Pad
                        break;
                    case 0x3: // Saturn/Shuttle Mouse
                    {
                        per_data = SMPC_REG_OREG(oreg_counter);
                        per[i+per_counter].but_push = (per_data & 0xF) << 8; // fix me
                        per[i+per_counter].but_push_once = (old_per_push[i+per_counter] ^ per[i+per_counter].but_push) & per[i+per_counter].but_push;
                        ((mousedata_struct *)per)[i+per_counter].flags = per_data >> 4;

                        ((mousedata_struct *)per)[i+per_counter].x = SMPC_REG_OREG(oreg_counter+1);
                        ((mousedata_struct *)per)[i+per_counter].y = SMPC_REG_OREG(oreg_counter+2);

                        // X Overflow
                        if (((mousedata_struct *)per)[i+per_counter].flags & 0x4)
                        ((mousedata_struct *)per)[i+per_counter].x++;

                        // X Sign
                        if (((mousedata_struct *)per)[i+per_counter].flags & 0x1)
                        ((mousedata_struct *)per)[i+per_counter].x = 0 - ((mousedata_struct *)per)[i+per_counter].x - 1;

                        // Y Overflow
                        if (((mousedata_struct *)per)[i+per_counter].flags & 0x8)
                        ((mousedata_struct *)per)[i+per_counter].y++;

                        // Y Sign
                        if (((mousedata_struct *)per)[i+per_counter].flags & 0x2)
                        ((mousedata_struct *)per)[i+per_counter].y = 0 - ((mousedata_struct *)per)[i+per_counter].y - 1;

                        break;
                    }
                    default:
                        break;
                    }

                    oreg_counter += (per_id & 0xF);
                    break;
                }
                default:
                    break;
                }

                /* Update pad_t structure with peripheral data. */
                p->old[per_counter] = p->new[per_counter];
                p->new[per_counter] = per[i+per_counter].but_push;
                p->delta[per_counter] = (p->new[per_counter] ^ p->old[per_counter]) & p->new[per_counter];
                pad_update_autorepeat(p, per_counter);

                /* Update peripheral count. */
                per_counter++;
                if(per_counter > p->per_count)
                {
                    p->per_count = per_counter;
                }
            }
        }
    }

    // Issue break
    SMPC_REG_IREG(0) = 0x40;
}

//////////////////////////////////////////////////////////////////////////////

unsigned char kbd_scancode_to_ascii(keyboarddata_struct *kbd)
{
   unsigned char key_table[0x90] = {
      0x00, // ??
      0x00, // F9
      0x00, // ??
      0x00, // F5
      0x00, // F3
      0x00, // F1
      0x00, // F2
      0x00, // F12
      0x00, // ??
      0x00, // F10
      0x00, // F8
      0x00, // F6
      0x00, // F4
      0x09, // TAB
      '`', // `
      0x00, // ??
      0x00, // ?? (0x10)
      0x00, // ALT
      0x00, // SHIFT
      0x00, // ??
      0x00, // CTRL
      'q',  // Q
      '1',  // 1
      0x00, // ALT
      0x00, // CTRL
      0x00, // KP Enter
      'z',  // Z
      's',  // S
      'a',  // A
      'w',  // W
      '2',  // 2
      0x00, // ??
      0x00, // ??(0x20)
      'c', // C
      'x', // X
      'd', // D
      'e', // E
      '4', // 4
      '3', // 3
      0x00, // ??
      0x00, // ??
      0x20, // Space
      'v', // V
      'f', // F
      't', // T
      'r', // R
      '5', // 5
      0x00, // ??
      0x00, // ?? (0x30)
      'n', // N
      'b', // B
      'h', // H
      'g', // G
      'y', // Y
      '6', // 6
      0x00, // ??
      0x00, // ??
      0x00, // ??
      'm', // M
      'j', // J
      'u', // U
      '7', // 7
      '8', // 8
      0x00, // ??
      0x00, // ?? (0x40)
      ',', // Comma
              'k', // K
      'i', // I
      'o', // O
      '0', // 0
      '9', // 9
      0x00, // ??
      0x00, // ??
      '.', // Period
      '/', // Forward slash
      'l', // L
      ';', // ;
      'p', // P
      '-', // Minus
      0x00, // ??
      0x00, // ??(0x50)
      0x00, // ??
      '\'', // Aspostrophe
      0x00, // ??
      '[', // Left Bracket
      '=', // Equals
      0x00, // ??
      0x00, // ??
      0x00, // Caps lock
      0x00, // Shift
      0x00, // Enter
      ']', // Right Bracket
      0x00, // ??
      '\\', // Back-slash
      0x00, // ??
      0x00, // ??
      0x00, // ?? (0x60)
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // Backspace
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // KP Home
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // ?? (0x70)
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // ESC
      0x00, // ??
      0x00, // F11
      '+',  // KP Plus
      0x00, // ??
      '-',  // KP Minus
      '*',  // KP Asterisk
      0x00, // ??
      0x00, // ??
      0x00, // ??
      '/',  // KP Forward Slash (0x80)
      0x00, // Insert
      0x00, // Pause
      0x00, // F7
      0x00, // Print Screen
      0x00, // Delete
      0x00, // Left Arrow
      0x00, // Home
      0x00, // End
      0x00, // Up Arrow
      0x00, // Down Arrow
      0x00, // Up?
      0x00, // Down?
      0x00, // Right Arrow
      0x00, // ??
      0x00  // ??
   };
   unsigned char key_table2[0x90] = {
      0x00, // ??
      0x00, // F9
      0x00, // ??
      0x00, // F5
      0x00, // F3
      0x00, // F1
      0x00, // F2
      0x00, // F12
      0x00, // ??
      0x00, // F10
      0x00, // F8
      0x00, // F6
      0x00, // F4
      0x09, // TAB
      '~', // `
      0x00, // ??
      0x00, // ?? (0x10)
      0x00, // ALT
      0x00, // SHIFT
      0x00, // ??
      0x00, // CTRL
      'Q',  // Q
      '!',  // 1
      0x00, // ALT
      0x00, // CTRL
      0x00, // KP Enter
      'Z',  // Z
      'S',  // S
      'A',  // A
      'W',  // W
      '@',  // 2
      0x00, // ??
      0x00, // ??(0x20)
      'C', // C
      'X', // X
      'D', // D
      'E', // E
      '$', // 4
      '#', // 3
      0x00, // ??
      0x00, // ??
      0x20, // Space
      'V', // V
      'F', // F
      'T', // T
      'R', // R
      '%', // 5
      0x00, // ??
      0x00, // ?? (0x30)
      'N', // N
      'B', // B
      'H', // H
      'G', // G
      'Y', // Y
      '^', // 6
      0x00, // ??
      0x00, // ??
      0x00, // ??
      'M', // M
      'J', // J
      'U', // U
      '&', // 7
      '*', // 8
      0x00, // ??
      0x00, // ?? (0x40)
      '<', // Comma
      'K', // K
      'I', // I
      'O', // O
      ')', // 0
      '(', // 9
      0x00, // ??
      0x00, // ??
      '>', // Period
      '?', // Forward slash
      'L', // L
      ':', // ;
      'P', // P
      '_', // Minus
      0x00, // ??
      0x00, // ??(0x50)
      0x00, // ??
      '\"', // Aspostrophe
      0x00, // ??
      '{', // Left Bracket
      '+', // Equals
      0x00, // ??
      0x00, // ??
      0x00, // Caps lock
      0x00, // Shift
      0x00, // Enter
      '}', // Right Bracket
      0x00, // ??
      '|', // Back-slash
      0x00, // ??
      0x00, // ??
      0x00, // ?? (0x60)
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // Backspace
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // KP Home
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // ?? (0x70)
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // ??
      0x00, // ESC
      0x00, // ??
      0x00, // F11
      '+',  // KP Plus
      0x00, // ??
      '-',  // KP Minus
      '*',  // KP Asterisk
      0x00, // ??
      0x00, // ??
      0x00, // ??
      '/',  // KP Forward Slash (0x80)
      0x00, // Insert
      0x00, // Pause
      0x00, // F7
      0x00, // Print Screen
      0x00, // Delete
      0x00, // Left Arrow
      0x00, // Home
      0x00, // End
      0x00, // Up Arrow
      0x00, // Down Arrow
      0x00, // Up?
      0x00, // Down?
      0x00, // Right Arrow
      0x00, // ??
      0x00  // ??
   };

   if (!(kbd->flags >> 4))
   {
      // Straight conversion
      if (!kbd->extra_state[0])
         return key_table[kbd->key];
      else if (key_table[kbd->key]) // SHIFT
         return key_table2[kbd->key];
   }

   // Depending on the other flags, adjust the returned ASCII code
   if (kbd->flags & 0x40) // CAPS LOCK
   {
      // Let's see if it's a letter
      if (key_table[kbd->key] >= 'a' && key_table[kbd->key] <= 'z')
         return (key_table[kbd->key] - 0x20);
   }

   return 0; // fix me
}


