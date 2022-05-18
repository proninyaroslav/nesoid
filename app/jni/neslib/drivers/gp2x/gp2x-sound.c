/* FCE Ultra - NES/Famicom Emulator
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

#include <stdio.h>

#include "../../driver.h"
#include "gp2x.h"
#include "minimal.h"
#include "throttle.h"


extern int soundvol;


void WriteSound(int16 *Buffer, int Count)
{
	gp2x_sound_write(Buffer, Count<<1);
//	SpeedThrottle();
}

void SilenceSound(int n)
{
	soundvol=0;
}

int InitSound(void)
{
	FCEUI_Sound(Settings.sound_rate);
	gp2x_sound_volume(soundvol, soundvol);
	return 1;
}

uint32 GetMaxSound(void)
{
	return(4096);
}

uint32 GetWriteSound(void)
{
	return 1024;
}

int KillSound(void)
{
	//FCEUI_Sound(0);

	return 1;
}

