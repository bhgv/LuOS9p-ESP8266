#ifndef _TEK_LIB_CACHEMANAGER_H
#define _TEK_LIB_CACHEMANAGER_H

/*
**	cachemanager.h - Cache manager
**	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>

/* msg to query interface from a cache manager: */
#define CacheManagerMsgQueryIFace	(TMSG_USER + 0)

struct CacheItem
{
	struct TNode node; /* linkage to cachemanager */
	struct THandle *handle;	/* points to some kind of cache record */
};

struct CacheManagerIFace
{
	TUINT (*hash)(struct THandle *, TUINT8 *s, TSIZE len);
	TBOOL (*put)(struct THandle *, TUINT8 *key, TSIZE len, TUINT hval, 
		struct THandle *value);
	TAPTR (*get)(struct THandle *, TUINT8 *key, TSIZE len, TUINT hval);
	TAPTR (*alloc)(struct THandle *, TSIZE size);
	void (*free)(struct THandle *, TAPTR mem);
	void (*additem)(struct THandle *, struct CacheItem *item);
	void (*remitem)(struct THandle *, struct CacheItem *item);
};

TLIBAPI struct THandle *cachemanager_create(TAPTR TExecBase);

#endif /* _TEK_LIB_CACHEMANAGER_H */
