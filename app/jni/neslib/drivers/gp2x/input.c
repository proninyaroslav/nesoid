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

#include "../../state.h"
#include "../../general.h"
#include "../../input.h"
#include "../../svga.h"
#include "../../video.h"
#include "usbjoy.h"

/* UsrInputType[] is user-specified.  InputType[] is current
       (game loading can override user settings)
*/
static int UsrInputType[2]={SI_GAMEPAD,SI_GAMEPAD};
static int UsrInputTypeFC={SI_NONE};

static int InputType[2];
static int InputTypeFC;

static uint32 JSreturn;

static int powerpadsc[2][12];
static int powerpadside=0;

static uint32 MouseData[3];
static uint8 fkbkeys[0x48];

static uint32 combo_acts = 0, combo_keys = 0;
static uint32 prev_emu_acts = 0;


static void setsoundvol(int soundvolume)
{
	int soundvolIndex;
	static char soundvolmeter[24];
	static int prev_snd_on = 0;

	if ((!!soundvolume) ^ prev_snd_on)
	{
		FCEUI_Sound(Settings.sound_rate);
		prev_snd_on = !!soundvolume;
	}

	// draw on screen :D
	gp2x_sound_volume(soundvolume, soundvolume);
	int meterval=soundvolume/5;
	for (soundvolIndex = 0; soundvolIndex < 20; soundvolIndex++)
	{
		if (soundvolIndex < meterval)
		{
			soundvolmeter[soundvolIndex]='*';
		}
		else
		{
			soundvolmeter[soundvolIndex]='_';
		}
	}
	soundvolmeter[20]=0;
	FCEU_DispMessage("|%s|", soundvolmeter);
}


static void do_emu_acts(uint32 acts)
{
	uint32 actsc = acts;
	acts &= acts ^ prev_emu_acts;
	prev_emu_acts = actsc;

	if (acts & (3 << 30))
	{
		unsigned long keys;
		int do_it = 1;
		if (acts & (1 << 30))
		{
			if (Settings.sstate_confirm & 2)
			{
				FCEU_DispMessage("LOAD STATE? (Y=yes, X=no)");
				FCEU_PutImage();
				FCEUD_Update(XBuf+8,NULL,0);
				while( !((keys = gp2x_joystick_read(1)) & (GP2X_X|GP2X_Y)) ) usleep(50*1024);
				if (keys & GP2X_X) do_it = 0;
				FCEU_CancelDispMessage();
			}
			if (do_it) FCEUI_LoadState();
		}
		else
		{
			if (Settings.sstate_confirm & 1)
			{
				char *fname = FCEU_MakeFName(FCEUMKF_STATE,CurrentState,0);
				FILE *st=fopen(fname,"rb");
				free(fname);
				if (st)
				{
					fclose(st);
					FCEU_DispMessage("OVERWRITE SAVE? (Y=yes, X=no)");
					FCEU_PutImage();
					FCEUD_Update(XBuf+8,NULL,0);
					while( !((keys = gp2x_joystick_read(1)) & (GP2X_X|GP2X_Y)) ) usleep(50*1024);
					if (keys & GP2X_X) do_it = 0;
					FCEU_CancelDispMessage();
				}
			}
			if (do_it) FCEUI_SaveState();
		}
		RefreshThrottleFPS();
	}
	else if (acts & (3 << 28)) // state slot next/prev
	{
		FILE *st;
		char *fname;

		CurrentState += (acts & (1 << 29)) ? 1 :  -1;
		if (CurrentState > 9) CurrentState = 0;
		if (CurrentState < 0) CurrentState = 9;

		fname = FCEU_MakeFName(FCEUMKF_STATE,CurrentState,0);
		st=fopen(fname,"rb");
		free(fname);
		FCEU_DispMessage("[%s] State Slot %i", st ? "USED" : "FREE", CurrentState);
		if (st) fclose(st);
	}
	else if (acts & (1 << 27)) // FDS insert/eject
	{
        	if(FCEUGameInfo.type == GIT_FDS)
			FCEU_DoSimpleCommand(FCEUNPCMD_FDSINSERT);
	}
	else if (acts & (1 << 26)) // FDS select
	{
        	if(FCEUGameInfo.type == GIT_FDS)
			FCEU_DoSimpleCommand(FCEUNPCMD_FDSSELECT);
	}
	else if (acts & (1 << 25)) // VS Unisystem insert coin
	{
        	if(FCEUGameInfo.type == GIT_VSUNI)
			FCEU_DoSimpleCommand(FCEUNPCMD_VSUNICOIN);
	}
}


#define down(b) (keys & GP2X_##b)
static void do_fake_mouse(unsigned long keys)
{
	static int x=256/2, y=240/2;
	int speed = 3;

	if (down(A)) speed = 1;
	if (down(Y)) speed = 5;

	if (down(LEFT))
	{
		x -= speed;
		if (x < 0) x = 0;
	}
	else if (down(RIGHT))
	{
		x += speed;
		if (x > 255) x = 255;
	}

	if (down(UP))
	{
		y -= speed;
		if (y < 0) y = 0;
	}
	else if (down(DOWN))
	{
		y += speed;
		if (y > 239) y = 239;
	}

	MouseData[0] = x;
	MouseData[1] = y;
	MouseData[2] = 0;
	if (down(B)) MouseData[2] |= 1;
	if (down(X)) MouseData[2] |= 2;
}


static void FCEUD_UpdateInput(void)
{
	static int volpushed_frames = 0;
	static int turbo_rate_cnt_a[2] = {0,0}, turbo_rate_cnt_b[2] = {0,0};
	unsigned long keys = gp2x_joystick_read(0);
	uint32 all_acts[2] = {0,0};
	int i;

	if ((down(VOL_DOWN) && down(VOL_UP)) || (keys & (GP2X_L|GP2X_START)) == (GP2X_L|GP2X_START))
	{
		Exit = 1;
		return;
	}
	else if (down(VOL_UP))
	{
		/* wait for at least 10 updates, because user may be just trying to enter menu */
		if (volpushed_frames++ > 10 && (volpushed_frames&1)) {
			soundvol++;
			if (soundvol > 100) soundvol=100;
			//FCEUI_SetSoundVolume(soundvol);
			setsoundvol(soundvol);
		}
	}
	else if (down(VOL_DOWN))
	{
		if (volpushed_frames++ > 10 && (volpushed_frames&1)) {
			soundvol-=1;
			if (soundvol < 0) soundvol=0;
			//FCEUI_SetSoundVolume(soundvol);
			setsoundvol(soundvol);
		}
	}
	else
	{
		volpushed_frames = 0;
	}

	JSreturn = 0; // RLDU SEBA

	if (InputType[1] != SI_GAMEPAD)
	{
		/* try to feed fake mouse there */
		do_fake_mouse(keys);
	}

	for (i = 0; i < 32; i++)
	{
		if (keys & (1 << i))
		{
			uint32 acts, u = 32;
			acts = Settings.KeyBinds[i];
			if (!acts) continue;
			if ((1 << i) & combo_keys)
			{
				// combo key detected, try to find if other is pressed
				for (u = i+1; u < 32; u++)
				{
					if ((keys & (1 << u)) && (Settings.KeyBinds[u] & acts))
					{
						keys &= ~(1 << u);
						acts &= Settings.KeyBinds[u];
						break;
					}
				}
			}
			if (u != 32) acts &=  combo_acts; // other combo key pressed
			else         acts &= ~combo_acts;
			all_acts[(acts>>16)&1] |= acts;
		}
	}

	// add joy inputs
	if (num_of_joys > 0)
	{
		int joy;
		gp2x_usbjoy_update();
		for (joy = 0; joy < num_of_joys; joy++) {
			int keys = gp2x_usbjoy_check2(joy);
			for (i = 0; i < 32; i++) {
				if (keys & (1 << i)) {
					int acts = Settings.JoyBinds[joy][i];
					all_acts[(acts>>16)&1] |= acts;
				}
			}
		}
	}

	// player 1
	JSreturn |= all_acts[0] & 0xff;
	if (all_acts[0] & 0x100) {		// A turbo
		turbo_rate_cnt_a[0] += Settings.turbo_rate_add;
		JSreturn |= (turbo_rate_cnt_a[0] >> 24) & 1;
	}
	if (all_acts[0] & 0x200) {		// B turbo
		turbo_rate_cnt_b[0] += Settings.turbo_rate_add;
		JSreturn |= (turbo_rate_cnt_b[0] >> 23) & 2;
	}

	// player 2
	JSreturn |= (all_acts[1] & 0xff) << 16;
	if (all_acts[1] & 0x100) {		// A turbo
		turbo_rate_cnt_a[1] += Settings.turbo_rate_add;
		JSreturn |= (turbo_rate_cnt_a[1] >> 8) & 0x10000;
	}
	if (all_acts[1] & 0x200) {		// B turbo
		turbo_rate_cnt_b[1] += Settings.turbo_rate_add;
		JSreturn |= (turbo_rate_cnt_b[1] >> 7) & 0x20000;
	}

	do_emu_acts(all_acts[0]|all_acts[1]);
}


static void InitOtherInput(void)
{

   void *InputDPtr;

   int t;
   int x;
   int attrib;

   printf("InitOtherInput: InputType[0]: %i, InputType[1]: %i, InputTypeFC: %i\n",
   	InputType[0], InputType[1], InputTypeFC);

   for(t=0,x=0;x<2;x++)
   {
    attrib=0;
    InputDPtr=0;
    switch(InputType[x])
    {
      //case SI_POWERPAD:InputDPtr=&powerpadbuf[x];break;
     case SI_GAMEPAD:InputDPtr=((uint8 *)&JSreturn)+(x<<1);break;
     case SI_ARKANOID:InputDPtr=MouseData;t|=1;break;
     case SI_ZAPPER:InputDPtr=MouseData;
                                t|=1;
                                attrib=1;
                                break;
    }
    FCEUI_SetInput(x,InputType[x],InputDPtr,attrib);
   }

   attrib=0;
   InputDPtr=0;
   switch(InputTypeFC)
   {
    case SIFC_SHADOW:InputDPtr=MouseData;t|=1;attrib=1;break;
    case SIFC_ARKANOID:InputDPtr=MouseData;t|=1;break;
    case SIFC_FKB:InputDPtr=fkbkeys;break;
   }

   FCEUI_SetInputFC(InputTypeFC,InputDPtr,attrib);
   FCEUI_DisableFourScore(eoptions&EO_NOFOURSCORE);

   inited|=16;
}


static void PrepareOtherInput(void)
{
	uint32 act;

	combo_acts = combo_keys = prev_emu_acts = 0;

	for (act = 0; act < 32; act++)
	{
		int u, keyc = 0, keyc2 = 0;
		if (act == 16 || act == 17) continue; // player2 flag
		if (act > 17)
		{
			for (u = 0; u < 32; u++)
				if (Settings.KeyBinds[u] & (1 << act)) keyc++;
		}
		else
		{
			for (u = 0; u < 32; u++)
				if ((Settings.KeyBinds[u] & 0x30000) == 0 && // pl. 1
					(Settings.KeyBinds[u] & (1 << act))) keyc++;
			for (u = 0; u < 32; u++)
				if ((Settings.KeyBinds[u] & 0x30000) == 1 && // pl. 2
					(Settings.KeyBinds[u] & (1 << act))) keyc2++;
			if (keyc2 > keyc) keyc = keyc2;
		}
		if (keyc > 1)
		{
			// loop again and mark those keys and actions as combo
			for (u = 0; u < 32; u++)
			{
				if (Settings.KeyBinds[u] & (1 << act)) {
					combo_keys |= 1 << u;
					combo_acts |= 1 << act;
				}
			}
		}
	}

	// printf("generated combo_acts: %08x, combo_keys: %08x\n", combo_acts, combo_keys);
}

