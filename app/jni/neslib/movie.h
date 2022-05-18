#ifndef __MOVIE_H_
#define __MOVIE_H_

#include "types.h"

void FCEUMOV_AddJoy(uint8 *);

#if 0
void FCEUMOV_CheckMovies(void);
void FCEUMOV_AddCommand(int cmd);
void FCEU_DrawMovies(uint8 *);
int FCEUMOV_IsPlaying(void);
int FCEUMOV_IsRecording(void);

int FCEUMOV_WriteState(FILE* st);
int FCEUMOV_ReadState(FILE* st, uint32 size);
void FCEUMOV_PreLoad(void);
int FCEUMOV_PostLoad(void);
#endif

extern int current;     // > 0 for recording, < 0 for playback
extern uint32 framecount;
void FCEUI_LoadMovie(char *fname, int _read_only);
void FCEUI_StopMovie(void);

#endif /* __MOVIE_H_ */
