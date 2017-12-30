#ifndef _TEK_LIB_REGION_C
#define _TEK_LIB_REGION_C

/*
**	region.c - Region library
**	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
**	See copyright notice in COPYRIGHT
*/

/*#define NDEBUG*/
#include <assert.h>

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/lib/tekui.h>
#include <tek/lib/region.h>

/*****************************************************************************/

/* number of merge operations: */
#define MERGE_NUMOPS					5
/* max number of rects buffered for re-use: */
#define MAXPOOLNODES 					1024
/* opportunistic merge min number of pixels: */
#define OPPORTUNISTIC_MERGE_THRESHOLD	1000
/* opportunistic merge ratio n/256: */
#define OPPORTUNISTIC_MERGE_RATIO		64		

/*****************************************************************************/

static void region_relinklist(struct TList *dlist, struct TList *slist)
{
	if (!TISLISTEMPTY(slist))
	{
		struct TNode *first = slist->tlh_Head.tln_Succ;
		struct TNode *last = slist->tlh_Tail.tln_Pred;
		struct TNode *dlast = dlist->tlh_Tail.tln_Pred;
		first->tln_Pred = dlast;
		last->tln_Succ = (struct TNode *) &dlist->tlh_Tail;
		dlast->tln_Succ = first;
		dlist->tlh_Tail.tln_Pred = last;
	}
}

static struct RectNode *region_allocrectnode(struct RectPool *pool,
	TINT x0, TINT y0, TINT x1, TINT y1)
{
	struct TNode *temp;
	struct RectNode *rn = (struct RectNode *) 
		TREMHEAD(&pool->p_Rects.rl_List, temp);
	if (rn)
	{
		pool->p_Rects.rl_NumNodes--;
		assert(pool->p_Rects.rl_NumNodes >= 0);
	}
	else
		rn = TExecAlloc(pool->p_ExecBase, TNULL, sizeof(struct RectNode));
	if (rn)
	{
		rn->rn_Rect[0] = x0;
		rn->rn_Rect[1] = y0;
		rn->rn_Rect[2] = x1;
		rn->rn_Rect[3] = y1;
	}
	return rn;
}

TLIBAPI void region_initrectlist(struct RectList *rl)
{
	TINITLIST(&rl->rl_List);
	rl->rl_NumNodes = 0;
}

static void region_relinkrects(struct RectList *d, struct RectList *s)
{
	region_relinklist(&d->rl_List, &s->rl_List);
	d->rl_NumNodes += s->rl_NumNodes;
	region_initrectlist(s);	
}

TLIBAPI void region_freerects(struct RectPool *p, struct RectList *list)
{
	struct TNode *temp;
	struct TExecBase *TExecBase = p->p_ExecBase;
	region_relinkrects(&p->p_Rects, list);
	while (p->p_Rects.rl_NumNodes > MAXPOOLNODES)
	{
		TFree(TREMTAIL(&p->p_Rects.rl_List, temp));
		p->p_Rects.rl_NumNodes--;
	}
}

TLIBAPI TBOOL region_insertrect(struct RectPool *pool, struct RectList *list,
	TINT s0, TINT s1, TINT s2, TINT s3)
{
	struct TNode *temp, *next, *node = list->rl_List.tlh_Head.tln_Succ;
	struct RectNode *rn;
	int i;

	#if defined(MERGE_NUMOPS)
	for (i = 0; i < MERGE_NUMOPS && (next = node->tln_Succ); node = next, ++i)
	{
		rn = (struct RectNode *) node;
		if (rn->rn_Rect[1] == s1 && rn->rn_Rect[3] == s3)
		{
			if (rn->rn_Rect[2] + 1 == s0)
			{
				rn->rn_Rect[2] = s2;
				return TTRUE;
			}
			else if (rn->rn_Rect[0] == s2 + 1)
			{
				rn->rn_Rect[0] = s0;
				return TTRUE;
			}
		}
		else if (rn->rn_Rect[0] == s0 && rn->rn_Rect[2] == s2)
		{
			if (rn->rn_Rect[3] + 1 == s1)
			{
				rn->rn_Rect[3] = s3;
				return TTRUE;
			}
			else if (rn->rn_Rect[1] == s3 + 1)
			{
				rn->rn_Rect[1] = s1;
				return TTRUE;
			}
		}
	}
	#endif

	rn = region_allocrectnode(pool, s0, s1, s2, s3);
	if (rn)
	{
		TADDHEAD(&list->rl_List, &rn->rn_Node, temp);
		list->rl_NumNodes++;
		return TTRUE;
	}

	return TFALSE;
}

static TBOOL region_cutrect(struct RectPool *pool, struct RectList *list,
	const RECTINT d[4], const RECTINT s[4])
{
	TINT d0 = d[0];
	TINT d1 = d[1];
	TINT d2 = d[2];
	TINT d3 = d[3];

	if (!REGION_OVERLAPRECT(d, s))
		return region_insertrect(pool, list, d[0], d[1], d[2], d[3]);

	for (;;)
	{
		if (d0 < s[0])
		{
			if (!region_insertrect(pool, list, d0, d1, s[0] - 1, d3))
				break;
			d0 = s[0];
		}

		if (d1 < s[1])
		{
			if (!region_insertrect(pool, list, d0, d1, d2, s[1] - 1))
				break;
			d1 = s[1];
		}

		if (d2 > s[2])
		{
			if (!region_insertrect(pool, list, s[2] + 1, d1, d2, d3))
				break;
			d2 = s[2];
		}

		if (d3 > s[3])
		{
			if (!region_insertrect(pool, list, d0, s[3] + 1, d2, d3))
				break;
		}

		return TTRUE;

	}
	return TFALSE;
}

static TBOOL region_cutrectlist(struct RectPool *pool, struct RectList *inlist,
	struct RectList *outlist, const RECTINT s[4])
{
	TBOOL success = TTRUE;
	struct TNode *next, *node = inlist->rl_List.tlh_Head.tln_Succ;
	for (; success && (next = node->tln_Succ); node = next)
	{
		struct RectNode *rn = (struct RectNode *) node;
		struct RectList temp;
		region_initrectlist(&temp);
		success = region_cutrect(pool, &temp, rn->rn_Rect, s);
		if (success)
		{
			struct TNode *next2, *node2 = temp.rl_List.tlh_Head.tln_Succ;
			for (; success && (next2 = node2->tln_Succ); node2 = next2)
			{
				struct RectNode *rn2 = (struct RectNode *) node2;
				success = region_insertrect(pool, outlist, rn2->rn_Rect[0],
					rn2->rn_Rect[1], rn2->rn_Rect[2], rn2->rn_Rect[3]);
				/* note that if unsuccessful, outlist is unusable as well */
			}
		}
		region_freerects(pool, &temp);
	}
	return success;
}

TLIBAPI TBOOL region_orrectlist(struct RectPool *pool, struct RectList *list, 
	TINT s[4], TBOOL opportunistic)
{
	if (list->rl_NumNodes > 0)
	{
		TINT x0 = s[0];
		TINT y0 = s[1];
		TINT x1 = s[2];
		TINT y1 = s[3];
		TUINT64 area = 0;
		
		struct TNode *next, *node = list->rl_List.tlh_Head.tln_Succ;
		for (; (next = node->tln_Succ); node = next)
		{
			struct RectNode *rn = (struct RectNode *) node;
			TINT *r = rn->rn_Rect;
			if (s[0] >= r[0] && s[1] >= r[1] &&
				s[2] <= r[2] && s[3] <= r[3])
				return TTRUE;
			if (!opportunistic)
				continue;
			area += (r[2] - r[0] + 1) * (r[3] - r[1] + 1);
			x0 = TMIN(x0, r[0]);
			y0 = TMIN(y0, r[1]);
			x1 = TMAX(x1, r[2]);
			y1 = TMAX(y1, r[3]);
		}
		if (opportunistic)
		{
			TUINT64 area2 = (x1 - x0 + 1) * (y1 - y0 + 1);
			if (area2 < OPPORTUNISTIC_MERGE_THRESHOLD ||
				(area * 256 / area2) > OPPORTUNISTIC_MERGE_RATIO)
			{
				/* merge list into a single rectangle */
				TDBPRINTF(TDB_TRACE,("merge %d rects\n",
					list->rl_NumNodes + 1));
				region_freerects(pool, list);
				assert(list->rl_NumNodes == 0);
				return region_insertrect(pool, list, x0, y0, x1, y1);
			}
		}
	}

	struct RectList temp;
	region_initrectlist(&temp);
	if (region_cutrectlist(pool, list, &temp, s))
	{
		if (region_insertrect(pool, &temp, s[0], s[1], s[2], s[3]))
		{
			region_freerects(pool, list);
			region_relinkrects(list, &temp);
			return TTRUE;
		}
	}
	region_freerects(pool, &temp);
	return TFALSE;
}

TLIBAPI TBOOL region_orregion(struct Region *region, 
	struct RectList *list, TBOOL opportunistic)
{
	TBOOL success = TTRUE;
	struct TNode *next, *node = list->rl_List.tlh_Head.tln_Succ;
	for (; success && (next = node->tln_Succ); node = next)
	{
		struct RectNode *rn = (struct RectNode *) node;
		success = region_orrectlist(region->rg_Pool, &region->rg_Rects,
			rn->rn_Rect, opportunistic);
	}
	return success;
}

static TBOOL region_andrect_internal(struct RectList *temp,
	struct Region *region, TINT s[], TINT dx, TINT dy)
{
	struct RectPool *pool = region->rg_Pool;
	struct TNode *next, *node = region->rg_Rects.rl_List.tlh_Head.tln_Succ;
	TBOOL success = TTRUE;
	TINT s0 = s[0] + dx;
	TINT s1 = s[1] + dy;
	TINT s2 = s[2] + dx;
	TINT s3 = s[3] + dy;
	for (; success && (next = node->tln_Succ); node = next)
	{
		struct RectNode *dr = (struct RectNode *) node;
		TINT x0 = dr->rn_Rect[0];
		TINT y0 = dr->rn_Rect[1];
		TINT x1 = dr->rn_Rect[2];
		TINT y1 = dr->rn_Rect[3];
		if (REGION_OVERLAP(x0, y0, x1, y1, s0, s1, s2, s3))
		{
			success = region_insertrect(pool, temp,
				TMAX(x0, s0), TMAX(y0, s1), TMIN(x1, s2), TMIN(y1, s3));
		}
	}
	if (!success)
		region_freerects(pool, temp);
	return success;
}

TLIBAPI TBOOL region_subrect(struct RectPool *pool, struct Region *region, 
	RECTINT s[])
{
	struct RectList r1;
	struct TNode *next, *node;
	TBOOL success = TTRUE;

	region_initrectlist(&r1);
	node = region->rg_Rects.rl_List.tlh_Head.tln_Succ;
	for (; success && (next = node->tln_Succ); node = next)
	{
		struct TNode *next2, *node2;
		struct RectNode *rn = (struct RectNode *) node;
		struct RectList temp;
		
		region_initrectlist(&temp);
		success = region_cutrect(pool, &temp, rn->rn_Rect, s);

		node2 = temp.rl_List.tlh_Head.tln_Succ;
		for (; success && (next2 = node2->tln_Succ); node2 = next2)
		{
			struct RectNode *rn2 = (struct RectNode *) node2;
			success = region_insertrect(pool, &r1, rn2->rn_Rect[0],
				rn2->rn_Rect[1], rn2->rn_Rect[2], rn2->rn_Rect[3]);
		}

		region_freerects(pool, &temp);
	}

	if (success)
	{
		region_freerects(pool, &region->rg_Rects);
		region_relinkrects(&region->rg_Rects, &r1);
	}
	else
		region_freerects(pool, &r1);

	return success;
}

TLIBAPI TBOOL region_subregion(struct RectPool *pool, struct Region *dregion,
	struct Region *sregion)
{
	TBOOL success = TTRUE;
	struct TNode *next, *node = sregion->rg_Rects.rl_List.tlh_Head.tln_Succ;
	for (; success && (next = node->tln_Succ); node = next)
	{
		struct RectNode *rn = (struct RectNode *) node;
		success = region_subrect(pool, dregion, rn->rn_Rect);
	}
	/* note: if unsucessful, dregion is of no use anymore */
	return success;
}

TLIBAPI TBOOL region_overlap(struct RectPool *pool, struct Region *region,
	TINT s[])
{
	struct TNode *next, *node = region->rg_Rects.rl_List.tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
	{
		struct RectNode *rn = (struct RectNode *) node;
		if (REGION_OVERLAPRECT(rn->rn_Rect, s))
			return TTRUE;
	}
	return TFALSE;
}

TLIBAPI TBOOL region_andrect(struct RectPool *pool, struct Region *region,
	TINT s[], TINT dx, TINT dy)
{
	struct RectList temp;
	region_initrectlist(&temp);
	if (region_andrect_internal(&temp, region, s, dx, dy))
	{
		region_freerects(pool, &region->rg_Rects);
		region_relinkrects(&region->rg_Rects, &temp);
		return TTRUE;
	}
	return TFALSE;
}

TLIBAPI TBOOL region_andregion(struct RectPool *pool, struct Region *dregion,
	struct Region *sregion)
{
	struct TNode *next, *node = sregion->rg_Rects.rl_List.tlh_Head.tln_Succ;
	TBOOL success = TTRUE;
	struct RectList temp;
	region_initrectlist(&temp);
	for (; success && (next = node->tln_Succ); node = next)
	{
		struct RectNode *sr = (struct RectNode *) node;
		success = region_andrect_internal(&temp, dregion,
			sr->rn_Rect, 0, 0);
	}
	if (success)
	{
		region_freerects(pool, &dregion->rg_Rects);
		region_relinkrects(&dregion->rg_Rects, &temp);
	}
	/* note: if unsucessful, dregion is of no use anymore */
	return success;
}

TLIBAPI TBOOL region_intersect(TINT *d, TINT *s)
{
	if (REGION_OVERLAP(d[0], d[1], d[2], d[3], s[0], s[1], s[2], s[3]))
	{
		d[0] = TMAX(d[0], s[0]);
		d[1] = TMAX(d[1], s[1]);
		d[2] = TMIN(d[2], s[2]);
		d[3] = TMIN(d[3], s[3]);
		return TTRUE;
	}
	return TFALSE;
}

TLIBAPI TBOOL region_xorrect(struct RectPool *pool, struct Region *region,
	RECTINT s[])
{
	struct TNode *next, *node;
	TBOOL success;
	struct RectList r1, r2;
	
	region_initrectlist(&r1);
	region_initrectlist(&r2);

	success = region_insertrect(pool, &r2, s[0], s[1], s[2], s[3]);

	node = region->rg_Rects.rl_List.tlh_Head.tln_Succ;
	for (; success && (next = node->tln_Succ); node = next)
	{
		struct TNode *next2, *node2;
		struct RectNode *rn = (struct RectNode *) node;
		struct RectList temp;

		region_initrectlist(&temp);
		success = region_cutrect(pool, &temp, rn->rn_Rect, s);

		node2 = temp.rl_List.tlh_Head.tln_Succ;
		for (; success && (next2 = node2->tln_Succ); node2 = next2)
		{
			struct RectNode *rn2 = (struct RectNode *) node2;
			success = region_insertrect(pool, &r1, rn2->rn_Rect[0],
				rn2->rn_Rect[1], rn2->rn_Rect[2], rn2->rn_Rect[3]);
		}
		
		region_freerects(pool, &temp);

		if (success)
		{
			success = region_cutrectlist(pool, &r2, &temp, rn->rn_Rect);
			region_freerects(pool, &r2);
			region_relinkrects(&r2, &temp);
		}
	}

	if (success)
	{
		region_freerects(pool, &region->rg_Rects);
		region_relinkrects(&region->rg_Rects, &r1);
		region_orregion(region, &r2, TFALSE);
		region_freerects(pool, &r2);
	}
	else
	{
		region_freerects(pool, &r1);
		region_freerects(pool, &r2);
	}
	return success;
}

TLIBAPI TBOOL region_isempty(struct RectPool *pool, struct Region *region)
{
	return TISLISTEMPTY(&region->rg_Rects.rl_List);
}

TLIBAPI TBOOL region_getminmax(struct RectPool *pool, struct Region *region,
	TINT *minmax)
{
	if (region_isempty(pool, region))
		return TFALSE;
	TINT minx = 1000000;
	TINT miny = 1000000;
	TINT maxx = 0;
	TINT maxy = 0;
	struct TNode *next, *node = region->rg_Rects.rl_List.tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
	{
		struct RectNode *rn = (struct RectNode *) node;
		minx = TMIN(minx, rn->rn_Rect[0]);
		miny = TMIN(miny, rn->rn_Rect[1]);
		maxx = TMAX(maxx, rn->rn_Rect[2]);
		maxy = TMAX(maxy, rn->rn_Rect[3]);
	}
	minmax[0] = minx;
	minmax[1] = miny;
	minmax[2] = maxx;
	minmax[3] = maxy;
	return TTRUE;
}

TLIBAPI TBOOL region_init(struct RectPool *pool, struct Region *region, 
	TINT *s)
{
	region->rg_Pool = pool;
	region_initrectlist(&region->rg_Rects);
	if (s && !region_insertrect(pool, &region->rg_Rects,
		s[0], s[1], s[2], s[3]))
		return TFALSE;
	return TTRUE;
}

TLIBAPI void region_free(struct RectPool *pool, struct Region *region)
{
	region_freerects(pool, &region->rg_Rects);
}

TLIBAPI struct Region *region_new(struct RectPool *pool, TINT *s)
{
	struct TExecBase *TExecBase = pool->p_ExecBase;
	struct Region *region = TAlloc(TNULL, sizeof(struct Region));
	if (region)
	{
		if (!region_init(pool, region, s))
		{
			TFree(region);
			region = TNULL;
		}
	}
	return region;
}

TLIBAPI void region_destroy(struct RectPool *pool, struct Region *region)
{
	region_freerects(pool, &region->rg_Rects);
	TExecFree(pool->p_ExecBase, region);
}

TLIBAPI TBOOL region_orrect(struct RectPool *pool, struct Region *region,
	TINT s[4], TBOOL opportunistic)
{
	return region_orrectlist(pool, &region->rg_Rects, s, opportunistic);
}

TLIBAPI void region_initpool(struct RectPool *pool, TAPTR TExecBase)
{
	region_initrectlist(&pool->p_Rects);
	pool->p_ExecBase = TExecBase;
}

TLIBAPI void region_destroypool(struct RectPool *pool)
{
	TAPTR TExecBase = pool->p_ExecBase;
	struct TNode *next, *node = pool->p_Rects.rl_List.tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
		TFree(node);
}

TLIBAPI void region_shift(struct Region *region, TINT sx, TINT sy)
{
	struct TNode *next, *node = region->rg_Rects.rl_List.tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
	{
		struct RectNode *rn = (struct RectNode *) node;
		rn->rn_Rect[0] += sx;
		rn->rn_Rect[1] += sy;
		rn->rn_Rect[2] += sx;
		rn->rn_Rect[3] += sy;
	}
}

#endif
