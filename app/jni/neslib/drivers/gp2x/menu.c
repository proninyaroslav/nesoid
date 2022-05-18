// menu system for gpfce - FCE Ultra port
// (c) Copyright 2006,2007 notaz, All rights reserved.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <dirent.h>

#include "minimal.h"
#include "usbjoy.h"
#include "asmutils.h"
#include "menu.h"
#include "main.h"
#include "fonts.h"
#include "gp2x.h"

#include "../../input.h"
#include "../../state.h"
#include "../../palette.h"
#include "readpng.h"

#ifndef _DIRENT_HAVE_D_TYPE
#error "need d_type for file browser
#endif

extern int GP2X_PORT_REV;
extern char lastLoadedGameName[PATH_MAX];
extern int mmuhack_status;
extern int soundvol;
extern uint8 Exit; // exit emu loop flag
extern int InitSound(void);

#define CONFIGURABLE_KEYS (GP2X_UP|GP2X_LEFT|GP2X_DOWN|GP2X_RIGHT|GP2X_START|GP2X_SELECT|GP2X_L|GP2X_R|GP2X_A|GP2X_B|GP2X_X|GP2X_Y|GP2X_PUSH)

static char *gp2xKeyNames[] = {
	"UP",    "01???",  "LEFT", "03???", "DOWN", "05???", "RIGHT",    "07???",
	"START", "SELECT", "L",    "R",     "A",    "B",     "X",        "Y",
	"10???", "11???",  "12???","13???", "14???","15???", "VOL DOWN", "VOL UP",
	"18???", "19???",  "1a???","PUSH",  "1c???","1d???", "1e???",    "1f???"
};


static char path_buffer[PATH_MAX];
static unsigned short *menu_bg = 0;
static int txt_xmin, txt_xmax, txt_ymin, txt_ymax;

char menuErrorMsg[40] = {0, };

static void menu_flip(void)
{
	gp2x_video_flush_cache();
	gp2x_video_flip();
}

static void menu_darken_reset(void)
{
	txt_xmin = 320; txt_xmax = 0;
	txt_ymin = 240; txt_ymax = 0;
}

static void gp2x_fceu_copy_bg(void)
{
	if (menu_bg)
	     memcpy(gp2x_screen, menu_bg, 320*240*2);
	else memset(gp2x_screen, 0, 320*240*2);
	menu_darken_reset();
}

static void menu_darken_text_bg(void)
{
	int x, y, xmin, xmax, ymax;
	unsigned short *screen = gp2x_screen;

	xmin = txt_xmin - 3;
	if (xmin < 0) xmin = 0;
	xmax = txt_xmax + 2;
	if (xmax > 319) xmax = 319;

	y = txt_ymin - 3;
	if (y < 0) y = 0;
	ymax = txt_ymax + 2;
	if (ymax > 239) ymax = 239;

	for (x = xmin; x <= xmax; x++)
		screen[y*320+x] = 0xa514;
	for (y++; y < ymax; y++)
	{
		screen[y*320+xmin] = 0xffff;
		for (x = xmin+1; x < xmax; x++)
		{
			unsigned int p = screen[y*320+x];
			if (p != 0xffff)
				screen[y*320+x] = ((p&0xf79e)>>1) - ((p&0xc618)>>3);
		}
		screen[y*320+xmax] = 0xffff;
	}
	for (x = xmin; x <= xmax; x++)
		screen[y*320+x] = 0xffff;
}

static void gp2x_fceu_darken_all(void)
{
	unsigned int *screen = gp2x_screen;
	int count = 320*240/2;

	while (count--)
	{
		unsigned int p = screen[count];
		screen[count] = ((p&0xf79ef79e)>>1) - ((p&0xc618c618)>>3);
	}
}

// draws white text to current bbp15 screen
static void gp2x_text_out15_(int x, int y, const char *text)
{
	int i,l;
	unsigned short *screen = gp2x_screen;

	screen = screen + x + y*320;

	for (i = 0; i < strlen(text); i++)
	{
		for (l=0;l<8;l++)
		{
			if(fontdata8x8[((text[i])*8)+l]&0x80) screen[l*320+0]=0xffff;
			if(fontdata8x8[((text[i])*8)+l]&0x40) screen[l*320+1]=0xffff;
			if(fontdata8x8[((text[i])*8)+l]&0x20) screen[l*320+2]=0xffff;
			if(fontdata8x8[((text[i])*8)+l]&0x10) screen[l*320+3]=0xffff;
			if(fontdata8x8[((text[i])*8)+l]&0x08) screen[l*320+4]=0xffff;
			if(fontdata8x8[((text[i])*8)+l]&0x04) screen[l*320+5]=0xffff;
			if(fontdata8x8[((text[i])*8)+l]&0x02) screen[l*320+6]=0xffff;
			if(fontdata8x8[((text[i])*8)+l]&0x01) screen[l*320+7]=0xffff;
		}
		screen += 8;
	}
	if (x < txt_xmin) txt_xmin = x;
	if (x+i*8 > txt_xmax) txt_xmax = x+i*8;
	if (y < txt_ymin) txt_ymin = y;
	if (y+8   > txt_ymax) txt_ymax = y+8;
}

void gp2x_text_out15(int x, int y, const char *texto, ...)
{
	va_list args;
	char    buffer[512];

	va_start(args,texto);
	vsprintf(buffer,texto,args);
	va_end(args);

	gp2x_text_out15_(x,y,buffer);
}


void gp2x_text_out15_lim(int x, int y, const char *texto, int max)
{
	char    buffer[320/8+1];

	strncpy(buffer, texto, 320/8);
	if (max > 320/8) max = 320/8;
	if (max < 0) max = 0;
	buffer[max] = 0;

	gp2x_text_out15(x,y,buffer);
}

static void gp2x_smalltext16(int x, int y, const char *texto, unsigned short color)
{
	int i;
	unsigned char  *src;
	unsigned short *dst;

	for (i = 0;; i++, x += 6)
	{
		unsigned char c = (unsigned char) texto[i];
		int h = 8;

		if (!c) break;

		src = fontdata6x8[c];
		dst = (unsigned short *)gp2x_screen + x + y*320;

		while (h--)
		{
			int w = 0x20;
			while (w)
			{
				if( *src & w ) *dst = color;
				dst++;
				w>>=1;
			}
			src++;

			dst += 320-6;
		}
	}
}

static void gp2x_smalltext16_lim(int x, int y, const char *texto, unsigned short color, int max)
{
	char    buffer[320/6+1];

	strncpy(buffer, texto, 320/6);
	if (max > 320/6) max = 320/6;
	if (max < 0) max = 0;
	buffer[max] = 0;

	gp2x_smalltext16(x, y, buffer, color);
}


static unsigned long inp_prev = 0;
static int inp_prevjoy = 0;

static unsigned long wait_for_input(unsigned long interesting)
{
	unsigned long ret;
	static int repeats = 0, wait = 50*1000;
	int release = 0, i;

	if (repeats == 2 || repeats == 4) wait /= 2;
	if (repeats == 6) wait = 15 * 1000;

	for (i = 0; i < 6 && inp_prev == gp2x_joystick_read(1); i++) {
		if (i == 0) repeats++;
		if (wait >= 30*1000) usleep(wait); // usleep sleeps for ~30ms minimum
		else spend_cycles(wait * Settings.cpuclock);
	}

	while ( !((ret = gp2x_joystick_read(1)) & interesting) ) {
		usleep(50000);
		release = 1;
	}

	if (release || ret != inp_prev) {
		repeats = 0;
		wait = 50*1000;
	}
	inp_prev = ret;
	inp_prevjoy = 0;

	// we don't need diagonals in menus
	if ((ret&GP2X_UP)   && (ret&GP2X_LEFT))  ret &= ~GP2X_LEFT;
	if ((ret&GP2X_UP)   && (ret&GP2X_RIGHT)) ret &= ~GP2X_RIGHT;
	if ((ret&GP2X_DOWN) && (ret&GP2X_LEFT))  ret &= ~GP2X_LEFT;
	if ((ret&GP2X_DOWN) && (ret&GP2X_RIGHT)) ret &= ~GP2X_RIGHT;

	return ret;
}

static unsigned long input2_read(unsigned long interesting, int *joy)
{
	unsigned long ret;
	int i;

	do
	{
		*joy = 0;
		if ((ret = gp2x_joystick_read(0) & interesting)) break;
		gp2x_usbjoy_update();
		for (i = 0; i < num_of_joys; i++) {
			ret = gp2x_usbjoy_check2(i);
			if (ret) { *joy = i + 1; break; }
		}
		if (ret) break;
	}
	while(0);

	return ret;
}

// similar to wait_for_input(), but returns joy num
static unsigned long wait_for_input_usbjoy(unsigned long interesting, int *joy)
{
	unsigned long ret;
	const int wait = 300*1000;
	int i;

	if (inp_prevjoy == 0) inp_prev &= interesting;
	for (i = 0; i < 6; i++) {
		ret = input2_read(interesting, joy);
		if (*joy != inp_prevjoy || ret != inp_prev) break;
		usleep(wait/6);
	}

	while ( !(ret = input2_read(interesting, joy)) ) {
		usleep(50000);
	}

	inp_prev = ret;
	inp_prevjoy = *joy;

	return ret;
}



// -------------- ROM selector --------------

// rrrr rggg gggb bbbb
static unsigned short file2color(const char *fname)
{
	const char *ext = fname + strlen(fname) - 3;
	static const char *rom_exts[]   = { "zip", "nes", "fds", "unf", "nez", "nif" }; // nif is for unif
	static const char *other_exts[] = { "nsf", "ips", "fcm" };
	int i;

	if (ext < fname) ext = fname;
	for (i = 0; i < sizeof(rom_exts)/sizeof(rom_exts[0]); i++)
		if (strcasecmp(ext, rom_exts[i]) == 0) return 0xbdff;
	for (i = 0; i < sizeof(other_exts)/sizeof(other_exts[0]); i++)
		if (strcasecmp(ext, other_exts[i]) == 0) return 0xaff5;
	return 0xffff;
}

static void draw_dirlist(char *curdir, struct dirent **namelist, int n, int sel)
{
	int start, i, pos;

	start = 12 - sel;
	n--; // exclude current dir (".")

	gp2x_fceu_copy_bg();
	gp2x_fceu_darken_all();

	if(start - 2 >= 0)
		gp2x_smalltext16_lim(14, (start - 2)*10, curdir, 0xffff, 53-2);
	for (i = 0; i < n; i++) {
		pos = start + i;
		if (pos < 0)  continue;
		if (pos > 23) break;
		if (namelist[i+1]->d_type == DT_DIR) {
			gp2x_smalltext16_lim(14,   pos*10, "/", 0xfff6, 1);
			gp2x_smalltext16_lim(14+6, pos*10, namelist[i+1]->d_name, 0xfff6, 53-3);
		} else {
			unsigned short color = file2color(namelist[i+1]->d_name);
			gp2x_smalltext16_lim(14,   pos*10, namelist[i+1]->d_name, color, 53-2);
		}
	}
	gp2x_text_out15(5, 120, ">");
	menu_flip();
}

static int scandir_cmp(const void *p1, const void *p2)
{
	struct dirent **d1 = (struct dirent **)p1, **d2 = (struct dirent **)p2;
	if ((*d1)->d_type == (*d2)->d_type) return alphasort(d1, d2);
	if ((*d1)->d_type == DT_DIR) return -1; // put before
	if ((*d2)->d_type == DT_DIR) return  1;
	return alphasort(d1, d2);
}

static char *filter_exts[] = {
	".gpe", ".png", "ck.o", ".txt", ".srm"
};

static int scandir_filter(const struct dirent *ent)
{
	const char *p;
	int i;

	if (ent == NULL || ent->d_name == NULL) return 0;
	if (strlen(ent->d_name) < 5) return 1;

	p = ent->d_name + strlen(ent->d_name) - 4;

	for (i = 0; i < sizeof(filter_exts)/sizeof(filter_exts[0]); i++)
	{
		if (strcmp(p, filter_exts[i]) == 0) return 0;
	}

	return 1;
}

static char *filesel_loop(char *curr_path, char *final_dest)
{
	struct dirent **namelist;
	DIR *dir;
	int n, newlen, sel = 0;
	unsigned long inp = 0;
	char *ret = NULL, *fname = NULL;

	// is this a dir or a full path?
	if ((dir = opendir(curr_path))) {
		closedir(dir);
	} else {
		char *p;
		for (p = curr_path + strlen(curr_path) - 1; p > curr_path && *p != '/'; p--);
		*p = 0;
		fname = p+1;
	}

	n = scandir(curr_path, &namelist, scandir_filter, scandir_cmp);
	if (n < 0) {
		// try root
		n = scandir("/", &namelist, scandir_filter, scandir_cmp);
		if (n < 0) {
			// oops, we failed
			printf("dir: %s\n", curr_path);
			perror("scandir");
			return NULL;
		}
	}

	// try to find sel
	if (fname != NULL) {
		int i;
		for (i = 1; i < n; i++) {
			if (strcmp(namelist[i]->d_name, fname) == 0) {
				sel = i - 1;
				break;
			}
		}
	}

	for (;;)
	{
		draw_dirlist(curr_path, namelist, n, sel);
		inp = wait_for_input(GP2X_UP|GP2X_DOWN|GP2X_LEFT|GP2X_RIGHT|GP2X_L|GP2X_R|GP2X_B|GP2X_X);
		if(inp & GP2X_UP  )  { sel--;   if (sel < 0)   sel = n-2; }
		if(inp & GP2X_DOWN)  { sel++;   if (sel > n-2) sel = 0; }
		if(inp & GP2X_LEFT)  { sel-=10; if (sel < 0)   sel = 0; }
		if(inp & GP2X_L)     { sel-=24; if (sel < 0)   sel = 0; }
		if(inp & GP2X_RIGHT) { sel+=10; if (sel > n-2) sel = n-2; }
		if(inp & GP2X_R)     { sel+=24; if (sel > n-2) sel = n-2; }
		if(inp & GP2X_B)     { // enter dir/select
			again:
			newlen = strlen(curr_path) + strlen(namelist[sel+1]->d_name) + 2;
			if (namelist[sel+1]->d_type == DT_REG) { // file selected
				if (final_dest == NULL) final_dest = malloc(newlen);
				if (final_dest == NULL) break;
				strcpy(final_dest, curr_path);
				strcat(final_dest, "/");
				strcat(final_dest, namelist[sel+1]->d_name);
				ret = final_dest;
				break;
			} else if (namelist[sel+1]->d_type == DT_DIR) {
				char *p, *newdir = malloc(newlen);
				if (newdir == NULL) break;
				if (strcmp(namelist[sel+1]->d_name, "..") == 0) {
					char *start = curr_path;
					p = start + strlen(start) - 1;
					while (*p == '/' && p > start) p--;
					while (*p != '/' && p > start) p--;
					if (p <= start) strcpy(newdir, "/");
					else { strncpy(newdir, start, p-start); newdir[p-start] = 0; }
				} else {
					strcpy(newdir, curr_path);
					p = newdir + strlen(newdir) - 1;
					while (p >= newdir && *p == '/') *p-- = 0;
					strcat(newdir, "/");
					strcat(newdir, namelist[sel+1]->d_name);
				}
				ret = filesel_loop(newdir, final_dest);
				free(newdir);
				break;
			} else {
				// unknown file type, happens on NTFS mounts. Try to guess.
				char *tstfn; FILE *tstf; int tmp;
				tstfn = malloc(newlen);
				if (tstfn == NULL) break;
				strcpy(tstfn, curr_path);
				strcat(tstfn, "/");
				strcat(tstfn, namelist[sel+1]->d_name);
				tstf = fopen(tstfn, "rb");
				free(tstfn);
				if (tstf != NULL)
				{
					if (fread(&tmp, 1, 1, tstf) > 0 || ferror(tstf) == 0)
						namelist[sel+1]->d_type = DT_REG;
					else	namelist[sel+1]->d_type = DT_DIR;
					fclose(tstf);
					goto again;
				}
			}
		}
		if(inp & GP2X_X) break; // cancel
	}

	if (n > 0) {
		while(n--) free(namelist[n]);
		free(namelist);
	}

	return ret;
}

// ------------ patch/gg menu ------------

extern void *cheats;
static int cheat_count = 0, cheat_start, cheat_pos;

static int countcallb(char *name, uint32 a, uint8 v, int compare, int s, int type, void *data)
{
	cheat_count++;
	return 1;
}

static int clistcallb(char *name, uint32 a, uint8 v, int compare, int s, int type, void *data)
{
	int pos;

	pos = cheat_start + cheat_pos;
	cheat_pos++;
	if (pos < 0)  return 1;
	if (pos > 23) return 0;

	gp2x_smalltext16_lim(14,     pos*10, s ? "ON " : "OFF", 0xffff, 3);
	gp2x_smalltext16_lim(14+6*4, pos*10, type ? "S" : "R", 0xffff, 1);
	gp2x_smalltext16_lim(14+6*6, pos*10, name, 0xffff, 53-8);

	return 1;
}

static void draw_patchlist(int sel)
{
	int pos;

	gp2x_fceu_copy_bg();
	gp2x_fceu_darken_all();

	cheat_start = 12 - sel;
	cheat_pos = 0;
	FCEUI_ListCheats(clistcallb,0);

	pos = cheat_start + cheat_pos;
	if (pos < 24) gp2x_smalltext16_lim(14, pos*10, "done", 0xffff, 4);

	gp2x_text_out15(5, 120, ">");
	menu_flip();
}

void patches_menu_loop(void)
{
	int menu_sel = 0;
	unsigned long inp = 0;

	cheat_count = 0;
	FCEUI_ListCheats(countcallb,0);

	for(;;)
	{
		draw_patchlist(menu_sel);
		inp = wait_for_input(GP2X_UP|GP2X_DOWN|GP2X_LEFT|GP2X_RIGHT|GP2X_L|GP2X_R|GP2X_B|GP2X_X);
		if(inp & GP2X_UP  ) { menu_sel--; if (menu_sel < 0) menu_sel = cheat_count; }
		if(inp & GP2X_DOWN) { menu_sel++; if (menu_sel > cheat_count) menu_sel = 0; }
		if(inp &(GP2X_LEFT|GP2X_L))  { menu_sel-=10; if (menu_sel < 0) menu_sel = 0; }
		if(inp &(GP2X_RIGHT|GP2X_R)) { menu_sel+=10; if (menu_sel > cheat_count) menu_sel = cheat_count; }
		if(inp & GP2X_B) { // action
			if (menu_sel < cheat_count)
			     FCEUI_ToggleCheat(menu_sel);
			else return;
		}
		if(inp & GP2X_X) return;
	}

}

// ------------ savestate loader ------------

#if 0
static void menu_prepare_bg(void);

static int state_slot_flags = 0;

static void state_check_slots(void)
{
	int slot;

	state_slot_flags = 0;

	for (slot = 0; slot < 10; slot++)
	{
		if (emu_check_save_file(slot))
		{
			state_slot_flags |= 1 << slot;
		}
	}
}

static void draw_savestate_bg(int slot)
{
	struct PicoVideo tmp_pv;
	unsigned short tmp_cram[0x40];
	unsigned short tmp_vsram[0x40];
	void *tmp_vram, *file;
	char *fname;

	fname = emu_GetSaveFName(1, 0, slot);
	if (!fname) return;

	tmp_vram = malloc(sizeof(Pico.vram));
	if (tmp_vram == NULL) return;

	memcpy(tmp_vram, Pico.vram, sizeof(Pico.vram));
	memcpy(tmp_cram, Pico.cram, sizeof(Pico.cram));
	memcpy(tmp_vsram, Pico.vsram, sizeof(Pico.vsram));
	memcpy(&tmp_pv, &Pico.video, sizeof(Pico.video));

	if (strcmp(fname + strlen(fname) - 3, ".gz") == 0) {
		file = gzopen(fname, "rb");
		emu_set_save_cbs(1);
	} else {
		file = fopen(fname, "rb");
		emu_set_save_cbs(0);
	}

	if (file) {
		if (PicoMCD & 1) {
			PicoCdLoadStateGfx(file);
		} else {
			areaSeek(file, 0x10020, SEEK_SET);  // skip header and RAM in state file
			areaRead(Pico.vram, 1, sizeof(Pico.vram), file);
			areaSeek(file, 0x2000, SEEK_CUR);
			areaRead(Pico.cram, 1, sizeof(Pico.cram), file);
			areaRead(Pico.vsram, 1, sizeof(Pico.vsram), file);
			areaSeek(file, 0x221a0, SEEK_SET);
			areaRead(&Pico.video, 1, sizeof(Pico.video), file);
		}
		areaClose(file);
	}

	emu_forced_frame();
	gp2x_memcpy_buffers((1<<2), gp2x_screen, 0, 320*240*2);
	menu_prepare_bg();

	memcpy(Pico.vram, tmp_vram, sizeof(Pico.vram));
	memcpy(Pico.cram, tmp_cram, sizeof(Pico.cram));
	memcpy(Pico.vsram, tmp_vsram, sizeof(Pico.vsram));
	memcpy(&Pico.video, &tmp_pv,  sizeof(Pico.video));
	free(tmp_vram);
}

static void draw_savestate_menu(int menu_sel, int is_loading)
{
	int tl_x = 25, tl_y = 60, y, i;

	if (state_slot_flags & (1 << menu_sel))
		draw_savestate_bg(menu_sel);
	gp2x_fceu_copy_bg();

	gp2x_text_out15(tl_x, 30, is_loading ? "Load state" : "Save state");

	/* draw all 10 slots */
	y = tl_y;
	for (i = 0; i < 10; i++, y+=10)
	{
		gp2x_text_out15(tl_x, y, "SLOT %i (%s)", i, (state_slot_flags & (1 << i)) ? "USED" : "free");
	}
	gp2x_text_out15(tl_x, y, "back");

	// draw cursor
	gp2x_text_out15(tl_x - 16, tl_y + menu_sel*10, ">");

	menu_flip();
}

static int savestate_menu_loop(int is_loading)
{
	int menu_sel = 10, menu_sel_max = 10;
	unsigned long inp = 0;

	state_check_slots();

	for(;;)
	{
		draw_savestate_menu(menu_sel, is_loading);
		inp = wait_for_input(GP2X_UP|GP2X_DOWN|GP2X_B|GP2X_X);
		if(inp & GP2X_UP  ) {
			do {
				menu_sel--; if (menu_sel < 0) menu_sel = menu_sel_max;
			} while (!(state_slot_flags & (1 << menu_sel)) && menu_sel != menu_sel_max && is_loading);
		}
		if(inp & GP2X_DOWN) {
			do {
				menu_sel++; if (menu_sel > menu_sel_max) menu_sel = 0;
			} while (!(state_slot_flags & (1 << menu_sel)) && menu_sel != menu_sel_max && is_loading);
		}
		if(inp & GP2X_B) { // save/load
			if (menu_sel < 10) {
				state_slot = menu_sel;
				if (emu_SaveLoadGame(is_loading, 0)) {
					strcpy(menuErrorMsg, is_loading ? "Load failed" : "Save failed");
					return 1;
				}
				return 0;
			} else	return 1;
		}
		if(inp & GP2X_X) return 1;
	}
}
#endif

// -------------- key config --------------

static char *usb_joy_key_name(int joy, int num)
{
	static char name[16];
	switch (num)
	{
		case 0: sprintf(name, "Joy%i UP", joy); break;
		case 1: sprintf(name, "Joy%i DOWN", joy); break;
		case 2: sprintf(name, "Joy%i LEFT", joy); break;
		case 3: sprintf(name, "Joy%i RIGHT", joy); break;
		default:sprintf(name, "Joy%i b%i", joy, num-3); break;
	}
	return name;
}

static char *action_binds(int player_idx, int action_mask)
{
	static char strkeys[32*5];
	int joy, i;

	strkeys[0] = 0;
	for (i = 0; i < 32; i++) // i is key index
	{
		if (Settings.KeyBinds[i] & action_mask)
		{
			if (player_idx >= 0 && ((Settings.KeyBinds[i] >> 16) & 3) != player_idx) continue;
			if (strkeys[0]) { strcat(strkeys, " + "); strcat(strkeys, gp2xKeyNames[i]); break; }
			else strcpy(strkeys, gp2xKeyNames[i]);
		}
	}
	for (joy = 0; joy < num_of_joys; joy++)
	{
		for (i = 0; i < 32; i++)
		{
			if (Settings.JoyBinds[joy][i] & action_mask)
			{
				if (player_idx >= 0 && ((Settings.JoyBinds[joy][i] >> 16) & 3) != player_idx) continue;
				if (strkeys[0]) {
					strcat(strkeys, ", "); strcat(strkeys, usb_joy_key_name(joy + 1, i));
					break;
				}
				else strcpy(strkeys, usb_joy_key_name(joy + 1, i));
			}
		}
	}

	return strkeys;
}

static void unbind_action(int action)
{
	int i, u;

	for (i = 0; i < 32; i++)
		Settings.KeyBinds[i] &= ~action;
	for (u = 0; u < 4; u++)
		for (i = 0; i < 32; i++)
			Settings.JoyBinds[u][i] &= ~action;
}

static int count_bound_keys(int action, int joy)
{
	int i, keys = 0;

	if (joy)
	{
		for (i = 0; i < 32; i++)
			if (Settings.JoyBinds[joy-1][i] & action) keys++;
	}
	else
	{
		for (i = 0; i < 32; i++)
			if (Settings.KeyBinds[i] & action) keys++;
	}
	return keys;
}

typedef struct { char *name; int mask; } bind_action_t;

static void draw_key_config(const bind_action_t *opts, int opt_cnt, int player_idx, int sel)
{
	int x, y, tl_y = 40, i;

	gp2x_fceu_copy_bg();
	if (player_idx >= 0)
	     gp2x_text_out15(80, 20, "Player %i controls", player_idx + 1);
	else gp2x_text_out15(80, 20, "Emulator controls");

	x = 40; y = tl_y;
	for (i = 0; i < opt_cnt; i++, y+=10)
		gp2x_text_out15(x, y, "%s : %s", opts[i].name, action_binds(player_idx, opts[i].mask));

	gp2x_text_out15(x, y, "Done");

	// draw cursor
	gp2x_text_out15(x - 16, tl_y + sel*10, ">");

	menu_darken_text_bg();
	menu_darken_reset();

	if (sel < opt_cnt) {
		gp2x_text_out15(30, 190, "Press a button to bind/unbind");
		gp2x_text_out15(30, 200, "Use VOL+ to clear");
		gp2x_text_out15(30, 210, "To bind UP/DOWN, hold VOL-");
		gp2x_text_out15(30, 220, "Select \"Done\" to exit");
	} else {
		gp2x_text_out15(30, 200, "Use Options -> Save cfg");
		gp2x_text_out15(30, 210, "to save controls");
		gp2x_text_out15(30, 220, "Press B or X to exit");
	}
	menu_darken_text_bg();
	menu_flip();
}

static void key_config_loop(const bind_action_t *opts, int opt_cnt, int player_idx)
{
	int joy = 0, sel = 0, menu_sel_max = opt_cnt, i;
	unsigned long inp = 0;

	for (;;)
	{
		draw_key_config(opts, opt_cnt, player_idx, sel);
		inp = wait_for_input_usbjoy(CONFIGURABLE_KEYS|GP2X_VOL_DOWN|GP2X_VOL_UP, &joy);
		// printf("got %08lX from joy %i\n", inp, joy);
		if (joy == 0) {
			if (!(inp & GP2X_VOL_DOWN)) {
				if(inp & GP2X_UP  ) { sel--; if (sel < 0) sel = menu_sel_max; continue; }
				if(inp & GP2X_DOWN) { sel++; if (sel > menu_sel_max) sel = 0; continue; }
			}
			if (sel >= opt_cnt) {
				if (inp & (GP2X_B|GP2X_X)) break;
				else continue;
			}
			// if we are here, we want to bind/unbind something
			if (inp & GP2X_VOL_UP)
				unbind_action(opts[sel].mask);
			inp &= CONFIGURABLE_KEYS;
			for (i = 0; i < 32; i++)
				if (inp & (1 << i)) {
					if (count_bound_keys(opts[sel].mask, 0) >= 2)
					     Settings.KeyBinds[i] &= ~opts[sel].mask; // allow to unbind only
					else Settings.KeyBinds[i] ^=  opts[sel].mask;
					if (player_idx >= 0) {
						Settings.KeyBinds[i] &= ~(3 << 16);
						Settings.KeyBinds[i] |= player_idx << 16;
					}
				}
		}
		else if (sel < opt_cnt)
		{
			for (i = 0; i < 32; i++)
				if (inp & (1 << i)) {
					if (count_bound_keys(opts[sel].mask, joy) >= 1) // disallow combos for usbjoy
					     Settings.JoyBinds[joy-1][i] &= ~opts[sel].mask;
					else Settings.JoyBinds[joy-1][i] ^=  opts[sel].mask;
					if (player_idx >= 0) {
						Settings.JoyBinds[joy-1][i] &= ~(3 << 16);
						Settings.JoyBinds[joy-1][i] |= player_idx << 16;
					}
				}
		}
	}
}

static void draw_kc_sel(int menu_sel)
{
	int tl_x = 25+40, tl_y = 60, y, i;
	char joyname[36];

	y = tl_y;
	gp2x_fceu_copy_bg();
	gp2x_text_out15(tl_x, y,       "Player 1");
	gp2x_text_out15(tl_x, (y+=10), "Player 2");
	gp2x_text_out15(tl_x, (y+=10), "Emulator controls");
	gp2x_text_out15(tl_x, (y+=10), "Done");

	// draw cursor
	gp2x_text_out15(tl_x - 16, tl_y + menu_sel*10, ">");

	tl_x = 25;
	gp2x_text_out15(tl_x, (y=110), "USB joys detected:");
	if (num_of_joys > 0) {
		for (i = 0; i < num_of_joys; i++) {
			strncpy(joyname, joy_name(joys[i]), 33); joyname[33] = 0;
			gp2x_text_out15(tl_x, (y+=10), "%i: %s", i+1, joyname);
		}
	} else {
		gp2x_text_out15(tl_x, (y+=10), "none");
	}

	menu_darken_text_bg();
	menu_flip();
}

// b_turbo,a_turbo  RLDU SEBA
static bind_action_t ctrl_actions[] =
{
	{ "UP     ", 0x010 },
	{ "DOWN   ", 0x020 },
	{ "LEFT   ", 0x040 },
	{ "RIGHT  ", 0x080 },
	{ "A      ", 0x001 },
	{ "B      ", 0x002 },
	{ "A TURBO", 0x100 },
	{ "B TURBO", 0x200 },
	{ "START  ", 0x008 },
	{ "SELECT ", 0x004 },
};

static bind_action_t emuctrl_actions[] =
{
	{ "Save State       ", 1<<31 },
	{ "Load State       ", 1<<30 },
	{ "Next State Slot  ", 1<<29 },
	{ "Prev State Slot  ", 1<<28 },
	{ "FDS Insert/Eject ", 1<<27 },
	{ "FDS Select Disk  ", 1<<26 },
	{ "VSUni Insert Coin", 1<<25 },
};

static void kc_sel_loop(void)
{
	int menu_sel = 3, menu_sel_max = 3;
	unsigned long inp = 0;

	for(;;)
	{
		draw_kc_sel(menu_sel);
		inp = wait_for_input(GP2X_UP|GP2X_DOWN|GP2X_B|GP2X_X);
		if(inp & GP2X_UP  ) { menu_sel--; if (menu_sel < 0) menu_sel = menu_sel_max; }
		if(inp & GP2X_DOWN) { menu_sel++; if (menu_sel > menu_sel_max) menu_sel = 0; }
		if(inp & GP2X_B) {
			switch (menu_sel) {
				case 0: key_config_loop(ctrl_actions, 10, 0); return;
				case 1: key_config_loop(ctrl_actions, 10, 1); return;
				case 2: key_config_loop(emuctrl_actions,
						sizeof(emuctrl_actions)/sizeof(emuctrl_actions[0]), -1); return;
				case 3: if (!fceugi) SaveConfig(NULL); return;
				default: return;
			}
		}
		if(inp & GP2X_X) return;
	}
}


// --------- FCEU options ----------

extern int ntsccol,ntschue,ntsctint;
extern int srendlinev[2];
extern int erendlinev[2];
extern int eoptions;
extern char *cpalette;
extern void LoadCPalette(void);


static void int_incdec(int *p, int inc, int min, int max)
{
	*p += inc;
	if      (*p < min) *p = min;
	else if (*p > max) *p = max;
}

static void draw_fcemenu_options(int menu_sel)
{
	int tl_x = 25, tl_y = 60, y;
	char cpal[32];

	if (cpalette != NULL)
	{
		char *p = cpalette + strlen(cpalette) - 1;
		while (*p != '/' && p > cpalette) p--;
		if (*p == '/') p++;
		strncpy(cpal, p, 16);
		cpal[16] = 0;
	}
	else strcpy(cpal, "           OFF");

	y = tl_y;
	gp2x_fceu_copy_bg();

	gp2x_text_out15(tl_x,  y,      "Custom palette: %s", cpal);				// 0
	gp2x_text_out15(tl_x, (y+=10), "NTSC Color Emulation       %s", ntsccol?"ON":"OFF");
	gp2x_text_out15(tl_x, (y+=10), "  Tint (default: 56)       %i", ntsctint);
	gp2x_text_out15(tl_x, (y+=10), "  Hue  (default: 72)       %i", ntschue);
	gp2x_text_out15(tl_x, (y+=10), "First visible line (NTSC)  %i", srendlinev[0]);
	gp2x_text_out15(tl_x, (y+=10), "Last visible line (NTSC)   %i", erendlinev[0]);		// 5
	gp2x_text_out15(tl_x, (y+=10), "First visible line (PAL)   %i", srendlinev[1]);
	gp2x_text_out15(tl_x, (y+=10), "Last visible line (PAL)    %i", erendlinev[1]);
	gp2x_text_out15(tl_x, (y+=10), "Clip 8 left/right columns  %s", (eoptions&EO_CLIPSIDES)?"ON":"OFF");
	gp2x_text_out15(tl_x, (y+=10), "Disable 8 sprite limit     %s", (eoptions&EO_NO8LIM)?"ON":"OFF");
	gp2x_text_out15(tl_x, (y+=10), "Enable authentic GameGenie %s", (eoptions&EO_GG)?"ON":"OFF");
	gp2x_text_out15(tl_x, (y+=10), "Done");							// 11

	// draw cursor
	gp2x_text_out15(tl_x - 16, tl_y + menu_sel*10, ">");

	if (menu_sel == 0) {
		menu_darken_text_bg();
		menu_darken_reset();

		gp2x_text_out15(30, 210, "Press B to browse,");
		gp2x_text_out15(30, 220, "START to use default");
	}

	menu_darken_text_bg();
	menu_flip();
}

static void fcemenu_loop_options(void)
{
	int menu_sel = 0, menu_sel_max = 11, i;
	unsigned long inp = 0;

	FCEUI_GetNTSCTH(&ntsctint, &ntschue);

	for(;;)
	{
		draw_fcemenu_options(menu_sel);
		inp = wait_for_input(GP2X_UP|GP2X_DOWN|GP2X_LEFT|GP2X_RIGHT|GP2X_B|GP2X_X|GP2X_A|GP2X_START);
		if(inp & GP2X_UP  ) { menu_sel--; if (menu_sel < 0) menu_sel = menu_sel_max; }
		if(inp & GP2X_DOWN) { menu_sel++; if (menu_sel > menu_sel_max) menu_sel = 0; }
		if((inp& GP2X_B)||(inp&GP2X_LEFT)||(inp&GP2X_RIGHT)) { // toggleable options
			switch (menu_sel) {
				case  1: ntsccol = !ntsccol; break;
				case  8: eoptions^=EO_CLIPSIDES; break;
				case  9: eoptions^=EO_NO8LIM; break;
				case 10: eoptions^=EO_GG; break;
				case 11: return;
			}
		}
		if(inp & (GP2X_X|GP2X_A)) {
			for(i=0;i<2;i++)
			{
				if(srendlinev[i]<0 || srendlinev[i]>239) srendlinev[i]=0;
				if(erendlinev[i]<srendlinev[i] || erendlinev[i]>239) erendlinev[i]=239;
			}
			FCEUI_SetNTSCTH(ntsccol, ntsctint, ntschue);
			FCEUI_SetRenderedLines(srendlinev[0],erendlinev[0],srendlinev[1],erendlinev[1]);
			FCEUI_DisableSpriteLimitation(eoptions&EO_NO8LIM);
			FCEUI_SetGameGenie(eoptions&EO_GG);
			if (cpalette) LoadCPalette();
			else FCEUI_SetPaletteArray(0); // set to default
			FCEU_ResetPalette();
			return;
		}
		if(inp & (GP2X_LEFT|GP2X_RIGHT)) { // multi choise
			switch (menu_sel) {
				case  2: int_incdec(&ntsctint,      (inp & GP2X_LEFT) ? -1 : 1, 0, 128); break;
				case  3: int_incdec(&ntschue,       (inp & GP2X_LEFT) ? -1 : 1, 0, 128); break;
				case  4: int_incdec(&srendlinev[0], (inp & GP2X_LEFT) ? -1 : 1, 0, 239); break;
				case  5: int_incdec(&erendlinev[0], (inp & GP2X_LEFT) ? -1 : 1, 0, 239); break;
				case  6: int_incdec(&srendlinev[1], (inp & GP2X_LEFT) ? -1 : 1, 0, 239); break;
				case  7: int_incdec(&erendlinev[1], (inp & GP2X_LEFT) ? -1 : 1, 0, 239); break;
			}
		}
		if(menu_sel == 0 && (inp & (GP2X_START|GP2X_B))) { // custom palette
			if ((inp & GP2X_START) && cpalette) {
				free(cpalette);
				cpalette=NULL;
			}
			else if (inp & GP2X_B) {
				char *selfname;
				if (cpalette) strncpy(path_buffer, cpalette, sizeof(path_buffer));
				else getcwd(path_buffer, PATH_MAX);
				path_buffer[sizeof(path_buffer)-1] = 0;

				selfname = filesel_loop(path_buffer, NULL);
				if (selfname) {
					if (cpalette) free(cpalette);
					cpalette = selfname;
				}
			}
		}
	}
}

// -------------- options --------------

static void draw_menu_options(int menu_sel)
{
	int tl_x = 25, tl_y = 20, y;
	char strframeskip[8], *strscaling, *strssconfirm;
	char *mms = mmuhack_status ? "active)  " : "inactive)";

	if (Settings.frameskip < 0)
	     strcpy(strframeskip, "Auto");
	else sprintf(strframeskip, "%i", Settings.frameskip);
	switch (Settings.scaling) {
		default: strscaling = "            OFF";   break;
		case 1:  strscaling = "hw horizontal";     break;
		case 2:  strscaling = "hw horiz. + vert."; break;
		case 3:  strscaling = "sw horizontal";     break;
	}
	switch (Settings.sstate_confirm) {
		default: strssconfirm = "OFF";    break;
		case 1:  strssconfirm = "writes"; break;
		case 2:  strssconfirm = "loads";  break;
		case 3:  strssconfirm = "both";   break;
	}

	y = tl_y;
	gp2x_fceu_copy_bg();

	gp2x_text_out15(tl_x,  y,      "Scaling:       %s", strscaling);				// 0
	gp2x_text_out15(tl_x, (y+=10), "Show FPS                   %s", Settings.showfps?"ON":"OFF");	// 1
	gp2x_text_out15(tl_x, (y+=10), "Frameskip                  %s", strframeskip);			// 2
	gp2x_text_out15(tl_x, (y+=10), "Accurate renderer (slow)   %s", Settings.accurate_mode?"ON":"OFF");
	gp2x_text_out15(tl_x, (y+=10), "Enable sound               %s", soundvol?"ON":"OFF");
	gp2x_text_out15(tl_x, (y+=10), "Sound Rate:           %5iHz", Settings.sound_rate);		// 5
	gp2x_text_out15(tl_x, (y+=10), "Force Region:              %s",
		Settings.region_force == 2 ? "PAL" : Settings.region_force == 1 ? "NTSC" : "OFF");	// 6
	gp2x_text_out15(tl_x, (y+=10), "Turbo rate                 %iHz", (Settings.turbo_rate_add*60/2) >> 24);
	gp2x_text_out15(tl_x, (y+=10), "Confirm savestate          %s", strssconfirm);			// 8
	gp2x_text_out15(tl_x, (y+=10), "Save slot                  %i", CurrentState);
	gp2x_text_out15(tl_x, (y+=10), "Faster RAM timings         %s", Settings.ramtimings?"ON":"OFF");
	gp2x_text_out15(tl_x, (y+=10), "squidgehack (now %s %s",   mms, Settings.mmuhack?"ON":"OFF");	// 11
	gp2x_text_out15(tl_x, (y+=10), "Gamma correction           %i.%02i", Settings.gamma / 100, Settings.gamma%100);
	gp2x_text_out15(tl_x, (y+=10), "Perfect VSYNC              %s", Settings.perfect_vsync?"ON":"OFF");
	gp2x_text_out15(tl_x, (y+=10), "GP2X CPU clock             %iMhz", Settings.cpuclock);		// 14
	gp2x_text_out15(tl_x, (y+=10), "[FCE Ultra options]");
	gp2x_text_out15(tl_x, (y+=10), "Save cfg as default");						// 16
	if (fceugi)
		gp2x_text_out15(tl_x, (y+=10), "Save cfg for current game only");

	// draw cursor
	gp2x_text_out15(tl_x - 16, tl_y + menu_sel*10, ">");

	if (menu_sel == 3) {
		gp2x_text_out15(tl_x, 210, "ROM reload required for this");
		gp2x_text_out15(tl_x, 220, "setting to take effect");
	} else if (menu_sel == 10 || menu_sel == 11) {
		gp2x_text_out15(tl_x, 210, "Emu restart required for this");
		gp2x_text_out15(tl_x, 220, "setting to take effect");
	}

	menu_darken_text_bg();
	menu_flip();
}

static int sndrate_prevnext(int rate, int dir)
{
	int i, rates[] = { 8000, 11025, 16000, 22050, 44100 };

	for (i = 0; i < 5; i++)
		if (rates[i] == rate) break;

	i += dir ? 1 : -1;
	if (i > 4) return dir ? 44100 : 22050;
	if (i < 0) return dir ? 11025 : 8000;
	return rates[i];
}

static void config_commit(void)
{
	if (Settings.region_force)
		FCEUI_SetVidSystem(Settings.region_force - 1);
}

static int menu_loop_options(void)
{
	static int menu_sel = 0;
	int ret, menu_sel_max = 16;
	unsigned long inp = 0;

	if (fceugi) menu_sel_max++;

	for(;;)
	{
		draw_menu_options(menu_sel);
		inp = wait_for_input(GP2X_UP|GP2X_DOWN|GP2X_LEFT|GP2X_RIGHT|GP2X_B|GP2X_X|GP2X_A);
		if(inp & GP2X_UP  ) { menu_sel--; if (menu_sel < 0) menu_sel = menu_sel_max; }
		if(inp & GP2X_DOWN) { menu_sel++; if (menu_sel > menu_sel_max) menu_sel = 0; }
		if((inp& GP2X_B)||(inp&GP2X_LEFT)||(inp&GP2X_RIGHT)) { // toggleable options
			switch (menu_sel) {
				case  1: Settings.showfps       = !Settings.showfps; break;
				case  3: Settings.accurate_mode = !Settings.accurate_mode; break;
				case  4: soundvol = soundvol ? 0 : 50; break;
				case 10: Settings.ramtimings    = !Settings.ramtimings; break;
				case 11: Settings.mmuhack       = !Settings.mmuhack; break;
				case 13: Settings.perfect_vsync = !Settings.perfect_vsync; break;
				case 15: fcemenu_loop_options(); break;
				case 16: // done (update and write)
					config_commit();
					ret = SaveConfig(NULL);
					strcpy(menuErrorMsg, ret == 0 ? "default config saved" : "config save failed");
					return 1;
				case 17: // done (update and write for current game)
					if (lastLoadedGameName[0])
					{
						config_commit();
						ret = SaveConfig(lastLoadedGameName);
						strcpy(menuErrorMsg, ret == 0 ? "game config saved" : "config save failed");
					}
					return 1;
			}
		}
		if(inp & (GP2X_X|GP2X_A)) {
			config_commit();
			return 0;  // done (update, no write)
		}
		if(inp & (GP2X_LEFT|GP2X_RIGHT)) { // multi choise
			switch (menu_sel) {
				case  0: int_incdec(&Settings.scaling,   (inp & GP2X_LEFT) ? -1 : 1,  0,  3); break;
				case  2: int_incdec(&Settings.frameskip, (inp & GP2X_LEFT) ? -1 : 1, -1, 32); break;
				case  5:
					Settings.sound_rate = sndrate_prevnext(Settings.sound_rate, inp & GP2X_RIGHT);
					InitSound();
					break;
				case  6: int_incdec(&Settings.region_force,   (inp & GP2X_LEFT) ? -1 : 1, 0, 2); break;
				case  7: {
					int hz = Settings.turbo_rate_add*60/2 >> 24;
					int_incdec(&hz, (inp & GP2X_LEFT) ? -1 : 1, 1, 30);
					Settings.turbo_rate_add = (hz*2 << 24) / 60 + 1;
					break;
				}
				case  8: int_incdec(&Settings.sstate_confirm, (inp & GP2X_LEFT) ? -1 : 1, 0, 3); break;
				case  9: int_incdec(&CurrentState,            (inp & GP2X_LEFT) ? -1 : 1, 0, 9); break;
				case 12: int_incdec(&Settings.gamma,          (inp & GP2X_LEFT) ? -1 : 1, 0, 300); break;
				case 14:
					while ((inp = gp2x_joystick_read(1)) & (GP2X_LEFT|GP2X_RIGHT)) {
						Settings.cpuclock += (inp & GP2X_LEFT) ? -1 : 1;
						if (Settings.cpuclock < 0) Settings.cpuclock = 0; // 0 ~ do not change
						draw_menu_options(menu_sel);
						usleep(50*1000);
					}
					break;
			}
		}
	}
}

// -------------- credits --------------

static void draw_menu_credits(void)
{
	char vstr[32];

	//int tl_x = 15, tl_y = 70;
	gp2x_fceu_copy_bg();

	sprintf(vstr, "GPFCE v" GP2X_PORT_VERSION " rev%i", GP2X_PORT_REV);
	gp2x_text_out15(20,  30, vstr);
	gp2x_text_out15(20,  40, "(c) notaz, 2007");

	gp2x_text_out15(20,  70, "Based on FCE Ultra versions");
	gp2x_text_out15(20,  80, "0.81 and 0.98.1x");

	gp2x_text_out15(20, 110, "         - Credits - ");
	gp2x_text_out15(20, 130, "Bero: FCE");
	gp2x_text_out15(20, 140, "Xodnizel: FCE Ultra");
	gp2x_text_out15(20, 150, "zzhu8192: original port");
	gp2x_text_out15(20, 160, "rlyeh: minimal lib");
	gp2x_text_out15(20, 170, "Hermes, theoddbot, god_at_hell:");
	gp2x_text_out15(20, 180, "  cpuctrl, gamma libs");
	gp2x_text_out15(20, 190, "Squidge: squidgehack");

	menu_darken_text_bg();
	menu_flip();
}


// -------------- root menu --------------

static void draw_menu_root(int menu_sel)
{
	int tl_x = 30, tl_y = 126, y;
	gp2x_fceu_copy_bg();

	y = tl_y;
	if (fceugi) {
		gp2x_text_out15(tl_x, y,       "Resume game");
		gp2x_text_out15(tl_x, (y+=10), "Save State");
		gp2x_text_out15(tl_x, (y+=10), "Load State");
		gp2x_text_out15(tl_x, (y+=10), "Reset game");
	} else {
		y += 30;
	}
	gp2x_text_out15(tl_x, (y+=10), "Load new ROM");
	gp2x_text_out15(tl_x, (y+=10), "Options");
	gp2x_text_out15(tl_x, (y+=10), "Controls");
	gp2x_text_out15(tl_x, (y+=10), "Credits");
	gp2x_text_out15(tl_x, (y+=10), "Exit");

	if (cheats)
		gp2x_text_out15(tl_x, (y+=10), "Cheats");

	// draw cursor
	gp2x_text_out15(tl_x - 16, tl_y + menu_sel*10, ">");

	menu_darken_text_bg();
	menu_darken_reset();

	// error / version
	if (menuErrorMsg[0]) gp2x_text_out15(1, 229, menuErrorMsg);
	else {
		char vstr[16];
		sprintf(vstr, "v" GP2X_PORT_VERSION " r%i", GP2X_PORT_REV);
		gp2x_text_out15(320-strlen(vstr)*8-1, 228, vstr);
	}
	menu_darken_text_bg();
	menu_flip();
}


static int menu_loop_root(void)
{
	int ret, menu_sel_max = 8, menu_sel_min = 4;
	static int menu_sel = 4;
	unsigned long inp = 0;

	if (fceugi) menu_sel_min = 0;
	if (cheats) menu_sel_max = 9;
	if (menu_sel < menu_sel_min || menu_sel > menu_sel_max)
		menu_sel = menu_sel_min;

	/* make sure action buttons are not pressed on entering menu */
	draw_menu_root(menu_sel);
	while (gp2x_joystick_read(1) & (GP2X_B|GP2X_X|GP2X_SELECT)) usleep(50*1000);

	for (;;)
	{
		draw_menu_root(menu_sel);
		inp = wait_for_input(GP2X_UP|GP2X_DOWN|GP2X_B|GP2X_X);
		if(inp & GP2X_UP  )  { menu_sel--; if (menu_sel < menu_sel_min) menu_sel = menu_sel_max; }
		if(inp & GP2X_DOWN)  { menu_sel++; if (menu_sel > menu_sel_max) menu_sel = menu_sel_min; }
		if(inp &(GP2X_SELECT|GP2X_X)){
			if (fceugi) {
				while (gp2x_joystick_read(1) & GP2X_X) usleep(50*1000); // wait until X is released
				Exit = 0;
				return 0;
			}
		}
		if(inp & GP2X_B   )  {
			switch (menu_sel) {
				case 0: // resume game
					if (fceugi) {
						while (gp2x_joystick_read(1) & GP2X_B) usleep(50*1000);
						Exit = 0;
						return 0;
					}
					break;
				case 1: // save state
					if (fceugi) {
						/*if(savestate_menu_loop(0))
							continue;*/
						FCEUI_SaveState();
						Exit = 0;
						while (gp2x_joystick_read(1) & GP2X_B) usleep(50*1000);
						return 0;
					}
					break;
				case 2: // load state
					if (fceugi) {
						/*if(savestate_menu_loop(1))
							continue;*/
						FCEUI_LoadState();
						Exit = 0;
						while (gp2x_joystick_read(1) & GP2X_B) usleep(50*1000);
						return 0;
					}
					break;
				case 3: // reset game
					if (fceugi) {
						FCEU_DoSimpleCommand(FCEUNPCMD_RESET);
						Exit = 0;
						return 0;
					}
					break;
				case 4: // select rom
				{
					FILE *tstf;
					char *selfname;

					if ( (tstf = fopen(lastLoadedGameName, "rb")) )
					{
						fclose(tstf);
						strncpy(path_buffer, lastLoadedGameName, sizeof(path_buffer));
						path_buffer[sizeof(path_buffer)-1] = 0;
					}
					else
					{
						getcwd(path_buffer, PATH_MAX);
					}

					selfname = filesel_loop(path_buffer, lastLoadedGameName);
					if (selfname) {
						printf("selected file: %s\n", selfname);
						while (gp2x_joystick_read(1) & GP2X_B) usleep(50*1000);
						return 2;
					}
					break;
				}
				case 5: // options
					ret = menu_loop_options();
					if (ret == 1) continue; // status update
					break;
				case 6: // controls
					kc_sel_loop();
					break;
				case 7: // credits
					draw_menu_credits();
					usleep(500*1000);
					inp = wait_for_input(GP2X_B|GP2X_X);
					break;
				case 8: // exit
					return 1;
				case 9: // patches/gg
					patches_menu_loop();
					break;
			}
		}
		menuErrorMsg[0] = 0; // clear error msg
	}
}


extern unsigned short gp2x_palette16[256];

static void menu_prepare_bg(void)
{
	menu_bg = malloc(320*240*2);
	if (menu_bg == NULL) return;

	if (fceugi)
	{
		/* raw emu frame should now be at gp2x_screen */
		if (Settings.scaling != 0)
		{
			soft_scale((char *)gp2x_screen + 32, gp2x_palette16, srendline, erendline-srendline);
			if (srendline)
				memset32((int *)((char *)gp2x_screen + 32), 0, srendline*320*2/4);
			memcpy(menu_bg, gp2x_screen + 32, 320*240*2);
		}
		else
		{
			convert2RGB555(menu_bg, gp2x_screen, gp2x_palette16, 320*240);
		}
	}
	else
	{
		memset32((int *)menu_bg, 0, 320*240*2/4);
		readpng(menu_bg, "background.png");
	}
}

static void menu_gfx_prepare(void)
{
	menu_prepare_bg();

	// switch bpp
	gp2x_video_changemode(16);
	gp2x_video_set_offs(0);
	gp2x_video_RGB_setscaling(320, 240);
	menu_flip();
}


int gp2x_menu_do(void)
{
	int ret;

	menu_gfx_prepare();

	ret = menu_loop_root();

	if (menu_bg) free(menu_bg);
	menu_bg = NULL;
	menuErrorMsg[0] = 0;

	return ret;
}


