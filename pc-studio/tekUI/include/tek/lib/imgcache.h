#ifndef _TEK_LIB_IMGCACHE_H
#define _TEK_LIB_IMGCACHE_H

/*
**	imgcache.h - Image cache
**	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/lib/cachemanager.h>
#include <tek/lib/pixconv.h>

struct ImageCacheRecord
{
	struct THandle handle; /* linkage to cache manager */
	struct TList list; /* list of rectangle nodes */
	struct CacheManagerIFace *iface;
	TINT numitems;
};

struct ImageCacheNode
{
	struct THandle handle; /* linkage to cache record */
	struct CacheItem item; /* linkage to cache manager */
	struct ImageCacheRecord *crec;
	TINT x0, y0, x1, y1; /* rectangle normalized by orig x/y */
	TUINT8 *buf; /* cached buffer */
	TUINT pixels; /* number of pixels */
};

struct ImageCacheState
{
	struct ImageCacheRecord *cr;
	struct CacheManagerIFace *cacheiface;
	TUINT hashvalue;
	struct TVPixBuf src, dst;
	TINT x0, y0, x1, y1, w, h;
	TINT (*convert)(struct TVPixBuf *src, struct TVPixBuf *dst, 
		TINT x0, TINT y0, TINT x1, TINT y1, TINT sx, TINT sy, TBOOL alpha, 
		TBOOL swap_byteorder);
};

TLIBAPI TINT imgcache_lookup(struct ImageCacheState *cs, struct TVImageCacheRequest *creq, TINT x, TINT y, TINT w, TINT h);
TLIBAPI TINT imgcache_store(struct ImageCacheState *cs, struct TVImageCacheRequest *creq);

#endif /* _TEK_LIB_IMGCACHE_H */
