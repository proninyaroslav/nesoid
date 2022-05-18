extern "C" {
#include "../../driver.h"
#include "../../fce.h"

void CloseGame();
}

#define LOG_TAG "libnes"
#include <utils/Log.h>
#include "emuengine.h"
#include <string.h>

#define SOUND_RATE			22050

#define SCREEN_W			256
#define SCREEN_H			240
#define SCREEN_PITCH		320

#define GAMEPAD_A			0x01
#define GAMEPAD_B			0x02
#define GAMEPAD_A_TURBO		(GAMEPAD_A << 8)
#define GAMEPAD_B_TURBO		(GAMEPAD_B << 8)

extern "C" void Blit8To16Asm(void *src, void *dst, void *pal, int width);
extern "C" void Blit8To16RevAsm(void *src, void *dst, void *pal, int width);

int soundvol = 100;

static EmuEngine *engine;
static EmuEngine::Callbacks *callbacks;
static unsigned int VPalette[256];

class NesEngine : public EmuEngine {
public:
	NesEngine();
	virtual ~NesEngine();

	virtual bool initialize(Callbacks *cbs);
	virtual void destroy();
	virtual void reset();
	virtual void power();
	virtual void fireLightGun(int x, int y);
	virtual Game *loadRom(char *file);
	virtual void unloadRom();
	virtual void renderFrame(const Surface &surface);
	virtual bool saveState(const char *file);
	virtual bool loadState(const char *file);
	virtual void runFrame(unsigned int keys, bool skip);
	virtual void setOption(const char *name, const char *value);

private:
	int secondController;
	uint32 JSreturn[2];
	uint32 lightGunEvent;
	uint32 MouseData[3];

	int accurateMode;
};


NesEngine::NesEngine()
		: accurateMode(0)
{
	engine = this;
}

NesEngine::~NesEngine()
{
	engine = NULL;
}

bool NesEngine::initialize(EmuEngine::Callbacks *cbs)
{
	callbacks = cbs;
	memset(VPalette, 0, sizeof(VPalette));

	if (!FCEUI_Initialize())
		return false;

	JSreturn[0] = JSreturn[1] = 0;
	lightGunEvent = 0;
	memset(MouseData, 0, sizeof(MouseData));

	FCEUI_SetInput(0, SI_GAMEPAD, &JSreturn[0], 0);
	return true;
}

void NesEngine::destroy()
{
	delete this;
}

void NesEngine::reset()
{
	ResetNES();
}

void NesEngine::power()
{
	PowerNES();
}

void NesEngine::fireLightGun(int x, int y)
{
	if (secondController == SI_ZAPPER)
		lightGunEvent = x | (y << 8);
}

NesEngine::Game *NesEngine::loadRom(char *file)
{
	FCEU_CancelDispMessage();
	FCEUI_SetEmuMode(accurateMode);

	FCEUGI *gi = FCEUI_LoadGame(file);
	if (gi == NULL)
		return NULL;

	static Game game;
	game.videoWidth = SCREEN_W;
	game.videoHeight = SCREEN_H;
	if (!PAL)
		game.videoHeight -= 8;

	game.soundRate = SOUND_RATE;
	game.soundBits = 16;
	game.soundChannels = 1;
	game.fps = (PAL ? 50 : 60);
	return &game;
}

void NesEngine::unloadRom()
{
	CloseGame();
}

void NesEngine::renderFrame(const Surface &surface)
{
	extern uint8 *XBuf;
	uint8 *xbuf = XBuf + 8;

	void (*blt)(void*, void*, void*, int) = Blit8To16Asm;
	uint8 *d = (uint8 *) surface.bits;
	uint8 *s = xbuf + 24;
	int h = SCREEN_H;
	if (!PAL) {
		s += SCREEN_PITCH * 8;
		h -= 8;
	}
	if (surface.bpr < 0) {
		blt = Blit8To16RevAsm;
		d += (h - 1) * -surface.bpr + SCREEN_W * 2;
	}
	while (--h >= 0) {
		blt(s, d, VPalette, SCREEN_W);
		d += surface.bpr;
		s += SCREEN_PITCH;
	}
}

bool NesEngine::saveState(const char *file)
{
	FCEUI_SaveState(file);
	return true;
}

bool NesEngine::loadState(const char *file)
{
	FCEUI_LoadState(file);
	return true;
}

void NesEngine::runFrame(unsigned int keys, bool skip)
{
	// gamepad
	if (secondController != SI_GAMEPAD)
		keys &= 0xffff;

	static int turbo = 0;
	turbo ^= 1;
	for (int i = 0; i < 2; i++) {
		if (turbo) {
			if (keys & GAMEPAD_A_TURBO)
				keys |= GAMEPAD_A;
			if (keys & GAMEPAD_B_TURBO)
				keys |= GAMEPAD_B;
		}
		JSreturn[i] = (keys & 0xff);
		keys >>= 16;
	}

	// light gun
	const int gun = lightGunEvent;
	if (gun) {
		MouseData[0] = gun & 0xff;
		MouseData[1] = (gun >> 8) & 0xff;
		MouseData[2] = 1;
	}

	extern int FSkip;
	FSkip = (skip ? 1 : 0);
	FCEUI_Emulate();

	// reset light gun
	if (gun) {
		lightGunEvent = 0;
		MouseData[2] = 0;
	}
}

static int getControllerDevice(const char *name)
{
	if (strcmp(name, "zapper") == 0)
		return SI_ZAPPER;
	if (strcmp(name, "gamepad") == 0)
		return SI_GAMEPAD;
	return SI_NONE;
}

void NesEngine::setOption(const char *name, const char *value)
{
	if (strcmp(name, "soundEnabled") == 0) {
		bool enabled = (strcmp(value, "true") == 0);
		FCEUI_Sound(enabled ? SOUND_RATE : 0);

	} else if (strcmp(name, "accurateRendering") == 0) {
		bool enabled = (strcmp(value, "true") == 0);
		accurateMode = (enabled ? 1 : 0);

	} else if (strcmp(name, "secondController") == 0) {
		secondController = getControllerDevice(value);
		switch (secondController) {
		case SI_ZAPPER:
			FCEUI_SetInput(1, SI_ZAPPER, &MouseData, 1);
			break;
		case SI_GAMEPAD:
			FCEUI_SetInput(1, SI_GAMEPAD, &JSreturn[1], 0);
			break;
		default:
			FCEUI_SetInput(1, SI_NONE, NULL, 0);
		}

	} else if (strcmp(name, "gameGenieRom") == 0) {
		FCEUI_SetGameGenie(value);
	}
}

inline void videoUpdate(uint8 *xbuf)
{
	EmuEngine::Surface surface;
	if (callbacks->lockSurface(&surface)) {
		engine->renderFrame(surface);
		callbacks->unlockSurface(&surface);
	}
}

extern "C"
void FCEUD_Update(uint8 *xbuf, int16 *Buffer, int Count)
{
	if (xbuf != NULL)
		videoUpdate(xbuf);
	if (Count > 0)
		callbacks->playAudio(Buffer, Count * 2);
}

extern "C"
void FCEUD_SetPalette(uint8 index, uint8 r, uint8 g, uint8 b)
{
	VPalette[index] =
		((r & 0xf8) << 8) |
		((g & 0xfc) << 3) |
		((b & 0xf8) >> 3);
}

extern "C"
void FCEUD_GetPalette(uint8 index, uint8 *r, uint8 *g, uint8 *b)
{
}

extern "C" __attribute__((visibility("default")))
EmuEngine *createObject()
{
	return new NesEngine;
}
