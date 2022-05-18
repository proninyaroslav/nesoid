/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 Bero
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

#include "driver.h"

typedef struct {
           int PAL;
           #ifdef NETWORK
           int NetworkPlay;
           #endif
	   int SoundVolume;
           const char *GameGenie;
	   int SUnderBase;

	   /* Current first and last rendered scanlines. */
	   int FirstSLine;
	   int LastSLine;

	   /* Driver code(user)-specified first and last rendered scanlines.
	      Usr*SLine[0] is for NTSC, Usr*SLine[1] is for PAL.
           */
	   int UsrFirstSLine[2];
	   int UsrLastSLine[2];
	   int SnapName;
	   unsigned int SndRate;
} FCEUS;

extern FCEUS FSettings;

void FCEU_PrintError(char *format, ...);
void FCEU_DispMessage(char *format, ...);
#define FCEU_printf printf

void DrawTextTrans(uint8 *dest, uint32 width, uint8 *textmsg, uint8 fgcolor);
void FCEU_PutImage(void);
#ifdef FRAMESKIP
void FCEU_PutImageDummy(void);
#endif

extern uint8 Exit;
extern uint8 pale;
extern uint8 vsdip;

#define JOY_A   1
#define JOY_B   2
#define JOY_SELECT      4
#define JOY_START       8
#define JOY_UP  0x10
#define JOY_DOWN        0x20
#define JOY_LEFT        0x40
#define JOY_RIGHT       0x80

void DoCommand(uint8 c);
extern uint8 CommandQueue;

void FlushCommandQueue(void);
