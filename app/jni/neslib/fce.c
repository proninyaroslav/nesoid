/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 BERO
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

#include        <string.h>
#include	<stdio.h>
#include	<stdlib.h>

#include	"types.h"
#include	"x6502.h"
#include	"fce.h"
#include	"fceu098.h"
#include	"sound.h"
#include	"svga.h"
#include	"netplay.h"
#include	"general.h"
#include	"_endian.h"
#include	"version.h"
#include        "_memory.h"

#include	"cart.h"
#include	"nsf.h"
#include	"fds.h"
#include	"ines.h"
#include	"unif.h"
#include	"vsuni.h"
#include        "cheat.h"

#include	"state.h"
#include        "video.h"
#include	"input.h"
#include	"file.h"
#include	"crc32.h"
#include        "ppu.h"
#include        "ppu098.h"

#include        "palette.h"
#include        "movie.h"

#include        "dprintf.h"

#ifdef GP2X
#include	"drivers/gp2x/asmutils.h"
#endif

#define Pal     (PALRAM)


static void (*RefreshLine)(uint8 *P, uint32 vofs) = NULL;
static void PRefreshLine(void);

static void ResetPPU(void);
static void PowerPPU(void);

uint64 timestampbase=0;

int ppudead=1;
int kook=0;

int MMC5Hack;
uint32 MMC5HackVROMMask;
uint8 *MMC5HackExNTARAMPtr;
uint8 *MMC5HackVROMPTR;
uint8 MMC5HackCHRMode=0;
uint8 MMC5HackSPMode;
uint8 MMC5HackSPScroll;
uint8 MMC5HackSPPage;

uint8 *MMC5SPRVPage[8];
uint8 *MMC5BGVPage[8];


uint8 VRAMBuffer,PPUGenLatch;

uint8 *vnapage[4];
uint8 PPUNTARAM;
uint8 PPUCHRRAM;

/* Color deemphasis emulation.  Joy... */
static uint8 deemp=0;
static int deempcnt[8];

FCEUGI FCEUGameInfo;
void (*GameInterface)(int h, void *param);

void FP_FASTAPASS(1) (*PPU_hook)(uint32 A);

void (*GameStateRestore)(int version);
void (*GameHBIRQHook)(void), (*GameHBIRQHook2)(void);

readfunc ARead[0x10000];
writefunc BWrite[0x10000];
static readfunc *AReadG;
static writefunc *BWriteG;
static int RWWrap=0;

#ifdef ASM_6502
#ifdef DEBUG_ASM_6502
extern uint8  nes_internal_ram[0x800];
#else
static void asmcpu_update(int32 cycles)
{
 // some code from x6502.c
 fhcnt-=cycles;
 if(fhcnt<=0)
 {
  FrameSoundUpdate();
  fhcnt+=fhinc;
 }

 if(PCMIRQCount>0)
 {
  PCMIRQCount-=cycles;
  if(PCMIRQCount<=0)
  {
   vdis=1;
   if((PSG[0x10]&0x80) && !(PSG[0x10]&0x40))
   {
    extern uint8 SIRQStat;
    SIRQStat|=0x80;
    X6502_IRQBegin(FCEU_IQDPCM);
   }
  }
 }
}
#endif

void asmcpu_unpack(void)
{
	nes_registers[0] = X.A << 24;
	nes_registers[1] = X.X;
	nes_registers[2] = X.Y;
	pc_base = 0;
	nes_registers[3] = X.PC;
	X6502_Rebase_a();
	nes_registers[4] = X.S << 24;
	nes_registers[4]|= X.IRQlow << 8;
	if (MapIRQHook)
		nes_registers[4] |= 1<<16; // MapIRQHook set bit
	nes_registers[7] = (uint32)X.count << 16;

	// NVUB DIZC
	nes_registers[4]|= X.P & 0x5d;
	nes_registers[5] = X.P << 24; // N
	if (!(X.P&0x02)) nes_registers[5] |= 1; // Z

#ifdef DEBUG_ASM_6502
	memcpy(nes_internal_ram, RAM, 0x800);
#endif
}

void asmcpu_pack(void)
{
	X.A = nes_registers[0] >> 24;
	X.X = nes_registers[1];
	X.Y = nes_registers[2];
	X.PC= nes_registers[3] - pc_base;
	X.S = nes_registers[4] >> 24;
	X.IRQlow = nes_registers[4] >> 8;
	X.count = (int32) nes_registers[7] >> 16;

	// NVUB DIZC
	X.P = nes_registers[4] & 0x5d;
	if (  nes_registers[5]&0x80000000)  X.P |= 0x80; // N
	if (!(nes_registers[5]&0x000000ff)) X.P |= 0x02; // Z
}
#endif

DECLFW(BNull)
{

}

DECLFR(ANull)
{
 //printf("open [%04x] %02x @ %04x (%04x)\n", A, X.DB, X.PC, X.PC&0x7ff);
 return(X.DB);
}

int AllocGenieRW(void)
{
 if(!(AReadG=FCEU_malloc(0x8000*sizeof(readfunc))))
  return 0;
 if(!(BWriteG=FCEU_malloc(0x8000*sizeof(writefunc))))
  return 0;
 RWWrap=1;
 return 1;
}

void FlushGenieRW(void)
{
 int32 x;

 if(RWWrap)
 {
  for(x=0;x<0x8000;x++)
  {
   ARead[x+0x8000]=AReadG[x];
   BWrite[x+0x8000]=BWriteG[x];
  }
#ifdef ASM_6502
  GenieSetPages(1);
#endif
  free(AReadG);
  free(BWriteG);
  AReadG=0;
  BWriteG=0;
  RWWrap=0;
 }
}

readfunc FASTAPASS(1) GetReadHandler(int32 a)
{
  if(a>=0x8000 && RWWrap)
   return AReadG[a-0x8000];
  else
   return ARead[a];
}

void FASTAPASS(3) SetReadHandler(int32 start, int32 end, readfunc func)
{
  int32 x;

  if(!func)
   func=ANull;

  if(RWWrap)
   for(x=end;x>=start;x--)
   {
    if(x>=0x8000)
     AReadG[x-0x8000]=func;
    else
     ARead[x]=func;
   }
  else

   for(x=end;x>=start;x--)
    ARead[x]=func;
}

writefunc FASTAPASS(1) GetWriteHandler(int32 a)
{
  if(RWWrap && a>=0x8000)
   return BWriteG[a-0x8000];
  else
   return BWrite[a];
}

void FASTAPASS(3) SetWriteHandler(int32 start, int32 end, writefunc func)
{
  int32 x;

  if(!func)
   func=BNull;

  if(RWWrap)
   for(x=end;x>=start;x--)
   {
    if(x>=0x8000)
     BWriteG[x-0x8000]=func;
    else
     BWrite[x]=func;
   }
  else
   for(x=end;x>=start;x--)
    BWrite[x]=func;
}

uint8 vtoggle=0;
uint8 XOffset=0;

uint32 TempAddr,RefreshAddr;


/* scanline is equal to the current visible scanline we're on. */

int scanline;

uint8 GameMemBlock[131072] __attribute__ ((aligned (4)));
uint8 NTARAM[0x800] __attribute__ ((aligned (4)));
uint8 PALRAM[0x20] __attribute__ ((aligned (4)));
#if !defined(ASM_6502) || defined(DEBUG_ASM_6502)
uint8 RAM[0x800] __attribute__ ((aligned (4)));
#endif

uint8 PPU[4];
uint8 PPUSPL;

uint8 PAL=0;


#define MMC5BGVRAMADR(V)      &MMC5BGVPage[(V)>>10][(V)]
#define	VRAMADR(V)	&VPage[(V)>>10][(V)]

static int linestartts;
static int tofix=0;

static uint8 *Plinef;

extern uint8 sprlinebuf[256+8];
extern int32 sphitx;
extern uint8 sphitdata;

extern int spork;       /* spork the world.  Any sprites on this line?
                           Then this will be set to 1.  Needed for zapper
                           emulation and *gasp* sprite emulation.
                        */

static void ResetRL(uint8 *target)
{
 if(InputScanlineHook)
  InputScanlineHook(0,0,0,0);
 Plinef=target;
 linestartts=timestamp*48+X6502_GetCycleCount();
 tofix=1;
}

static INLINE void Fixit1(void);

/* faking FCEUPPU_LineUpdate() from later versions of the emu */
static void FakedLineUpdate(void)
{
 #define TOFIXNUM (272-0x4)
 int lastpixel;

 if (scanline >= 240) return;

 if (tofix || sphitx != 0x100)
 {
  lastpixel = (timestamp*48-linestartts)>>4;
  if (PAL) lastpixel += lastpixel>>4;
  //printf("lastpixel: %i\n", lastpixel);
 }

 if (tofix && lastpixel>=TOFIXNUM)
 {
  Fixit1();
  tofix=0;
 }

 // CheckSpriteHit()
 if(sphitx!=0x100)
 {
  int l=lastpixel-16;
  int x;

  for(x=sphitx;x<(sphitx+8) && x<l;x++)
  {
   if((sphitdata&(0x80>>(x-sphitx))) && !(Plinef[x]&64))
   {
    PPU_status|=0x40;
    sphitx=0x100;
    break;
   }
  }
 }
}


static DECLFW(BRAML)
{
        RAM[A]=V;
}

static DECLFW(BRAMH)
{
        RAM[A&0x7FF]=V;
}

static DECLFR(ARAML)
{
        return RAM[A];
}

static DECLFR(ARAMH)
{
        return RAM[A&0x7FF];
}


static DECLFR(A2002)
{
	/* merged */
                        uint8 ret;

			FakedLineUpdate();
                        ret = PPU_status;
                        ret|=PPUGenLatch&0x1F;
                        vtoggle=0;
                        PPU_status&=0x7F;
			 PPUGenLatch=ret;
			//dprintf("r [2002] %02x",ret);
                        return ret;
}

static DECLFR(A200x)
{
	/* merged */
			FakedLineUpdate();
                        return PPUGenLatch;
}

static DECLFR(A2007)
{
	/* merged */
                        uint8 ret;
			uint32 tmp=RefreshAddr&0x3FFF;

			FakedLineUpdate();

                        ret=VRAMBuffer;

			if(PPU_hook) PPU_hook(tmp);
			PPUGenLatch=VRAMBuffer;
                        if(tmp<0x2000)
			{
			 VRAMBuffer=VPage[tmp>>10][tmp];
			}
                        else
			{
			 VRAMBuffer=vnapage[(tmp>>10)&0x3][tmp&0x3FF];
			}

                        if (INC32) RefreshAddr+=32;
                        else RefreshAddr++;
			if(PPU_hook) PPU_hook(RefreshAddr&0x3fff);
			dprintf("r [2007] %02x",ret);
                        return ret;
}

static DECLFW(B2000)
{
	/* NMI2? */
		FakedLineUpdate();
                PPUGenLatch=V;
                PPU[0]=V;
		TempAddr&=0xF3FF;
		TempAddr|=(V&3)<<10;
}

static DECLFW(B2001)
{
	/* merged */
		  FakedLineUpdate();
                  PPUGenLatch=V;
 	          PPU[1]=V;
		  if(V&0xE0)
		   deemp=V>>5;
		  //printf("$%04x:$%02x, %d\n",X.PC,V,scanline);
}

static DECLFW(B2002)
{
	/* merged */
                 PPUGenLatch=V;
}

static DECLFW(B2003)
{
	/* merged */
                PPUGenLatch=V;
                PPU[3]=V;
		PPUSPL=V&0x7;
}

static DECLFW(B2004)
{
	/* merged */
                PPUGenLatch=V;
		if(PPUSPL>=8)
		{
		 if(PPU[3]>=8)
  		  SPRAM[PPU[3]]=V;
		}
		else
		{
		 //printf("$%02x:$%02x\n",PPUSPL,V);
		 SPRAM[PPUSPL]=V;
		}
		PPU[3]++;
		PPUSPL++;

}

static DECLFW(B2005)
{
	/* merged */
		uint32 tmp=TempAddr;
		FakedLineUpdate();
		PPUGenLatch=V;
		if (!vtoggle)
                {
		 tmp&=0xFFE0;
		 tmp|=V>>3;
                 XOffset=V&7;
                }
                else
                {
                 tmp&=0x8C1F;
                 tmp|=((V&~0x7)<<2);
		 tmp|=(V&7)<<12;
                }

		TempAddr=tmp;
                vtoggle^=1;
}

static DECLFW(B2006)
{
	/* merged */
		       FakedLineUpdate();

                       PPUGenLatch=V;
                       if(!vtoggle)
                       {
			TempAddr&=0x00FF;
                        TempAddr|=(V&0x3f)<<8;
                       }
                       else
                       {
			TempAddr&=0xFF00;
		        TempAddr|=V;

                        RefreshAddr=TempAddr;
			if(PPU_hook)
			 PPU_hook(RefreshAddr);
                       }
                      vtoggle^=1;
}

static DECLFW(B2007)
{
	/* merged */
			uint32 tmp=RefreshAddr&0x3FFF;
                        PPUGenLatch=V;
                        if(tmp>=0x3F00)
                        {
                         // hmmm....
                         if(!(tmp&0xf))
                          PALRAM[0x00]=PALRAM[0x04]=PALRAM[0x08]=PALRAM[0x0C]=V&0x3f;
                         else if(tmp&3) PALRAM[(tmp&0x1f)]=V&0x3f;
                        }
                        else if(tmp<0x2000)
                        {
                          if(PPUCHRRAM&(1<<(tmp>>10)))
                            VPage[tmp>>10][tmp]=V;
                        }
                        else
			{
                         if(PPUNTARAM&(1<<((tmp&0xF00)>>10)))
                          vnapage[((tmp&0xF00)>>10)][tmp&0x3FF]=V;
                        }
                        if (INC32) RefreshAddr+=32;
                        else RefreshAddr++;
                        if(PPU_hook) PPU_hook(RefreshAddr&0x3fff);
}

static DECLFW(B4014)
{
	uint32 t=V<<8;
	int x;

	for(x=0;x<256;x++)
	 B2004(0x2004,X.DB=ARead[t+x](t+x));
	X6502_AddCycles(512);
}

void BGRender(uint8 *target)
{
	uint32 tem, vofs;
	vofs=((PPU[0]&0x10)<<8) | ((RefreshAddr>>12)&7);

        Pal[0]|=64;
        Pal[4]|=64;
        Pal[8]|=64;
        Pal[0xC]|=64;
        RefreshLine(target-XOffset, vofs);
        Pal[0]&=63;
        Pal[4]&=63;
        Pal[8]&=63;
        Pal[0xC]&=63;

        if(!(PPU[1]&2))
        {
         tem=Pal[0]|0x40;
	 tem|=tem<<8;
	 tem|=tem<<16;
         *(uint32 *)target=*(uint32 *)(target+4)=tem;
        }
}

#ifdef FRAMESKIP
int FSkip=0;
void FCEUI_FrameSkip(int x)
{
 FSkip=x;
}
#endif

/*	This is called at the beginning of each visible scanline */
static void LineUpdate(uint8 *target)
{
	uint32 tem;
	int y;

	/* PRefreshLine() will not get called on skipped frames.  This
	 * could cause a problem, but the solution would be rather complex,
	 * due to the current sprite 0 hit code.
	 */
	if(FSkip)
	{
	 y=(int)SPRAM[0] + 1;
	 if(scanline==y && SpriteON) PPU_status|=0x40; // hack
	 return;
	}

	if(scanline < FSettings.FirstSLine || scanline > FSettings.LastSLine)
	{
	   if(PPU_hook)
 	    PRefreshLine();
	   y=(int)SPRAM[0] + 1;
	   if(scanline==y && SpriteON) PPU_status|=0x40;
	}
	else
	{
	 if(ScreenON)
	 {
	   BGRender(target);
	 }
	 else
	 {
	   tem=Pal[0]|0x40;
	   tem|=tem << 8;
	   tem|=tem << 16;
	   FCEU_dwmemset(target,tem,256);
	 }
	}

        if(InputScanlineHook)
         InputScanlineHook(target,spork?sprlinebuf:0,linestartts,256);
}


static void LineUpdateEnd(uint8 *target)
{
#ifdef GP2X
 if(ScreenON || SpriteON)  // Yes, very el-cheapo.
 {
  if(PPU[1]&0x01)
   block_and(target, 256, 0x30);
 }
 if((PPU[1]>>5)==0x7)
  block_or(target, 256, 0xc0);
 else if(PPU[1]&0xE0)
  block_or(target, 256, 0x40);
 else
  block_andor(target, 256, 0x3f, 0x80);
#else
 int x;

 if(ScreenON || SpriteON)  // Yes, very el-cheapo.
 {
  if(PPU[1]&0x01)
  {
   for(x=63;x>=0;x--)
   *(uint32 *)&target[x<<2]=(*(uint32*)&target[x<<2])&0x30303030;
  }
 }
 if((PPU[1]>>5)==0x7)
 {
  for(x=63;x>=0;x--)
   *(uint32 *)&target[x<<2]=((*(uint32*)&target[x<<2])&0x3f3f3f3f)|0xc0c0c0c0;
 }
 else if(PPU[1]&0xE0)
  for(x=63;x>=0;x--)
   *(uint32 *)&target[x<<2]=(*(uint32*)&target[x<<2])|0x40404040;
 else
  for(x=63;x>=0;x--)
   *(uint32 *)&target[x<<2]=((*(uint32*)&target[x<<2])&0x3f3f3f3f)|0x80808080;
#endif

 // black borders
 ((uint32 *)target)[-2]=((uint32 *)target)[-1]=0;
 ((uint32 *)target)[64]=((uint32 *)target)[65]=0;
}

#define PAL(c)  ((c)+cc)


static void PRefreshLine(void)
{
         uint32 vofs;
	 int X1;
	 vofs=((PPU[0]&0x10)<<8) | ((RefreshAddr>>12)&7);
	 void (*PPU_hook_)(uint32 A) = PPU_hook;


         for(X1=33;X1;X1--)
         {
                uint32 zz2;
                uint32 vadr;

                zz2=(RefreshAddr>>10)&3;
                PPU_hook_(0x2000|(RefreshAddr&0xFFF));

                vadr=(vnapage[zz2][RefreshAddr&0x3ff]<<4)+vofs;

	        PPU_hook_(vadr);

                if((RefreshAddr&0x1f)==0x1f)
                 RefreshAddr^=0x41F;
                else
                 RefreshAddr++;
         }
}

/* This high-level graphics MMC5 emulation code was written
   for MMC5 carts in "CL" mode.  It's probably not totally
   correct for carts in "SL" mode.
   */
static void RefreshLine_MMC5Hack1(uint8 *P, uint32 vofs)
{
	  int8 tochange, X1;

          tochange=MMC5HackSPMode&0x1F;

          for(X1=33;X1;X1--,P+=8)
          {
                uint8 *C;
                uint8 cc,zz,zz2;
                uint32 vadr;

                if((tochange<=0 && MMC5HackSPMode&0x40) ||
		   (tochange>0 && !(MMC5HackSPMode&0x40)))
                {
                 uint8 xs,ys;

                 xs=33-X1;
                 ys=((scanline>>3)+MMC5HackSPScroll)&0x1F;
                 if(ys>=0x1E) ys-=0x1E;
                 vadr=(MMC5HackExNTARAMPtr[xs|(ys<<5)]<<4)+(vofs&7);

                 C = MMC5HackVROMPTR+vadr;
                 C += ((MMC5HackSPPage & 0x3f & MMC5HackVROMMask) << 12);

                 cc=MMC5HackExNTARAMPtr[0x3c0+(xs>>2)+((ys&0x1C)<<1)];
                 cc=((cc >> ((xs&2) + ((ys&0x2)<<1))) &3) <<2;
                }
                else
                {
                 zz=RefreshAddr&0x1F;
                 zz2=(RefreshAddr>>10)&3;
                 vadr=(vnapage[zz2][RefreshAddr&0x3ff]<<4)+vofs;
                 C = MMC5BGVRAMADR(vadr);
                 cc=vnapage[zz2][0x3c0+(zz>>2)+((RefreshAddr&0x380)>>4)];
                 cc=((cc >> ((zz&2) + ((RefreshAddr&0x40)>>4))) &3) <<2;
                }
                #include "fceline.h"

                if((RefreshAddr&0x1f)==0x1f)
                 RefreshAddr^=0x41F;
                else
                 RefreshAddr++;
                tochange--;
          }
}

static void RefreshLine_MMC5Hack2(uint8 *P, uint32 vofs)
{
          int8 tochange, X1;

          tochange=MMC5HackSPMode&0x1F;

          for(X1=33;X1;X1--,P+=8)
          {
                uint8 *C;
                uint8 cc;
                uint8 zz2;
                uint32 vadr;

                if((tochange<=0 && MMC5HackSPMode&0x40) ||
                   (tochange>0 && !(MMC5HackSPMode&0x40)))
                {
                 uint8 xs,ys;

                 xs=33-X1;
                 ys=((scanline>>3)+MMC5HackSPScroll)&0x1F;
                 if(ys>=0x1E) ys-=0x1E;
                 vadr=(MMC5HackExNTARAMPtr[xs|(ys<<5)]<<4)+(vofs&7);

                 C = MMC5HackVROMPTR+vadr;
                 C += ((MMC5HackSPPage & 0x3f & MMC5HackVROMMask) << 12);

                 cc=MMC5HackExNTARAMPtr[0x3c0+(xs>>2)+((ys&0x1C)<<1)];
                 cc=((cc >> ((xs&2) + ((ys&0x2)<<1))) &3) <<2;
                }
                else
                {
                 C=MMC5HackVROMPTR;
                 zz2=(RefreshAddr>>10)&3;
                 vadr = (vnapage[zz2][RefreshAddr & 0x3ff] << 4) + vofs;
                 C += (((MMC5HackExNTARAMPtr[RefreshAddr & 0x3ff]) & 0x3f &
                         MMC5HackVROMMask) << 12) + (vadr & 0xfff);
                 vadr = (MMC5HackExNTARAMPtr[RefreshAddr & 0x3ff] & 0xC0)>> 4;
                 cc = vadr;
		}
                #include "fceline.h"
                if((RefreshAddr&0x1f)==0x1f)
                 RefreshAddr^=0x41F;
                else
                 RefreshAddr++;
		tochange--;
          }
}

static void RefreshLine_MMC5Hack3(uint8 *P, uint32 vofs)
{
          int8 X1;

          for(X1=33;X1;X1--,P+=8)
          {
                uint8 *C;
                uint8 cc;
                uint8 zz2;
                uint32 vadr;

                C=MMC5HackVROMPTR;
                zz2=(RefreshAddr>>10)&3;
                vadr = (vnapage[zz2][RefreshAddr & 0x3ff] << 4) + vofs;
                C += (((MMC5HackExNTARAMPtr[RefreshAddr & 0x3ff]) & 0x3f &
			MMC5HackVROMMask) << 12) + (vadr & 0xfff);
                vadr = (MMC5HackExNTARAMPtr[RefreshAddr & 0x3ff] & 0xC0)>> 4;
                cc = vadr;

                #include "fceline.h"
                if((RefreshAddr&0x1f)==0x1f)
                 RefreshAddr^=0x41F;
                else
                 RefreshAddr++;
          }
}

static void RefreshLine_MMC5Hack4(uint8 *P, uint32 vofs)
{
          int8 X1;

          for(X1=33;X1;X1--,P+=8)
          {
                uint8 *C;
                uint8 cc,zz,zz2;
                uint32 vadr;

                zz=RefreshAddr&0x1F;
                zz2=(RefreshAddr>>10)&3;
                vadr=(vnapage[zz2][RefreshAddr&0x3ff]<<4)+vofs;
                C = MMC5BGVRAMADR(vadr);
                cc=vnapage[zz2][0x3c0+(zz>>2)+((RefreshAddr&0x380)>>4)];
                cc=((cc >> ((zz&2) + ((RefreshAddr&0x40)>>4))) &3) <<2;

		#include "fceline.h"

		if((RefreshAddr&0x1f)==0x1f)
                 RefreshAddr^=0x41F;
                else
                 RefreshAddr++;
          }
}

static void RefreshLine_PPU_hook(uint8 *P, uint32 vofs)
{
         int8 X1;
	 void (*PPU_hook_)(uint32 A) = PPU_hook;
	 uint32 rfraddr = RefreshAddr;
	 uint8 *page = vnapage[(rfraddr>>10)&3];

         for(X1=33;X1;X1--,P+=8)
         {
                uint8 *C;
                uint8 cc,zz;
                uint32 vadr;

                zz=rfraddr&0x1F;
                PPU_hook_(0x2000|(rfraddr&0xFFF));
                cc=page[0x3c0+(zz>>2)+((rfraddr&0x380)>>4)];
                cc=((cc >> ((zz&2) + ((rfraddr&0x40)>>4))) &3) <<2;
                vadr=(page[rfraddr&0x3ff]<<4)+vofs;
                C = VRAMADR(vadr);

	        #include "fceline.h"

	        PPU_hook_(vadr);

                if((rfraddr&0x1f)==0x1f) {
                 rfraddr^=0x41F;
	         page = vnapage[(rfraddr>>10)&3];
                } else
                 rfraddr++;
         }
	 RefreshAddr = rfraddr;
}

static void RefreshLine_normal(uint8 *P, uint32 vofs) // vofs is 0x107 max
{
         int8 X1;
	 uint32 rfraddr = RefreshAddr;
	 uint8 *page = vnapage[(rfraddr>>10)&3];
         uint32 cc2=0;

	 if ((rfraddr&0xc)!=0)
	  cc2=*(uint32 *) (page + ((rfraddr&0x380)>>4) + ((rfraddr&0x10)>>2) + 0x3c0);

         for (X1=33;X1;X1--,P+=8)
         {
                uint8 cc,*C;
                uint32 vadr;

                vadr=(page[rfraddr&0x3ff]<<4)+vofs;
                C = VRAMADR(vadr);
		if ((rfraddr&0xc)==0)
	         cc2=*(uint32 *) (page + ((rfraddr&0x380)>>4) + ((rfraddr&0x10)>>2) + 0x3c0);
	        cc=((cc2 >> ((rfraddr&2) + ((rfraddr&0x40)>>4) + ((rfraddr&0xc)<<1))) & 3) << 2;

	        #include "fceline.h"

                if((rfraddr&0x1f)==0x1f) {
                 rfraddr^=0x41F;
	         page = vnapage[(rfraddr>>10)&3];
                } else
                 rfraddr++;
         }
	 RefreshAddr = rfraddr;
}

static void SetRefreshLine(void)
{
        if(MMC5Hack && geniestage!=1)
        {
	 if(MMC5HackCHRMode==0 && (MMC5HackSPMode&0x80))
	 {
		 if (RefreshLine != RefreshLine_MMC5Hack1) printf("set refr RefreshLine_MMC5Hack1\n");
		 RefreshLine = RefreshLine_MMC5Hack1;
	 }
	 else if(MMC5HackCHRMode==1 && (MMC5HackSPMode&0x80))
	 {
		if (RefreshLine != RefreshLine_MMC5Hack2) printf("set refr RefreshLine_MMC5Hack2\n");
		 RefreshLine = RefreshLine_MMC5Hack2;
	 }
         else if(MMC5HackCHRMode==1)
         {
		if (RefreshLine != RefreshLine_MMC5Hack3) printf("set refr RefreshLine_MMC5Hack3\n");
		 RefreshLine = RefreshLine_MMC5Hack3;
         }
         else
         {
		if (RefreshLine != RefreshLine_MMC5Hack4) printf("set refr RefreshLine_MMC5Hack4\n");
		 RefreshLine = RefreshLine_MMC5Hack4;
         }
        }       // End if(MMC5Hack)
        else if(PPU_hook)
        {
		if (RefreshLine != RefreshLine_PPU_hook) printf("set refr RefreshLine_PPU_hook\n");
		RefreshLine = RefreshLine_PPU_hook;
        }
        else
        {
		if (RefreshLine != RefreshLine_normal) printf("set refr RefreshLine_normal\n");
		RefreshLine = RefreshLine_normal;
        }
}

static INLINE
void Fixit2(void)
{
   if(ScreenON || SpriteON)
   {
    uint32 rad=RefreshAddr;
    rad&=0xFBE0;
    rad|=TempAddr&0x041f;
    RefreshAddr=rad;
    //PPU_hook(RefreshAddr,-1);
   }
}

static INLINE
void Fixit1(void)
{
   if(ScreenON || SpriteON)
   {
    uint32 rad=RefreshAddr;

    if((rad&0x7000)==0x7000)
    {
     rad^=0x7000;
     if((rad&0x3E0)==0x3A0)
     {
      rad^=0x3A0;
      rad^=0x800;
     }
     else
     {
      if((rad&0x3E0)==0x3e0)
       rad^=0x3e0;
      else rad+=0x20;
     }
    }
    else
     rad+=0x1000;
    RefreshAddr=rad;
    //PPU_hook(RefreshAddr,-1);
   }
}


// ============================//
// end of new code
// ===========================//

void ResetMapping(void)
{
	int x;

        SetReadHandler(0x0000,0xFFFF,ANull);
	SetWriteHandler(0x0000,0xFFFF,BNull);

        SetReadHandler(0,0x7FF,ARAML);
        SetWriteHandler(0,0x7FF,BRAML);

        SetReadHandler(0x800,0x1FFF,ARAMH);  /* Part of a little */
        SetWriteHandler(0x800,0x1FFF,BRAMH); /* hack for a small speed boost. */

        for(x=0x2000;x<0x4000;x+=8)
        {
         ARead[x]=A200x;
         BWrite[x]=B2000;
         ARead[x+1]=A200x;
         BWrite[x+1]=B2001;
         ARead[x+2]=A2002;
         BWrite[x+2]=B2002;
         ARead[x+3]=A200x;
         BWrite[x+3]=B2003;
         ARead[x+4]=A200x;
         BWrite[x+4]=B2004;
         ARead[x+5]=A200x;
         BWrite[x+5]=B2005;
         ARead[x+6]=A200x;
         BWrite[x+6]=B2006;
         ARead[x+7]=A2007;
         BWrite[x+7]=B2007;
        }

        BWrite[0x4014]=B4014;
        SetNESSoundMap();
	InitializeInput();
}

int GameLoaded=0;
void CloseGame(void)
{
 FCEUI_StopMovie();
 if(GameLoaded)
 {
  if(FCEUGameInfo.type!=GIT_NSF)
   FCEU_FlushGameCheats(0,0);
  #ifdef NETWORK
  if(FSettings.NetworkPlay) KillNetplay();
  #endif
  GameInterface(GI_CLOSE, 0);
  CloseGenie();
  GameLoaded=0;
 }
}

void ResetGameLoaded(void)
{
        if(GameLoaded) CloseGame();
        GameStateRestore=0;
        PPU_hook=0;
        GameHBIRQHook=GameHBIRQHook2=0;
	GameExpSound.Fill=0;
	GameExpSound.RChange=0;
        if(GameExpSound.Kill)
         GameExpSound.Kill();
        GameExpSound.Kill=0;
        MapIRQHook=0;
        MMC5Hack=0;
        PAL&=1;
	pale=0;

	FCEUGameInfo.name=0;
	FCEUGameInfo.type=GIT_CART;
	FCEUGameInfo.vidsys=GIV_USER;
	FCEUGameInfo.input[0]=FCEUGameInfo.input[1]=-1;
	FCEUGameInfo.inputfc=-1;

	FCEUGameInfo.soundchan=0;
	FCEUGameInfo.soundrate=0;
        FCEUGameInfo.cspecial=0;
}

char lastLoadedGameName [2048];
int LoadGameLastError = 0;
int UNIFLoad(const char *name, int fp);
int iNESLoad(const char *name, int fp);
int FDSLoad(const char *name, int fp);
int NSFLoad(int fp);

FCEUGI *FCEUI_LoadGame(char *name)
{
	char name2[512];
	int have_movie = 0, have_ips = 0;
        int fp;

        //Exit=1;
	LoadGameLastError = 0;
        ResetGameLoaded();

	strncpy(name2, name, sizeof(name2));
	name2[sizeof(name2)-1] = 0;

	fp=FCEU_fopen(name2,"rb");
	if(!fp)
        {
 	 FCEU_PrintError("Error opening \"%s\"!",name);
	 LoadGameLastError = 1;
	 return 0;
	}

        {
	 char *p = name2 + strlen(name2) - 4;
	 if (strcasecmp(p, ".fcm") == 0) printf("movie detected\n"), have_movie = 1;
	 if (strcasecmp(p, ".ips") == 0) printf("ips detected\n"), have_ips = 1;
	 if (have_movie || have_ips)
	 {
	  // movie detected
	  FCEU_fclose(fp);
	  *p = 0;
	  fp=FCEU_fopen(name2,"rb");
	  if (!fp && p - name2 > 2)
	  {
	   for (p--; p > name2 && *p != '.'; p--)
           *p = 0;
	   fp=FCEU_fopen(name2,"rb");
	  }
	  if (!fp) {
	   printf("no ROM for ips/movie\n");
	   LoadGameLastError = 2;
	   return 0;
	  }
	 }
	}

	// do IPS patch
	if (have_ips)
	{
	 FCEU_fclose(fp);
	 FILE *ips = fopen(name, "rb");
	 if (!ips) return 0;
         fp=FCEU_fopen_forcemem(name2);
	 if (!fp) { fclose(ips); return 0; }
	 ApplyIPS(ips, fp); // closes ips
	}

        GetFileBase(name2);
        if(iNESLoad(name2,fp))
         goto endlseq;
        if(NSFLoad(fp))
         goto endlseq;
        if(FDSLoad(name2,fp))
         goto endlseq;
        if(UNIFLoad(name2,fp))
         goto endlseq;

        FCEU_PrintError("An error occurred while loading the file.");
        FCEU_fclose(fp);
	// format handlers may set LoadGameLastError themselves.
	if (LoadGameLastError == 0) LoadGameLastError = 3;
        return 0;

        endlseq:
        FCEU_fclose(fp);
        GameLoaded=1;

        FCEU_ResetVidSys();
        if(FCEUGameInfo.type!=GIT_NSF)
         if(FSettings.GameGenie)
	  OpenGenie(FSettings.GameGenie);

        PowerNES();
	#ifdef NETWORK
        if(FSettings.NetworkPlay) InitNetplay();
	#endif
        SaveStateRefresh();
        if(FCEUGameInfo.type!=GIT_NSF)
        {
	 FCEU_LoadGamePalette();
         FCEU_LoadGameCheats(0);
        }

	FCEU_ResetPalette();
        Exit=0;

	if (have_movie)
		FCEUI_LoadMovie(name, 1);

	strcpy(lastLoadedGameName, name2);

        return(&FCEUGameInfo);
}


void FCEU_ResetVidSys(void)
{
 int w;

 if(FCEUGameInfo.vidsys==GIV_NTSC)
  w=0;
 else if(FCEUGameInfo.vidsys==GIV_PAL)
  w=1;
 else
  w=FSettings.PAL;

 if(w)
 {
  PAL=1;
  FSettings.FirstSLine=FSettings.UsrFirstSLine[1];
  FSettings.LastSLine=FSettings.UsrLastSLine[1];
 }
 else
 {
  PAL=0;
  FSettings.FirstSLine=FSettings.UsrFirstSLine[0];
  FSettings.LastSLine=FSettings.UsrLastSLine[0];
 }
 printf("ResetVidSys: PAL = %i\n", PAL);
 SetSoundVariables();
}

int FCEUI_Initialize(void)
{
        if(!InitVirtualVideo())
         return 0;
	memset(&FSettings,0,sizeof(FSettings));
	FSettings.UsrFirstSLine[0]=8;
	FSettings.UsrFirstSLine[1]=0;
        FSettings.UsrLastSLine[0]=FSettings.UsrLastSLine[1]=239;
	FSettings.SoundVolume=100;

	FCEUI_Initialize098();
	FCEUI_SetEmuMode(0);

        return 1;
}

void FCEUI_Kill(void)
{
 FCEU_KillGenie();
}

static void EmLoop(void);

int use098code = 0;
void (*ResetNES)(void) = 0;
void (*FCEUI_Emulate)(void) = 0;

void FCEUI_SetEmuMode(int is_new)
{
   use098code = is_new;
   if (is_new)
   {
    ResetNES=ResetNES098;
    FCEUI_Emulate=FCEUI_Emulate098;
   }
   else
   {
    ResetNES=ResetNES081;
    FCEUI_Emulate=EmLoop;
   }
}

void MMC5_hb(int);     /* Ugh ugh ugh. */
static void DoLine(void)
{
   uint8 *target=XBuf+scanline*320+32;

   LineUpdate(target);

   if(MMC5Hack && (ScreenON || SpriteON)) MMC5_hb(scanline);

   X6502_Run(256);

   // check: Battletoads & Double Dragon, Addams Family
   // sky glitches in SMB1 if done wrong
   FakedLineUpdate();

#ifdef FRAMESKIP
   if(!FSkip)
#endif
   if(scanline>=FSettings.FirstSLine && scanline<=FSettings.LastSLine)
   {
    if(SpriteON && spork)
     CopySprites(target);

    LineUpdateEnd(target);
   }
   sphitx=0x100;

   if(ScreenON || SpriteON)
    FetchSpriteData();

   // DoHBlank();
   if(GameHBIRQHook && (ScreenON || SpriteON) && ((PPU[0]&0x38)!=0x18))
   {
    X6502_Run(6);
    Fixit2();
    X6502_Run(4);
    GameHBIRQHook();
    X6502_Run(85-10-16);
   }
   else
   {
    X6502_Run(6);  // Tried 65, caused problems with Slalom(maybe others)
    Fixit2();
    X6502_Run(85-6-16);
   }

   if(SpriteON)
    RefreshSprites();
   if(GameHBIRQHook2 && (ScreenON || SpriteON))
    GameHBIRQHook2();
   scanline++;
   if (scanline<240)
    ResetRL(XBuf+scanline*320+32);
   X6502_Run(16);
}


static void EmLoop(void)
{
  int x;
  uint32 scanlines_per_frame = PAL ? 312 : 262;
  UpdateInput();
  FCEU_ApplyPeriodicCheats();

  // FCEUPPU_Loop:
  if(ppudead) /* Needed for Knight Rider, possibly others. */
  {
   //memset(XBuf, 0, 320*240);
   //X6502_Run(scanlines_per_frame*(256+85));
   int lines;
   for (lines=scanlines_per_frame;lines;lines--)
     X6502_Run(256+85);
   ppudead--;
   goto update;
  }

  X6502_Run(256+85);

  PPU[2]|=0x80;
  PPU[3]=PPUSPL=0;	       /* Not sure if this is correct.  According
				  to Matt Conte and my own tests, it is.  Timing is probably
			 	  off, though.  NOTE:  Not having this here
				  breaks a Super Donkey Kong game. */

  X6502_Run(12);		/* I need to figure out the true nature and length
				   of this delay.
			 	*/
  if(FCEUGameInfo.type==GIT_NSF)
   DoNSFFrame();
  else if(VBlankON)
   TriggerNMI();

  // Note: this is needed for asm core
  // Warning: using 'scanline' var here breaks Castlevania III
  {
   int lines;
   X6502_Run(256+85-12);
   for (lines=scanlines_per_frame-242-1;lines;lines--)
     X6502_Run(256+85);
  }
  // X6502_Run((scanlines_per_frame-242)*(256+85)-12);
  PPU_status&=0x1f;
  X6502_Run(256);

  {
   if(ScreenON || SpriteON)
   {
    if(GameHBIRQHook)
     GameHBIRQHook();
     if(PPU_hook)
      for(x=0;x<42;x++) {PPU_hook(0x2000); PPU_hook(0);} // ugh
    if(GameHBIRQHook2)
     GameHBIRQHook2();
   }

   X6502_Run(85-16);

   if(ScreenON || SpriteON)
   {
    RefreshAddr=TempAddr;
    if(PPU_hook) PPU_hook(RefreshAddr&0x3fff);
   }
   spork=0;
   ResetRL(XBuf+32);

   X6502_Run(16-kook);
   kook ^= 1;
  }

  if(FCEUGameInfo.type==GIT_NSF)
  {
   // run scanlines for asm core to fuction
   for(scanline=0;scanline<240;scanline++)
    X6502_Run(256+85);
  }
  else
  {
   int x,max,maxref;

   deemp=PPU[1]>>5;
   SetRefreshLine();
   for(scanline=0;scanline<240;)       // scanline is incremented in  DoLine.  Evil. :/
   {
    deempcnt[deemp]++;
    DoLine();
   }
   if(MMC5Hack && (ScreenON || SpriteON)) MMC5_hb(scanline);
   for(x=1,max=0,maxref=0;x<7;x++)
   {
    if(deempcnt[x]>max)
    {
     max=deempcnt[x];
     maxref=x;
    }
    deempcnt[x]=0;
   }
   SetNESDeemph(maxref,0);
  }

update:
  {
   int ssize;

   ssize=FlushEmulateSound();

   timestampbase += timestamp;
   timestamp = 0;

   #ifdef FRAMESKIP
   if(FSkip)
   {
    FCEU_PutImageDummy();
    FSkip--;
    FCEUD_Update(0,WaveFinalMono,ssize);
   }
   else
   #endif
   {
    FCEU_PutImage();
    FCEUD_Update(XBuf+8,WaveFinalMono,ssize);
   }
  }
}

void FCEUI_CloseGame(void)
{
        Exit=1;
}

static void ResetPPU(void)
{
        VRAMBuffer=PPU[0]=PPU[1]=PPU[2]=PPU[3]=0;
        PPUSPL=0;
	PPUGenLatch=0;
        RefreshAddr=TempAddr=0;
        vtoggle = 0;
        ppudead = 2;
	kook = 0;
}

static void PowerPPU(void)
{
        memset(NTARAM,0x00,0x800);
        memset(PALRAM,0x00,0x20);
        memset(SPRAM,0x00,0x100);
	ResetPPU();
}

void ResetNES081(void)
{
        if(!GameLoaded) return;
        GameInterface(GI_RESETM2, 0);
        ResetSound();
        ResetPPU();
        X6502_Reset();
}

#ifndef DEBUG_ASM_6502
static void FCEU_MemoryRand(uint8 *ptr, uint32 size)
{
 int x=0;
 while(size)
 {
  *ptr=(x&4)?0xFF:0x00;
  x++;
  size--;
  ptr++;
 }
}
#endif

void PowerNES(void)
{
        if(!GameLoaded) return;

	FCEU_CheatResetRAM();
	FCEU_CheatAddRAM(2,0,RAM);

        GeniePower();

#ifndef DEBUG_ASM_6502
        FCEU_MemoryRand(RAM,0x800);
#else
        memset(RAM,0x00,0x800);
	memset(nes_internal_ram,0x00,0x800);
#endif
        ResetMapping();
        PowerSound();
	PowerPPU();

	if (use098code)
	 FCEUPPU_Power();

	/* Have the external game hardware "powered" after the internal NES stuff.
	   Needed for the NSF code and VS System code.
	*/
	GameInterface(GI_POWER, 0);
        if(FCEUGameInfo.type==GIT_VSUNI)
         FCEU_VSUniPower();
#ifdef ASM_6502
	if (geniestage)
	 GenieSetPages(0);
#endif
	timestampbase=0;
	X6502_Power();
	FCEU_PowerCheats();
}


/* savestate stuff */
uint16 TempAddrT,RefreshAddrT;

SFORMAT FCEUPPU_STATEINFO[]={
 { NTARAM, 0x800, "NTAR"},
 { PALRAM, 0x20, "PRAM"},
 { SPRAM, 0x100, "SPRA"},
 { PPU, 0x4, "PPUR"},
 { &XOffset, 1, "XOFF"},
 { &vtoggle, 1, "VTOG"},
 { &RefreshAddrT, 2|RLSB, "RADD"},
 { &TempAddrT, 2|RLSB, "TADD"},
 { &VRAMBuffer, 1, "VBUF"},
 { &PPUGenLatch, 1, "PGEN"},
 // from 0.98.15
 { &kook, 1, "KOOK"},
 { &ppudead, 1, "DEAD"},
 { &PPUSPL, 1, "PSPL"},
 { 0 }
 };


