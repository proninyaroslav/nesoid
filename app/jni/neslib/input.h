typedef struct {
        uint8 FP_FASTAPASS(1) (*Read)(int w);
	void FP_FASTAPASS(1) (*Write)(uint8 v);
        void FP_FASTAPASS(1) (*Strobe)(int w);
	void FP_FASTAPASS(3) (*Update)(int w, void *data, int arg);
	void FP_FASTAPASS(3) (*SLHook)(int w, uint8 *bg, uint8 *spr, uint32 linets, int final);
	void FP_FASTAPASS(3) (*Draw)(int w, uint8 *buf, int arg);
} INPUTC;

typedef struct {
	uint8 FP_FASTAPASS(2) (*Read)(int w, uint8 ret);
	void FP_FASTAPASS(1) (*Write)(uint8 v);
	void (*Strobe)(void);
        void FP_FASTAPASS(2) (*Update)(void *data, int arg);
        void FP_FASTAPASS(3) (*SLHook)(uint8 *bg, uint8 *spr, uint32 linets, int final);
        void FP_FASTAPASS(2) (*Draw)(uint8 *buf, int arg);
} INPUTCFC;

void DrawInput(uint8 *buf);
void UpdateInput(void);
void InitializeInput(void);
extern void (*PStrobe[2])(void);
extern void (*InputScanlineHook)(uint8 *bg, uint8 *spr, uint32 linets, int final);

#define FCEUNPCMD_RESET   0x01
#define FCEUNPCMD_POWER   0x02

#define FCEUNPCMD_VSUNICOIN     0x07
#define FCEUNPCMD_VSUNIDIP0     0x08
#define FCEUNPCMD_FDSINSERT     0x18
#define FCEUNPCMD_FDSSELECT     0x1A
void FCEU_DoSimpleCommand(int cmd);

void FCEUI_FDSSelect(void);
int FCEUI_FDSInsert(void);
void FCEUI_VSUniToggleDIP(int w);
void FCEUI_VSUniCoin(void);
void FCEUI_ResetNES(void);
void FCEUI_PowerNES(void);

