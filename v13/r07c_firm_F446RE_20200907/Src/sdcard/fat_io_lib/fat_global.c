//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//                            FAT16/32 File IO Library
//                                    V2.6
//                              Ultra-Embedded.com
//                            Copyright 2003 - 2012
//
//                         Email: admin@ultra-embedded.com
//
//                                License: GPL
//   If you would like a version with a more permissive license for use in
//   closed source commercial applications please contact me for details.
//-----------------------------------------------------------------------------
//
// This file is part of FAT File IO Library.
//
// FAT File IO Library is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// FAT File IO Library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with FAT File IO Library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#include <string.h>

#include "fat_internal.h"
#include "fat_global.h"



/* Definition of SD card internals. */
#include "../sdcard.h"




/** 
 * int media_read(uint32 sector, uint8 *buffer, uint32 sector_count)
 * 
 * Params:
 * 	Sector: 32-bit sector number
 * 	Buffer: Target buffer to read n sectors of data into.
 * 	Sector_count: Number of sectors to read.
 * 
 * Return: 
 * 	int, 1 = success, 0 = failure.
 * 
 * Description:
 *    Application/target specific disk/media read function.
 *    Sector number (sectors are usually 512 byte pages) to read.
**/
int fat_media_read(uint32 sector, uint8 *buffer, uint32 sector_count)
{
    sdc_read_multiple_blocks(sector, buffer, sector_count);
    return (sc_get_sdcard_buff()->packet_timeout ? 0 : 1);
}





/**
 * Media Write API
 * 
 * int media_write(uint32 sector, uint8 *buffer, uint32 sector_count)
 * 
 * Params:
 * 	Sector: 32-bit sector number
 * 	Buffer: Target buffer to write n sectors of data from.
 * 	Sector_count: Number of sectors to write.
 * 
 * Return: 
 * 	int, 1 = success, 0 = failure.
 * 
 * Description:
 *    Application/target specific disk/media write function.
 *    Sector number (sectors are usually 512 byte pages) to write to.
 * 
 * File IO Library Linkage
 *    Use the following API to attach the media IO functions to the File IO library.
**/
int fat_media_write(uint32 sector, uint8 *buffer, uint32 sector_count)
{
    sdc_write_multiple_blocks(sector, buffer, sector_count);
    return (sc_get_sdcard_buff()->packet_timeout ? 0 : 1);
}


FL_GLOBAL* fat_get_global(void)
{
    sdcard_t* sd = sc_get_sdcard_buff();
    return &(sd->fl_global);
}
