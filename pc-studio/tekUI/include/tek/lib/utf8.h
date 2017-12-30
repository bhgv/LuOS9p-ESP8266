#ifndef _TEK_LIB_UTF8_H
#define _TEK_LIB_UTF8_H

/*
**	utf8.c - UTF-8 library
**	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
**	Placed in the Public Domain
*/

#include <stddef.h>
#include <tek/exec.h>

struct utf8readstringdata
{
	const unsigned char *src;
	size_t srclen;
};

struct utf8reader
{
	/* character reader callback: */
	int (*readchar)(struct utf8reader *);
	/* reader state: */
	int accu, numa, min, bufc;
	/* userdata to reader */
	void *udata;
	struct utf8readstringdata rsdata;
};

TLIBAPI int utf8read(struct utf8reader *rd);
TLIBAPI void utf8initreader(struct utf8reader *rd, 
	const unsigned char *utf8text, size_t bytelen);
TLIBAPI unsigned char *utf8encode(unsigned char *buf, int c);
TLIBAPI int utf8readstring(struct utf8reader *rd);
TLIBAPI size_t utf8tolatin(const unsigned char *utf8, size_t utf8len,
	unsigned char *latin, size_t latinbuflen, unsigned char repl);
TLIBAPI size_t utf8getlen(const unsigned char *raws, size_t rawlen);

#endif /* _TEK_LIB_UTF8_H */
