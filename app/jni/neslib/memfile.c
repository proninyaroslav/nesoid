#include <stdlib.h>
#include <string.h>
#include "memfile.h"
#include "fileio.h"
#include "_endian.h"

struct _MEMFILE {
	const char *filename;
	char *buffer;
	long cursor;
	long size;
	long capacity;
};

MEMFILE *mem_fopen_read(const char *filename)
{
	int size;
	char *buffer = load_archive(filename, "NESOID", &size);
	if (buffer == NULL) {
		FILE *file = fopen(filename, "rb");
		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);

		buffer = (char *) malloc(size);
		fread(buffer, size, 1, file);
		fclose(file);
	}

	MEMFILE *file = (MEMFILE *) malloc(sizeof(MEMFILE));
	memset(file, 0, sizeof(MEMFILE));
	file->buffer = buffer;
	file->size = size;
	return file;
}

MEMFILE *mem_fopen_write(const char *filename)
{
	MEMFILE *file = (MEMFILE *) malloc(sizeof(MEMFILE));
	memset(file, 0, sizeof(MEMFILE));
	file->filename = filename;
	return file;
}

void mem_fclose(MEMFILE *file)
{
	if (file->filename != NULL)
		save_archive(file->filename, "NESOID", file->buffer, file->size);

	if (file->buffer != NULL)
		free(file->buffer);
	free(file);
}

long mem_ftell(MEMFILE *file)
{
	return file->cursor;
}

int mem_fseek(MEMFILE *file, long offset, int whence)
{
	long cursor;

	switch (whence) {
	case SEEK_CUR:
		cursor = file->cursor;
		break;
	case SEEK_END:
		cursor = file->size;
		break;
	case SEEK_SET:
	default:
		cursor = 0;
		break;
	}

	cursor += offset;
	if (cursor < 0)
		cursor = 0;
	else if (cursor > file->size)
		cursor = file->size;

	file->cursor = cursor;
	return 0;
}

int mem_fputc(int c, MEMFILE *file)
{
	unsigned char buf = c;
	return mem_fwrite(&buf, sizeof(buf), 1, file);
}

int mem_fgetc(MEMFILE *file)
{
	unsigned char buf;
	if (mem_fread(&buf, sizeof(buf), 1, file) < 1)
		return EOF;
	return buf;
}

size_t mem_fread(void *buf, size_t size, size_t nmemb, MEMFILE *file)
{
	size_t n = size * nmemb;
	size_t left = file->size - file->cursor;
	if (n > left)
		n = left;

	memcpy(buf, file->buffer + file->cursor, n);
	file->cursor += n;
	return (n / size);
}

size_t mem_fwrite(const void *buf, size_t size, size_t nmemb, MEMFILE *file)
{
	size_t n = size * nmemb;
	long capacity = (file->cursor + n + 8191) & ~8191;
	if (file->capacity < capacity) {
		file->buffer = (char *) realloc(file->buffer, capacity);
		file->capacity = capacity;
	}
	memcpy(file->buffer + file->cursor, buf, n);
	file->cursor += n;
	if (file->cursor > file->size)
		file->size = file->cursor;
	return (n / size);
}

static uint8 s[4];

int mem_write16(uint16 b, MEMFILE *fp)
{
 s[0]=b;
 s[1]=b>>8;
 return((mem_fwrite(s,1,2,fp)<2)?0:2);
}

int mem_write32(uint32 b, MEMFILE *fp)
{
 s[0]=b;
 s[1]=b>>8;
 s[2]=b>>16;
 s[3]=b>>24;
 return((mem_fwrite(s,1,4,fp)<4)?0:4);
}

int mem_read32(void *Bufo, MEMFILE *fp)
{
 uint32 buf;
 if(mem_fread(&buf,1,4,fp)<4)
  return 0;
 #ifdef LSB_FIRST
 *(uint32*)Bufo=buf;
 #else
 *(uint32*)Bufo=((buf&0xFF)<<24)|((buf&0xFF00)<<8)|((buf&0xFF0000)>>8)|((buf&0xFF000000)>>24);
 #endif
 return 1;
}

int mem_read16(char *d, MEMFILE *fp)
{
 #ifdef LSB_FIRST
 return((mem_fread(d,1,2,fp)<2)?0:2);
 #else
 int ret;
 ret=mem_fread(d+1,1,1,fp);
 ret+=mem_fread(d,1,1,fp);
 return ret<2?0:2;
 #endif
}

