#include <stdio.h>

FILE *FCEUD_UTF8fopen(const char *fn, const char *mode)
{
	return fopen(fn, mode);
}
