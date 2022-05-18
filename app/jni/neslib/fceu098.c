/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2003 Xodnizel
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

#include	<stdio.h>

#include	"types.h"
#include	"fce.h"
#include	"ppu098.h"
#include	"sound.h"
#include	"input.h"
#include	"cart.h"
#include	"cheat.h"
#include	"x6502.h"
#include	"video.h"
#include	"svga.h"


int FCEUI_Initialize098(void)
{
	FCEUPPU_Init();
        return 1;
}


#ifdef FRAMESKIP
extern int FSkip;
#endif

void FCEUI_Emulate098(void)
{
  int ssize;

  UpdateInput();
  if(geniestage!=1) FCEU_ApplyPeriodicCheats();
  FCEUPPU_Loop(FSkip);

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


void ResetNES098(void)
{
 ResetNES081();
 // it was decided not to use 098 sound because of problems it causes
 //FCEUSND_Reset();
 FCEUPPU_Reset();
}

