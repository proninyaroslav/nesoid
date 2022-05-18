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

#include "memfile.h"

void SaveState(const char *fname);
void LoadState(const char *fname);
int FCEUSS_LoadFP(MEMFILE *st, int make_backup);

extern uint8 StateName[2048];
extern uint8 StateFile[2048];
extern int CurrentState;
extern char SaveStateStatus[10];
void CheckStates(void);
void SaveStateRefresh(void);

typedef struct {
           void *v;
           uint32 s;
	   char *desc;
} SFORMAT;

void ResetExState(void (*PreSave)(void), void (*PostSave)(void));
void AddExState(void *v, uint32 s, int type, char *desc);

#define RLSB 		0x80000000
#define FCEUSTATE_RLSB RLSB

