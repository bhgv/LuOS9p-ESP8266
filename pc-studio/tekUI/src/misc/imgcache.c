#ifndef _TEK_LIB_IMGCACHE_C
#define _TEK_LIB_IMGCACHE_C

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/lib/imgcache.h>

static THOOKENTRY TTAG destroy_cachenode(struct THook *hook, TAPTR obj, 
	TTAG msg)
{
	if (msg != TMSG_DESTROY)
		return 0;
	struct ImageCacheNode *cn = obj;
	struct THandle *cache = cn->handle.thn_Owner;
	struct CacheManagerIFace *iface = cn->crec->iface;
	TRemove(&cn->handle.thn_Node);
	iface->remitem(cache, &cn->item);
	iface->free(cache, cn);
	return 0;
}

static THOOKENTRY TTAG destroy_cacherecord(struct THook *hook, TAPTR obj,
	TTAG msg)
{
	if (msg != TMSG_DESTROY)
		return 0;
	struct ImageCacheRecord *cr = obj;
	struct THandle *cache = cr->handle.thn_Owner;
	struct TNode *next, *node = cr->list.tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
		TDestroy(&((struct ImageCacheNode *) node)->handle);
	cr->iface->free(cache, cr);
	return 0;
}

TLIBAPI TINT imgcache_lookup(struct ImageCacheState *cs, struct TVImageCacheRequest *creq, 
	TINT x, TINT y, TINT w, TINT h)
{
	cs->x0 = x - creq->tvc_OrigX;
	cs->y0 = y - creq->tvc_OrigY;
	cs->w = w;
	cs->h = h;
	cs->x1 = cs->x0 + w - 1;
	cs->y1 = cs->y0 + h - 1;
	
	struct THandle *cache = creq->tvc_CacheManager;
 	struct CacheManagerIFace *iface = cs->cacheiface = (struct CacheManagerIFace *)
 		TCallHookPkt(&cache->thn_Hook, cache, CacheManagerMsgQueryIFace);
	cs->hashvalue = iface->hash(cache, creq->tvc_Key, creq->tvc_KeyLen);
	cs->cr = iface->get(cache, creq->tvc_Key, creq->tvc_KeyLen, 
		cs->hashvalue);
	if (cs->cr)
	{
		struct TNode *next, *node = cs->cr->list.tlh_Head.tln_Succ;
		for (; (next = node->tln_Succ); node = next)
		{
			struct ImageCacheNode *cn = (struct ImageCacheNode *) node;
			if (cs->x0 >= cn->x0 && cs->x1 <= cn->x1 && cs->y0 >= cn->y0
				&& cs->y1 <= cn->y1)
			{
				int cw = cn->x1 - cn->x0 + 1;
				/*int ch = cn->y1 - cn->y0 + 1;*/
				TINT bpp = TVPIXFMT_BYTES_PER_PIXEL(cs->dst.tpb_Format);
				cs->dst.tpb_Data = cn->buf + 
					bpp * (cs->x0 - cn->x0 + (cs->y0 - cn->y0) * cw);
				cs->dst.tpb_BytesPerLine = cw * bpp;
				TNodeUp(&cn->item.node); /* bubble up */
				/*TDBPRINTF(TDB_INFO,("pixcache: found %dx%d fmt=%08x\n",
					cw, ch, cs->dst.fmt));*/
				return creq->tvc_Result = TVIMGCACHE_FOUND;
			}
		}
	}
	return creq->tvc_Result = TVIMGCACHE_NOTFOUND;
}

TLIBAPI TINT imgcache_store(struct ImageCacheState *cs, struct TVImageCacheRequest *creq)
{
	struct THandle *cache = creq->tvc_CacheManager;
	struct CacheManagerIFace *iface = cs->cacheiface;
	struct ImageCacheRecord *cr = cs->cr;
	if (!cr)
	{
		cr = cs->cr = iface->alloc(cache, sizeof(struct ImageCacheRecord));
		if (!cr)
			return creq->tvc_Result = TVIMGCACHE_STORE_FAILED;
		TInitList(&cr->list);
		cr->handle.thn_Owner = cache;
		cr->numitems = 0;
		cr->iface = iface;
		TInitHook(&cr->handle.thn_Hook, destroy_cacherecord, cr);
		iface->put(cache, creq->tvc_Key, creq->tvc_KeyLen,
			cs->hashvalue, &cr->handle);
	}
	
	if (cr->numitems >= 64)
		return creq->tvc_Result = TVIMGCACHE_STORE_FAILED;
	
	TUINT numpixels = cs->w * cs->h;
	TINT bpp = TVPIXFMT_BYTES_PER_PIXEL(cs->dst.tpb_Format);
	struct ImageCacheNode *cn = iface->alloc(cache, 
		sizeof(struct ImageCacheNode) + numpixels * bpp);
	if (!cn)
		return creq->tvc_Result = TVIMGCACHE_STORE_FAILED;
	cn->buf = (TUINT8 *) (cn + 1);
	cn->pixels = numpixels;
	cn->crec = cr;
	cn->x0 = cs->x0;
	cn->y0 = cs->y0;
	cn->x1 = cs->x1;
	cn->y1 = cs->y1;
	cs->dst.tpb_Data = (TUINT8 *) (cn + 1);
	cs->dst.tpb_BytesPerLine = cs->w * bpp;
	cs->convert(&cs->src, &cs->dst, 0, 0, cs->w - 1, cs->h - 1, 0, 0, 0, 0);
	struct TNode *next, *node = cr->list.tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
	{
		struct ImageCacheNode *cn = (struct ImageCacheNode *) node;
		if (cs->x0 <= cn->x0 && cs->x1 >= cn->x1 && 
			cs->y0 <= cn->y0 && cs->y1 >= cn->y1)
			TDestroy(&cn->handle);
	}
	cn->handle.thn_Owner = cache;
	TInitHook(&cn->handle.thn_Hook, destroy_cachenode, cn);
	cn->item.handle = &cn->handle; /* backptr */
	iface->additem(cache, &cn->item);
	TAddHead(&cr->list, &cn->handle.thn_Node);
	cr->numitems++;
	TDBPRINTF(TDB_INFO,("pixcache: stored %dx%d fmt=%08x\n",
		cs->w, cs->h, cs->dst.tpb_Format));
	return creq->tvc_Result = TVIMGCACHE_STORED;
}

#endif
