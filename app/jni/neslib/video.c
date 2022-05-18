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

/****************************************/
/*		FCE Ultra		*/
/*					*/
/*		video.c			*/
/*					*/
/*  Some generic high-level video	*/
/*  related functions.			*/
/****************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef GP2X
#include <unistd.h>
#endif

#include "types.h"
#include "video.h"
#include "fce.h"
#include "svga.h"
#include "version.h"
#include "general.h"
#include "_memory.h"

uint8 *XBuf=NULL;

int InitVirtualVideo(void)
{
 if(!XBuf)		/* Some driver code may allocate XBuf externally. */
  if(!(XBuf = (uint8*) (FCEU_malloc(320 * 240))))
   return 0;
/*
 if(sizeof(uint8*)==4)
 {
  m=(uint32) XBuf;
  m+=8;m&=0xFFFFFFF8;
  XBuf=(uint8 *)m;
 }
*/
 memset(XBuf,0,320*240);
 return 1;
}

#ifndef ZLIB
static uint8 pcxheader[128] =
{
 10,5,1,8,1,0,1,0,0,1,240,0,2,1,234,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

int SaveSnapshot(void)
{
 char *fn=0;
 uint8 *tmp;
 int x,u,y;
 FILE *pp=NULL;

 for(u=0;u<999;u++)
 {
  pp=fopen((fn=FCEU_MakeFName(FCEUMKF_SNAP,u,"pcx")),"rb");
  if(pp==NULL) break;
  fclose(pp);
 }

 if(!(pp=fopen(fn,"wb")))
  return 0;

 {
  int totallines=FSettings.LastSLine-FSettings.FirstSLine+1;

  tmp=XBuf+8+FSettings.FirstSLine*272;

  pcxheader[10]=totallines;
  fwrite(pcxheader,1,128,pp);
  for(y=0;y<totallines;y++)
  {
   for(x=0;x<256;x++)
   {
    if(*tmp>=0xc0) fputc(0xC1,pp);
     fputc(*tmp,pp);
    tmp++;
   }
   tmp+=16;
  }
 }

 fputc(0xC,pp);
 for(x=0;x<256;x++)
 {
  uint8 r,g,b;

  FCEUD_GetPalette(x,&r,&g,&b);
  fputc(r,pp);
  fputc(g,pp);
  fputc(b,pp);
 }
 fclose(pp);

 return u+1;
}

#else

#include <zlib.h>
#include "crc32.h"

static int WritePNGChunk(FILE *fp, uint32 size, char *type, uint8 *data)
{
 uint32 crc;

 uint8 tempo[4];

 tempo[0]=size>>24;
 tempo[1]=size>>16;
 tempo[2]=size>>8;
 tempo[3]=size;

 if(fwrite(tempo,4,1,fp)!=1)
  return 0;
 if(fwrite(type,4,1,fp)!=1)
  return 0;

 if(size)
  if(fwrite(data,1,size,fp)!=size)
   return 0;

 crc=CalcCRC32(0,(uint8 *)type,4);
 if(size)
  crc=CalcCRC32(crc,data,size);

 tempo[0]=crc>>24;
 tempo[1]=crc>>16;
 tempo[2]=crc>>8;
 tempo[3]=crc;

 if(fwrite(tempo,4,1,fp)!=1)
  return 0;
 return 1;
}

int SaveSnapshot(void)
{
 char *fn=0;
 int totallines=FSettings.LastSLine-FSettings.FirstSLine+1;
 int x,u,y;
 FILE *pp=NULL;
 uint8 *compmem=NULL;
 unsigned long compmemsize=totallines*263+12;

 if(!(compmem=FCEU_malloc(compmemsize)))
  return 0;

 for(u=0;u<999;u++)
 {
  pp=fopen((fn=FCEU_MakeFName(FCEUMKF_SNAP,u,"png")),"rb");
  if(pp==NULL) break;
  fclose(pp);
 }

 if(!(pp=fopen(fn,"wb")))
  return 0;
 {
  static uint8 header[8]={137,80,78,71,13,10,26,10};
  if(fwrite(header,8,1,pp)!=1)
   goto PNGerr;
 }

 {
  uint8 chunko[13];

  chunko[0]=chunko[1]=chunko[3]=0;
  chunko[2]=0x1;			// Width of 256

  chunko[4]=chunko[5]=chunko[6]=0;
  chunko[7]=totallines;			// Height

  chunko[8]=8;				// bit depth
  chunko[9]=3;				// Color type; indexed 8-bit
  chunko[10]=0;				// compression: deflate
  chunko[11]=0;				// Basic adapative filter set(though none are used).
  chunko[12]=0;				// No interlace.

  if(!WritePNGChunk(pp,13,"IHDR",chunko))
   goto PNGerr;
 }

 {
  char pdata[256*3];

  //void FCEUD_GetPalette(uint8 i,uint8 *r, unsigned char *g, unsigned char *b);
  for(x=0;x<256;x++)
   FCEUD_GetPalette(x,(uint8*)(pdata+x*3),(unsigned char*)(pdata+x*3+1),(unsigned char*)(pdata+x*3+2));
   // static int WritePNGChunk(FILE *fp, uint32 size, char *type, uint8 *data)
  if(!WritePNGChunk(pp,256*3,"PLTE",(uint8 *)pdata))
   goto PNGerr;
 }

 {
  uint8 *tmp=XBuf+FSettings.FirstSLine*272+8;
  uint8 *dest,*mal,*mork;

  /* If memory couldn't be allocated, just use XBuf(screen contents
     will be corrupted for one frame, though.
  */
  if(!(mal=mork=dest=malloc((totallines<<8)+totallines)))
   mork=dest=XBuf;

  for(y=0;y<totallines;y++)
  {
   *dest=0;			// No filter.
   dest++;
   for(x=256;x;x--,tmp++,dest++)
    *dest=*tmp;
   tmp+=16;
  }

  if(compress(compmem,&compmemsize,mork,(totallines<<8)+totallines)!=Z_OK)
  {
   if(mal) free(mal);
   goto PNGerr;
  }
  if(mal) free(mal);
  if(!WritePNGChunk(pp,compmemsize,"IDAT",compmem))
   goto PNGerr;
 }
 if(!WritePNGChunk(pp,0,"IEND",0))
  goto PNGerr;

 free(compmem);
 fclose(pp);
#ifdef GP2X
 sync();
#endif

 return u+1;


 PNGerr:
 if(compmem)
  free(compmem);
 if(pp)
  fclose(pp);
 return(0);
}

#endif
