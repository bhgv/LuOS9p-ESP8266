#ifndef _TEK_LIB_CACHEMANAGER_C
#define _TEK_LIB_CACHEMANAGER_C

#include <assert.h>
#include <string.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/lib/cachemanager.h>

struct Hash 
{
	struct THandle handle;
	struct TMemManager *memmgr;
	struct CacheManagerIFace iface;
	struct HashNode **buckets;
	TINT nument, numbuckets, primeidx;
	TSIZE allocbytes, maxbytes;
	struct TList items;
	TINT numitems;
};

struct HashNode 
{
	struct HashNode *next;
	struct THandle *value;
	TUINT len;
	TUINT hash;
};

/* Lua's string hashing function, see lstring.c */
static TUINT cm_hash(struct THandle *hnd, TUINT8 *str, TSIZE l)
{
	TUINT h = (TUINT) l; /* seed */
	TSIZE step = (l >> 5) + 1;
	TSIZE l1;
	for (l1 = l; l1 >= step; l1 -= step)
		h = h ^ ((h << 5) + (h >> 2) + ((TUINT8) str[l1 - 1]));
	return h;
}

static const int cm_primes[] = { 7, 17, 31, 61, 127, 251, 509, 1021, 2039, 4093 };
static const int cm_numprimes = sizeof(cm_primes) / sizeof(cm_primes[0]);

static struct HashNode **cm_lookup(struct Hash *hash, TUINT8 *key,
	TSIZE len, TUINT hval)
{
	struct HashNode **bucket = &hash->buckets[hval % hash->numbuckets];
	for (; *bucket; bucket = &(*bucket)->next)
		if ((*bucket)->len == len && 
			memcmp((char *) (*bucket + 1), key, len) == 0) break;
	return bucket;
}

static TAPTR cm_get(struct THandle *hnd, TUINT8 *key, TSIZE len, TUINT hval)
{
	struct Hash *hash = (struct Hash *) hnd;
	struct HashNode *node = *cm_lookup(hash, key, len, hval);
	if (node)
		return node->value;
	return TNULL;
}

static void cm_resize(struct Hash *hash) 
{
	struct HashNode **newbuckets, *next, *node;
	int numbuckets = hash->numbuckets, newnumbuckets;
	int load = hash->nument / numbuckets;
	int i, pi = hash->primeidx;
	if (numbuckets < cm_primes[cm_numprimes - 1] && load >= 3)
		for (; pi < cm_numprimes - 1 && cm_primes[pi + 1] < hash->nument; ++pi);
	if (pi == hash->primeidx) 
		return;
	newnumbuckets = cm_primes[pi];
	TAPTR TExecBase = TGetExecBase(hash);
	newbuckets = TAlloc0(hash->memmgr, sizeof(struct HashNode) * newnumbuckets);
	if (!newbuckets) 
		return;
	for (i = 0; i < hash->numbuckets; ++i)
	{
		for (node = hash->buckets[i]; node; node = next) 
		{
			int hashidx = node->hash % newnumbuckets;
			next = node->next;
			node->next = newbuckets[hashidx];
			newbuckets[hashidx] = node;
		}
	}
	TFree(hash->buckets);
	hash->buckets = newbuckets;
	hash->numbuckets = newnumbuckets;
	hash->primeidx = pi;
}

static TBOOL cm_put(struct THandle *hnd, TUINT8 *key, TSIZE len, TUINT hval, 
	struct THandle *value)
{
	struct Hash *hash = (struct Hash *) hnd;
	char *newkey;
	struct HashNode *newnode, **bucket = cm_lookup(hash, key, len, hval);
	if (*bucket) 
	{
		TDBPRINTF(TDB_WARN,("Overwrite cache node\n"));
		(*bucket)->value = value;
		return TTRUE;
	}
	TAPTR TExecBase = TGetExecBase(hash);
	newnode = TAlloc(hash->memmgr, sizeof(struct HashNode) + len);
	if (!newnode) return 0;
	newkey = (char *) (newnode + 1);
	memcpy(newkey, key, len);
	newnode->next = NULL;
	newnode->len = len;
	newnode->hash = hval;
	newnode->value = value;
	*bucket = newnode;
	hash->nument++;
	TDBPRINTF(TDB_INFO,("Cache records: %d\n", hash->nument));
	cm_resize(hash);
	return TFALSE;
}

static TAPTR cm_alloc(struct THandle *hnd, TSIZE size)
{
	struct Hash *hash = (struct Hash *) hnd;
	if (hash->maxbytes > 0)
	{
		while (hash->allocbytes + size > hash->maxbytes)
		{
			struct CacheItem *item = (struct CacheItem *) TLASTNODE(&hash->items);
			if (!item)
				break;
			TDestroy(item->handle);
		}
	}
	TAPTR TExecBase = TGetExecBase(hash);
	TAPTR mem = TAlloc(hash->memmgr, size);
	if (mem)
		hash->allocbytes += size;
	return mem;
}

static void cm_free(struct THandle *hnd, TAPTR mem)
{
	struct Hash *hash = (struct Hash *) hnd;
	TAPTR TExecBase = TGetExecBase(hash);
	if (mem)
		hash->allocbytes -= TGetSize(mem);
	TFree(mem);
}

static void cm_additem(struct THandle *hnd, struct CacheItem *item)
{
	struct Hash *hash = (struct Hash *) hnd;
	TAddTail(&hash->items, &item->node);
	hash->numitems++;
	/*fprintf(stderr, "allocated: %ld - items: %d\n", hash->allocbytes, hash->numitems);*/
}

static void cm_remitem(struct THandle *hnd, struct CacheItem *item)
{
	struct Hash *hash = (struct Hash *) hnd;
	TRemove(&item->node);
	hash->numitems--;
}

static THOOKENTRY TTAG cm_msg(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct Hash *hash = obj;
	if (msg == TMSG_DESTROY)
	{
		int i;
		TAPTR TExecBase = hash->handle.thn_Owner;
		struct HashNode *node, *next;
		for (i = 0; i < hash->numbuckets; ++i)
			for (next = hash->buckets[i]; (node = next); 
				next = node->next, TDestroy(node->value), TFree(node));
		TFree(hash->buckets);
		assert(hash->allocbytes == 0);
		TDestroy((struct THandle *) hash->memmgr);
		assert(hash->numitems == 0);
		TFree(hash);
	}
	else if (msg == CacheManagerMsgQueryIFace)
		return (TTAG) &hash->iface;
	return 0;
}

TLIBAPI struct THandle *cachemanager_create(TAPTR TExecBase)
{
	struct TMemManager *mmgr = TNULL; /*TCreateMemManager(TNULL, TMMT_Tracking, TNULL);*/
	struct Hash *hash = TAlloc0(mmgr, sizeof(struct Hash));
	if (hash)
	{
		hash->buckets = TAlloc0(mmgr, sizeof(struct HashNode) * cm_primes[0]);
		if (hash->buckets)
		{
			hash->handle.thn_Owner = TExecBase;
			TInitHook(&hash->handle.thn_Hook, cm_msg, hash);
			hash->memmgr = mmgr;
			hash->numbuckets = cm_primes[0];
			hash->iface.hash = cm_hash;
			hash->iface.put = cm_put;
			hash->iface.get = cm_get;
			hash->iface.alloc = cm_alloc;
			hash->iface.free = cm_free;
			hash->iface.additem = cm_additem;
			hash->iface.remitem = cm_remitem;
			hash->allocbytes = 0;
			hash->maxbytes = 1000000;
			hash->numitems = 0;
			TInitList(&hash->items);
			return &hash->handle;
		}
		TFree(hash);
	}
	return TNULL;
}

#endif
