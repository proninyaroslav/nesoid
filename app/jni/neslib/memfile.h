#ifndef MEMFILE_H
#define MEMFILE_H

#include <stdio.h>
#include "types.h"

struct _MEMFILE;
typedef struct _MEMFILE MEMFILE;

MEMFILE *mem_fopen_read(const char *filename);
MEMFILE *mem_fopen_write(const char *filename);
void mem_fclose(MEMFILE *file);

int mem_fputc(int c, MEMFILE *file);
int mem_fgetc(MEMFILE *file);

int mem_write16(uint16 b, MEMFILE *fp);
int mem_write32(uint32 b, MEMFILE *fp);
int mem_read32(void *Bufo, MEMFILE *fp);

size_t mem_fread(void *buf, size_t size, size_t nmemb, MEMFILE *file);
size_t mem_fwrite(const void *buf, size_t size, size_t nmemb, MEMFILE *file);

long mem_ftell(MEMFILE *file);
int mem_fseek(MEMFILE *file, long offset, int whence);

#define mem_write32le mem_write32
#define mem_read32le mem_read32

#endif

