#ifndef _TEK_LIB_IMGLOAD_H
#define _TEK_LIB_IMGLOAD_H

#include <stdio.h>
#include <tek/exec.h>
#include <tek/teklib.h>
#include <tek/mod/visual.h>

struct ImgMemLoader
{
	const char *src;
	size_t len;
	size_t left;
};

struct ImgFileLoader
{
	FILE *fd;
};

#define IMLFL_HAS_ALPHA	0x0001

struct ImgLoader
{
	struct TExecBase *iml_ExecBase;
	struct TVPixBuf iml_Image;
	TUINT iml_Width;
	TUINT iml_Height;
	TUINT iml_Flags;
	TBOOL (*iml_ReadFunc)(struct ImgLoader *ld, TUINT8 *buf, TSIZE nbytes);
	long (*iml_SeekFunc)(struct ImgLoader *ld, long offs, int whence);
	union 
	{
		struct ImgMemLoader Memory;
		struct ImgFileLoader File;
	} iml_Loader;
};

TLIBAPI TBOOL imgload_init_file(struct ImgLoader *ld, struct TExecBase *TExecBase, 
	FILE *fd);
TLIBAPI TBOOL imgload_init_memory(struct ImgLoader *ld, struct TExecBase *TExecBase,
	const char *src, size_t len);
TLIBAPI TBOOL imgload_load(struct ImgLoader *ld);

#endif /* _TEK_LIB_IMGLOAD_H */
