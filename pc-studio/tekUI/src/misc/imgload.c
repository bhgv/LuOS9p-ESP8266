
#include <string.h>
#include <tek/debug.h>
#include <tek/inline/exec.h>
#include <tek/lib/imgload.h>

#if defined(ENABLE_PNG)

#include <png.h>

static void imgload_pngread(png_structp png, png_bytep buf, png_size_t nbytes)
{
	struct ImgLoader *ld = png_get_io_ptr(png);
	if (!(*ld->iml_ReadFunc)(ld, buf, nbytes))
		png_error(png, "error reading");
}

static TBOOL imgload_load_png(struct ImgLoader *ld)
{
	struct TExecBase *TExecBase = ld->iml_ExecBase;
	unsigned int x, y;
	TUINT8 header[8];
	
	png_structp png;
	png_infop pnginfo = NULL;
	png_bytep *row_pointers = NULL;
	
	if (!((*ld->iml_ReadFunc)(ld, header, sizeof header) &&
		png_sig_cmp(header, 0, 8) == 0))
		return TFALSE;
	if ((*ld->iml_SeekFunc)(ld, 0, SEEK_SET) != 0)
		return TFALSE;
	
	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png == NULL)
		return TFALSE;
	
	if (setjmp(png_jmpbuf(png)))
	{
		if (ld->iml_Height && row_pointers)
		{
			for (y = 0; y < ld->iml_Height; y++)
			{
				if (row_pointers[y])
					TFree(row_pointers[y]);
			}
			TFree(row_pointers);
		}
		TDBPRINTF(TDB_INFO,("png load failed\n"));
		png_destroy_read_struct(&png, &pnginfo, NULL);
		return TFALSE;
	}
	
	pnginfo = png_create_info_struct(png);
	if (!png) png_error(png, "no memory");
	
	png_set_read_fn(png, ld, imgload_pngread);

	png_read_info(png, pnginfo);
	
	ld->iml_Width = png_get_image_width(png, pnginfo);
	ld->iml_Height = png_get_image_height(png, pnginfo);
	png_set_interlace_handling(png);
	int numchan = png_get_channels(png, pnginfo);
	int coltype = png_get_color_type(png, pnginfo);
	png_set_packing(png);
	png_read_update_info(png, pnginfo);

	png_color *palette = NULL;
	int numpal = 0;
	if (coltype == PNG_COLOR_TYPE_PALETTE)
	{
		if (png_get_PLTE(png, pnginfo, &palette, &numpal) != PNG_INFO_PLTE)
			png_error(png, "could not get palette");
	}

	row_pointers = (png_bytep *) TAlloc0(TNULL, 
		ld->iml_Height * sizeof(png_bytep));
	if (!row_pointers) png_error(png, "no memory");

	png_uint_32 rowbytes = png_get_rowbytes(png, pnginfo);
	for (y = 0; y < ld->iml_Height; y++)
	{
		row_pointers[y] = (png_byte *) TAlloc(TNULL, rowbytes);
		if (!row_pointers[y]) png_error(png, "no memory");
	}
	
	/*fprintf(stderr, "numchan=%d coltype=%d width=%d rowbytes=%d numpal=%d\n",
		numchan, coltype, ld->iml_Width, rowbytes, numpal);*/
	
	png_read_image(png, row_pointers);
	
	TUINT *buf = TAlloc(TNULL, ld->iml_Width * ld->iml_Height * sizeof(TUINT));
	if (!buf) png_error(png, "no memory");

	TUINT dstfmt = TVPIXFMT_08R8G8B8;
	typedef enum { PNG_PALETTE, PNG_GRAY, PNG_GRAYALPHA, PNG_RGB, 
		PNG_RGB_ALPHA } pngtype_t;
	pngtype_t mode;
	if (palette)
		mode = PNG_PALETTE;
	else if (coltype == PNG_COLOR_TYPE_GRAY)
		mode = PNG_GRAY;
	else if (coltype == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		mode = PNG_GRAYALPHA;
		dstfmt = TVPIXFMT_A8R8G8B8;
		ld->iml_Flags |= IMLFL_HAS_ALPHA;
	}
	else if (numchan == 4)
	{
		mode = PNG_RGB_ALPHA;
		dstfmt = TVPIXFMT_A8R8G8B8;
		ld->iml_Flags |= IMLFL_HAS_ALPHA;
	}
	else
		mode = PNG_RGB;

	ld->iml_Image.tpb_Data = (TUINT8 *) buf;
	ld->iml_Image.tpb_Format = dstfmt;
	ld->iml_Image.tpb_BytesPerLine = ld->iml_Width * 4;
	for (y = 0; y < ld->iml_Height; ++y)
	{
		switch (mode)
		{
			case PNG_PALETTE:
				for (x = 0; x < ld->iml_Width; ++x)
				{
					TUINT8 idx = row_pointers[y][x];
					TUINT r = palette[idx].red;
					TUINT g = palette[idx].green;
					TUINT b = palette[idx].blue;
					*buf++ = 0xff000000 | (r << 16) | (g << 8) | b;
				}
				break;
			case PNG_GRAY:
				for (x = 0; x < ld->iml_Width; ++x)
				{
					TUINT8 c = row_pointers[y][x];
					*buf++ = 0xff000000 | (c << 16) | (c << 8) | c;
				}
				break;
			case PNG_GRAYALPHA:
				for (x = 0; x < ld->iml_Width * 2; x += 2)
				{
					TUINT8 c = row_pointers[y][x];
					TUINT8 a = row_pointers[y][x + 1];
					*buf++ = (a << 24) | (c << 16) | (c << 8) | c;
				}
				break;
			case PNG_RGB_ALPHA:
				for (x = 0; x < ld->iml_Width * 4; x += 4)
				{
					TUINT r = row_pointers[y][x];
					TUINT g = row_pointers[y][x + 1];
					TUINT b = row_pointers[y][x + 2];
					TUINT a = row_pointers[y][x + 3];
					*buf++ = (a << 24) | (r << 16) | (g << 8) | b;
				}
				break;
			case PNG_RGB:
				for (x = 0; x < ld->iml_Width * 3; x += 3)
				{
					TUINT r = row_pointers[y][x];
					TUINT g = row_pointers[y][x + 1];
					TUINT b = row_pointers[y][x + 2];
					*buf++ = (r << 16) | (g << 8) | b;
				}
				break;
		}
	}

	png_destroy_read_struct(&png, &pnginfo, NULL);
	
	for (y = 0; y < ld->iml_Height; y++)
		TFree(row_pointers[y]);
	TFree(row_pointers);
	TDBPRINTF(TDB_TRACE,("successfully loaded PNG\n"));
	return TTRUE;
}

#endif /* defined(ENABLE_PNG) */


static TBOOL imgload_load_ppm(struct ImgLoader *ld)
{
	struct TExecBase *TExecBase = ld->iml_ExecBase;
	int tw, th, maxv = 0, x, y;
	char header[256];
	if (!(*ld->iml_ReadFunc)(ld, (TUINT8 *) header, sizeof header))
		return TFALSE;
	if (!((sscanf(header, "P6\n%d %d\n%d\n", &tw, &th, &maxv) == 3 ||
		sscanf(header, "P6\n#%*80[^\n]\n%d %d\n%d\n", &tw, &th, &maxv) == 3) &&
		maxv > 0 && maxv < 256))
		return TFALSE;
	TDBPRINTF(TDB_TRACE,("identified PPM w=%d h=%d\n", tw, th));
	long offs = (*ld->iml_SeekFunc)(ld, 0, SEEK_END);
	if (offs == -1)
		return TFALSE;
	offs -= 3 * tw * th;
	if (offs <= 0 || (*ld->iml_SeekFunc)(ld, offs, SEEK_SET) == -1)
		return TFALSE;
	size_t len = tw * th * sizeof(TUINT);
	TUINT *dstbuf = TAlloc(TNULL, len + tw * 3);
	if (dstbuf == TNULL)
		return TFALSE;
	ld->iml_Image.tpb_Data = (TUINT8 *) dstbuf;
	for (y = 0; y < th; ++y)
	{
		TUINT8 r, g, b;
		char *srcbuf = (char *) ld->iml_Image.tpb_Data + len;
		if (!(*ld->iml_ReadFunc)(ld, (TUINT8 *) srcbuf, tw * 3))
		{
			TFree(ld->iml_Image.tpb_Data);
			return TFALSE;
		}
		for (x = 0; x < tw; ++x)
		{
			r = *srcbuf++;
			g = *srcbuf++;
			b = *srcbuf++;
			*dstbuf++ = (r << 16) | (g << 8) | b;
		}
	}
	ld->iml_Image.tpb_Format = TVPIXFMT_08R8G8B8;
	ld->iml_Image.tpb_BytesPerLine = tw * 4;
	ld->iml_Width = tw;
	ld->iml_Height = th;
	/* TRealloc(ld->buf, len); */
	TDBPRINTF(TDB_TRACE,("successfully loaded PPM\n"));
	return TTRUE;
}


TLIBAPI TBOOL imgload_load(struct ImgLoader *ld)
{
#if defined(ENABLE_PNG)
	if (imgload_load_png(ld))
		return TTRUE;
#endif
	(*ld->iml_SeekFunc)(ld, 0, SEEK_SET);
	if (imgload_load_ppm(ld))
		return TTRUE;
	return TFALSE;
}


static TBOOL imgload_read_file(struct ImgLoader *ld, TUINT8 *buf, TSIZE nbytes)
{
	return fread(buf, nbytes, 1, ld->iml_Loader.File.fd) == 1;
}

static long imgload_seek_file(struct ImgLoader *ld, long offs, int whence)
{
	long res = fseek(ld->iml_Loader.File.fd, offs, whence);
	if (res != -1)
		res = ftell(ld->iml_Loader.File.fd);
	return res;
}

TLIBAPI TBOOL imgload_init_file(struct ImgLoader *ld, struct TExecBase *TExecBase,
	FILE *fd)
{
	memset(ld, 0, sizeof *ld);
	ld->iml_ExecBase = TExecBase;
	ld->iml_Loader.File.fd = fd;
	ld->iml_ReadFunc = imgload_read_file;
	ld->iml_SeekFunc = imgload_seek_file;
	return TTRUE;
}

static TBOOL imgload_read_mem(struct ImgLoader *ld, TUINT8 *buf, TSIZE nbytes)
{
	nbytes = TMIN(ld->iml_Loader.Memory.left, nbytes);
	memcpy(buf, ld->iml_Loader.Memory.src + ld->iml_Loader.Memory.len - 
		ld->iml_Loader.Memory.left, nbytes);
	ld->iml_Loader.Memory.left -= nbytes;
	return TTRUE;
}

static long imgload_seek_mem(struct ImgLoader *ld, long offs, int whence)
{
	switch (whence)
	{
		default:
			return -1;
		case SEEK_SET:
			ld->iml_Loader.Memory.left = ld->iml_Loader.Memory.len - offs;
			break;
		case SEEK_CUR:
			ld->iml_Loader.Memory.left -= offs;
			break;
		case SEEK_END:
			ld->iml_Loader.Memory.left = offs;
			break;
	}
	return ld->iml_Loader.Memory.len - ld->iml_Loader.Memory.left;
}

TLIBAPI TBOOL imgload_init_memory(struct ImgLoader *ld, struct TExecBase *TExecBase,
	const char *src, size_t len)
{
	memset(ld, 0, sizeof *ld);
	ld->iml_ExecBase = TExecBase;
	ld->iml_Loader.Memory.src = src;
	ld->iml_Loader.Memory.len = len;
	ld->iml_Loader.Memory.left = len;
	ld->iml_ReadFunc = imgload_read_mem;
	ld->iml_SeekFunc = imgload_seek_mem;
	return TTRUE;
}
