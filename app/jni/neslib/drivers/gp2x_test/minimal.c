/* emulate minimal lib using SDL */

#include <stdio.h>
#include <SDL.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <errno.h>

#include "../gp2x/minimal.h"
#include "../gp2x/usbjoy.h"
#include "../gp2x/cpuctrl.h"

SDL_Surface *screen;
void *gp2x_screen;
static int sounddev = 0;


/* video stuff */
void gp2x_video_flip(void)
{
	if(SDL_MUSTLOCK(screen))
		SDL_LockSurface(screen);

	memcpy(screen->pixels, gp2x_screen, 320*240*screen->format->BytesPerPixel);

	if(SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);

	SDL_UpdateRect(screen, 0, 0, 0, 0);
}


void gp2x_video_changemode2(int bpp)
{
	const SDL_VideoInfo *vinf;
	int flags=0;

	vinf=SDL_GetVideoInfo();

	if(vinf->hw_available)
		flags|=SDL_HWSURFACE;

	if (bpp == 8)
		flags|=SDL_HWPALETTE;

	screen = SDL_SetVideoMode(320, 240, bpp, flags);
	if(!screen)
	{
		puts(SDL_GetError());
		return;
	}

	SDL_WM_SetCaption("FCE Ultra","FCE Ultra");
}


static SDL_Color psdl[256];

void gp2x_video_changemode(int bpp)
{
	gp2x_video_changemode2(bpp);
	if (bpp == 8)
		SDL_SetPalette(screen,SDL_PHYSPAL,psdl,0,256);
	gp2x_video_flip();
}


void gp2x_video_setpalette(int *pal, int len)
{
	int i;

	for (i = 0; i < len; i++)
	{
		psdl[i].r = pal[i] >> 16;
		psdl[i].g = pal[i] >> 8;
		psdl[i].b = pal[i];
	}

	SDL_SetPalette(screen,SDL_PHYSPAL,psdl,0,len);
}


void gp2x_video_RGB_setscaling(int W, int H)
{
}

void gp2x_video_set_offs(int offs)
{
}

void gp2x_video_flush_cache(void)
{
}

void gp2x_memcpy_buffers(int buffers, void *data, int offset, int len)
{
}


void gp2x_memcpy_all_buffers(void *data, int offset, int len)
{
}


void gp2x_memset_all_buffers(int offset, int byte, int len)
{
	memset((char *)gp2x_screen + offset, byte, len);
}


unsigned long gp2x_joystick_read(int allow_usb_joy)
{
	unsigned long keys_out = 0;
	Uint8 *keys;
	int i;

	SDL_PumpEvents();
	keys = SDL_GetKeyState(NULL);

	if (keys[SDLK_UP])	keys_out |= GP2X_UP;
	if (keys[SDLK_LEFT])	keys_out |= GP2X_LEFT;
	if (keys[SDLK_DOWN])	keys_out |= GP2X_DOWN;
	if (keys[SDLK_RIGHT])	keys_out |= GP2X_RIGHT;
	if (keys[SDLK_RETURN])	keys_out |= GP2X_START;
	if (keys[SDLK_BACKSLASH]) keys_out |= GP2X_SELECT;
	if (keys[SDLK_s])	keys_out |= GP2X_L;
	if (keys[SDLK_d])	keys_out |= GP2X_R;
	if (keys[SDLK_z])	keys_out |= GP2X_A;
	if (keys[SDLK_x])	keys_out |= GP2X_X;
	if (keys[SDLK_c])	keys_out |= GP2X_B;
	if (keys[SDLK_v])	keys_out |= GP2X_Y;
	if (keys[SDLK_q])	keys_out |= GP2X_VOL_DOWN;
	if (keys[SDLK_w])	keys_out |= GP2X_VOL_UP;
	if (keys[SDLK_RIGHTBRACKET]) keys_out |= GP2X_PUSH;

	if (allow_usb_joy && num_of_joys > 0) {
		// check the usb joy as well..
		gp2x_usbjoy_update();
		for (i = 0; i < num_of_joys; i++)
			keys_out |= gp2x_usbjoy_check(i);
	}

	return keys_out;
}

static int s_oldrate = 0, s_oldbits = 0, s_oldstereo = 0;

void gp2x_start_sound(int rate, int bits, int stereo)
{
	int frag = 0, bsize, buffers;

	// if no settings change, we don't need to do anything
	if (rate == s_oldrate && s_oldbits == bits && s_oldstereo == stereo) return;

	if (sounddev > 0) close(sounddev);
	sounddev = open("/dev/dsp", O_WRONLY|O_ASYNC);
	if (sounddev == -1)
	{
		printf("open(\"/dev/dsp\") failed with %i\n", errno);
		return;
	}

	ioctl(sounddev, SNDCTL_DSP_SPEED,  &rate);
	ioctl(sounddev, SNDCTL_DSP_SETFMT, &bits);
	ioctl(sounddev, SNDCTL_DSP_STEREO, &stereo);
	// calculate buffer size
	buffers = 16;
	bsize = rate / 32;
	if (rate > 22050) { bsize*=4; buffers*=2; } // 44k mode seems to be very demanding
	while ((bsize>>=1)) frag++;
	frag |= buffers<<16; // 16 buffers
	ioctl(sounddev, SNDCTL_DSP_SETFRAGMENT, &frag);
	printf("gp2x_set_sound: %i/%ibit/%s, %i buffers of %i bytes\n",
		rate, bits, stereo?"stereo":"mono", frag>>16, 1<<(frag&0xffff));

	s_oldrate = rate; s_oldbits = bits; s_oldstereo = stereo;
	usleep(100000);
}


void gp2x_sound_write(void *buff, int len)
{
	if (sounddev > 0)
		write(sounddev, buff, len);
}

void gp2x_sound_sync(void)
{
	if (sounddev > 0)
		ioctl(sounddev, SOUND_PCM_SYNC, 0);
}

void gp2x_sound_volume(int l, int r)
{
}


void gp2x_init(void)
{
	printf("entering init()\n"); fflush(stdout);

	gp2x_screen = malloc(320*240*2 + 32);
	if(gp2x_screen == NULL) return;
	memset(gp2x_screen, 0, 320*240*2 + 32);

	if(SDL_Init(SDL_INIT_NOPARACHUTE))
	{
	 printf("Could not initialize SDL: %s.\n", SDL_GetError());
	 return;
	}

	if(SDL_InitSubSystem(SDL_INIT_VIDEO)==-1)
	{
		puts(SDL_GetError());
		return;
	}

	SDL_ShowCursor(0);

	gp2x_video_changemode(8);

	/* init usb joys -GnoStiC */
	gp2x_usbjoy_init();

	SDL_PumpEvents();

	printf("gp2x_init done.\n");
}


char *ext_menu = 0, *ext_state = 0;

void gp2x_deinit(void)
{
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_Quit();

	free(gp2x_screen);
	if (sounddev > 0) close(sounddev);
	gp2x_usbjoy_deinit();
}


/* other stuff to be faked */
void cpuctrl_init(void)
{
}

void cpuctrl_deinit(void)
{
}

void set_FCLK(unsigned MHZ)
{
}

void set_RAM_Timings(int tRC, int tRAS, int tWR, int tMRD, int tRFC, int tRP, int tRCD)
{
}

void set_gamma(int g100)
{
}

void set_LCD_custom_rate(lcd_rate_t rate)
{
}

void unset_LCD_custom_rate(void)
{
}


int mmuhack(void)
{
	return 1;
}

int mmuunhack(void)
{
	return 1;
}

void memset32(int *dest, int c, int count)
{
	memset(dest, c, count*4);
}

void spend_cycles(int c)
{
	usleep(c/200);
}

void convert2RGB555(unsigned short *dst, unsigned char *src, unsigned short *pal, int count)
{
	while (count--)
		*dst++ = pal[*src++];
}

/* don't scale, just convert */
void soft_scale(void *dst, unsigned short *pal, int line_offs, int lines)
{
	unsigned char *src = (unsigned char *)dst + (line_offs + lines) * 320;
	unsigned short *dest = (unsigned short *)dst + (line_offs + lines) * 320;
	int count = lines*320;

	while (count--)
		*(--dest) = pal[*(--src)];
}

