#include "types.h"
#include "git.h"

/* Prototypes for platform interface functions follow: */

void FCEUD_Update(uint8 *buf, int16 *Buffer, int Count);

/* Video interface */
void FCEUD_SetPalette(uint8 index, uint8 r, uint8 g, uint8 b);
void FCEUD_GetPalette(uint8 i,uint8 *r, unsigned char *g, unsigned char *b);

/* Sound interface */
void FCEUD_WriteSoundData(int32 *Buffer, int Count);

/* Displays an error.  Can block or not. */
void FCEUD_PrintError(char *s);

#ifdef NETWORK
/* Network interface */

int FCEUD_NetworkConnect(void);
int FCEUD_NetworkRecvData(uint8 *data, uint32 len, int block);
int FCEUD_NetworkSendData(uint8 *data, uint32 len);
void FCEUD_NetworkClose(void);

#endif

#define DES_NTSCCOL     2

#define DES_RESET       0x10
#define DES_POWER       0x11

#define DES_GETNTSCHUE  0x20
#define DES_GETNTSCTINT  0x21
#define DES_SETNTSCHUE  0x22
#define DES_SETNTSCTINT  0x23

#define DES_NSFINC	0x50
#define DES_NSFDEC	0x51
#define DES_NSFRES	0x52

#define DES_VSUNIDIPSET 0x70
#define DES_VSUNITOGGLEDIPVIEW	0x71
#define	DES_VSUNICOIN	0x72
#define DES_FDSINSERT	0x80
#define DES_FDSEJECT	0x81
#define DES_FDSSELECT	0x82

#define DES_NTSCSELHUE	0x90
#define DES_NTSCSELTINT	0x91
#define DES_NTSCDEC	0x92
#define DES_NTSCINC	0x93

void DriverInterface(int w, void *d);

void FCEUI_SetInput(int port, int type, void *ptr, int attrib);
void FCEUI_SetInputFC(int type, void *ptr, int attrib);
void FCEUI_DisableFourScore(int s);

void FCEUI_GetNTSCTH(int *tint, int *hue);
void FCEUI_SetNTSCTH(int n, int tint, int hue);

#include "version.h"

#define SI_NONE      0
#define SI_GAMEPAD   1
#define SI_ZAPPER    2
#define SI_POWERPADA  3
#define SI_POWERPADB  4
#define SI_ARKANOID   5

#define SIFC_NONE      0
#define SIFC_ARKANOID  1
#define SIFC_SHADOW      2
#define SIFC_4PLAYER    3
#define SIFC_FKB      4
#define SIFC_SUBORKB    5
#define SIFC_HYPERSHOT  6
#define SIFC_MAHJONG  7
#define SIFC_QUIZKING  8
#define SIFC_FTRAINERA  9
#define SIFC_FTRAINERB  10
#define SIFC_OEKAKIDS  11
#define SIFC_BWORLD      12
#define SIFC_TOPRIDER  13

#define SIS_NONE  0
#define SIS_DATACH  1
#define SIS_NWC    2
#define SIS_VSUNISYSTEM  3
#define SIS_NSF    4


/* New interface functions */

/* 0 to order screen snapshots numerically(0.png), 1 to order them file base-numerically(smb3-0.png). */
void FCEUI_SetSnapName(int a);

/* 0 to keep 8-sprites limitation, 1 to remove it */
void FCEUI_DisableSpriteLimitation(int a);

/* 0 to save extra game data(*.sav) in same dir as game, 1 to save under base dir */
void FCEUI_SaveExtraDataUnderBase(int a);

/* name=path and file to load.  returns 0 on failure, 1 on success */
FCEUGI *FCEUI_LoadGame(char *name);
extern int LoadGameLastError;

/* allocates memory.  0 on failure, 1 on success. */
int FCEUI_Initialize(void);

void FCEUI_Kill(void);
void FCEUI_SetEmuMode(int is_new);

/* begins emulation.  Returns after FCEUI_CloseGame() is called */
extern void (*FCEUI_Emulate)(void);

/* Closes currently loaded game, causes FCEUI_Emulate to return */
void FCEUI_CloseGame(void);

/* Enable/Disable game genie. a=0 disable, a=1 enable */
void FCEUI_SetGameGenie(const char *rom);

/* Set video system a=0 NTSC, a=1 PAL */
void FCEUI_SetVidSystem(int a);

/* Convenience function; returns currently emulated video system(0=NTSC, 1=PAL).  */
int FCEUI_GetCurrentVidSystem(int *slstart, int *slend);

#ifdef FRAMESKIP
/* Should be called from FCEUD_BlitScreen().  Specifies how many frames
   to skip until FCEUD_BlitScreen() is called.  FCEUD_BlitScreenDummy()
   will be called instead of FCEUD_BlitScreen() when when a frame is skipped.
*/
void FCEUI_FrameSkip(int x);
#endif

/* First and last scanlines to render, for ntsc and pal emulation. */
void FCEUI_SetRenderedLines(int ntscf, int ntscl, int palf, int pall);

/* Sets the base directory(save states, snapshots, etc. are saved in directories
   below this directory. */
void FCEUI_SetBaseDirectory(char *dir);

/* Tells FCE Ultra to copy the palette data pointed to by pal and use it.
   Data pointed to by pal needs to be 64*3 bytes in length.
*/
void FCEUI_SetPaletteArray(uint8 *pal);

/* Sets up sound code to render sound at the specified rate, in samples
   per second.  The sample rate should be as close to 44100 hz as possible.
   Sample rates from 8192 through 65535 are ok, though if the sample rate
   is too low, some sound channels(noise) won't sound right.
   If "Rate" equals 0, sound is disabled.
*/
void FCEUI_Sound(int Rate);
void FCEUI_SetSoundVolume(uint32 volume);

#ifdef NETWORK
/* Set network play stuff. type=1 for server, type=2 for client.
   skip is only used for server */
void FCEUI_SetNetworkPlay(int type);
#endif

void FCEUI_SelectState(int w);
void FCEUI_SaveState(const char *fname);
void FCEUI_LoadState(const char *fname);
int32 FCEUI_GetDesiredFPS(void);
void FCEUI_SaveSnapshot(void);
void FCEU_DispMessage(char *format, ...);
void FCEU_CancelDispMessage(void);
#define FCEUI_DispMessage FCEU_DispMessage

int FCEUI_AddCheat(const char *name, uint32 addr, uint8 val, int compare, int type);
int FCEUI_DelCheat(uint32 which);

int32 FCEUI_CheatSearchGetCount(void);
void FCEUI_CheatSearchGetRange(uint32 first, uint32 last, int (*callb)(uint32 a, uint8 last, uint8 current));
void FCEUI_CheatSearchGet(int (*callb)(uint32 a, uint8 last, uint8 current, void *data),void *data);
void FCEUI_CheatSearchBegin(void);
void FCEUI_CheatSearchEnd(int type, uint8 v1, uint8 v2);
void FCEUI_ListCheats(int (*callb)(char *name, uint32 a, uint8 v, int compare, int s, int type, void *data), void *data);

int FCEUI_GetCheat(uint32 which, char **name, uint32 *a, uint8 *v, int *compare, int *s, int *type);
int FCEUI_SetCheat(uint32 which, const char *name, int32 a, int32 v, int compare,int s, int type);
void FCEUI_CheatSearchShowExcluded(void);
void FCEUI_CheatSearchSetCurrentAsOriginal(void);

int FCEUI_DecodePAR(const char *code, uint16 *a, uint8 *v, int *c, int *type);
int FCEUI_DecodeGG(const char *str, uint16 *a, uint8 *v, int *c);
int FCEUI_ToggleCheat(uint32 which);

#define FCEUIOD_STATE   0
#define FCEUIOD_SNAPS   1
#define FCEUIOD_NV      2
#define FCEUIOD_CHEATS  3
#define FCEUIOD_MISC    4

#define FCEUIOD__COUNT  5

void FCEUI_SetDirOverride(int which, char *n);

