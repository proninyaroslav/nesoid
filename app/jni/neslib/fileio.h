#ifndef FILEIO_H
#define FILEIO_H

char *load_archive(const char *filename, const char *entry, int *size);
int save_archive(const char *filename, const char *entry,
		const char *buffer, int size);

#endif
