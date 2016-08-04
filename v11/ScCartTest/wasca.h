/**
 *  Cartridge Test Utility
 *  by cafe-alpha.
 *  WWW: http://ppcenter.free.fr/pskai/
 *
 *  See LICENSE file for details.
**/

#ifndef WASCA_H
#define WASCA_H


//---------------------------------------------------------------------------------
// Wasca Registers definitions.
// 
//---------------------------------------------------------------------------------


// SD card
#define REG_WASCA_SD_BUFFER (*(volatile unsigned char *)(0x23FF0000))
#define REG_WASCA_SD_CID    (*(volatile unsigned char *)(0x23FF0200))
#define REG_WASCA_SD_CSD    (*(volatile unsigned char *)(0x23FF0210))
#define REG_WASCA_SD_OCR    (*(volatile unsigned long *)(0x23FF0220))
#define REG_WASCA_SD_SR     (*(volatile unsigned long *)(0x23FF0224))
#define REG_WASCA_SD_RCA    (*(volatile unsigned short*)(0x23FF0228))
#define REG_WASCA_SD_CMDARG (*(volatile unsigned long *)(0x23FF022C))
#define REG_WASCA_SD_CMD    (*(volatile unsigned short*)(0x23FF0230))
#define REG_WASCA_SD_ASR    (*(volatile unsigned short*)(0x23FF0234))
#define REG_WASCA_SD_RR1    (*(volatile unsigned short*)(0x23FF0238))

#define REG_WASCA_SD_WSSCR  (*(volatile unsigned short*)(0x23FF0FFE))



// Wasca system area
#define REG_WASCA_SYS_PCNTR  (*(volatile unsigned short*)(0x23FFFFF0))
#define REG_WASCA_SYS_STATUS (*(volatile unsigned short*)(0x23FFFFF2))

#define REG_WASCA_SYS_MODE   (*(volatile unsigned short*)(0x23FFFFF4))
#define WASCA_MODE_INIT  0x0000
#define WASCA_MODE_MEM05 0x0001
#define WASCA_MODE_MEM10 0x0002
#define WASCA_MODE_MEM20 0x0003
#define WASCA_MODE_MEM40 0x0004
#define WASCA_RAM_1      0x0010
#define WASCA_RAM_2      0x0020
#define WASCA_ROM_KOF    0x0100
#define WASCA_ROM_ULTRA  0x0200
#define WASCA_BOOT       0x1000

#define REG_WASCA_SYS_HWVER  (*(volatile unsigned short*)(0x23FFFFF6))
#define REG_WASCA_SYS_SWVER  (*(volatile unsigned short*)(0x23FFFFF8))
#define REG_WASCA_SYS_SIG_PTR ((volatile unsigned short*)(0x23FFFFFA))

#define REG_WASCA_SYSTEM_BASE 0x23FFF000
#define REG_WASCA_CS1_ID      0x24FFFFFC


#endif // WASCA_H
