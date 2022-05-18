#include "zlib/zip.h"
#include "zlib/unzip.h"
#include <stdio.h>
#include <stdlib.h>

char *load_archive(const char *filename, const char *entry, int *size)
{
    unzFile fd = NULL;
	unz_file_info info;
	char *buffer;
    int ret = 0;

	/* Attempt to open the archive */
	fd = unzOpen(filename);
	if(!fd)
	{
		return NULL;
	}

	/* Go to first file in archive */
	ret = unzLocateFile(fd, entry, 0);
	if(ret != UNZ_OK)
	{
		unzClose(fd);
		return NULL;
	}

	unzGetCurrentFileInfo(fd, &info, NULL, 0, NULL, 0, NULL, 0);

	/* Open the file for reading */
	ret = unzOpenCurrentFile(fd);
	if(ret != UNZ_OK)
	{
		unzClose(fd);
		return NULL;
	}

	/* Read (decompress) the file */
	buffer = (char *) malloc(info.uncompressed_size);
	ret = unzReadCurrentFile(fd, buffer, info.uncompressed_size);
	*size = info.uncompressed_size;

	/* Close the current file */
	unzCloseCurrentFile(fd);
	unzClose(fd);

	return buffer;
}

int save_archive(const char *filename, const char *entry,
		const char *buffer, int size)
{
    zipFile fd = NULL;
    int ret = 0;
    fd=zipOpen(filename, APPEND_STATUS_ADDINZIP);
    if(!fd)
       fd=zipOpen(filename, APPEND_STATUS_CREATE);
    if(!fd)
    {
       return (0);
    }

    ret=zipOpenNewFileInZip(fd, entry,
			    NULL,
				NULL,0,
			    NULL,0,
			    NULL,
			    Z_DEFLATED,
			    Z_DEFAULT_COMPRESSION);
			    
    if(ret != ZIP_OK)
    {
       zipClose(fd,NULL);
       return (0);    
    }

    ret=zipWriteInFileInZip(fd,buffer,size);
    if(ret != ZIP_OK)
    {
      zipCloseFileInZip(fd);
      zipClose(fd,NULL);
      return (0);
    }

    ret=zipCloseFileInZip(fd);
    if(ret != ZIP_OK)
    {
      zipClose(fd,NULL);
      return (0);
    }

    ret=zipClose(fd,NULL);
    if(ret != ZIP_OK)
    {
      return (0);
    }
	
    return(1);
}

