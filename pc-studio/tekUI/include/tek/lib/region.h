#ifndef _TEK_LIB_REGION_H
#define _TEK_LIB_REGION_H

#include <tek/exec.h>

/*
**	region.h - Region library
**	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

typedef TINT RECTINT;

struct Rect
{
	RECTINT r[4];
};

#define REGION_RECT_WIDTH(rect) ((rect)->r[2] - (rect)->r[0] + 1)
#define REGION_RECT_HEIGHT(rect) ((rect)->r[3] - (rect)->r[1] + 1)
#define REGION_RECT_SET(rect, x0, y0, x1, y1) do { \
	(rect)->r[0] = x0; \
	(rect)->r[1] = y0; \
	(rect)->r[2] = x1; \
	(rect)->r[3] = y1; \
} while(0);

#define REGION_POINT_IN_RECT(rect, x, y) ((x) >= (rect)->r[0] && \
	(x) <= (rect)->r[2] && (y) >= (rect)->r[1] && (y) <= (rect)->r[2])

#define REGION_OVERLAP(d0, d1, d2, d3, s0, s1, s2, s3) \
	((s2) >= (d0) && (s0) <= (d2) && (s3) >= (d1) && (s1) <= (d3))
#define REGION_OVERLAPRECT(d, s) REGION_OVERLAP((d)[0], (d)[1], \
	(d)[2], (d)[3], (s)[0], (s)[1], (s)[2], (s)[3])


struct RectList
{
	struct TList rl_List;
	TINT rl_NumNodes;
};

struct RectPool
{
	struct RectList p_Rects;
	struct TExecBase *p_ExecBase;
};

struct Region
{
	struct RectList rg_Rects;
	struct RectPool *rg_Pool;
};

struct RectNode
{
	struct TNode rn_Node;
	RECTINT rn_Rect[4];
};

TLIBAPI TBOOL region_intersect(TINT *d, TINT *s);
TLIBAPI void region_initrectlist(struct RectList *rl);

TLIBAPI TBOOL region_init(struct RectPool *pool, struct Region *region, TINT *s);
TLIBAPI void region_free(struct RectPool *pool, struct Region *region);
TLIBAPI struct Region *region_new(struct RectPool *, TINT *s);
TLIBAPI void region_destroy(struct RectPool *, struct Region *region);
TLIBAPI TBOOL region_overlap(struct RectPool *, struct Region *region, TINT s[]);
TLIBAPI TBOOL region_subrect(struct RectPool *, struct Region *region, TINT s[]);
TLIBAPI TBOOL region_subregion(struct RectPool *, struct Region *dregion, struct Region *sregion);
TLIBAPI TBOOL region_andrect(struct RectPool *, struct Region *region, TINT s[], TINT dx, TINT dy);
TLIBAPI TBOOL region_andregion(struct RectPool *, struct Region *dregion, struct Region *sregion);
TLIBAPI TBOOL region_isempty(struct RectPool *, struct Region *region);
TLIBAPI TBOOL region_orrect(struct RectPool *, struct Region *region, TINT r[], TBOOL opportunistic);
TLIBAPI void region_initpool(struct RectPool *pool, TAPTR TExecBase);
TLIBAPI void region_destroypool(struct RectPool *pool);
TLIBAPI TBOOL region_insertrect(struct RectPool *pool, struct RectList *list, TINT s0, TINT s1, TINT s2, TINT s3);
TLIBAPI void region_freerects(struct RectPool *p, struct RectList *list);
TLIBAPI TBOOL region_orrectlist(struct RectPool *pool, struct RectList *list, TINT s[4], TBOOL opportunistic);
TLIBAPI TBOOL region_xorrect(struct RectPool *pool, struct Region *region, RECTINT s[]);
TLIBAPI TBOOL region_orregion(struct Region *region, struct RectList *list, TBOOL opportunistic);
TLIBAPI TBOOL region_getminmax(struct RectPool *pool, struct Region *region, TINT *minmax);
TLIBAPI void region_shift(struct Region *region, TINT dx, TINT dy);

#endif /* _TEK_LIB_REGION_H */
