extern CFGSTRUCT DriverConfig[];
extern ARGPSTRUCT DriverArgs[];
extern char *DriverUsage;

void DoDriverArgs(void);
void GetBaseDirectory(char *BaseDirectory);

int InitSound(void);
void WriteSound(int16 *Buffer, int Count);

void KillSound(void);
void SilenceSound(int s);


int InitVideo(void);
void KillVideo(void);
void BlitPrepare(int skip);
void BlitScreen(int skip);
void LockConsole(void);
void UnlockConsole(void);
void ToggleFS();
