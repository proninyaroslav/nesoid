/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Ben Parnell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*	Contains file I/O functions that write/read data    */
/*	LSB first.				            */


#include <stdio.h>
#include "types.h"
#include "_endian.h"

static uint8 s[4];

int write16(uint16 b, FILE *fp)
{
 s[0]=b;
 s[1]=b>>8;
 return((fwrite(s,1,2,fp)<2)?0:2);
}

int write32(uint32 b, FILE *fp)
{
 s[0]=b;
 s[1]=b>>8;
 s[2]=b>>16;
 s[3]=b>>24;
 return((fwrite(s,1,4,fp)<4)?0:4);
}

int read32(void *Bufo, FILE *fp)
{
 uint32 buf;
 if(fread(&buf,1,4,fp)<4)
  return 0;
 #ifdef LSB_FIRST
 *(uint32*)Bufo=buf;
 #else
 *(uint32*)Bufo=((buf&0xFF)<<24)|((buf&0xFF00)<<8)|((buf&0xFF0000)>>8)|((buf&0xFF000000)>>24);
 #endif
 return 1;
}

int read16(char *d, FILE *fp)
{
 #ifdef LSB_FIRST
 return((fread(d,1,2,fp)<2)?0:2);
 #else
 int ret;
 ret=fread(d+1,1,1,fp);
 ret+=fread(d,1,1,fp);
 return ret<2?0:2;
 #endif
}

