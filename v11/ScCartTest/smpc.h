/**
 *  Cartridge Test Utility
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/pskai/
 *
 *  See LICENSE file for details.
**/

#ifndef _SMPC_H_
#define _SMPC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define SMPC_REG_IREG(i)        *((volatile unsigned char *)0x20100001+((i) * 2))
#define SMPC_REG_OREG(o)        *((volatile unsigned char *)0x20100021+((o) * 2))

#define SMPC_CMD_MSHON				0x00
#define SMPC_CMD_SSHON				0x02
#define SMPC_CMD_SSHOFF				0x03
#define SMPC_CMD_SNDON				0x06
#define SMPC_CMD_SNDOFF				0x07
#define SMPC_CMD_CDON				0x08
#define SMPC_CMD_CDOFF				0x09
#define SMPC_CMD_CARTON				0x0A
#define SMPC_CMD_CARTOFF			0x0B
#define SMPC_CMD_SYSRES				0x0D
#define SMPC_CMD_CKCHG352			0x0E
#define SMPC_CMD_CKCHG320			0x0F
#define SMPC_CMD_INTBACK			0x10
#define SMPC_CMD_SETTIM				0x16
#define SMPC_CMD_SETSM				0x17
#define SMPC_CMD_NMIREQ				0x18
#define SMPC_CMD_RESENA				0x19
#define SMPC_CMD_RESDIS				0x1A


#define SMPC_BASE   0x20100000
#define IREG0       (*(volatile unsigned char*)(SMPC_BASE+0x01))
#define IREG1       (*(volatile unsigned char*)(SMPC_BASE+0x03))
#define IREG2       (*(volatile unsigned char*)(SMPC_BASE+0x05))
#define IREG3       (*(volatile unsigned char*)(SMPC_BASE+0x07))
#define IREG4       (*(volatile unsigned char*)(SMPC_BASE+0x09))
#define IREG5       (*(volatile unsigned char*)(SMPC_BASE+0x0b))
#define IREG6       (*(volatile unsigned char*)(SMPC_BASE+0x0d))
#define COMREG      (*(volatile unsigned char*)(SMPC_BASE+0x1f))
#define OREG0       (*(volatile unsigned char*)(SMPC_BASE+0x21))
#define OREG1       (*(volatile unsigned char*)(SMPC_BASE+0x23))
#define OREG2       (*(volatile unsigned char*)(SMPC_BASE+0x25))
#define OREG3       (*(volatile unsigned char*)(SMPC_BASE+0x27))
#define OREG4       (*(volatile unsigned char*)(SMPC_BASE+0x29))
#define OREG5       (*(volatile unsigned char*)(SMPC_BASE+0x2b))
#define OREG6       (*(volatile unsigned char*)(SMPC_BASE+0x2d))
#define OREG7       (*(volatile unsigned char*)(SMPC_BASE+0x2f))
#define OREG8       (*(volatile unsigned char*)(SMPC_BASE+0x31))
#define OREG9       (*(volatile unsigned char*)(SMPC_BASE+0x33))
#define OREG10      (*(volatile unsigned char*)(SMPC_BASE+0x35))
#define OREG11      (*(volatile unsigned char*)(SMPC_BASE+0x37))
#define OREG12      (*(volatile unsigned char*)(SMPC_BASE+0x39))
#define OREG13      (*(volatile unsigned char*)(SMPC_BASE+0x3b))
#define OREG14      (*(volatile unsigned char*)(SMPC_BASE+0x3d))
#define OREG15      (*(volatile unsigned char*)(SMPC_BASE+0x3f))
#define OREG16      (*(volatile unsigned char*)(SMPC_BASE+0x41))
#define OREG17      (*(volatile unsigned char*)(SMPC_BASE+0x43))
#define OREG18      (*(volatile unsigned char*)(SMPC_BASE+0x45))
#define OREG19      (*(volatile unsigned char*)(SMPC_BASE+0x47))
#define OREG20      (*(volatile unsigned char*)(SMPC_BASE+0x49))
#define OREG21      (*(volatile unsigned char*)(SMPC_BASE+0x4b))
#define OREG22      (*(volatile unsigned char*)(SMPC_BASE+0x4d))
#define OREG23      (*(volatile unsigned char*)(SMPC_BASE+0x4f))
#define OREG24      (*(volatile unsigned char*)(SMPC_BASE+0x51))
#define OREG25      (*(volatile unsigned char*)(SMPC_BASE+0x53))
#define OREG26      (*(volatile unsigned char*)(SMPC_BASE+0x55))
#define OREG27      (*(volatile unsigned char*)(SMPC_BASE+0x57))
#define OREG28      (*(volatile unsigned char*)(SMPC_BASE+0x59))
#define OREG29      (*(volatile unsigned char*)(SMPC_BASE+0x5b))
#define OREG30      (*(volatile unsigned char*)(SMPC_BASE+0x5d))
#define OREG31      (*(volatile unsigned char*)(SMPC_BASE+0x5f))
#define SR          (*(volatile unsigned char*)(SMPC_BASE+0x61))
#define SF          (*(volatile unsigned char*)(SMPC_BASE+0x63))
#define PDR1        (*(volatile unsigned char*)(SMPC_BASE+0x75))
#define PDR2        (*(volatile unsigned char*)(SMPC_BASE+0x77))
#define DDR1        (*(volatile unsigned char*)(SMPC_BASE+0x79))
#define DDR2        (*(volatile unsigned char*)(SMPC_BASE+0x7b))
#define IOSEL       (*(volatile unsigned char*)(SMPC_BASE+0x7d))
#define IOSEL1      1
#define IOSEL2      2

#define EXLE        (*(volatile unsigned char*)(SMPC_BASE+0x7f))
#define EXLE1       1
#define EXLE2       2

/* SMPC commands */
#define MSHON       0x00
#define SSHON       0x02
#define SSHOFF      0x03
#define SNDON       0x06
#define SNDOFF      0x07
#define CDON        0x08
#define CDOFF       0x09
#define SYSRES      0x0d
#define CKCHG352    0x0e
#define CKCHG320    0x0f
#define INTBACK     0x10
#define SETTIME     0x16
#define SETSMEM     0x17
#define NMIREQ      0x18
#define RESENAB     0x19
#define RESDISA     0x1a


//////////////////////////////////////////////////////////////////////////////

static inline void smpc_wait_till_ready ()
{
   // Wait until SF register is cleared
   while(SF & 0x1) { }
}

//////////////////////////////////////////////////////////////////////////////

static inline void smpc_issue_command(unsigned char cmd)
{
   // Set SF register so that no other command can be issued.
   SF = 1;

   // Writing COMREG starts execution
   COMREG = cmd;
}

//////////////////////////////////////////////////////////////////////////////
// Peripheral related
//////////////////////////////////////////////////////////////////////////////

#define MAX_PERIPHERALS 12

#define PER_STANDARDPAD         0x02
#define PER_RACINGWHEEL         0x13
#define PER_ANALOGPAD           0x15
#define PER_MOUSE               0x23
#define PER_GUN                 0x25
#define PER_KEYBOARD            0x34
#define PER_MEGADRIVE3BUTTON    0xE1
#define PER_MEGADRIVE6BUTTON    0xE2
#define PER_SHUTTLEMOUSE        0xE3

typedef struct
{
   unsigned short but_push; // currently pressed down buttons, etc.
   unsigned short but_push_once; // If button is freshly pressed
   unsigned long other[2];
   unsigned char id; // peripheral id
} per_data_struct;

typedef struct
{
   unsigned short but_push; // currently pressed down buttons, etc.
   unsigned short but_push_once; // If button is freshly pressed
   unsigned char kbd_type;
   unsigned char flags;
   unsigned char key;
   unsigned char extra_state[5];
   unsigned char id; // peripheral id
} keyboarddata_struct;

typedef struct
{
   unsigned short but_push; // currently pressed down buttons, etc.
   unsigned short but_push_once; // If button is freshly pressed
   short x;
   short y;
   unsigned long flags;
   unsigned char id; // peripheral id
} mousedata_struct;

typedef struct
{
   unsigned short but_push; // currently pressed down buttons, etc.
   unsigned short but_push_once; // If button is freshly pressed
   unsigned char x;
   unsigned char y;
   unsigned char z;
   unsigned char other[5];
   unsigned char id; // peripheral id
} analogdata_struct;

extern per_data_struct per[MAX_PERIPHERALS];

#define PAD_RIGHT       0x8000
#define PAD_LEFT        0x4000
#define PAD_DOWN        0x2000
#define PAD_UP          0x1000
#define PAD_START       0x0800
#define PAD_A           0x0400
#define PAD_C           0x0200
#define PAD_B           0x0100
#define PAD_RBUTTON     0x0080
#define PAD_X           0x0040
#define PAD_Y           0x0020
#define PAD_Z           0x0010
#define PAD_LBUTTON     0x0008

//OLD (cgfm2 sample)
//#define PAD_LBUTTON (1<<15) -> 0x0008 1<< 3 -12
//#define PAD_RBUTTON (1<< 3) -> 0x0080 1<< 7 +4
//#define PAD_X       (1<< 2) -> 0x0040 1<< 6 +4
//#define PAD_Y       (1<< 1) -> 0x0020 1<< 5 +4
//#define PAD_Z       (1<< 0) -> 0x0010 1<< 4 +4
//#define PAD_START   (1<<11) -> 0x0800 1<<11  0
//#define PAD_A       (1<<10) -> 0x0400 1<<10  0
//#define PAD_C       (1<< 9) -> 0x0200 1<< 9  0
//#define PAD_B       (1<< 8) -> 0x0100 1<< 8  0
//#define PAD_RIGHT   (1<< 7) -> 0x8000 1<<15 +8
//#define PAD_LEFT    (1<< 6) -> 0x4000 1<<14 +8
//#define PAD_DOWN    (1<< 5) -> 0x2000 1<<13 +8
//#define PAD_UP      (1<< 4) -> 0x1000 1<<12 +8
//////////////////////////////////////////////////////////////////////////////
// Keyboard related
//////////////////////////////////////////////////////////////////////////////

#define KEY_ESC         0x76
#define KEY_F1          0x05
#define KEY_F2          0x06
#define KEY_F3          0x04
#define KEY_F4          0x0C
#define KEY_F5          0x03
#define KEY_F6          0x0B
#define KEY_F7          0x83
#define KEY_F8          0x0A
#define KEY_F9          0x01
#define KEY_F10         0x09
#define KEY_F11         0x78
#define KEY_F12         0x07
#define KEY_PRINTSCREEN 0x84
#define KEY_SCROLLLOCK  0x7E
#define KEY_PAUSE       0x82
#define KEY_TILDE       0x0E
#define KEY_1           0x16
#define KEY_2           0x1E
#define KEY_3           0x26
#define KEY_4           0x25
#define KEY_5           0x2E
#define KEY_6           0x36
#define KEY_7           0x3D
#define KEY_8           0x3E
#define KEY_9           0x46
#define KEY_0           0x45
#define KEY_MINUS       0x4E
#define KEY_EQUAL       0x55
#define KEY_BACKSLASH   0x5D
#define KEY_BACKSPACE   0x66
#define KEY_TAB         0x0D
#define KEY_Q           0x15
#define KEY_W           0x1D
#define KEY_E           0x24
#define KEY_R           0x2D
#define KEY_T           0x2C
#define KEY_Y           0x35
#define KEY_U           0x3C
#define KEY_I           0x43
#define KEY_O           0x44
#define KEY_P           0x4D
#define KEY_LEFTBRACKET 0x54
#define KEY_RIGHTBRACKET 0x5B
#define KEY_ENTER       0x5A
#define KEY_CAPSLOCK    0x58
#define KEY_A           0x1C
#define KEY_S           0x1B
#define KEY_D           0x23
#define KEY_F           0x2B
#define KEY_G           0x34
#define KEY_H           0x33
#define KEY_J           0x3B
#define KEY_K           0x42
#define KEY_L           0x4B
#define KEY_COLON       0x4C
#define KEY_QUOTE       0x52
#define KEY_LEFTSHIFT   0x12
#define KEY_Z           0x1A
#define KEY_X           0x22
#define KEY_C           0x21
#define KEY_V           0x2A
#define KEY_B           0x32
#define KEY_N           0x31
#define KEY_M           0x3A
#define KEY_COMMA       0x41
#define KEY_PERIOD      0x49
#define KEY_FORWARDSLASH 0x4A
#define KEY_RIGHTSHIFT  0x59
#define KEY_LEFTCTRL    0x14
#define KEY_LEFTALT     0x11
#define KEY_SPACE       0x29
#define KEY_RIGHTALT    0x17
#define KEY_RIGHTCTRL   0x18
#define KEY_INSERT      0x81
#define KEY_HOME        0x87
#define KEY_PAGEUP      0x8B
#define KEY_DELETE      0x85
#define KEY_END         0x88
#define KEY_PAGEDOWN    0x8C
#define KEY_UPARROW     0x89
#define KEY_LEFTARROW   0x86
#define KEY_DOWNARROW   0x8A
#define KEY_RIGHTARROW  0x8D
#define KEY_NUMLOCK     0x77
#define KEY_KPFORWARDSLASH 0x80
#define KEY_KPASTERISK  0x7C
#define KEY_KPMINUS     0x7B
#define KEY_KPHOME      0x6C
#define KEY_KPUPARROW   0x75
#define KEY_KPPAGEUP    0x7D
#define KEY_KPPLUS      0x79
#define KEY_KPLEFTARROW 0x6B
#define KEY_KPCENTER    0x73
#define KEY_KPRIGHTARROW 0x74
#define KEY_KPEND       0x69
#define KEY_KPDOWNARROW 0x72
#define KEY_KPPAGEDOWN  0x7A
#define KEY_KPENTER     0x19
#define KEY_KPINSERT    0x70
#define KEY_KPDELETE    0x71

unsigned char kbd_scancode_to_ascii(keyboarddata_struct *kbd);


//////////////////////////////////////////////////////////////////////////////
// Peripheral related
//////////////////////////////////////////////////////////////////////////////

/**
 * Several ways to acquire peripheral data from smpc :
 * - Direct mode is relatively fast and simple, 
 *   but works fine only with one standard pad.
 * - Intback poll mode takes time to execute, but is
 *   compatible with a lot of peripheral devices.
 * - Intback interrupt mode is fast and compatible
 *   with a lot of devices, but requires 2 interrupts.
**/
typedef enum _pad_acq_t
{
    PAD_ACQ_DIRECT,
    PAD_ACQ_INTBACK_POLL,
    PAD_ACQ_INTBACK_INTERRUPT,

    PAD_ACQ_DEFAULT // Intback, polling mode
} pad_acq_t;

typedef struct
{
    /* Acquisition type. */
    pad_acq_t type;

    /* Port connection status (bitfield). */
    unsigned char port_connected;

    /* Peripehal connection count. */
    unsigned short per_count;

    /* Buttons status. */
    unsigned short new  [MAX_PERIPHERALS];
    unsigned short old  [MAX_PERIPHERALS];
    unsigned short delta[MAX_PERIPHERALS];

    /* Number of times button hold is considered as button push. */
    unsigned char repeat_th;
    /* Number of times button hold counters. */
    unsigned char repeat_cnt[MAX_PERIPHERALS][13];
} pad_t;


/**
 * pad_init
 * Init SMPC registers.
 * Call this function during main program startup.
:*/
void pad_init(void);

/**
 * pad_shutdown
 * Reset SMPC registers.
 * Call this function before main program exits.
**/
void pad_shutdown(void);

/**
 * pad_set_autorepeat
 * Set key autorepeat count.
 * autorepeat count unit is number of times pad_read function is called.
 * autorepeat count range is zero~255.
 * Call this function after calling pad_init function.
**/
#define PAD_AUTOREPEAT_OFF      0
#define PAD_AUTOREPEAT_DEFAULT 30
void pad_set_autorepeat(int val);

/**
 * pad_read
 * Call this function in order to retrieve pad status.
**/
pad_t* pad_read(void);


#endif /* _SMPC_H_ */

