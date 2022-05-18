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
#include <string.h>
#include <sys/time.h>

#include "../../video.h"

#include "main.h"
#include "gp2x.h"
#include "minimal.h"
#include "fonts.h"
#include "asmutils.h"

static char fps_str[32];
static int framesEmulated, framesRendered;

int gp2x_palette[256];
unsigned short gp2x_palette16[256];

int paletterefresh;

extern int eoptions;

#define FPS_COLOR 1


static void gp2x_text(unsigned char *screen, int x, int y, char *text, int color, int flip)
{
	int i,l,slen;
        slen=strlen(text);

	screen=screen+x+y*320;

	for (i=0;i<slen;i++)
        {
		for (l=0;l<8;l++)
                {

			screen[l*320+0]=(fontdata8x8[((text[i])*8)+l]&0x80)?color:0;
			screen[l*320+1]=(fontdata8x8[((text[i])*8)+l]&0x40)?color:0;
			screen[l*320+2]=(fontdata8x8[((text[i])*8)+l]&0x20)?color:0;
			screen[l*320+3]=(fontdata8x8[((text[i])*8)+l]&0x10)?color:0;
			screen[l*320+4]=(fontdata8x8[((text[i])*8)+l]&0x08)?color:0;
			screen[l*320+5]=(fontdata8x8[((text[i])*8)+l]&0x04)?color:0;
			screen[l*320+6]=(fontdata8x8[((text[i])*8)+l]&0x02)?color:0;
			screen[l*320+7]=(fontdata8x8[((text[i])*8)+l]&0x01)?color:0;

		}
		screen+=8;
	}
}


void CleanSurface(void)
{
	int c=4;
	while (c--)
	{
		memset32(gp2x_screen, 0, 320*240*2/4);
		gp2x_video_flip();
	}
	XBuf = gp2x_screen;
}


void KillVideo(void)
{
}


int InitVideo(void)
{
	fps_str[0]=0;

	CleanSurface();

	puts("Initialized GP2X VIDEO via minimal");

	srendline=0;
	erendline=239;
	XBuf = gp2x_screen;
	return 1;
}


// 16: rrrr rggg gg0b bbbb
void FCEUD_SetPalette(uint8 index, uint8 r, uint8 g, uint8 b)
{
	/* note: menu depends on bit5 being 0 */
	gp2x_palette[index] = (r << 16) | (g << 8) | b;
	gp2x_palette16[index] = ((r & 0xf8) << 8) | ((g & 0xf8) << 3) | (b >> 3);
	gp2x_video_setpalette(gp2x_palette, index + 1);

	paletterefresh = 1;
}


void FCEUD_GetPalette(uint8 index, uint8 * r, uint8 * g, uint8 * b)
{
	int pix = gp2x_palette[index];
	*r = (uint8) (pix >> 16);
	*g = (uint8) (pix >> 8);
	*b = (uint8)  pix;
}


static INLINE void printFps(uint8 *screen)
{
	struct timeval tv_now;
	static int prevsec, needfpsflip = 0;

	gettimeofday(&tv_now, 0);
	if (prevsec != tv_now.tv_sec)
	{
		sprintf(fps_str, "%2i/%2i", framesRendered, framesEmulated);
		framesEmulated = framesRendered = 0;
		needfpsflip = 4;
		prevsec = tv_now.tv_sec;
	}

	if (!Settings.showfps || !screen) return;

	if (Settings.scaling == 0)
	{
		if (needfpsflip)
		{
			int sep;
			for (sep=1; sep < 5; sep++)
				if (fps_str[sep] == '/' || fps_str[sep] == 0) break;
			fps_str[sep] = 0;
			gp2x_text(screen, 0,  0, fps_str,       FPS_COLOR, 0);
			gp2x_text(screen, 0, 10, fps_str+sep+1, FPS_COLOR, 0);
			needfpsflip--;
		}
	}
	else
	{
		gp2x_text(screen+32, 0, srendline, fps_str, FPS_COLOR, 0);
	}
}


void BlitPrepare(int skip)
{
	framesEmulated++;

	if (skip) {
		printFps(0);
		return;
	}

	framesRendered++;

	if (eoptions & EO_CLIPSIDES)
	{
		int i, *p = (int *) ((char *)gp2x_screen + 32);
		for (i = 240; i; i--, p += 320/4)
		{
			p[0] = p[1] = p[62] = p[63] = 0;
		}
	}

	if (Settings.accurate_mode && Settings.scaling < 2)
	{
		int i, *p = (int *)gp2x_screen + 32/4;
		if (srendline > 0)
			for (i = srendline; i > 0; i--, p += 320/4)
				memset32(p, 0, 256/4);
		if (erendline < 239)
		{
			int *p = (int *)gp2x_screen + erendline*320/4 + 32/4;
			for (i = 239-srendline; i > 0; i--, p += 320/4)
				memset32(p, 0, 256/4);
		}
	}

	printFps(gp2x_screen);

	if (Settings.scaling == 3)
	{
		soft_scale((char *)gp2x_screen + 32, gp2x_palette16, srendline, erendline-srendline);
		if (srendline)
			memset32((int *)((char *)gp2x_screen + 32), 0, srendline*320*2/4);
	}

	/* at this point we should be done with the frame */
	gp2x_video_flush_cache();
}


void BlitScreen(int skip)
{
	if (!skip)
	{
		gp2x_video_flip();
		XBuf = gp2x_screen;
	}
}


