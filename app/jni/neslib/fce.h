#ifndef _FCEH

#ifdef ASM_6502
void asmcpu_unpack(void);
void asmcpu_pack(void);
#endif

#define fceuindbg 0

extern int use098code;

extern int GameLoaded;
void ResetGameLoaded(void);

#define DECLFR(x) uint8 FP_FASTAPASS(1) x (uint32 A)
#define DECLFW(x) void FP_FASTAPASS(2) x (uint32 A, uint8 V)

void FASTAPASS(3) SetReadHandler(int32 start, int32 end, readfunc func);
void FASTAPASS(3) SetWriteHandler(int32 start, int32 end, writefunc func);
writefunc FASTAPASS(1) GetWriteHandler(int32 a);
readfunc FASTAPASS(1) GetReadHandler(int32 a);

int AllocGenieRW(void);
void FlushGenieRW(void);

void FCEU_ResetVidSys(void);

void ResetMapping(void);

extern void (*ResetNES)(void);

void ResetNES081(void);
void PowerNES(void);


extern uint64 timestampbase;
extern uint32 MMC5HackVROMMask;
extern uint8 *MMC5HackExNTARAMPtr;
extern int MMC5Hack;
extern uint8 *MMC5HackVROMPTR;
extern uint8 MMC5HackCHRMode;
extern uint8 MMC5HackSPMode;
extern uint8 MMC5HackSPScroll;
extern uint8 MMC5HackSPPage;

extern uint8 GameMemBlock[131072];
extern uint8 NTARAM[0x800],PALRAM[0x20];

extern uint8 RAM[0x800];


extern uint32 RefreshAddr,TempAddr;
extern uint8 vtoggle,XOffset,VRAMBuffer,PPUGenLatch;
extern uint8 PPU[4];

extern int scanline;
extern uint8 *vnapage[4];

extern uint8 PPUNTARAM;
extern uint8 PPUCHRRAM;
extern uint8 VPAL[8];
extern uint8 PAL;

extern readfunc ARead[0x10000];
extern writefunc BWrite[0x10000];

#define	VBlankON	(PPU[0]&0x80)	/* Generate VBlank NMI */
#define	SpHitON		(PPU[0]&0x40)
#define	Sprite16	(PPU[0]&0x20)	/* Sprites 8x16/8x8        */
#define	BGAdrHI		(PPU[0]&0x10)	/* BG pattern adr $0000/$1000 */
#define	SpAdrHI		(PPU[0]&0x08)	/* Sprite pattern adr $0000/$1000 */
#define	INC32		(PPU[0]&0x04)	/* auto increment 1/32  */
#define	NameTable	(PPU[0]&0x3)	/* name table $2000/$2400/$2800/$2C00 */

#define SpriteON        (PPU[1]&0x10)   /* Show Sprite             */
#define ScreenON        (PPU[1]&0x08)   /* Show screen             */
#define PPU_status      (PPU[2])


extern void (*GameInterface)(int h, void *param);
extern void FP_FASTAPASS(1) (*PPU_hook)(uint32 A);
extern void (*GameHBIRQHook)(void), (*GameHBIRQHook2)(void);
extern void (*GameStateRestore)(int version);

#define GI_RESETM2	1
#define GI_POWER	2
#define GI_CLOSE	3
#define GI_INFOSTRING	11

#include "git.h"
extern FCEUGI FCEUGameInfo;
extern int GameAttributes;

extern uint8 pale;
extern uint8 vsdip;


#endif

#define _FCEH

