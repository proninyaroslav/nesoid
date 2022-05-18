#include <stdio.h>
#include <string.h>
#include <png.h>

void readpng(unsigned short *dest, const char *fname)
{
	FILE *fp;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_bytepp row_ptr = NULL;
	int height, width, h;

	if (dest == NULL || fname == NULL)
	{
		return;
	}

	fp = fopen(fname, "rb");
	if (fp == NULL)
	{
		printf(__FILE__ ": failed to open: %s\n", fname);
		return;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		printf(__FILE__ ": png_create_read_struct() failed\n");
		fclose(fp);
		return;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		printf(__FILE__ ": png_create_info_struct() failed\n");
		goto done;
	}

	// Start reading
	png_init_io(png_ptr, fp);
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_STRIP_ALPHA | PNG_TRANSFORM_PACKING, NULL);
	row_ptr = png_get_rows(png_ptr, info_ptr);
	if (row_ptr == NULL)
	{
		printf(__FILE__ ": png_get_rows() failed\n");
		goto done;
	}

	height = info_ptr->height;
	if (height > 240) height = 240;
	width = info_ptr->width;
	if (width > 320) width = 320;

	for (h = 0; h < height; h++)
	{
		unsigned char *src = row_ptr[h];
		int len = width;
		while (len--)
		{
			*dest++ = ((src[0]&0xf8)<<8) | ((src[1]&0xf8)<<3) | (src[2] >> 3);
			src += 3;
		}
		dest += 320 - width;
	}


done:
	png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : NULL, (png_infopp)NULL);
	fclose(fp);
}

