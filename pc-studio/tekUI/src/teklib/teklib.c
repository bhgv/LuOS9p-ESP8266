#ifndef _TEK_TEKLIB_C
#define _TEK_TEKLIB_C

/*
**	$Id: teklib.c,v 1.5 2006/09/10 20:30:25 tmueller Exp $
**	teklib/src/teklib/teklib.c - 'tek' link library implementation
**
**	The library functions operate on public exec structures, do not
**	depend on private data or functions, and constitute the most
**	conservative and immutable part of TEKlib.
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/teklib.h>
#include <tek/inline/exec.h>

/*****************************************************************************/
/*
**	TInitList(list)
**	Prepare list header
*/

TLIBAPI void TInitList(struct TList *list)
{
	list->tlh_Head.tln_Succ = &list->tlh_Tail;
	list->tlh_Head.tln_Pred = TNULL;
	list->tlh_Tail.tln_Succ = TNULL;
	list->tlh_Tail.tln_Pred = &list->tlh_Head;
}

/*****************************************************************************/
/*
**	TAddHead(list, node)
**	Add a node at the head of a list
*/

TLIBAPI void TAddHead(struct TList *list, struct TNode *node)
{
	struct TNode *temp = list->tlh_Head.tln_Succ;
	list->tlh_Head.tln_Succ = node;
	node->tln_Succ = temp;
	node->tln_Pred = &list->tlh_Head;
	temp->tln_Pred = node;
}

/*****************************************************************************/
/*
**	TAddTail(list, node)
**	Add a node at the tail of a list
*/

TLIBAPI void TAddTail(struct TList *list, struct TNode *node)
{
	struct TNode *temp = list->tlh_Tail.tln_Pred;
	list->tlh_Tail.tln_Pred = node;
	node->tln_Succ = &list->tlh_Tail;
	node->tln_Pred = temp;
	temp->tln_Succ = node;
}

/*****************************************************************************/
/*
**	node = TRemHead(list)
**	Unlink and return a list's first node
*/

TLIBAPI struct TNode *TRemHead(struct TList *list)
{
	struct TNode *temp = list->tlh_Head.tln_Succ;
	if (temp->tln_Succ)
	{
		list->tlh_Head.tln_Succ = temp->tln_Succ;
		temp->tln_Succ->tln_Pred = &list->tlh_Head;
		return temp;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	node = TRemTail(list)
**	Unlink and return a list's last node
*/

TLIBAPI struct TNode *TRemTail(struct TList *list)
{
	struct TNode *temp = list->tlh_Tail.tln_Pred;
	if (temp->tln_Pred)
	{
		list->tlh_Tail.tln_Pred = temp->tln_Pred;
		temp->tln_Pred->tln_Succ = &list->tlh_Tail;
		return temp;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	TRemove(node)
**	Unlink node from a list
*/

TLIBAPI void TRemove(struct TNode *node)
{
	struct TNode *temp = node->tln_Succ;
	node->tln_Pred->tln_Succ = temp;
	temp->tln_Pred = node->tln_Pred;
}

/*****************************************************************************/
/*
**	TNodeUp(node)
**	Move a node one position up in a list
*/

TLIBAPI void TNodeUp(struct TNode *node)
{
	struct TNode *temp = node->tln_Pred;
	if (temp->tln_Pred)
	{
		temp->tln_Pred->tln_Succ = node;
		node->tln_Pred = temp->tln_Pred;
		temp->tln_Succ = node->tln_Succ;
		temp->tln_Pred = node;
		node->tln_Succ->tln_Pred = temp;
		node->tln_Succ = temp;
	}
}

/*****************************************************************************/
/*
**	TInsert(list, node, prednode)
**	Insert node after prednode
*/

TLIBAPI void TInsert(struct TList *list, struct TNode *node,
	struct TNode *prednode)
{
	if (list)
	{
		if (prednode)
		{
			struct TNode *temp = prednode->tln_Succ;
			if (temp)
			{
				node->tln_Succ = temp;
				node->tln_Pred = prednode;
				temp->tln_Pred = node;
				prednode->tln_Succ = node;
			}
			else
			{
				node->tln_Succ = prednode;
				temp = prednode->tln_Pred;
				node->tln_Pred = temp;
				prednode->tln_Pred = node;
				temp->tln_Succ = node;
			}
		}
		else
			TAddHead(list, node);
	}
}

/*****************************************************************************/
/*
**	TDestroy(handle)
**	Invoke destructor on a handle
*/

TLIBAPI void TDestroy(struct THandle *handle)
{
	if (handle)
		TCALLHOOKPKT(&handle->thn_Hook, handle, TMSG_DESTROY);
}

/*****************************************************************************/
/*
**	TDestroyList(list)
**	Unlink and invoke destructor on handles in a list
*/

TLIBAPI void TDestroyList(struct TList *list)
{
	struct TNode *nextnode, *node = list->tlh_Head.tln_Succ;
	while ((nextnode = node->tln_Succ))
	{
		TREMOVE(node);
		TCALLHOOKPKT(&((struct THandle *) node)->thn_Hook, node, TMSG_DESTROY);
		node = nextnode;
	}
}

/*****************************************************************************/
/*
**	modinst = TNewInstance(mod, possize, negsize)
**	Get module instance copy
*/

TLIBAPI struct TModule *TNewInstance(struct TModule *mod, TSIZE possize,
	TSIZE negsize)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	TAPTR inst = TAlloc(TNULL, possize + negsize);
	if (inst)
	{
		TSIZE size = TMIN(((struct TModule *) mod)->tmd_NegSize, negsize);
		inst = (TINT8 *) inst + negsize;
		if (size > 0)
			TCopyMem((TINT8 *) mod - size, (TINT8 *) inst - size, size);
		size = TMIN(((struct TModule *) mod)->tmd_PosSize, possize);
		TCopyMem(mod, inst, size);
		((struct TModule *) inst)->tmd_PosSize = possize;
		((struct TModule *) inst)->tmd_NegSize = negsize;
		((struct TModule *) inst)->tmd_InitTask = TFindTask(TNULL);
	}
	return inst;
}

/*****************************************************************************/
/*
**	TFreeInstance(mod)
**	Free module instance
*/

TLIBAPI void TFreeInstance(struct TModule *mod)
{
	if (mod)
	{
		struct TExecBase *TExecBase = TGetExecBase(mod);
		TFree((TINT8 *) mod - ((struct TModule *) mod)->tmd_NegSize);
	}
}

/*****************************************************************************/
/*
**	TInitVectors(mod, vectors, num)
**	Init module vectors
*/

TLIBAPI void TInitVectors(struct TModule *mod, const TMFPTR *vectors,
	TUINT numv)
{
	TMFPTR *vecp = (TMFPTR *) mod;
	while (numv--)
		*(--vecp) = *vectors++;
}

/*****************************************************************************/
/*
**	complete = TForEachTag(taglist, hook)
*/

TLIBAPI TBOOL TForEachTag(struct TTagItem *taglist, struct THook *hook)
{
	TBOOL complete = TFALSE;
	if (hook)
	{
		complete = TTRUE;
		while (taglist && complete)
		{
			switch ((TUINT) taglist->tti_Tag)
			{
				case TTAG_DONE:
					return complete;

				case TTAG_MORE:
					taglist = (struct TTagItem *) taglist->tti_Value;
					break;

				case TTAG_SKIP:
					taglist += 1 + (TINT) taglist->tti_Value;
					break;

				case TTAG_GOSUB:
					complete =
						TForEachTag((struct TTagItem *) taglist->tti_Value,
							hook);
					taglist++;
					break;

				default:
					complete = (TBOOL)
						TCALLHOOKPKT(hook, taglist, TMSG_FOREACHTAG);

				case TTAG_IGNORE:
					taglist++;
					break;
			}
		}
	}
	return TFALSE;
}

/*****************************************************************************/
/*
**	tag = TGetTag(taglist, tag, defvalue)
**	Get tag value
*/

TLIBAPI TTAG TGetTag(struct TTagItem *taglist, TUINT tag, TTAG defvalue)
{
	TUINT listtag;
	while (taglist)
	{
		listtag = taglist->tti_Tag;
		switch (listtag)
		{
			case TTAG_DONE:
				return defvalue;

			case TTAG_MORE:
				taglist = (struct TTagItem *) taglist->tti_Value;
				break;

			case TTAG_SKIP:
				taglist += 1 + (TINT) taglist->tti_Value;
				break;

			case TTAG_GOSUB:
			{
				TTAG res = TGetTag((struct TTagItem *) taglist->tti_Value,
					tag,defvalue);
				if (res != defvalue)
					return res;
				taglist++;
				break;
			}

			default:
				if (tag == listtag)
					return taglist->tti_Value;

			case TTAG_IGNORE:
				taglist++;
				break;
		}
	}
	return defvalue;
}

/*****************************************************************************/
/*
**	handle = TFindHandle(list, name)
**	Find named handle
*/

TLIBAPI struct THandle *TFindHandle(struct TList *list, TSTRPTR name)
{
	struct TNode *nnode, *node;
	for (node = list->tlh_Head.tln_Succ; (nnode = node->tln_Succ); node = nnode)
	{
		TSTRPTR s1 = ((struct THandle *) node)->thn_Name;
		if (s1 && name)
		{
			TSTRPTR s2 = name;
			TINT a;
			while ((a = *s1++) == *s2++)
			{
				if (a == 0)
					return (struct THandle *) node;
			}
		}
		else if (s1 == TNULL && name == TNULL)
			return (struct THandle *) node;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	TEKlib hooks allow transitions from e.g. register- to stack-based
**	calling conventions. hook->thk_Entry is invoked by TCallHookPkt()
**	and follows TEKlib's per-platform calling conventions (as declared
**	with THOOKENTRY). It however may point to a stub function that calls
**	the actual user function in hook->thk_SubEntry, which in turn may
**	be entirely language/compiler specific.
*/

static THOOKENTRY TTAG _THookEntry(struct THook *hook, TAPTR obj, TTAG msg)
{
	TTAG (*func)(struct THook *, TAPTR, TTAG) =
		(TTAG (*)(struct THook *, TAPTR, TTAG)) hook->thk_SubEntry;
	return (*func)(hook, obj, msg);
}

TLIBAPI void TInitHook(struct THook *hook, THOOKFUNC func, TAPTR data)
{
	hook->thk_Entry = _THookEntry;
	hook->thk_SubEntry = func;
	hook->thk_Data = data;
}

TLIBAPI TTAG TCallHookPkt(struct THook *hook, TAPTR obj, TTAG msg)
{
	return (*hook->thk_Entry)(hook, obj, msg);
}

/*****************************************************************************/
/*
**	TAddTime(a, b) - Add time: a + b -> a
*/

TLIBAPI void TAddTime(TTIME *a, TTIME *b)
{
	a->tdt_Int64 += b->tdt_Int64;
}

/*****************************************************************************/
/*
**	TSubTime(a, b) - Subtract time: a - b -> a
*/

TLIBAPI void TSubTime(TTIME *a, TTIME *b)
{
	a->tdt_Int64 -= b->tdt_Int64;
}

/*****************************************************************************/
/*
**	TCmpTime(a, b) - a > b: 1, a < b: -1, a = b: 0
*/

TLIBAPI TINT TCmpTime(TTIME *a, TTIME *b)
{
	if (a->tdt_Int64 < b->tdt_Int64) return -1;
	if (a->tdt_Int64 > b->tdt_Int64) return 1;
	return 0;
}

/*****************************************************************************/
/*
**	TAddDate(date, d, ndays, time)
**	Add a number of days, and optionally a time, to date d1.
*/

TLIBAPI void TAddDate(TDATE *d, TINT ndays, TTIME *tm)
{
	d->tdt_Int64 += (TINT64) ndays * 86400000000ULL;
	if (tm)
		d->tdt_Int64 += tm->tdt_Int64;
}

/*****************************************************************************/
/*
**	TSubDate(date, d, ndays, time)
**	Subtract a number of days, and optionally a time, from a date.
*/

TLIBAPI void TSubDate(TDATE *d, TINT ndays, TTIME *tm)
{
	d->tdt_Int64 -= (TINT64) ndays * 86400000000ULL;
	if (tm)
		d->tdt_Int64 -= tm->tdt_Int64;
}

/*****************************************************************************/
/*
**	ndays = TDiffDate(date, d1, d2, tm)
**	Get the number of days difference between two dates,
**	and optionally the number of seconds/microseconds
*/

TLIBAPI TINT TDiffDate(TDATE *d1, TDATE *d2, TTIME *tm)
{
	TUINT64 dd = d1->tdt_Int64 - d2->tdt_Int64;
	if (tm)
		tm->tdt_Int64 = dd % 86400000000ULL;
	return dd / 86400000000ULL;
}

/*****************************************************************************/
/*
**	success = TCreateTime(time, days, seconds, microseconds)
**	Compose a TTIME from days, seconds, microseconds
*/

TLIBAPI TBOOL TCreateTime(TTIME *t, TINT d, TINT s, TINT us)
{
	if (t)
	{
		TINT64 x = (TINT64) d * 86400000000ULL;
		x += (TINT64) s * 1000000;
		t->tdt_Int64 = x + us;
		return TTRUE;
	}
	return TFALSE;
}

/*****************************************************************************/
/*
**	success = TExtractTime(time, pdays, pseconds, pmicroseconds)
**	Extract time in days, seconds, microseconds. Returns TFALSE if
**	an overflow occurred.
*/

TLIBAPI TBOOL TExtractTime(TTIME *t, TINT *d, TINT *s, TINT *us)
{
	if (t)
	{
		TINT64 x = t->tdt_Int64;
		if (us)
		{
			if (s == TNULL && d == TNULL)
			{
				*us = x;
				return (x >= -0x80000000 && x <= 0x7fffffff);
			}
			*us = x % 1000000;
		}
		x /= 1000000;
		if (s)
		{
			if (d == TNULL)
			{
				*s = x;
				return (x >= -0x80000000 && x <= 0x7fffffff);
			}
			*s = x % 86400;
		}
		x /= 86400;
		if (d)
		{
			*d = x;
			return (x >= -0x80000000 && x <= 0x7fffffff);
		}
		return TTRUE;
	}
	return TFALSE;
}

/*****************************************************************************/
/*
**	TInitInterface(interface, module, name, version)
**	Initialize an interface structure
*/

static THOOKENTRY TTAG teklib_destroyiface(struct THook *h, TAPTR obj,
	TTAG msg)
{
	if (msg == TMSG_DESTROY)
	{
		struct TInterface *iface = obj;
		struct TModule *mod = iface->tif_Module;
		if (mod->tmd_Flags & TMODF_QUERYIFACE)
			TCALLHOOKPKT(&mod->tmd_Handle.thn_Hook, iface, TMSG_DROPIFACE);
	}
	return 0;
}

TLIBAPI void TInitInterface(struct TInterface *iface, struct TModule *mod,
	TSTRPTR name, TUINT16 version)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	TInitHook(&iface->tif_Handle.thn_Hook, teklib_destroyiface, TNULL);
	iface->tif_Handle.thn_Owner = TExecBase;
	iface->tif_Handle.thn_Name = name;
	iface->tif_Module = mod;
	iface->tif_Reserved = TNULL;
	iface->tif_Version = version;
}

/*****************************************************************************/
/*
**	entry = TGetNextEntry(handle)
**	Invoke 'getnextentry' method on a handle
*/

TLIBAPI TAPTR TGetNextEntry(struct THandle *handle)
{
	if (handle)
		return (TAPTR) TCALLHOOKPKT(&handle->thn_Hook, handle,
			TMSG_GETNEXTENTRY);
	return TNULL;
}

#endif /* _TEK_TEKLIB_C */
