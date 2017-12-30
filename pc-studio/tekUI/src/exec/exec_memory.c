
/*
**	$Id: exec_memory.c,v 1.6 2006/11/11 14:19:09 tmueller Exp $
**	teklib/src/exec/exec_memory.c - Recursive memory management
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include "exec_mod.h"

static const union TMemMsg msg_destroy = { TMMSG_DESTROY };

/*****************************************************************************/
/*
**	mmu = exec_CreateMemManager(exec, allocator, mmutype, tags)
**	Create a memory manager
*/

static THOOKENTRY TTAG
exec_destroymmu(struct THook *hook, TAPTR obj, TTAG msg)
{
	if (msg == TMSG_DESTROY)
	{
		struct TMemManager *mmu = obj;
		struct TExecBase *TExecBase = TGetExecBase(mmu);
		TCALLHOOKPKT(&mmu->tmm_Hook, mmu, (TTAG) &msg_destroy);
		TFree(mmu);
	}
	return 0;
}

static THOOKENTRY TTAG
exec_destroymmu_and_allocator(struct THook *hook, TAPTR obj, TTAG msg)
{
	if (msg == TMSG_DESTROY)
	{
		struct TMemManager *mmu = obj;
		struct TExecBase *TExecBase = TGetExecBase(mmu);
		TCALLHOOKPKT(&mmu->tmm_Hook, mmu, (TTAG) &msg_destroy);
		TDESTROY(mmu->tmm_Allocator);
		TFree(mmu);
	}
	return 0;
}

static THOOKENTRY TTAG
exec_destroymmu_and_free(struct THook *hook, TAPTR obj, TTAG msg)
{
	if (msg == TMSG_DESTROY)
	{
		struct TMemManager *mmu = obj;
		struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
		TCALLHOOKPKT(&mmu->tmm_Hook, mmu, (TTAG) &msg_destroy);
		TFree(mmu->tmm_Allocator);
		TFree(mmu);
	}
	return 0;
}

EXPORT struct TMemManager *
exec_CreateMemManager(struct TExecBase *TExecBase, TAPTR allocator,
	TUINT mmutype, struct TTagItem *tags)
{
	struct TMemManager *mmu = TAlloc(TNULL, sizeof(struct TMemManager));
	if (mmu)
	{
		THOOKENTRY THOOKFUNC destructor = TNULL;
		TBOOL success = TTRUE;
		TAPTR staticmem = TNULL;

		if ((mmutype & TMMT_Pooled) && allocator == TNULL)
		{
			/* Create a MM based on an internal pooled allocator */
			allocator = TCreatePool(tags);
			destructor = exec_destroymmu_and_allocator;
		}
		else if (mmutype & TMMT_Static)
		{
			/* Create a MM based on a static memory block */
			TUINT blocksize = (TUINT) TGetTag(tags, TMem_StaticSize, 0);
			success = TFALSE;
			if (blocksize > sizeof(union TMemHead))
			{
				if (allocator == TNULL)
				{
					allocator = staticmem = TAlloc(TNULL, blocksize);
					if (allocator)
					{
						destructor = exec_destroymmu_and_free;
						success = TTRUE;
					}
				}
				else
				{
					destructor = exec_destroymmu;
					success = TTRUE;
				}

				if (success)
				{
					TUINT flags = TGetTag(tags, TMem_LowFrag, TFALSE) ?
						TMEMHF_LOWFRAG : TMEMHF_NONE;
					TAPTR block = (TAPTR)
						(((union TMemHead *) allocator) + 1);
					blocksize -= sizeof(union TMemHead);
					success = exec_initmemhead(allocator, block, blocksize,
						flags, 0);
					if (!success)
						TDBPRINTF(TDB_FAIL,("static allocator failed\n"));
				}
			}
		}
		else
			destructor = exec_destroymmu;

		if (success)
		{
			if (exec_initmm(TExecBase, mmu, allocator, mmutype, tags))
			{
				/* Overwrite destructor. The one provided by exec_initmm()
				doesn't know how to free the memory manager. */
				mmu->tmm_Handle.thn_Hook.thk_Entry = destructor;
				return mmu;
			}
		}

		TFree(staticmem);
		TFree(mmu);
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	mem = exec_Alloc(exec, mmu, size)
**	Allocate memory via memory manager
*/

EXPORT TAPTR
exec_Alloc(struct TExecBase *TExecBase, struct TMemManager *mmu, TSIZE size)
{
	union TMemManagerInfo *mem = TNULL;
	if (size)
	{
		union TMemMsg msg;
		msg.tmmsg_Type = TMMSG_ALLOC;
		msg.tmmsg_Alloc.tmmsg_Size = size + sizeof(union TMemManagerInfo);

		if (mmu == TNULL)
			mmu = &TExecBase->texb_BaseMemManager;

		mem = (union TMemManagerInfo *) TCALLHOOKPKT(&mmu->tmm_Hook, mmu,
			(TTAG) &msg);
		if (mem)
		{
			mem->tmu_Node.tmu_UserSize = size;
			mem->tmu_Node.tmu_MemManager = mmu;
			mem++;
		}
		else
			TDBPRINTF(TDB_INFO,("alloc failed size %d\n", size));
	}
	else
		TDBPRINTF(TDB_WARN,("called with size=0\n"));

	return (TAPTR) mem;
}

/*****************************************************************************/
/*
**	mem = exec_Alloc0(exec, mmu, size)
**	Allocate memory via memory manager, zero'ed out
*/

EXPORT TAPTR exec_Alloc0(struct TExecBase *TExecBase,
	struct TMemManager *mmu, TSIZE size)
{
	TAPTR mem = TAlloc(mmu, size);
	if (mem) TFillMem(mem, size, 0);
	return mem;
}

/*****************************************************************************/
/*
**	mem = exec_Realloc(exec, mem, newsize)
**	Reallocate an existing memory manager allocation. Note that it is not
**	possible to allocate a fresh block of memory with this function.
*/

EXPORT TAPTR exec_Realloc(struct TExecBase *TExecBase, TAPTR mem,
	TSIZE newsize)
{
	union TMemManagerInfo *newmem = TNULL;
	if (mem)
	{
		union TMemManagerInfo *mmuinfo = (union TMemManagerInfo *) mem - 1;
		struct TMemManager *mmu = mmuinfo->tmu_Node.tmu_MemManager;
		union TMemMsg msg;

		if (mmu == TNULL)
			mmu = &TExecBase->texb_BaseMemManager;

		if (newsize)
		{
			TSIZE oldsize = mmuinfo->tmu_Node.tmu_UserSize;
			if (oldsize == newsize)
				return mem;

			msg.tmmsg_Type = TMMSG_REALLOC;
			msg.tmmsg_Realloc.tmmsg_Ptr = mmuinfo;
			msg.tmmsg_Realloc.tmmsg_OSize = oldsize + sizeof(union TMemManagerInfo);
			msg.tmmsg_Realloc.tmmsg_NSize = newsize + sizeof(union TMemManagerInfo);

			newmem = (union TMemManagerInfo *)
				TCALLHOOKPKT(&mmu->tmm_Hook, mmu, (TTAG) &msg);

			if (newmem)
			{
				newmem->tmu_Node.tmu_UserSize = newsize;
				newmem++;
			}
		}
		else
		{
			msg.tmmsg_Type = TMMSG_FREE;
			msg.tmmsg_Free.tmmsg_Ptr = mmuinfo;
			msg.tmmsg_Free.tmmsg_Size =
				mmuinfo->tmu_Node.tmu_UserSize + sizeof(union TMemManagerInfo);
			TCALLHOOKPKT(&mmu->tmm_Hook, mmu, (TTAG) &msg);
		}
	}
	return newmem;
}

/*****************************************************************************/
/*
**	exec_Free(exec, mem)
**	Return memory allocated from a memory manager
*/

EXPORT void
exec_Free(TEXECBASE *exec, TAPTR mem)
{
	if (mem)
	{
		union TMemManagerInfo *mmuinfo = (union TMemManagerInfo *) mem - 1;
		struct TMemManager *mmu = mmuinfo->tmu_Node.tmu_MemManager;
		union TMemMsg msg;
		if (mmu == TNULL)
			mmu = &exec->texb_BaseMemManager;
		msg.tmmsg_Type = TMMSG_FREE;
		msg.tmmsg_Free.tmmsg_Ptr = mmuinfo;
		msg.tmmsg_Free.tmmsg_Size =
			mmuinfo->tmu_Node.tmu_UserSize + sizeof(union TMemManagerInfo);
		TCALLHOOKPKT(&mmu->tmm_Hook, mmu, (TTAG) &msg);
	}
}

/*****************************************************************************/
/*
**	success = exec_initmemhead(mh, mem, size, flags, bytealign)
**	Init memheader
*/

LOCAL TBOOL
exec_initmemhead(union TMemHead *mh, TAPTR mem, TSIZE size, TUINT flags,
	TUINT bytealign)
{
	TUINT align = sizeof(TAPTR);

	bytealign = TMAX(bytealign, sizeof(union TMemNode));
	while (align < bytealign) align <<= 1;
	if (size >= align)
	{
		--align;
		size &= ~align;

		mh->tmh_Node.tmh_Mem = mem;
		mh->tmh_Node.tmh_MemEnd = ((TINT8 *) mem) + size;
		mh->tmh_Node.tmh_Free = size;
		mh->tmh_Node.tmh_Align = align;
		mh->tmh_Node.tmh_Flags = flags;

		mh->tmh_Node.tmh_FreeList = (union TMemNode *) mem;
		((union TMemNode *) mem)->tmn_Node.tmn_Next = TNULL;
		((union TMemNode *) mem)->tmn_Node.tmn_Size = size;

		return TTRUE;
	}

	return TFALSE;
}

/*****************************************************************************/
/*
**	mem = exec_staticalloc(memhead, size)
**	Allocate from a static memheader
*/

LOCAL TAPTR
exec_staticalloc(union TMemHead *mh, TSIZE size)
{
	size = (size + mh->tmh_Node.tmh_Align) & ~mh->tmh_Node.tmh_Align;

	if (mh->tmh_Node.tmh_Free >= size)
	{
		union TMemNode **mnp, *mn, **x;

		mnp = &mh->tmh_Node.tmh_FreeList;
		x = TNULL;

		if (mh->tmh_Node.tmh_Flags & TMEMHF_LOWFRAG)
		{
			/* bestfit strategy */

			TUINT bestsize = 0xffffffff;
			while ((mn = *mnp))
			{
				if (mn->tmn_Node.tmn_Size == size)
				{
exactfit:			*mnp = mn->tmn_Node.tmn_Next;
					mh->tmh_Node.tmh_Free -= size;
					return (TAPTR) mn;
				}
				else if (mn->tmn_Node.tmn_Size > size)
				{
					if (mn->tmn_Node.tmn_Size < bestsize)
					{
						bestsize = mn->tmn_Node.tmn_Size;
						x = mnp;
					}
				}
				mnp = &mn->tmn_Node.tmn_Next;
			}
		}
		else
		{
			/* firstfit strategy */

			while ((mn = *mnp))
			{
				if (mn->tmn_Node.tmn_Size == size)
				{
					goto exactfit;
				}
				else if (mn->tmn_Node.tmn_Size > size)
				{
					x = mnp;
					break;
				}
				mnp = &mn->tmn_Node.tmn_Next;
			}
		}

		if (x)
		{
			mn = *x;
			*x = (union TMemNode *) ((TINT8 *) mn + size);
			(*x)->tmn_Node.tmn_Next = mn->tmn_Node.tmn_Next;
			(*x)->tmn_Node.tmn_Size = mn->tmn_Node.tmn_Size - size;
			mh->tmh_Node.tmh_Free -= size;
			return (TAPTR) mn;
		}
	}

	return TNULL;
}

/*****************************************************************************/
/*
**	exec_staticfree(mh, mem, size)
**	Return memory to a static allocator
*/

LOCAL void
exec_staticfree(union TMemHead *mh, TAPTR mem, TSIZE size)
{
	union TMemNode **mnp, *mn, *pmn;

	size = (size + mh->tmh_Node.tmh_Align) & ~mh->tmh_Node.tmh_Align;
	mh->tmh_Node.tmh_Free += size;
	mnp = &mh->tmh_Node.tmh_FreeList;
	pmn = TNULL;

	while ((mn = *mnp))
	{
		if ((TINT8 *) mem < (TINT8 *) mn) break;
		pmn = mn;
		mnp = &mn->tmn_Node.tmn_Next;
	}

	if (mn && ((TINT8 *) mem + size == (TINT8 *) mn))
	{
		size += mn->tmn_Node.tmn_Size;
		/* concatenate with following free node */
		mn = mn->tmn_Node.tmn_Next;
	}

	if (pmn && ((TINT8 *) pmn + pmn->tmn_Node.tmn_Size == (TINT8 *) mem))
	{
		/* concatenate with previous free node */
		size += pmn->tmn_Node.tmn_Size;
		mem = pmn;
	}

	*mnp = (union TMemNode *) mem;
	((union TMemNode *) mem)->tmn_Node.tmn_Next = mn;
	((union TMemNode *) mem)->tmn_Node.tmn_Size = size;
}

/*****************************************************************************/
/*
**	newmem = exec_staticrealloc(exec, mh, oldmem, oldsize, newsize)
**	Realloc static memheader allocation
*/

LOCAL TAPTR exec_staticrealloc(struct TExecBase *TExecBase, union TMemHead *mh,
	TAPTR oldmem, TUINT oldsize, TUINT newsize)
{
	TAPTR newmem;
	union TMemNode **mnp, *mn, *mend;
	TUINT diffsize;

	oldsize = (oldsize + mh->tmh_Node.tmh_Align) & ~mh->tmh_Node.tmh_Align;
	newsize = (newsize + mh->tmh_Node.tmh_Align) & ~mh->tmh_Node.tmh_Align;

	if (newsize == oldsize)
		return oldmem;

	/* end of old allocation */
	mend = (union TMemNode *) (((TINT8 *) oldmem) + oldsize);
	mnp = &mh->tmh_Node.tmh_FreeList;

scan:

	mn = *mnp;
	if (mn == TNULL)
		goto notfound;
	if (mn < mend)
	{
		mnp = &mn->tmn_Node.tmn_Next;
		goto scan;
	}

	if (newsize > oldsize)
	{
		/* grow allocation */
		if (mn == mend)
		{
			/* there is a free node at end */
			diffsize = newsize - oldsize;
			if (mn->tmn_Node.tmn_Size == diffsize)
			{
				/* exact match: swallow free node */
				*mnp = mn->tmn_Node.tmn_Next;
				mh->tmh_Node.tmh_Free -= diffsize;
				return oldmem;
			}
			else if (mn->tmn_Node.tmn_Size > diffsize)
			{
				/* free node is larger: move free node */
				mend = (union TMemNode *) (((TINT8 *) mend) + diffsize);
				*mnp = mend;
				mend->tmn_Node.tmn_Next = mn->tmn_Node.tmn_Next;
				mend->tmn_Node.tmn_Size = mn->tmn_Node.tmn_Size - diffsize;
				mh->tmh_Node.tmh_Free -= diffsize;
				return oldmem;
			}
			/* else not enough space */
		}
		/* else no free node at end */
	}
	else
	{
		/* shrink allocation */
		diffsize = oldsize - newsize;
		if (mn == mend)
		{
			/* merge with following free node */
			mend = (union TMemNode *) (((TINT8 *) mend) - diffsize);
			*mnp = mend;
			mend->tmn_Node.tmn_Next = mn->tmn_Node.tmn_Next;
			mend->tmn_Node.tmn_Size = mn->tmn_Node.tmn_Size + diffsize;
		}
		else
		{
			/* add new free node */
			mend = (union TMemNode *) (((TINT8 *) mend) - diffsize);
			*mnp = mend;
			mend->tmn_Node.tmn_Next = mn;
			mend->tmn_Node.tmn_Size = diffsize;
		}
		mh->tmh_Node.tmh_Free += diffsize;
		return oldmem;
	}

notfound:

	newmem = exec_staticalloc(mh, newsize);
	if (newmem)
	{
		TCopyMem(oldmem, newmem, TMIN(oldsize, newsize));
		exec_staticfree(mh, oldmem, oldsize);
	}

	return newmem;
}

/*****************************************************************************/
/*
**	size = exec_GetSize(exec, allocation)
**	Get size of an allocation made from a memory manager
*/

EXPORT TSIZE
exec_GetSize(TEXECBASE *exec, TAPTR mem)
{
	return mem ? ((union TMemManagerInfo *) mem - 1)->tmu_Node.tmu_UserSize : 0;
}

/*****************************************************************************/
/*
**	mmu = exec_GetMemManager(exec, allocation)
**	Get the memory manager an allocation was made from
*/

EXPORT TAPTR
exec_GetMemManager(TEXECBASE *exec, TAPTR mem)
{
	return mem ? ((union TMemManagerInfo *) mem - 1)->tmu_Node.tmu_MemManager : TNULL;
}

/*****************************************************************************/
/*
**	pool = exec_CreatePool(exec, tags)
**	Create memory pool
*/

static THOOKENTRY TTAG
exec_destroypool(struct THook *hook, TAPTR obj, TTAG msg)
{
	if (msg == TMSG_DESTROY)
	{
		struct TMemPool *pool = obj;
		struct TExecBase *TExecBase = TGetExecBase(pool);
		union TMemHead *node = (union TMemHead *) pool->tpl_List.tlh_Head.tln_Succ;
		struct TNode *nnode;
		if (pool->tpl_Flags & TMEMHF_FREE)
		{
			while ((nnode = ((struct TNode *) node)->tln_Succ))
			{
				TFree(node);
				node = (union TMemHead *) nnode;
			}
		}
		TFree(pool);
	}
	return 0;
}

EXPORT TAPTR
exec_CreatePool(struct TExecBase *TExecBase, struct TTagItem *tags)
{
	TUINT fixedsize = TGetTag(tags, TPool_StaticSize, 0);
	if (fixedsize)
	{
		struct TMemPool *pool = TAlloc(TNULL, sizeof(struct TMemPool));
		if (pool)
		{
			union TMemHead *fixed = (TAPTR) TGetTag(tags, TPool_Static, TNULL);
			pool->tpl_Flags = TMEMHF_FIXED;
			if (fixed == TNULL)
			{
				TAPTR mmu = (TAPTR) TGetTag(tags, TPool_MemManager, TNULL);
				fixed = TAlloc(mmu, fixedsize);
				pool->tpl_Flags |= TMEMHF_FREE;
			}

			if (fixed)
			{
				pool->tpl_Align = sizeof(union TMemNode) - 1;
				pool->tpl_Flags |= ((TBOOL) TGetTag(tags, TMem_LowFrag,
					(TTAG) TFALSE)) ? TMEMHF_LOWFRAG : TMEMHF_NONE;

				pool->tpl_Handle.thn_Owner = (struct TModule *) TExecBase;
				pool->tpl_Handle.thn_Hook.thk_Entry = exec_destroypool;

				TINITLIST(&pool->tpl_List);
				exec_initmemhead(fixed, fixed + 1,
					fixedsize - sizeof(union TMemHead),
					pool->tpl_Flags, pool->tpl_Align + 1);
				TAddHead(&pool->tpl_List, (struct TNode *) fixed);
			}
			else
			{
				TFree(pool);
				pool = TNULL;
			}
		}

		return pool;
	}
	else
	{
		TUINT pudsize = (TUINT) TGetTag(tags, TPool_PudSize, (TTAG) 1024);
		TUINT thressize = (TUINT) TGetTag(tags, TPool_ThresSize, (TTAG) 256);
		if (pudsize >= thressize)
		{
			struct TMemPool *pool = TAlloc(TNULL, sizeof(struct TMemPool));
			if (pool)
			{
				pool->tpl_Align = sizeof(union TMemNode) - 1;
				pool->tpl_PudSize = pudsize;
				pool->tpl_ThresSize = thressize;

				pool->tpl_Flags = TMEMHF_FREE;
				pool->tpl_Flags |= ((TBOOL) TGetTag(tags,
					TPool_AutoAdapt, (TTAG) TTRUE)) ?
						TMEMHF_AUTO : TMEMHF_NONE;
				pool->tpl_Flags |= ((TBOOL) TGetTag(tags, TMem_LowFrag,
					(TTAG) TFALSE)) ? TMEMHF_LOWFRAG : TMEMHF_NONE;

				pool->tpl_Handle.thn_Owner = (struct TModule *) TExecBase;
				pool->tpl_Handle.thn_Hook.thk_Entry = exec_destroypool;

				pool->tpl_MemManager = (TAPTR) TGetTag(tags, TPool_MemManager, TNULL);

				TINITLIST(&pool->tpl_List);
			}

			return pool;
		}
	}

	return TNULL;
}

/*****************************************************************************/
/*
**	mem = exec_AllocPool(exec, pool, size)
**	Alloc memory from a pool
*/

EXPORT TAPTR exec_AllocPool(struct TExecBase *TExecBase, struct TMemPool *pool,
	TSIZE size)
{
	if (size && pool)
	{
		union TMemHead *node;

		if (pool->tpl_Flags & TMEMHF_FIXED)
		{
			node = (union TMemHead *) pool->tpl_List.tlh_Head.tln_Succ;
			return exec_staticalloc(node, size);
		}

		if (pool->tpl_Flags & TMEMHF_AUTO)
		{
			/* auto-adapt pool parameters */

			TINTPTR p, t;

			t = pool->tpl_ThresSize;
			/* tend to 4x typical size */
			t += (((TINTPTR) size) - (t >> 2)) >> 2;
			p = pool->tpl_PudSize;
			/* tend to 8x thressize */
			p += (t - (p >> 3)) >> 3;
			if (t > p) p = t;

			pool->tpl_ThresSize = t;
			pool->tpl_PudSize = p;
		}

		if (size <= ((pool->tpl_ThresSize + pool->tpl_Align) &
			~pool->tpl_Align))
		{
			/* regular pool allocation */

			TAPTR mem;
			struct TNode *tempnode;
			TSIZE pudsize;

			node = (union TMemHead *) pool->tpl_List.tlh_Head.tln_Succ;
			while ((tempnode = ((struct TNode *) node)->tln_Succ))
			{
				if (node->tmh_Node.tmh_Flags & TMEMHF_LARGE)
					break;
				mem = exec_staticalloc(node, size);
				if (mem)
					return mem;
				node = (union TMemHead *) tempnode;
			}

			pudsize = (pool->tpl_PudSize + pool->tpl_Align) & ~pool->tpl_Align;
			node = TAlloc(pool->tpl_MemManager, sizeof(union TMemHead) + pudsize);
			if (node)
			{
				exec_initmemhead(node, node + 1, pudsize, pool->tpl_Flags,
					pool->tpl_Align + 1);
				TAddHead(&pool->tpl_List, (struct TNode *) node);
				return exec_staticalloc(node, size);
			}
		}
		else
		{
			/* large allocation */

			size = (size + pool->tpl_Align) & ~pool->tpl_Align;
			node = TAlloc(pool->tpl_MemManager, sizeof(union TMemHead) + size);
			if (node)
			{
				exec_initmemhead(node, node + 1, size,
					pool->tpl_Flags | TMEMHF_LARGE, pool->tpl_Align + 1);
				TAddTail(&pool->tpl_List, (struct TNode *) node);
				return exec_staticalloc(node, size);
			}
		}
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	exec_FreePool(exec, pool, mem, size)
**	Return memory to a pool
*/

EXPORT void exec_FreePool(struct TExecBase *TExecBase, struct TMemPool *pool,
	TINT8 *mem, TSIZE size)
{
	TDBASSERT(99, (mem == TNULL) == (size == 0));
	if (mem)
	{
		union TMemHead *node = (union TMemHead *) pool->tpl_List.tlh_Head.tln_Succ;
		if (pool->tpl_Flags & TMEMHF_FIXED)
			exec_staticfree(node, mem, size);
		else
		{
			struct TNode *temp;
			TINT8 *memend = mem + size;
			while ((temp = ((struct TNode *) node)->tln_Succ))
			{
				if (mem >= node->tmh_Node.tmh_Mem && memend <=
					node->tmh_Node.tmh_MemEnd)
				{
					exec_staticfree(node, mem, size);
					if (node->tmh_Node.tmh_Free ==
						(TUINTPTR) node->tmh_Node.tmh_MemEnd -
						(TUINTPTR) node->tmh_Node.tmh_Mem)
					{
						/* flush puddle */
						TREMOVE((struct TNode *) node);
						TFree(node);
					}
					else
					{
						/* recently used node moves up */
						TNodeUp((struct TNode *) node);
					}

					return;
				}
				node = (union TMemHead *) temp;
			}
			TDBASSERT(99, TFALSE);
		}
	}
}

/*****************************************************************************/
/*
**	newmem = exec_ReallocPool(exec, pool, oldmem, oldsize, newsize)
**	Realloc pool allocation
*/

EXPORT TAPTR exec_ReallocPool(struct TExecBase *TExecBase,
	struct TMemPool *pool, TINT8 *oldmem, TSIZE oldsize, TSIZE newsize)
{
	if (oldmem && oldsize)
	{
		struct TNode *tempnode;
		union TMemHead *node;
		TINT8 *memend;

		if (newsize == 0)
		{
			TFreePool(pool, oldmem, oldsize);
			return TNULL;
		}

		node = (union TMemHead *) pool->tpl_List.tlh_Head.tln_Succ;

		if (pool->tpl_Flags & TMEMHF_FIXED)
		{
			return exec_staticrealloc(TExecBase, node, oldmem, oldsize,
				newsize);
		}

		memend = oldmem + oldsize;

		while ((tempnode = ((struct TNode *) node)->tln_Succ))
		{
			if (oldmem >= node->tmh_Node.tmh_Mem && memend <=
				node->tmh_Node.tmh_MemEnd)
			{
				TAPTR newmem;

				newmem = exec_staticrealloc(TExecBase, node,
					oldmem, oldsize, newsize);
				if (newmem)
				{
					if (node->tmh_Node.tmh_Flags & TMEMHF_LARGE)
					{
						/* this is no longer a large node */
						TREMOVE((struct TNode *) node);
						TAddHead(&pool->tpl_List, (struct TNode *) node);
						node->tmh_Node.tmh_Flags &= ~TMEMHF_LARGE;
					}
					return newmem;
				}

				newmem = TAllocPool(pool, newsize);
				if (newmem)
				{
					TCopyMem(oldmem, newmem, TMIN(oldsize, newsize));
					exec_staticfree(node, oldmem, oldsize);
					if (node->tmh_Node.tmh_Free ==
						(TUINTPTR) node->tmh_Node.tmh_MemEnd -
						(TUINTPTR) node->tmh_Node.tmh_Mem)
					{
						/* flush puddle */
						TREMOVE((struct TNode *) node);
						TFree(node);
					}
					else
					{
						/* recently used node moves up */
						TNodeUp((struct TNode *) node);
					}
				}
				return newmem;
			}
			node = (union TMemHead *) tempnode;
		}
	}
	else if ((oldmem == TNULL) && (oldsize == 0))
		return TAllocPool(pool, newsize);

	TDBASSERT(99, TFALSE);
	return TNULL;
}

/*****************************************************************************/
/*
**	TNULL message allocator -
**	Note that this kind of allocator is using execbase->texb_Lock, not
**	the lock supplied with the MM. There may be no 'self'-context available
**	when a message MM is used to allocate the initial task structures
**	during the Exec setup.
*/

static TAPTR exec_mmu_msgalloc(struct TMemManager *mmu, TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TAPTR hal = TExecBase->texb_HALBase;
	TINT8 *mem;
	THALLock(hal, &TExecBase->texb_Lock);
	mem = THALAlloc(hal, size +
		sizeof(struct TNode) + sizeof(struct TMessage));
	if (mem)
	{
		((struct TMessage *) (mem + sizeof(struct TNode)))->tmsg_RPort = TNULL;
		TAddTail(&mmu->tmm_TrackList, (struct TNode *) mem);
		THALUnlock(hal, &TExecBase->texb_Lock);
		((struct TMessage *) (mem + sizeof(struct TNode)))->tmsg_Flags =
			TMSG_STATUS_FAILED;
		return (TAPTR) (mem + sizeof(struct TNode) + sizeof(struct TMessage));
	}
	THALUnlock(hal, &TExecBase->texb_Lock);
	return TNULL;
}

static void exec_mmu_msgfree(struct TMemManager *mmu, TINT8 *mem, TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TAPTR hal = TExecBase->texb_HALBase;
	THALLock(hal, &TExecBase->texb_Lock);
	TREMOVE((struct TNode *) (mem - sizeof(struct TNode) -
		sizeof(struct TMessage)));
	THALFree(hal, mem - sizeof(struct TNode) - sizeof(struct TMessage),
		size + sizeof(struct TNode) + sizeof(struct TMessage));
	THALUnlock(hal, &TExecBase->texb_Lock);
}

static void exec_mmu_msgdestroy(struct TMemManager *mmu)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TAPTR hal = TExecBase->texb_HALBase;
	struct TNode *nextnode, *node = mmu->tmm_TrackList.tlh_Head.tln_Succ;
	TINT numfreed = 0;
	while ((nextnode = node->tln_Succ))
	{
		THALFree(hal, node, ((union TMemManagerInfo *) ((TINT8 *) (node + 1) +
			sizeof(struct TMessage)))->tmu_Node.tmu_UserSize +
				sizeof(struct TNode) +
			sizeof(struct TMessage) + sizeof(union TMemManagerInfo));
		numfreed++;
		node = nextnode;
	}
	if (numfreed)
		TDBPRINTF(TDB_WARN,("freed %d pending messages\n", numfreed));
}

static THOOKENTRY TTAG exec_mmu_msg(struct THook *hook, TAPTR obj, TTAG m)
{
	struct TMemManager *mmu = obj;
	union TMemMsg *msg = (union TMemMsg *) m;
	switch (msg->tmmsg_Type)
	{
		case TMMSG_DESTROY:
			exec_mmu_msgdestroy(mmu);
			break;
		case TMMSG_ALLOC:
			return (TTAG) exec_mmu_msgalloc(mmu,
				msg->tmmsg_Alloc.tmmsg_Size);
		case TMMSG_FREE:
			exec_mmu_msgfree(mmu,
				msg->tmmsg_Free.tmmsg_Ptr,
				msg->tmmsg_Free.tmmsg_Size);
			break;
		case TMMSG_REALLOC:
			TDBPRINTF(TDB_ERROR,("messages cannot be reallocated\n"));
			break;
		default:
			TDBPRINTF(TDB_ERROR,("unknown hookmsg\n"));
	}
	return 0;
}

/*****************************************************************************/
/*
**	static memheader allocator
*/

#if defined(ENABLE_ADVANCED_MEMORY_MANAGERS)

static TAPTR exec_mmu_staticalloc(struct TMemManager *mmu, TSIZE size)
{
	return exec_staticalloc(mmu->tmm_Allocator, size);
}

static void exec_mmu_staticfree(struct TMemManager *mmu, TINT8 *mem,
	TSIZE size)
{
	exec_staticfree(mmu->tmm_Allocator, mem, size);
}

static TAPTR exec_mmu_staticrealloc(struct TMemManager *mmu, TINT8 *oldmem,
	TUINT oldsize, TUINT newsize)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	return exec_staticrealloc(TExecBase, mmu->tmm_Allocator, oldmem, oldsize,
		newsize);
}

static THOOKENTRY TTAG exec_mmu_static(struct THook *hook, TAPTR obj, TTAG m)
{
	struct TMemManager *mmu = obj;
	union TMemMsg *msg = (union TMemMsg *) m;
	switch (msg->tmmsg_Type)
	{
		case TMMSG_DESTROY:
			break;
		case TMMSG_ALLOC:
			return (TTAG) exec_mmu_staticalloc(mmu,
				msg->tmmsg_Alloc.tmmsg_Size);
		case TMMSG_FREE:
			exec_mmu_staticfree(mmu,
				msg->tmmsg_Free.tmmsg_Ptr,
				msg->tmmsg_Free.tmmsg_Size);
			break;
		case TMMSG_REALLOC:
			return (TTAG) exec_mmu_staticrealloc(mmu,
				msg->tmmsg_Realloc.tmmsg_Ptr,
				msg->tmmsg_Realloc.tmmsg_OSize,
				msg->tmmsg_Realloc.tmmsg_NSize);
		default:
			TDBPRINTF(TDB_ERROR,("unknown hookmsg\n"));
	}
	return 0;
}

/*****************************************************************************/
/*
**	pooled allocator
*/

static TAPTR exec_mmu_poolalloc(struct TMemManager *mmu, TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	return TAllocPool(mmu->tmm_Allocator, size);
}

static void exec_mmu_poolfree(struct TMemManager *mmu, TINT8 *mem, TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TFreePool(mmu->tmm_Allocator, mem, size);
}

static TAPTR exec_mmu_poolrealloc(struct TMemManager *mmu, TINT8 *oldmem,
	TUINT oldsize, TUINT newsize)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	return TReallocPool(mmu->tmm_Allocator, oldmem, oldsize, newsize);
}

static THOOKENTRY TTAG exec_mmu_pool(struct THook *hook, TAPTR obj, TTAG m)
{
	struct TMemManager *mmu = obj;
	union TMemMsg *msg = (union TMemMsg *) m;
	switch (msg->tmmsg_Type)
	{
		case TMMSG_DESTROY:
			break;
		case TMMSG_ALLOC:
			return (TTAG) exec_mmu_poolalloc(mmu,
				msg->tmmsg_Alloc.tmmsg_Size);
		case TMMSG_FREE:
			exec_mmu_poolfree(mmu,
				msg->tmmsg_Free.tmmsg_Ptr,
				msg->tmmsg_Free.tmmsg_Size);
			break;
		case TMMSG_REALLOC:
			return (TTAG) exec_mmu_poolrealloc(mmu,
				msg->tmmsg_Realloc.tmmsg_Ptr,
				msg->tmmsg_Realloc.tmmsg_OSize,
				msg->tmmsg_Realloc.tmmsg_NSize);
		default:
			TDBPRINTF(TDB_ERROR,("unknown hookmsg\n"));
	}
	return 0;
}

/*****************************************************************************/
/*
**	pooled+tasksafe allocator
*/

static TAPTR exec_mmu_pooltaskalloc(struct TMemManager *mmu, TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TINT8 *mem;
	TLock(&mmu->tmm_Lock);
	mem = TAllocPool(mmu->tmm_Allocator, size);
	TUnlock(&mmu->tmm_Lock);
	return mem;
}

static void exec_mmu_pooltaskfree(struct TMemManager *mmu, TINT8 *mem,
	TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TLock(&mmu->tmm_Lock);
	TFreePool(mmu->tmm_Allocator, mem, size);
	TUnlock(&mmu->tmm_Lock);
}

static TAPTR exec_mmu_pooltaskrealloc(struct TMemManager *mmu, TINT8 *oldmem,
	TUINT oldsize, TUINT newsize)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TINT8 *newmem;
	TLock(&mmu->tmm_Lock);
	newmem = TReallocPool(mmu->tmm_Allocator, oldmem, oldsize, newsize);
	TUnlock(&mmu->tmm_Lock);
	return newmem;
}

static void exec_mmu_pooltaskdestroy(struct TMemManager *mmu)
{
	TDESTROY(&mmu->tmm_Lock);
}

static THOOKENTRY TTAG exec_mmu_pooltask(struct THook *hook, TAPTR obj, TTAG m)
{
	struct TMemManager *mmu = obj;
	union TMemMsg *msg = (union TMemMsg *) m;
	switch (msg->tmmsg_Type)
	{
		case TMMSG_DESTROY:
			exec_mmu_pooltaskdestroy(mmu);
			break;
		case TMMSG_ALLOC:
			return (TTAG) exec_mmu_pooltaskalloc(mmu,
				msg->tmmsg_Alloc.tmmsg_Size);
		case TMMSG_FREE:
			exec_mmu_pooltaskfree(mmu,
				msg->tmmsg_Free.tmmsg_Ptr,
				msg->tmmsg_Free.tmmsg_Size);
			break;
		case TMMSG_REALLOC:
			return (TTAG) exec_mmu_pooltaskrealloc(mmu,
				msg->tmmsg_Realloc.tmmsg_Ptr,
				msg->tmmsg_Realloc.tmmsg_OSize,
				msg->tmmsg_Realloc.tmmsg_NSize);
		default:
			TDBPRINTF(TDB_ERROR,("unknown hookmsg\n"));
	}
	return 0;
}

/*****************************************************************************/
/*
**	static memheader allocator, task-safe
*/

static TAPTR exec_mmu_statictaskalloc(struct TMemManager *mmu, TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TINT8 *mem;
	TLock(&mmu->tmm_Lock);
	mem = exec_staticalloc(mmu->tmm_Allocator, size);
	TUnlock(&mmu->tmm_Lock);
	return mem;
}

static void exec_mmu_statictaskfree(struct TMemManager *mmu, TINT8 *mem,
	TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TLock(&mmu->tmm_Lock);
	exec_staticfree(mmu->tmm_Allocator, mem, size);
	TUnlock(&mmu->tmm_Lock);
}

static TAPTR exec_mmu_statictaskrealloc(struct TMemManager *mmu, TINT8 *oldmem,
	TUINT oldsize, TUINT newsize)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TINT8 *newmem;
	TLock(&mmu->tmm_Lock);
	newmem = exec_staticrealloc(TExecBase, mmu->tmm_Allocator, oldmem, oldsize,
		newsize);
	TUnlock(&mmu->tmm_Lock);
	return newmem;
}

static void exec_mmu_statictaskdestroy(struct TMemManager *mmu)
{
	TDESTROY(&mmu->tmm_Lock);
}

static THOOKENTRY TTAG exec_mmu_statictask(struct THook *hook, TAPTR obj,
	TTAG m)
{
	struct TMemManager *mmu = obj;
	union TMemMsg *msg = (union TMemMsg *) m;
	switch (msg->tmmsg_Type)
	{
		case TMMSG_DESTROY:
			exec_mmu_statictaskdestroy(mmu);
			break;
		case TMMSG_ALLOC:
			return (TTAG) exec_mmu_statictaskalloc(mmu,
				msg->tmmsg_Alloc.tmmsg_Size);
		case TMMSG_FREE:
			exec_mmu_statictaskfree(mmu,
				msg->tmmsg_Free.tmmsg_Ptr,
				msg->tmmsg_Free.tmmsg_Size);
			break;
		case TMMSG_REALLOC:
			return (TTAG) exec_mmu_statictaskrealloc(mmu,
				msg->tmmsg_Realloc.tmmsg_Ptr,
				msg->tmmsg_Realloc.tmmsg_OSize,
				msg->tmmsg_Realloc.tmmsg_NSize);
		default:
			TDBPRINTF(TDB_ERROR,("unknown hookmsg\n"));
	}
	return 0;
}

/*****************************************************************************/
/*
**	tasksafe MM allocator
*/

static TAPTR exec_mmu_taskalloc(struct TMemManager *mmu, TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TINT8 *mem;
	TLock(&mmu->tmm_Lock);
	mem = TAlloc(mmu->tmm_Allocator, size);
	TUnlock(&mmu->tmm_Lock);
	return mem;
}

static TAPTR exec_mmu_taskrealloc(struct TMemManager *mmu, TINT8 *oldmem,
	TUINT oldsize, TUINT newsize)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TINT8 *newmem;
	TLock(&mmu->tmm_Lock);
	newmem = TRealloc(oldmem, newsize);
	TUnlock(&mmu->tmm_Lock);
	return newmem;
}

static void exec_mmu_taskfree(struct TMemManager *mmu, TINT8 *mem, TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TLock(&mmu->tmm_Lock);
	TFree(mem);
	TUnlock(&mmu->tmm_Lock);
}

static void exec_mmu_taskdestroy(struct TMemManager *mmu)
{
	TDESTROY(&mmu->tmm_Lock);
}

static THOOKENTRY TTAG exec_mmu_task(struct THook *hook, TAPTR obj, TTAG m)
{
	struct TMemManager *mmu = obj;
	union TMemMsg *msg = (union TMemMsg *) m;
	switch (msg->tmmsg_Type)
	{
		case TMMSG_DESTROY:
			exec_mmu_taskdestroy(mmu);
			break;
		case TMMSG_ALLOC:
			return (TTAG) exec_mmu_taskalloc(mmu,
				msg->tmmsg_Alloc.tmmsg_Size);
		case TMMSG_FREE:
			exec_mmu_taskfree(mmu,
				msg->tmmsg_Free.tmmsg_Ptr,
				msg->tmmsg_Free.tmmsg_Size);
			break;
		case TMMSG_REALLOC:
			return (TTAG) exec_mmu_taskrealloc(mmu,
				msg->tmmsg_Realloc.tmmsg_Ptr,
				msg->tmmsg_Realloc.tmmsg_OSize,
				msg->tmmsg_Realloc.tmmsg_NSize);
		default:
			TDBPRINTF(TDB_ERROR,("unknown hookmsg\n"));
	}
	return 0;
}

/*****************************************************************************/
/*
**	tracking MM allocator
*/

static TAPTR exec_mmu_trackalloc(struct TMemManager *mmu, TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TINT8 *mem = TAlloc(mmu->tmm_Allocator, size + sizeof(struct TNode));
	if (mem)
	{
		TAddHead(&mmu->tmm_TrackList, (struct TNode *) mem);
		return (TAPTR) (mem + sizeof(struct TNode));
	}
	return TNULL;
}

static TAPTR exec_mmu_trackrealloc(struct TMemManager *mmu, TINT8 *oldmem,
	TUINT oldsize, TUINT newsize)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TINT8 *newmem;
	TREMOVE((struct TNode *) (oldmem - sizeof(struct TNode)));
	newmem = TRealloc(oldmem - sizeof(struct TNode),
		newsize + sizeof(struct TNode));
	if (newmem)
	{
		TAddHead(&mmu->tmm_TrackList, (struct TNode *) newmem);
		newmem += sizeof(struct TNode);
	}
	return (TAPTR) newmem;
}

static void exec_mmu_trackfree(struct TMemManager *mmu, TINT8 *mem, TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TREMOVE((struct TNode *) (mem - sizeof(struct TNode)));
	TFree(mem - sizeof(struct TNode));
}

static void exec_mmu_trackdestroy(struct TMemManager *mmu)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	struct TNode *nextnode, *node = mmu->tmm_TrackList.tlh_Head;
	TINT numfreed = 0;
	while ((nextnode = node->tln_Succ))
	{
		TFree(node);
		numfreed++;
		node = nextnode;
	}
	if (numfreed)
		TDBPRINTF(TDB_WARN,("freed %d pending allocations\n", numfreed));
}

static THOOKENTRY TTAG exec_mmu_track(struct THook *hook, TAPTR obj, TTAG m)
{
	struct TMemManager *mmu = obj;
	union TMemMsg *msg = (union TMemMsg *) m;
	switch (msg->tmmsg_Type)
	{
		case TMMSG_DESTROY:
			exec_mmu_trackdestroy(mmu);
			break;
		case TMMSG_ALLOC:
			return (TTAG) exec_mmu_trackalloc(mmu,
				msg->tmmsg_Alloc.tmmsg_Size);
		case TMMSG_FREE:
			exec_mmu_trackfree(mmu,
				msg->tmmsg_Free.tmmsg_Ptr,
				msg->tmmsg_Free.tmmsg_Size);
			break;
		case TMMSG_REALLOC:
			return (TTAG) exec_mmu_trackrealloc(mmu,
				msg->tmmsg_Realloc.tmmsg_Ptr,
				msg->tmmsg_Realloc.tmmsg_OSize,
				msg->tmmsg_Realloc.tmmsg_NSize);
		default:
			TDBPRINTF(TDB_ERROR,("unknown hookmsg\n"));
	}
	return 0;
}

/*****************************************************************************/
/*
**	tasksafe+tracking MM allocator
*/

static TAPTR exec_mmu_tasktrackalloc(struct TMemManager *mmu, TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TINT8 *mem;
	TLock(&mmu->tmm_Lock);
	mem = TAlloc(mmu->tmm_Allocator, size + sizeof(struct TNode));
	if (mem)
	{
		TAddHead(&mmu->tmm_TrackList, (struct TNode *) mem);
		mem += sizeof(struct TNode);
	}
	TUnlock(&mmu->tmm_Lock);
	return (TAPTR) mem;
}

static TAPTR exec_mmu_tasktrackrealloc(struct TMemManager *mmu, TINT8 *oldmem,
	TUINT oldsize, TUINT newsize)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TINT8 *newmem;
	TLock(&mmu->tmm_Lock);
	TREMOVE((struct TNode *) (oldmem - sizeof(struct TNode)));
	newmem = TRealloc(oldmem - sizeof(struct TNode),
		newsize + sizeof(struct TNode));
	if (newmem)
	{
		TAddHead(&mmu->tmm_TrackList, (struct TNode *) newmem);
		newmem += sizeof(struct TNode);
	}
	TUnlock(&mmu->tmm_Lock);
	return (TAPTR) newmem;
}

static void exec_mmu_tasktrackfree(struct TMemManager *mmu, TINT8 *mem,
	TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TLock(&mmu->tmm_Lock);
	TREMOVE((struct TNode *) (mem - sizeof(struct TNode)));
	TFree(mem - sizeof(struct TNode));
	TUnlock(&mmu->tmm_Lock);
}

static void exec_mmu_tasktrackdestroy(struct TMemManager *mmu)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	struct TNode *nextnode, *node = mmu->tmm_TrackList.tlh_Head;
	TINT numfreed = 0;
	while ((nextnode = node->tln_Succ))
	{
		TFree(node);
		numfreed++;
		node = nextnode;
	}
	TDESTROY(&mmu->tmm_Lock);
	if (numfreed)
		TDBPRINTF(TDB_WARN,("freed %d pending allocations\n", numfreed));
}

static THOOKENTRY TTAG
exec_mmu_tasktrack(struct THook *hook, TAPTR obj, TTAG m)
{
	struct TMemManager *mmu = obj;
	union TMemMsg *msg = (union TMemMsg *) m;
	switch (msg->tmmsg_Type)
	{
		case TMMSG_DESTROY:
			exec_mmu_tasktrackdestroy(mmu);
			break;
		case TMMSG_ALLOC:
			return (TTAG) exec_mmu_tasktrackalloc(mmu,
				msg->tmmsg_Alloc.tmmsg_Size);
		case TMMSG_FREE:
			exec_mmu_tasktrackfree(mmu,
				msg->tmmsg_Free.tmmsg_Ptr,
				msg->tmmsg_Free.tmmsg_Size);
			break;
		case TMMSG_REALLOC:
			return (TTAG) exec_mmu_tasktrackrealloc(mmu,
				msg->tmmsg_Realloc.tmmsg_Ptr,
				msg->tmmsg_Realloc.tmmsg_OSize,
				msg->tmmsg_Realloc.tmmsg_NSize);
		default:
			TDBPRINTF(TDB_ERROR,("unknown hookmsg\n"));
	}
	return 0;
}

/*****************************************************************************/
/*
**	TNULL tasksafe+tracking allocator
*/

static TAPTR exec_mmu_kntasktrackalloc(struct TMemManager *mmu, TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	struct TNode *mem;
	TLock(&mmu->tmm_Lock);
	mem = TAlloc(&TExecBase->texb_BaseMemManager, size + sizeof(struct TNode));
	if (mem)
	{
		TAddHead(&mmu->tmm_TrackList, mem);
		mem++;
	}
	TUnlock(&mmu->tmm_Lock);
	return (TAPTR) mem;
}

static TAPTR exec_mmu_kntasktrackrealloc(struct TMemManager *mmu,
	TINT8 *oldmem, TUINT oldsize, TUINT newsize)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	struct TNode *newmem;
	oldmem -= sizeof(struct TNode);
	TLock(&mmu->tmm_Lock);
	TREMOVE((struct TNode *) oldmem);
	newmem = TRealloc(oldmem, newsize + sizeof(struct TNode));
	if (newmem)
	{
		TAddHead(&mmu->tmm_TrackList, newmem);
		newmem++;
	}
	TUnlock(&mmu->tmm_Lock);
	return (TAPTR) newmem;
}

static void exec_mmu_kntasktrackfree(struct TMemManager *mmu, TINT8 *mem,
	TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	mem -= sizeof(struct TNode);
	TLock(&mmu->tmm_Lock);
	TREMOVE((struct TNode *) mem);
	TFree(mem);
	TUnlock(&mmu->tmm_Lock);
}

static void exec_mmu_kntasktrackdestroy(struct TMemManager *mmu)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	struct TNode *nextnode, *node = mmu->tmm_TrackList.tlh_Head;
	TINT numfreed = 0;

	while ((nextnode = node->tln_Succ))
	{
		TFree(node);
		numfreed++;
		node = nextnode;
	}

	TDESTROY(&mmu->tmm_Lock);

	#ifdef TDEBUG
	if (numfreed > 0)
		TDBPRINTF(TDB_WARN,("freed %d pending allocations\n",numfreed));
	#endif
}

static THOOKENTRY TTAG exec_mmu_kntasktrack(struct THook *hook, TAPTR obj,
	TTAG m)
{
	struct TMemManager *mmu = obj;
	union TMemMsg *msg = (union TMemMsg *) m;
	switch (msg->tmmsg_Type)
	{
		case TMMSG_DESTROY:
			exec_mmu_kntasktrackdestroy(mmu);
			break;
		case TMMSG_ALLOC:
			return (TTAG) exec_mmu_kntasktrackalloc(mmu,
				msg->tmmsg_Alloc.tmmsg_Size);
		case TMMSG_FREE:
			exec_mmu_kntasktrackfree(mmu,
				msg->tmmsg_Free.tmmsg_Ptr,
				msg->tmmsg_Free.tmmsg_Size);
			break;
		case TMMSG_REALLOC:
			return (TTAG) exec_mmu_kntasktrackrealloc(mmu,
				msg->tmmsg_Realloc.tmmsg_Ptr,
				msg->tmmsg_Realloc.tmmsg_OSize,
				msg->tmmsg_Realloc.tmmsg_NSize);
		default:
			TDBPRINTF(TDB_ERROR,("unknown hookmsg\n"));
	}
	return 0;
}

/*****************************************************************************/
/*
**	MM-on-MM allocator
*/

static TAPTR exec_mmu_mmualloc(struct TMemManager *mmu, TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	return TAlloc(mmu->tmm_Allocator, size);
}

static TAPTR exec_mmu_mmurealloc(struct TMemManager *mmu, TINT8 *oldmem,
	TUINT oldsize, TUINT newsize)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	return TRealloc(oldmem, newsize);
}

static void exec_mmu_mmufree(struct TMemManager *mmu, TINT8 *mem, TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	TFree(mem);
}

static THOOKENTRY TTAG exec_mmu_mmu(struct THook *hook, TAPTR obj, TTAG m)
{
	struct TMemManager *mmu = obj;
	union TMemMsg *msg = (union TMemMsg *) m;
	switch (msg->tmmsg_Type)
	{
		case TMMSG_DESTROY:
			break;
		case TMMSG_ALLOC:
			return (TTAG) exec_mmu_mmualloc(mmu,
				msg->tmmsg_Alloc.tmmsg_Size);
		case TMMSG_FREE:
			exec_mmu_mmufree(mmu,
				msg->tmmsg_Free.tmmsg_Ptr,
				msg->tmmsg_Free.tmmsg_Size);
			break;
		case TMMSG_REALLOC:
			return (TTAG) exec_mmu_mmurealloc(mmu,
				msg->tmmsg_Realloc.tmmsg_Ptr,
				msg->tmmsg_Realloc.tmmsg_OSize,
				msg->tmmsg_Realloc.tmmsg_NSize);
		default:
			TDBPRINTF(TDB_ERROR,("unknown hookmsg\n"));
	}
	return 0;
}

#endif /* defined(ENABLE_ADVANCED_MEMORY_MANAGERS) */

/*****************************************************************************/
/*
**	TNULL allocator
*/

static TAPTR exec_mmu_kernelalloc(struct TMemManager *mmu, TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	return THALAlloc(TExecBase->texb_HALBase, size);
}

static void exec_mmu_kernelfree(struct TMemManager *mmu, TINT8 *mem,
	TSIZE size)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	THALFree(TExecBase->texb_HALBase, mem, size);
}

static TAPTR exec_mmu_kernelrealloc(struct TMemManager *mmu, TINT8 *oldmem,
	TUINT oldsize, TUINT newsize)
{
	struct TExecBase *TExecBase = (TEXECBASE *) TGetExecBase(mmu);
	return THALRealloc(TExecBase->texb_HALBase, oldmem, oldsize, newsize);
}

static THOOKENTRY TTAG exec_mmu_kernel(struct THook *hook, TAPTR obj, TTAG m)
{
	struct TMemManager *mmu = obj;
	union TMemMsg *msg = (union TMemMsg *) m;
	switch (msg->tmmsg_Type)
	{
		case TMMSG_DESTROY:
			break;
		case TMMSG_ALLOC:
			return (TTAG) exec_mmu_kernelalloc(mmu,
				msg->tmmsg_Alloc.tmmsg_Size);
		case TMMSG_FREE:
			exec_mmu_kernelfree(mmu,
				msg->tmmsg_Free.tmmsg_Ptr,
				msg->tmmsg_Free.tmmsg_Size);
			break;
		case TMMSG_REALLOC:
			return (TTAG) exec_mmu_kernelrealloc(mmu,
				msg->tmmsg_Realloc.tmmsg_Ptr,
				msg->tmmsg_Realloc.tmmsg_OSize,
				msg->tmmsg_Realloc.tmmsg_NSize);
		default:
			TDBPRINTF(TDB_FAIL,("unknown hookmsg\n"));
	}
	return 0;
}

/*****************************************************************************/
/*
**	void allocator
*/

static THOOKENTRY TTAG exec_mmu_void(struct THook *hook, TAPTR obj, TTAG m)
{
	union TMemMsg *msg = (union TMemMsg *) m;
	switch (msg->tmmsg_Type)
	{
		case TMMSG_DESTROY:
		case TMMSG_ALLOC:
		case TMMSG_FREE:
		case TMMSG_REALLOC:
			break;
		default:
			TDBPRINTF(TDB_ERROR,("unknown hookmsg\n"));
	}
	return 0;
}

/*****************************************************************************/
/*
**	success = exec_initmm(exec, mmu, allocator, mmutype, tags)
**	Initialize Memory Manager
*/

static THOOKENTRY TTAG exec_mmudestroyfunc(struct THook *hook, TAPTR obj,
	TTAG msg)
{
	if (msg == TMSG_DESTROY)
	{
		struct TMemManager *mmu = obj;
		TCALLHOOKPKT(&mmu->tmm_Hook, mmu, (TTAG) &msg_destroy);
	}
	return 0;
}

LOCAL TBOOL exec_initmm(TEXECBASE *TExecBase, struct TMemManager *mmu,
	TAPTR allocator, TUINT mmutype, TTAGITEM *tags)
{
	/* used before TExecFillMem is functional: */
	THALFillMem(TExecBase->texb_HALBase, mmu, sizeof(struct TMemManager), 0);

	mmu->tmm_Handle.thn_Hook.thk_Entry = exec_mmudestroyfunc;
	mmu->tmm_Type = mmutype;
	mmu->tmm_Handle.thn_Owner = (struct TModule *) TExecBase;
	mmu->tmm_Allocator = allocator;

	switch (mmutype)
	{
		case TMMT_MemManager:
			/* MM on top of another MM - no additional functionality */
			if (allocator)
			{
#if defined(ENABLE_ADVANCED_MEMORY_MANAGERS)
				mmu->tmm_Hook.thk_Entry = exec_mmu_mmu;
#else
				break;
#endif /* defined(ENABLE_ADVANCED_MEMORY_MANAGERS) */
			}
			else
				mmu->tmm_Hook.thk_Entry = exec_mmu_kernel;
			return TTRUE;

		case TMMT_Message:
			if (allocator == TNULL) /* must be TNULL for now */
			{
				/* note that we use the execbase lock */
				TINITLIST(&mmu->tmm_TrackList);
				mmu->tmm_Hook.thk_Entry = exec_mmu_msg;
				return TTRUE;
			}
			break;

		case TMMT_Void:
			mmu->tmm_Hook.thk_Entry = exec_mmu_void;
			mmu->tmm_Allocator = TNULL;
			mmu->tmm_Type = TMMT_Void;
			return TTRUE;

#if defined(ENABLE_ADVANCED_MEMORY_MANAGERS)
		case TMMT_Tracking:
			TINITLIST(&mmu->tmm_TrackList);
			if (allocator)
			{
				/* implement memory tracking on top of another MM */
				mmu->tmm_Hook.thk_Entry = exec_mmu_track;
				return TTRUE;
			}
			else
			{
				/* If tracking is requested for a TNULL allocator, we must
				additionally provide locking for the tracklist, because
				kernel allocators are tasksafe by definition. */
				if (exec_initlock(TExecBase, &mmu->tmm_Lock))
				{
					mmu->tmm_Hook.thk_Entry = exec_mmu_kntasktrack;
					return TTRUE;
				}
			}
			break;

		case TMMT_TaskSafe:
			if (allocator)
			{
				/* implement task-safety on top of another MM */
				if (exec_initlock(TExecBase, &mmu->tmm_Lock))
				{
					mmu->tmm_Hook.thk_Entry = exec_mmu_task;
					return TTRUE;
				}
			}
			else
			{
				/* a TNULL MM is task-safe by definition */
				mmu->tmm_Hook.thk_Entry = exec_mmu_kernel;
				return TTRUE;
			}
			break;

		case TMMT_TaskSafe | TMMT_Tracking:
			/* implement task-safety and tracking on top of another MM */
			if (exec_initlock(TExecBase, &mmu->tmm_Lock))
			{
				TINITLIST(&mmu->tmm_TrackList);
				if (allocator)
					mmu->tmm_Hook.thk_Entry = exec_mmu_tasktrack;
				else
					mmu->tmm_Hook.thk_Entry = exec_mmu_kntasktrack;
				return TTRUE;
			}
			break;

		case TMMT_Static:
			/*	MM on top of memheader */
			if (allocator)
			{
				mmu->tmm_Hook.thk_Entry = exec_mmu_static;
				return TTRUE;
			}
			break;

		case TMMT_Static | TMMT_TaskSafe:
			/*	MM on top of memheader, task-safe */
			if (allocator)
			{
				if (exec_initlock(TExecBase, &mmu->tmm_Lock))
				{
					mmu->tmm_Hook.thk_Entry = exec_mmu_statictask;
					return TTRUE;
				}
			}
			break;

		case TMMT_Pooled:
			/*	MM on top of a pool */
			if (allocator)
			{
				mmu->tmm_Hook.thk_Entry = exec_mmu_pool;
				return TTRUE;
			}
			break;

		case TMMT_Pooled | TMMT_TaskSafe:
			/*	MM on top of a pool, task-safe */
			if (allocator)
			{
				if (exec_initlock(TExecBase, &mmu->tmm_Lock))
				{
					mmu->tmm_Hook.thk_Entry = exec_mmu_pooltask;
					return TTRUE;
				}
			}
			break;
#endif /* defined(ENABLE_ADVANCED_MEMORY_MANAGERS) */
	}

	/* As a fallback, initialize a void MM that is incapable of allocating */

	mmu->tmm_Hook.thk_Entry = exec_mmu_void;
	mmu->tmm_Allocator = TNULL;
 	mmu->tmm_Type = TMMT_Void;
	return TFALSE;
}
