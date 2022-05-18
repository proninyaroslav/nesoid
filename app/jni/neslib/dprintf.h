#if 0
#include <stdio.h>

extern uint32 timestamp;
extern uint32 framecount;
extern int scanline;

#undef dprintf
#define dprintf(f,...) printf("%05u:%05u:%03i: " f "\n",timestamp,framecount,scanline,##__VA_ARGS__)

#else

#define dprintf(x...)

#endif

