#include "../types.h"
#include "../input.h"
#include "../fce.h"
#include "../ppu.h"
#include "../ppu098.h"
#include "../x6502.h"
#include "../palette.h"

void FCEU_DrawCursor(uint8 *buf, int xc, int yc);
void FCEU_DrawGunSight(uint8 *buf, int xc, int yc);

#define SCREEN_WIDTH 320
#define SCREEN_OFFS 32

