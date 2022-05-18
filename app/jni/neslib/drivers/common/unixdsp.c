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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>
#include <sys/soundcard.h>

#include "../../types.h"

static int format;
static int dspfd;

// fsize is in samples, not bytes(gets translated before ioctl())
int InitUNIXDSPSound(int *rate, int bits, int fsize, int nfrags)
{
 int x;

 printf("  Opening /dev/dsp...");
 dspfd=open("/dev/dsp",O_WRONLY);
 if(dspfd==-1) goto __disperror;

 if(!bits) goto skip16check;
 x=AFMT_S16_LE;
 format=0;
 printf("\n   Setting format to 16-bit, signed, LSB first...");
 if(ioctl(dspfd,SNDCTL_DSP_SETFMT,&x)==-1)
 {
  skip16check:
  x=AFMT_U8;
  printf("\n   Setting format to 8-bit, unsigned...");
  if(ioctl(dspfd,SNDCTL_DSP_SETFMT,&x)==-1) goto __disperror;
  format=1;
 }

 printf("\n   Setting fragment size to %d samples and number of fragments to %d...",1<<fsize,nfrags);

 if(!format)
  fsize++;
 x=fsize|(nfrags<<16);

 if(ioctl(dspfd,SNDCTL_DSP_SETFRAGMENT,&x)==-1)
  printf("ERROR (continuing anyway)\n");
 x=0;
 printf("\n   Setting mono sound...");  
 if(ioctl(dspfd,SNDCTL_DSP_STEREO,&x)==-1) goto __disperror;
 printf("\n   Setting playback rate of %d hz...",*rate);
 if(ioctl(dspfd,SNDCTL_DSP_SPEED,rate)==-1) goto __disperror;
 printf("Set to %d hz\n",*rate);
 if(*rate<8192 || *rate>65535)
 {
  printf("    Sample rate is out of the acceptable range(8192-65535).\n");
  close(dspfd);
  return(0);
 }
 return 1;
 __disperror:
 printf("ERROR\n");
 return 0;
}

void KillUNIXDSPSound(void)
{
  close(dspfd);
}

static int16 MBuffer[2048];
void WriteUNIXDSPSound(int32 *Buffer, int Count, int noblocking)
{
 int P,c;
 int32 *src=Buffer;

 if(format)
 {
  uint8 *dest=(uint8 *)MBuffer;
  for(P=Count;P;P--,dest++,src++)
   *dest=(uint8)((*src)>>8)^128;
  c=Count;
 }
 else
 {
  int16 *dest=MBuffer;
  for(P=Count;P;P--,dest++,src++)
   *dest=*src;
  c=Count<<1;
 }

// noblocking=!noblocking; // speed testing
 if(noblocking)
 {
  struct audio_buf_info ai;
  if(!ioctl(dspfd,SNDCTL_DSP_GETOSPACE,&ai))
   if(ai.bytes<c)
    return;
 }
 write(dspfd,(uint8 *)MBuffer,c);
}
