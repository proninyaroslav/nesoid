void FCEUPPU_Init(void);
void FCEUPPU_Reset(void);
void FCEUPPU_Power(void);
void FCEUPPU_Loop(int skip);

void FCEUPPU_LineUpdate098();

#define FCEUPPU_LineUpdate() \
	if (use098code) FCEUPPU_LineUpdate098()
