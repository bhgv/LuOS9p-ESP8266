#ifndef _TEK_LIB_PIXCONV_H
#define _TEK_LIB_PIXCONV_H

/*
**	pixconv.h - Pixel array conversion
**	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/visual.h>

TLIBAPI TINT pixconv_convert(struct TVPixBuf *src, struct TVPixBuf *dst,
	TINT x0, TINT y0, TINT x1, TINT y1, TINT sx, TINT sy, TBOOL alpha, 
	TBOOL swap_byteorder);

TLIBAPI void pixconv_writealpha(TUINT8 *dbuf, TUINT8 *sbuf, TINT w, TUINT dfmt, 
	TINT r, TINT g, TINT b);

TLIBAPI void pixconv_line_set(TUINT8 *dbuf, TUINT dfmt, TINT w, TUINT p);
TLIBAPI void pixconv_buf_line_set(struct TVPixBuf *d, TINT x, TINT y, TINT w, TUINT p);
TLIBAPI TUINT pixconv_rgbfmt(TUINT dfmt, TUINT rgb);
TLIBAPI void pixconv_setpixel(TUINT8 *dbuf, TUINT dfmt, TUINT p);
TLIBAPI TUINT pixconv_getpixel(TUINT8 *buf, TUINT fmt);

#define pixconv_setpixelbuf(d, x, y, p) \
	pixconv_setpixel(TVPB_GETADDRESS(d, x, y), (d)->tpb_Format, p)

#define pixconv_getpixelbuf(d, x, y) \
	pixconv_getpixel(TVPB_GETADDRESS(d, x, y), (d)->tpb_Format)

#endif /* _TEK_LIB_PIXCONV_H */
