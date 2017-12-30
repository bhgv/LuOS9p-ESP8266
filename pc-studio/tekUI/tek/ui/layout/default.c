/*-----------------------------------------------------------------------------
--
--	tek.ui.layout.default
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		[[#ClassOverview]] :
--		[[#tek.class : Class]] /
--		[[#tek.ui.class.layout : Layout]] /
--		DefaultLayout
--
--		This class implements a [[#tek.ui.class.group : Group]]'s ''default''
--		layouting strategy. The standard strategy is to adapt a Group's
--		contents dynamically to the free space available. It takes into
--		account an element's {{HAlign}}, {{VAlign}}, {{Width}}, and {{Height}}
--		attributes.
--
--	OVERRIDES::
--		- Layout:askMinMax()
--		- Layout:layout()
--		- Class.new()
--
-------------------------------------------------------------------------------

module("tek.ui.layout.default", tek.ui.class.layout)
_VERSION = "Default Layout 9.2"
local DefaultLayout = _M

******************************************************************************/

#include <string.h>
#include <tek/lib/tek_lua.h>
#include <tek/lib/tekui.h>
#include <tek/lib/region.h>

/* Name of superclass: */
#define DEFLAYOUT_SUPERCLASS_NAME "tek.ui.class.layout"

/* Name of this class: */
#define DEFLAYOUT_CLASS_NAME "tek.ui.layout.default"

/* Version: */
#define DEFLAYOUT_CLASS_VERSION "Default Layout 9.2"

/* Required tekui version: */
#define DEFLAYOUT_TEKUI_VERSION 112

/*****************************************************************************/

static const char *ALIGN[2][6] =
{
	{ "HAlign", "VAlign", "right", "bottom", "Width", "Height" },
	{ "VAlign", "HAlign", "bottom", "right", "Height", "Width" }
};

static const int INDICES[2][6] =
{
	{ 1, 2, 3, 4, 5, 6 },
	{ 2, 1, 4, 3, 6, 5 },
};

typedef struct { int orientation, width, height; } layout_struct;

typedef struct
{
	int free, i1, i3, n, isgrid;
	RECTINT padding[4];
	RECTINT margin[4];
	RECTINT rect[4];
	RECTINT minmax[4];

} layout;

/*****************************************************************************/

static int layout_getsamesize(lua_State *L, int groupindex, int axis)
{
	int res;
	lua_getfield(L, groupindex, "SameSize");
	if (lua_isboolean(L, -1) && lua_toboolean(L, -1))
		res = 1;
	else
	{
		const char *key = lua_tostring(L, -1);
		res = key && ((axis == 1 && key[0] == 'w') ||
			(axis == 2 && key[0] == 'h'));
	}
	lua_pop(L, 1);
	return res;
}

/*****************************************************************************/

static layout_struct layout_getstructure(lua_State *L, int group)
{
	layout_struct res;
	
	size_t nc;
	int gw, gh;
	lua_getfield(L, group, "Columns");
	gw = lua_isnumber(L, -1) ? lua_tointeger(L, -1) : 0;
	lua_getfield(L, group, "Rows");
	gh = lua_isnumber(L, -1) ? lua_tointeger(L, -1) : 0;
	lua_getfield(L, group, "Children");
#if LUA_VERSION_NUM < 502
	nc = lua_objlen(L, -1);
#else
	nc = lua_rawlen(L, -1);
#endif
	lua_pop(L, 3);
	
	if (gw)
	{
		res.orientation = 1;
		res.width = gw;
		res.height = (nc + gw - 1) / gw;
	}
	else if (gh)
	{
		res.orientation = 2;
		res.width = (nc + gh - 1) / gh;
		res.height = gh;
	}
	else
	{
		const char *key;
		lua_getfield(L, group, "Orientation");
		key = lua_tostring(L, -1);
		if (key && key[0] == 'h') /* "horizontal" */
		{
			res.orientation = 1;
			res.width = nc;
			res.height = 1;
		}
		else
		{
			res.orientation = 2;
			res.width = 1;
			res.height = nc;
		}
		lua_pop(L, 1);
	}
	
	return res;
}

/*****************************************************************************/

static void layout_calcweights(lua_State *L, layout_struct *lstruct)
{
	int cidx = 1;
	int y, x;
	lua_getfield(L, 1, "Weights");
	lua_newtable(L);
	lua_newtable(L);
	lua_getfield(L, 2, "Children");
	/* weights, wx, wy, children */
	for (y = 1; y <= lstruct->height; ++y)
	{
		for (x = 1; x <= lstruct->width; ++x)
		{
			lua_rawgeti(L, -1, cidx);
			/* weights, wx, wy, children, c */
			if (lua_isnil(L, -1))
			{
				lua_pop(L, 1);
				break;
			}
			lua_getfield(L, -1, "Invisible");
			TBOOL invisible = lua_toboolean(L, -1);
			lua_pop(L, 1);
			if (!invisible)
			{
				lua_getfield(L, -1, "Weight");
				/* weights, wx, wy, children, c, weight */
				if (lua_isnumber(L, -1))
				{
					lua_Integer w = lua_tonumber(L, -1);
					lua_rawgeti(L, -5, x);
					/* weights, wx, wy, children, c, weight, wx[x] */
					lua_rawgeti(L, -5, y);
					/* weights, wx, wy, children, c, weight, wx[x], wy[y] */
					lua_pushinteger(L, luaL_optinteger(L, -2, 0) + w);
					/* weights, wx, wy, children, c, weight, wx[x], wy[y], wx' */
					lua_rawseti(L, -8, x);
					/* weights, wx, wy, children, c, weight, wx[x], wy[y] */
					lua_pushinteger(L, luaL_optinteger(L, -1, 0) + w);
					/* weights, wx, wy, children, c, weight, wx[x], wy[y], wy' */
					lua_rawseti(L, -7, y);
					/* weights, wx, wy, children, c, weight, wx[x], wy[y] */
					lua_pop(L, 2);
				}
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
			/* weights, wx, wy, children */
			cidx++;
		}
	}
	lua_pop(L, 1);
	/* weights, wx, wy */
	lua_rawseti(L, -3, 2);
	/* weights, wx */
	lua_rawseti(L, -2, 1);
	/* weights */
	lua_pop(L, 1);
}

/*****************************************************************************/
/*
**	list = layoutAxis(self, group, layout)
*/

static int layout_layoutaxis(lua_State *L)
{
	layout *layout = lua_touserdata(L, 3);
	int free = layout->free;
	int i1 = layout->i1;
	int i3 = layout->i3;
	int n = layout->n;
	RECTINT *padding = layout->padding;
	RECTINT *margin = layout->margin;
	RECTINT *minmax = layout->minmax;

	int it = 0;
	size_t len;
	int ssb, ssn = 0;
	int i;

	/**
	**	local fw, tw = 0, 0
	**	local list = { }
	**	local tmm = self.TempMinMax
	**	local wgh = group.Weights
	**	for i = 1, n do
	**		local mins, maxs = tmm[i1][i], tmm[i3][i]
	**		local free = maxs and (maxs > mins)
	**		local weight = wgh[i1][i]
	**		list[i] = { free, mins, maxs, weight, nil }
	**		if free then
	**			if weight then
	**				tw = tw + weight
	**			else
	**				fw = fw + 0x100
	**			end
	**		end
	**	end
	**/

	lua_Integer fw0 = 0;
	lua_Integer tw0 = 0;
	lua_Number fw;
	lua_Number tw;

	lua_createtable(L, n, 0);
	lua_getfield(L, 1, "TempMinMax");
	lua_rawgeti(L, -1, i1);
	lua_rawgeti(L, -2, i3);
	/* s: list, tmm, tmm[i1], tmm[i3] */
	lua_remove(L, -3);
	/* s: list, tmm[i1], tmm[i3] */
	
	lua_getfield(L, 1, "Weights");
	/* s: list, tmm[i1], tmm[i3], weights */
	lua_rawgeti(L, -1, i1);
	/* s: list, tmm[i1], tmm[i3], weights, wgh[i1] */
	lua_remove(L, -2);

	for (i = 1; i <= n; ++i)
	{
		int free;
		lua_rawgeti(L, -3, i);
		/* s: l, tmm[i1], tmm[i3], wgh[i1], mins */
		lua_rawgeti(L, -3, i);
		/* s: l, tmm[i1], tmm[i3], wgh[i1], mins, maxs */
		lua_rawgeti(L, -3, i);
		/* s: l, tmm[i1], tmm[i3], wgh[i1], mins, maxs, weight */
		free = lua_toboolean(L, -2) ?
			lua_tointeger(L, -2) > lua_tointeger(L, -3) : 0;
		lua_createtable(L, 5, 0);
		/* s: l, tmm[i1], tmm[i3], wgh[i1], mins, maxs, weight, t */
		lua_pushboolean(L, free);
		lua_rawseti(L, -2, 1);
		lua_pushvalue(L, -4);
		lua_rawseti(L, -2, 2);
		lua_pushvalue(L, -3);
		lua_rawseti(L, -2, 3);
		lua_pushvalue(L, -2);
		lua_rawseti(L, -2, 4);
		lua_pushnil(L);
		lua_rawseti(L, -2, 5);
		/* s: l, tmm[i1], tmm[i3], wgh[i1], mins, maxs, weight, t */
		lua_rawseti(L, -8, i);
		/* s: l, tmm[i1], tmm[i3], wgh[i1], mins, maxs, weight */
		if (free)
		{
			if (lua_toboolean(L, -1))
				tw0 += lua_tointeger(L, -1);
			else
				fw0 += 0x100;
		}
		lua_pop(L, 3);
	}
	lua_pop(L, 3);

	/**
	**	if tw < 0x10000 then
	**		if fw == 0 then
	**			tw = 0x10000
	**		else
	**			fw, tw = (0x10000 - tw) * 0x100 / fw, 0x10000
	**		end
	**	else
	**		fw = 0
	**	end
	**/

	if (tw0 < 0x10000)
	{
		if (fw0 == 0)
			tw0 = 0x10000;
		else
		{
			fw = 0x10000;
			fw -= tw0;
			fw *= 0x100;
			fw0 = fw / fw0;
			tw0 = 0x10000;
		}
	}
	else
		fw0 = 0;

	tw = tw0 / 0x100;
	fw = fw0 / 0x100;

	/**
	**	local mab = group.MarginAndBorder
	**	local ss = group:getSameSize(i1) and
	**		(group.MinMax[i1] - mab[i1] - mab[i3] - pad[i1] - pad[i3]) / n
	**/

	ssb = layout_getsamesize(L, 2, i1);
	if (ssb)
	{
		ssn = minmax[i1 - 1];
		ssn -= margin[i1 - 1];
		ssn -= margin[i3 - 1];
		ssn -= padding[i1 - 1];
		ssn -= padding[i3 - 1];
		ssn /= n;
	}

	lua_getglobal(L, "table");
	lua_getfield(L, -1, "insert");
	lua_getfield(L, -2, "remove");
	lua_remove(L, -3);
	/* s: list, insert, remove */

	/**
	**	local e = { unpack(list) }
	**/

#if LUA_VERSION_NUM < 502
	len = lua_objlen(L, -3);
#else
	len = lua_rawlen(L, -3);
#endif
	lua_createtable(L, len, 0);
	/* s: list, insert, remove, e */
	for (i = 1; i <= (int) len; ++i)
	{
		lua_rawgeti(L, -4, i);
		lua_rawseti(L, -2, i);
	}

	/**
	**	local it = 0
	**	while #e > 0 do
	**/

#if LUA_VERSION_NUM < 502
	while ((len = lua_objlen(L, -1)) > 0)
#else
	while ((len = lua_rawlen(L, -1)) > 0)
#endif
	{
		/**
		**	local rest = free
		**	local newfree = free
		**	it = it + 1
		**	local e2 = { }
		**/

		lua_Integer rest = free;
		lua_Integer newfree = free;
		it++;

		lua_createtable(L, len, 0);
		/* s: insert, remove, e, e2 */

		/**
		**	repeat
		**		...
		**	until #e == 0
		**/

		do
		{
			lua_Integer olds, news, ti;

			/**
			**	delta = 0
			**
			**	c = remove(e, 1)
			**
			**	if c[1] then -- free
			**		if c[4] then -- weight
			**			delta = free * (c[4] / 0x100) * (tw / 0x100) / 0x10000
			**		else
			**			delta = free * 0x100 * (fw / 0x100) / 0x10000
			**		end
			**		delta = floor(delta)
			**	end
			**/

			lua_Integer delta = 0;

			/* s: insert, remove, e, e2 */
			lua_pushvalue(L, -3);
			/* s: insert, remove, e, e2, remove */
			lua_pushvalue(L, -3);
			/* s: insert, remove, e, e2, remove, e */
			lua_pushinteger(L, 1);
			/* s: insert, remove, e, e2, remove, e, 1 */
			lua_call(L, 2, 1);
			/* s: insert, remove, e, e2, c */

			lua_rawgeti(L, -1, 1);
			/* s: insert, remove, e, e2, c, c[1] */
			if (lua_toboolean(L, -1))
			{
				lua_Number t;
				lua_rawgeti(L, -2, 4);
				/* s: insert, remove, e, e2, c, c[1], c[4] */
				if (lua_toboolean(L, -1))
				{
					t = lua_tointeger(L, -1);
					t /= 0x100;
					t *= tw;
					t *= free;
				}
				else
				{
					t = free;
					t *= 0x100;
					t *= fw;
				}
				t /= 0x10000;
				delta = t;
				lua_pop(L, 1);
			}
			/* s: insert, remove, e, e2, c, c[1] */

			/**
			**	if delta == 0 and it > 1 then
			**		delta = rest
			**	end
			**/

			if (delta == 0 && it > 1)
				delta = rest;

			/**
			**	olds = c[5] or ss or c[2] -- size, mins
			**	news = max(olds + delta, ss or c[2]) -- mins
			**	if not (ss and isgrid) and c[3] and news > c[3] then -- maxs
			**		news = c[3] -- maxs
			**	end
			**	c[5] = news
			**/

			lua_rawgeti(L, -2, 5);
			lua_rawgeti(L, -3, 2);
			/* s: insert, remove, e, e2, c, c[1], c[5], c[2] */

			if (lua_toboolean(L, -2))
				olds = lua_tointeger(L, -2);
			else if (ssb)
				olds = ssn;
			else
				olds = lua_tointeger(L, -1);
			/* s: insert, remove, e, e2, c, c[1], c[5], c[2] */

			ti = ssb ? ssn : lua_tointeger(L, -1);
			news = TMAX(olds + delta, ti);

			lua_rawgeti(L, -4, 3);
			/* s: insert, remove, e, e2, c, c[1], c[5], c[2], c[3] */
			if (!(ssb && layout->isgrid) && lua_toboolean(L, -1) && 
				news > lua_tointeger(L, -1))
				news = lua_tointeger(L, -1);

			lua_pushinteger(L, news);
			lua_rawseti(L, -6, 5);

			/**
			**	delta = news - olds
			**	newfree = newfree - delta
			**	rest = rest - delta
			**/

			delta = news - olds;
			newfree -= delta;
			rest -= delta;

			/**
			**	if not c[3] or c[3] >= HUGE or c[5] < c[3] then -- maxs
			**		insert(e2, c)
			**	end
			**/

			if (!lua_toboolean(L, -1) || lua_tointeger(L, -1) >= TEKUI_HUGE ||
				lua_tointeger(L, -3) < lua_tointeger(L, -1))
			{
				/* redo in next iteration: */
				/* s: ins, rem, e, e2, c, c[1], c[5], c[2], c[3] */
				lua_pushvalue(L, -9);
				/* s: ins, rem, e, e2, c, c[1], c[5], c[2], c[3], ins */
				lua_pushvalue(L, -7);
				/* s: ins, rem, e, e2, c, c[1], c[5], c[2], c[3], ins, e2 */
				lua_pushvalue(L, -7);
				/* s: ins, rem, e, e2, c, c[1], c[5], c[2], c[3], ins, e2, c */
				lua_call(L, 2, 0);
				/* s: insert, remove, e, e2, c, c[1], c[5], c[2], c[3] */
			}

			lua_pop(L, 5);
			/* s: insert, remove, e, e2 */
#if LUA_VERSION_NUM < 502
		} while (lua_objlen(L, -2) > 0);
#else
		} while (lua_rawlen(L, -2) > 0);
#endif
		/**
		**	if newfree < 1 then
		**		break
		**	end
		**
		**	free = newfree
		**	e = e2
		**/

		free = newfree;
		if (free < 1)
		{
			lua_pop(L, 1);
			break;
		}

		/* s: insert, remove, e, e2 */
		lua_replace(L, -2);
		/* s: insert, remove, e */
	}

	lua_pop(L, 3);
	/* s: list */
	return 1;
}

/*****************************************************************************/
/*
**	layout(self, group, r1, r2, r3, r4, markdamage)
*/

static int layout_layout(lua_State *L)
{
	layout layout;

	/**
	**	local ori, gs1, gs2 = getStructure(group)
	**	if gs1 > 0 and gs2 > 0 then
	**/

	layout_struct lstruct = layout_getstructure(L, 2);
	int ori = lstruct.orientation;
	int gs1 = lstruct.width;
	int gs2 = lstruct.height;

	if (gs1 > 0 && gs2 > 0)
	{
		/**
		**	local isgrid = gs1 > 1 and gs2 > 1
		**	local i1, i2, i3, i4, i5, i6 = unpack(INDICES[ori])
		**
		**	local c, isz, osz, m3, m4, mm, a
		**	local cidx = 1
		**	local A = ALIGN[ori]
		**
		**	if i1 == 2 then
		**		gs1, gs2 = gs2, gs1
		**		r1, r2, r3, r4 = r2, r1, r4, r3
		**	end
		**
		**	local gm = { group:getMargin() }
		**	local gr = { group:getRect() }
		**	local gp = { group:getPadding() }
		**	local goffs = gm[i1] + gp[i1]
		**	local xywh = self.XYWH
		**	local minmax = { group.MinMax:get() }
		**
		**	if group.Flags:checkClear(FL_STRUCTURE) then
		**		self:calcWeights(group)
		**	end
		**/

		const int *I = INDICES[ori - 1];
		int i1 = I[0], i2 = I[1], i3 = I[2], i4 = I[3], i5 = I[4], i6 = I[5];

		lua_Integer isz, osz, oszmax, t, iidx;
		lua_Integer m3, m4, oidx, goffs;
		lua_Integer r1, r2, r3, r4;
		lua_Integer cidx = 1;

		const char **A = ALIGN[ori - 1], *s;

		layout.isgrid = (gs1 > 1) && (gs2 > 1);

		if (i1 == 2)
		{
			r2 = lua_tointeger(L, 3);
			r1 = lua_tointeger(L, 4);
			r4 = lua_tointeger(L, 5);
			r3 = lua_tointeger(L, 6);
			t = gs1;
			gs1 = gs2;
			gs2 = t;
		}
		else
		{
			r1 = lua_tointeger(L, 3);
			r2 = lua_tointeger(L, 4);
			r3 = lua_tointeger(L, 5);
			r4 = lua_tointeger(L, 6);
		}
		
		lua_getfield(L, 2, "getMargin");
		lua_pushvalue(L, 2);
		lua_call(L, 1, 4);
		layout.margin[0] = lua_tointeger(L, -4);
		layout.margin[1] = lua_tointeger(L, -3);
		layout.margin[2] = lua_tointeger(L, -2);
		layout.margin[3] = lua_tointeger(L, -1);
		lua_pop(L, 4);
		
		lua_getfield(L, 2, "getRect");
		lua_pushvalue(L, 2);
		lua_call(L, 1, 4);
		layout.rect[0] = lua_tointeger(L, -4);
		layout.rect[1] = lua_tointeger(L, -3);
		layout.rect[2] = lua_tointeger(L, -2);
		layout.rect[3] = lua_tointeger(L, -1);
		lua_pop(L, 4);
		
		lua_getfield(L, 2, "getPadding");
		lua_pushvalue(L, 2);
		lua_call(L, 1, 4);
		layout.padding[0] = lua_tointeger(L, -4);
		layout.padding[1] = lua_tointeger(L, -3);
		layout.padding[2] = lua_tointeger(L, -2);
		layout.padding[3] = lua_tointeger(L, -1);
		lua_pop(L, 4);
		
		lua_getfield(L, 2, "MinMax");
		lua_getfield(L, -1, "get");
		lua_pushvalue(L, -2);
		lua_call(L, 1, 4);
		layout.minmax[0] = lua_tointeger(L, -4);
		layout.minmax[1] = lua_tointeger(L, -3);
		layout.minmax[2] = lua_tointeger(L, -2);
		layout.minmax[3] = lua_tointeger(L, -1);
		lua_pop(L, 5);
		
		goffs = layout.margin[i1 - 1] + layout.padding[i1 - 1];
		
		lua_createtable(L, 6, 0);
		/* s: xywh */

		lua_getfield(L, 2, "Flags");
		if (lua_tointeger(L, -1) & TEKUI_FL_CHANGED)
			layout_calcweights(L, &lstruct);
		lua_pop(L, 1);
		
		/**
		**	local olist = self:layoutAxis(group,
		**		r4 - r2 + 1 - group.MinMax[i2], i2, i4, gs2, isgrid, gp, gm, minmax)
		**
		**	local ilist = self:layoutAxis(group,
		**		r3 - r1 + 1 - group.MinMax[i1], i1, i3, gs1, isgrid, gp, gm, minmax)
		**/

		/* layout on outer axis: */
		layout.free = r4 - r2 + 1 - layout.minmax[i2 - 1];
		layout.i1 = i2;
		layout.i3 = i4;
		layout.n = gs2;
		lua_getfield(L, 1, "layoutAxis");
		lua_pushvalue(L, 1);
		lua_pushvalue(L, 2);
		lua_pushlightuserdata(L, &layout);
		/* s: xywh, layoutAxis, self, group, layout */	
		lua_call(L, 3, 1);
		/* s: xywh, olist */
		
		/* layout on inner axis: */
		layout.free = r3 - r1 + 1 - layout.minmax[i1 - 1];
		layout.i1 = i1;
		layout.i3 = i3;
		layout.n = gs1;
		lua_getfield(L, 1, "layoutAxis");
		lua_pushvalue(L, 1);
		lua_pushvalue(L, 2);
		lua_pushlightuserdata(L, &layout);
		/* s: xywh, olist, layoutAxis, self, group, layout */
		lua_call(L, 3, 1);
		/* s: xywh, olist, ilist */

		/**
		**	local children = group.Children
		**	local oszmax = gr[i4] - gr[i2] + 1 - gp[i2] - gp[i4]
		**	xywh[i6] = r2 + mab[i2] + gp[i2]
		**/

		lua_getfield(L, 2, "Children");
		/* s: xywh, olist, ilist, children */

		/* size on outer axis: */
		oszmax = layout.rect[i4 - 1] - layout.rect[i2 - 1] + 1 -
			layout.padding[i2 - 1] - layout.padding[i4 - 1];

		/* starting position on outer axis: */
		lua_pushinteger(L,
			r2 + layout.padding[i2 - 1] + layout.margin[i2 - 1]);
		lua_rawseti(L, -5, i6);

		/**
		**	for oidx = 1, gs2 do
		**		if gs2 > 1 then
		**			oszmax = olist[oidx][5]
		**		end
		**		xywh[i5] = r1 + goffs
		**/

		/* loop outer axis: */
		for (oidx = 1; oidx <= gs2; ++oidx)
		{
			if (gs2 > 1)
			{
				lua_rawgeti(L, -3, oidx);
				lua_rawgeti(L, -1, 5);
				/* s: xywh, olist, ilist, children, olist[oidx], olist[oidx][5] */
				oszmax = lua_tointeger(L, -1);
				lua_pop(L, 2);
			}

			/* starting position on inner axis: */
			lua_pushinteger(L, r1 + goffs);
			/* s: xywh, olist, ilist, children, val */
			lua_rawseti(L, -5, i5);

			/**
			**	for iidx = 1, gs1 do
			**		c = children[cidx]
			**		if not c then
			**			return
			**		end
			**/

			/* loop inner axis: */
			for (iidx = 1; iidx <= gs1; ++iidx)
			{
				/* s: xywh, olist, ilist, children */
				lua_rawgeti(L, -1, cidx);
				/* s: xywh, olist, ilist, children, c */
				if (!lua_toboolean(L, -1))
				{
					lua_pop(L, 5);
					return 0;
				}
				
				lua_getfield(L, -1, "Invisible");
				TBOOL invisible = lua_toboolean(L, -1);
				lua_pop(L, 1);
				if (!invisible)
				{
					/**
					**	xywh[1] = xywh[5]
					**	xywh[2] = xywh[6]
					**	mm = c.MinMax
					**	m3, m4 = mm[i3], mm[i4]
					**	isz = ilist[iidx][5] -- size
					**/

					/* x0, y0 of child rectangle: */
					lua_rawgeti(L, -5, 5);
					lua_rawseti(L, -6, 1);
					lua_rawgeti(L, -5, 6);
					lua_rawseti(L, -6, 2);

					/* element minmax: */
					lua_getfield(L, -1, "MinMax");
					/* s: mm */
					lua_getfield(L, -1, "get");
					/* s: mm, get() */
					lua_pushvalue(L, -2);
					/* s: mm, get(), mm */
					lua_call(L, 1, 4);
					/* s: mm, mm1, mm2, mm3, mm4 */
					layout.minmax[0] = lua_tointeger(L, -4);
					layout.minmax[1] = lua_tointeger(L, -3);
					layout.minmax[2] = lua_tointeger(L, -2);
					layout.minmax[3] = lua_tointeger(L, -1);
					lua_pop(L, 5);
					m3 = layout.minmax[i3 - 1];
					m4 = layout.minmax[i4 - 1];

					/* inner size: */
					lua_pushvalue(L, -3);
					/* s: xywh, olist, ilist, children, c, ilist */
					lua_rawgeti(L, -1, iidx);
					/* s: xywh, olist, ilist, children, c, ilist, ilist[iidx] */
					lua_rawgeti(L, -1, 5);
					/* s: xywh, olist, ilist, children, c, ilist, ilist[iidx], ilist[iidx][5] */
					isz = lua_tointeger(L, -1);
					lua_pop(L, 3);
					/* s: xywh, olist, ilist, children, c */

					/**
					**	a = c:getAttr(A[5])
					**	if a == "free" or a == "fill" then
					**		m3 = gr[i3] - gr[i1] + 1 - gp[i1] - gp[i3]
					**	end
					**/
					
					lua_getfield(L, -1, "getAttr");
					lua_pushvalue(L, -2);
					lua_pushstring(L, A[4]);
					lua_call(L, 2, 1);
					/* s: xywh, olist, ilist, children, c, c[A[5]] */
					s = lua_tostring(L, -1);
					if (s)
					{
						if (s[0] == 'f') /* "free" or "fill" */
						{
							m3 = layout.rect[i3 - 1] + 1;
							m3 -= layout.rect[i1 - 1];
							m3 -= layout.padding[i1 - 1];
							m3 -= layout.padding[i3 - 1];
						}
					}
					lua_pop(L, 1);
					/* s: xywh, olist, ilist, children, c */

					/**
					** if m3 < isz then
					**		a = c:getAttr(A[1])
					**		if a == "center" then
					**			xywh[i1] = xywh[i1] + floor((isz - m3) / 2)
					**		elseif a == A[3] then
					**			-- opposite side:
					**			xywh[i1] = xywh[i1] + isz - m3
					**		end
					**		isz = m3
					**	end
					**/

					if (m3 < isz)
					{
						lua_getfield(L, -1, "getAttr");
						lua_pushvalue(L, -2);
						lua_pushstring(L, A[0]);
						lua_call(L, 2, 1);
						/* s: xywh, olist, ilist, children, c, c[A[1]] */
						s = lua_tostring(L, -1);
						if (s)
						{
							if (strcmp(s, "center") == 0)
							{
								lua_rawgeti(L, -6, i1);
								/* s: xywh, olist, ilist, children, c, c[A[1]], xywh[i1] */
								t = lua_tointeger(L, -1);
								t += (isz - m3) / 2;
								lua_pushinteger(L, t);
								lua_rawseti(L, -8, i1);
								lua_pop(L, 1);
							}
							else if (strcmp(s, A[2]) == 0)
							{
								/* opposite side: */
								lua_rawgeti(L, -6, i1);
								/* s: xywh, olist, ilist, children, c, c[A[1]], xywh[i1] */
								t = lua_tointeger(L, -1);
								t += isz - m3;
								lua_pushinteger(L, t);
								lua_rawseti(L, -8, i1);
								lua_pop(L, 1);
							}
							/* s: xywh, olist, ilist, children, c, c[A[1]] */
						}
						isz = m3;
						lua_pop(L, 1);
						/* s: xywh, olist, ilist, children, c */
					}

					/**
					**	a = c:getAttr(A[6])
					**	if a == "fill" or a == "free" then
					**		osz = oszmax
					**	else
					**		osz = min(olist[oidx][5], m4)
					**		-- align if element does not fully occupy outer size:
					**		if osz < oszmax then
					**			a = c:getAttr(A[2])
					**			if a == "center" then
					**				xywh[i2] = xywh[i2] + floor((oszmax - osz) / 2)
					**			elseif a == A[4] then
					**				-- opposite side:
					**				xywh[i2] = xywh[i2] + oszmax - osz
					**			end
					**		end
					**	end
					**/

					/* outer size: */
					lua_getfield(L, -1, "getAttr");
					lua_pushvalue(L, -2);
					lua_pushstring(L, A[5]);
					lua_call(L, 2, 1);
					/* s: xywh, olist, ilist, children, c, c[A[6]] */
					s = lua_tostring(L, -1);
					if (s && s[0] == 'f') /* "free" or "fill" */
						osz = oszmax;
					else
					{
						lua_rawgeti(L, -5, oidx);
						/* s: xywh, olist, ilist, children, c, c[A[6]], olist[oidx] */
						lua_rawgeti(L, -1, 5);
						/* s: xywh, olist, ilist, children, c, c[A[6]], olist[oidx], olist[oidx][5] */
						osz = lua_tointeger(L, -1);
						osz = TMIN(osz, m4);
						lua_pop(L, 2);
						/* align if element does not fully occupy outer size: */
						/* s: xywh, olist, ilist, children, c, c[A[6]] */
						if (osz < oszmax)
						{
							lua_getfield(L, -2, "getAttr");
							lua_pushvalue(L, -3);
							lua_pushstring(L, A[1]);
							lua_call(L, 2, 1);
							/* s: xywh, olist, ilist, children, c, c[A[6]], c[A[2]] */
							s = lua_tostring(L, -1);
							if (s)
							{
								if (strcmp(s, "center") == 0)
								{
									lua_rawgeti(L, -7, i2);
									/* s: xywh, olist, ilist, children, c, c[A[6]], c[A[2]], xywh[i2] */
									t = lua_tointeger(L, -1);
									t += (oszmax - osz) / 2;
									lua_pushinteger(L, t);
									lua_rawseti(L, -9, i2);
									lua_pop(L, 1);
								}
								else if (strcmp(s, A[3]) == 0)
								{
									/* opposite side: */
									lua_rawgeti(L, -7, i2);
									/* s: xywh, olist, ilist, children, c, c[A[6]], c[A[2]], xywh[i2] */
									t = lua_tointeger(L, -1);
									t += oszmax - osz;
									lua_pushinteger(L, t);
									lua_rawseti(L, -9, i2);
									lua_pop(L, 1);
								}
							}
							lua_pop(L, 1);
							/* s: xywh, olist, ilist, children, c, c[A[6]] */
						}
					}
					lua_pop(L, 1);
					/* s: xywh, olist, ilist, children, c */

					/**
					**	xywh[i3] = xywh[i1] + isz - 1
					**	xywh[i4] = xywh[i2] + osz - 1
					**/

					/* x1, y1 of child rectangle: */
					lua_rawgeti(L, -5, i1);
					/* s: xywh, olist, ilist, children, c, xywh[i1] */
					t = lua_tointeger(L, -1);
					lua_pushinteger(L, t + isz - 1);
					/* s: xywh, olist, ilist, children, c, xywh[i1], val */
					lua_rawseti(L, -7, i3);
					lua_pop(L, 1);

					lua_rawgeti(L, -5, i2);
					/* s: xywh, olist, ilist, children, c, xywh[i2] */
					t = lua_tointeger(L, -1);
					lua_pushinteger(L, t + osz - 1);
					lua_rawseti(L, -7, i4);
					lua_pop(L, 1);
					/* s: xywh, olist, ilist, children, c */

					/**
					**	c:layout(xywh[1], xywh[2], xywh[3], xywh[4], markdamage)
					**	c:punch(group.FreeRegion)
					**/

					/* enter recursion: */
					lua_getfield(L, -1, "layout");
					lua_pushvalue(L, -2);
					/* s: xywh, olist, ilist, children, c, c.layout, c */
					lua_rawgeti(L, -7, 1);
					lua_rawgeti(L, -8, 2);
					lua_rawgeti(L, -9, 3);
					lua_rawgeti(L, -10, 4);
					lua_pushvalue(L, 7);
					/* s: xywh, olist, ilist, children, c, c.layout, c, xywh[1], xywh[2], xywh[3], xywh[4], markdamage */
					lua_call(L, 6, 0);
					/* s: xywh, olist, ilist, children, c */

					/* punch a hole for the element into the background: */
					lua_getfield(L, -1, "punch");
					lua_pushvalue(L, -2);
					lua_getfield(L, 2, "FreeRegion");
					/* s: xywh, olist, ilist, children, c, c.punch, c, group.FreeRegion */
					lua_call(L, 2, 0);
					/* s: xywh, olist, ilist, children, c */

					/**
					**	xywh[i5] = xywh[i5] + ilist[iidx][5] -- size
					**	cidx = cidx + 1
					**/

					/* update x0: */
					lua_rawgeti(L, -5, i5);
					/* s: xywh, olist, ilist, children, c, xywh[i5] */
					lua_rawgeti(L, -4, iidx);
					lua_rawgeti(L, -1, 5);
					/* s: xywh, olist, ilist, children, c, xywh[i5], ilist[iidx], ilist[iidx][5] */
					t = lua_tointeger(L, -3);
					t += lua_tointeger(L, -1);
					lua_pushinteger(L, t);
					/* s: xywh, olist, ilist, children, c, xywh[i5], ilist[iidx], ilist[iidx][5], val */
					lua_rawseti(L, -9, i5);
					/* s: xywh, olist, ilist, children, c, xywh[i5], ilist[iidx], ilist[iidx][5] */
					lua_pop(L, 3);
					/* s: xywh, olist, ilist, children */
				}
				lua_pop(L, 1);
				
				/* next child index: */
				cidx++;
			}

			/**
			**	xywh[i6] = xywh[i6] + olist[oidx][5] -- size
			**/

			/* update y0: */
			lua_rawgeti(L, -4, i6);
			/* s: xywh, olist, ilist, children, xywh[i5] */
			lua_rawgeti(L, -4, oidx);
			lua_rawgeti(L, -1, 5);
			/* s: xywh, olist, ilist, children, xywh[i5], olist[oidx], olist[oidx][5] */
			t = lua_tointeger(L, -3);
			t += lua_tointeger(L, -1);
			lua_pushinteger(L, t);
			/* s: xywh, olist, ilist, children, xywh[i5], olist[oidx], olist[oidx][5], val */
			lua_rawseti(L, -8, i6);
			/* s: xywh, olist, ilist, children, xywh[i5], olist[oidx], olist[oidx][5] */
			lua_pop(L, 3);
			/* s: xywh, olist, ilist, children */
		}

		lua_pop(L, 4);
	
	}
	return 0;
}

/*****************************************************************************/
/*
**	m1, m2, m3, m4 = askMinMax(self, group, m1, m2, m3, m4)
*/

static int layout_askMinMax(lua_State *L)
{
	/**
	**	local m = self.TempRect
	**	m[1], m[2], m[3], m[4] = 0, 0, 0, 0
	**	local ori, gw, gh = group:getStructure()
	**	if gw > 0 and gh > 0 then
	**/

	int m[5] = { 0, 0, 0, 0, 0 };
	layout_struct lstruct = layout_getstructure(L, 2);
	int ori = lstruct.orientation;
	int gw = lstruct.width;
	int gh = lstruct.height;

	if (gw > 0 && gh > 0)
	{
		/**
		**	local cidx = 1
		**	local gss = group:getSameSize(ori)
		**	local minx, miny, maxx, maxy = { }, { }, { }, { }
		**	local tmm = { minx, miny, maxx, maxy }
		**	self.TempMinMax = tmm
		**	local children = self.Children
		**
		**	for y = 1, gh do
		**		for x = 1, gw do
		**/

		int i1, gs, y, x;
		int cidx = 1;
		
		lua_createtable(L, 4, 0);
		lua_pushvalue(L, -1);
		lua_setfield(L, 1, "TempMinMax");
		lua_createtable(L, gw, 0);
		lua_createtable(L, gh, 0);
		lua_createtable(L, gw, 0);
		lua_createtable(L, gh, 0);
		lua_getfield(L, 2, "Children");

		/* s: tmm, minx, miny, maxx, maxy, children */

		for (y = 1; y <= gh; ++y)
		{
			for (x = 1; x <= gw; ++x)
			{
				int mm1, mm2, mm3, mm4;
				int minxx, minyy;
				const char *s;
				
				/**
				**	c = group.Children[cidx]
				**	if not c then
				**		break
				**	end
				**	cidx = cidx + 1
				**/

				lua_rawgeti(L, -1, cidx);
				/* s: tmm, minx, miny, maxx, maxy, children, c */

				if (!lua_toboolean(L, -1))
				{
					lua_pop(L, 1);
					break;
				}
				cidx++;
				
				lua_getfield(L, -1, "Invisible");
				TBOOL invisible = lua_toboolean(L, -1);
				lua_pop(L, 1);
				if (!invisible)
				{
					/**
					**	mm1, mm2, mm3, mm4 = c:askMinMax(m1, m2, m3, m4)
					**/

					lua_getfield(L, -1, "askMinMax");
					lua_pushvalue(L, -2);
					lua_pushvalue(L, 3);
					lua_pushvalue(L, 4);
					lua_pushvalue(L, 5);
					lua_pushvalue(L, 6);
					/* s: c, c.askMinMax, c, m1, m2, m3, m4 */
					lua_call(L, 5, 4);
					/* s: c, mm1, mm2, mm3, mm4 */
					mm1 = lua_tointeger(L, -4);
					mm2 = lua_tointeger(L, -3);
					mm3 = lua_tointeger(L, -2);
					mm4 = lua_tointeger(L, -1);
					lua_pop(L, 4);
					/* s: c */

					/**
					**	local cw = c:getAttr("Width")
					**	if cw == "fill" then
					**		mm3 = nil
					**	elseif cw == "free" then
					**		mm3 = ui.HUGE
					**	end
					**/

					lua_getfield(L, -1, "getAttr");
					lua_pushvalue(L, -2);
					lua_pushstring(L, "Width");
					lua_call(L, 2, 1);
					/* s: c, c.Width */
					s = lua_tostring(L, -1);
					if (s)
					{
						if (strcmp(s, "fill") == 0)
							mm3 = -1; /* nil */
						else if (strcmp(s, "free") == 0)
							mm3 = TEKUI_HUGE;
					}
					lua_pop(L, 1);

					/**
					**	local ch = c:getAttr("Height")
					**	if ch == "fill" then
					**		mm4 = nil
					**	elseif ch == "free" then
					**		mm4 = ui.HUGE
					**	end
					**/

					lua_getfield(L, -1, "getAttr");
					lua_pushvalue(L, -2);
					lua_pushstring(L, "Height");
					lua_call(L, 2, 1);
					/* s: c, c.Height */
					s = lua_tostring(L, -1);
					if (s)
					{
						if (strcmp(s, "fill") == 0)
							mm4 = -1; /* nil */
						else if (strcmp(s, "free") == 0)
							mm4 = TEKUI_HUGE;
					}
					lua_pop(L, 2);
					/* s: tmm, minx, miny, maxx, maxy, children */

					/**
					**	mm3 = mm3 or ori == 2 and mm1
					**	mm4 = mm4 or ori == 1 and mm2
					**/

					if (mm3 < 0 && ori == 2)
						mm3 = mm1;
					if (mm4 < 0 && ori == 1)
						mm4 = mm2;

					/**
					**	minx[x] = max(minx[x] or 0, mm1)
					**	miny[y] = max(miny[y] or 0, mm2)
					**/

					lua_rawgeti(L, -5, x);
					/* s: tmm, minx, miny, maxx, maxy, children, minx[x] */
					minxx = lua_isnil(L, -1) ? 0 : lua_tointeger(L, -1);
					minxx = TMAX(minxx, mm1);
					lua_pushinteger(L, minxx);
					/* s: tmm, minx, miny, maxx, maxy, children, minx[x], minxx */
					lua_rawseti(L, -7, x);
					/* s: tmm, minx, miny, maxx, maxy, children, minx[x] */
					lua_pop(L, 1);
					/* s: tmm, minx, miny, maxx, maxy, children */

					lua_rawgeti(L, -4, y);
					/* s: tmm, minx, miny, maxx, maxy, children, miny[y] */
					minyy = lua_isnil(L, -1) ? 0 : lua_tointeger(L, -1);
					minyy = TMAX(minyy, mm2);
					lua_pushinteger(L, minyy);
					/* s: tmm, minx, miny, maxx, maxy, children, miny[y], minyy */
					lua_rawseti(L, -6, y);
					/* s: tmm, minx, miny, maxx, maxy, children, miny[y] */
					lua_pop(L, 1);
					/* s: tmm, minx, miny, maxx, maxy, children */

					/**
					**	if mm3 and (not maxx[x] or mm3 > maxx[x]) then
					**		maxx[x] = max(mm3, minx[x])
					**	end
					**/

					if (mm3 >= 0)
					{
						lua_rawgeti(L, -3, x);
						/* s: tmm, minx, miny, maxx, maxy, children, maxx[x] */
						if (lua_isnil(L, -1) || mm3 > lua_tointeger(L, -1))
						{
							lua_pushinteger(L, TMAX(mm3, minxx));
							/* s: tmm, minx, miny, maxx, maxy, children, maxx[x], val */
							lua_rawseti(L, -5, x);
							/* s: tmm, minx, miny, maxx, maxy, children, maxx[x] */
						}
						lua_pop(L, 1);
					}

					/**
					**	if mm4 and (not maxy[y] or mm4 > maxy[y]) then
					**		maxy[y] = max(mm4, miny[y])
					**	end
					**/

					if (mm4 >= 0)
					{
						lua_rawgeti(L, -2, y);
						/* s: tmm, minx, miny, maxx, maxy, children, maxy[y] */
						if (lua_isnil(L, -1) || mm4 > lua_tointeger(L, -1))
						{
							lua_pushinteger(L, TMAX(mm4, minyy));
							/* s: tmm, minx, miny, maxx, maxy, children, maxy[y], val */
							lua_rawseti(L, -4, y);
							/* s: tmm, minx, miny, maxx, maxy, children, maxy[y] */
						}
						lua_pop(L, 1);
					}
					
				}
				else
					lua_pop(L, 1);

			}
		}

		/* s: tmm, minx, miny, maxx, maxy, children */
		lua_pop(L, 1);
		/* s: tmm, minx, miny, maxx, maxy */
		lua_rawseti(L, -5, 4);
		lua_rawseti(L, -4, 3);
		lua_rawseti(L, -3, 2);
		lua_rawseti(L, -2, 1);
		/* s: tmm */

		/**
		**	local gs = gw
		**	for i1 = 1, 2 do
		**		local i3 = i1 + 2
		**		local ss = group:getSameSize(i1)
		**		local numfree = 0
		**		local remainder = 0
		**		for n = 1, gs do
		**/

		gs = gw;
		for (i1 = 1; i1 <= 2; i1++)
		{
			int mins, maxs, ss, n;
			int i3 = i1 + 2;
			int numfree = 0;
			int remainder = 0;
			
			ss = layout_getsamesize(L, 2, i1);

			for (n = 1; n <= gs; ++n)
			{
				/**
				**	local mins = tmm[i1][n] or 0
				**	local maxs = tmm[i3][n]
				**	if ss then
				**		if not maxs or (maxs > mins) then
				**			numfree = numfree + 1
				**		else
				**			remainder = remainder + mins
				**		end
				**		m[i1] = max(m[i1], mins)
				**	else
				**		m[i1] = m[i1] + mins
				**	end
				**/

				/* s: tmm */
				lua_rawgeti(L, -1, i1);
				lua_rawgeti(L, -1, n);
				/* s: tmm, tmm[i1], tmm[i1][n] */
				mins = lua_tointeger(L, -1);
				lua_rawgeti(L, -3, i3);
				lua_rawgeti(L, -1, n);
				maxs = lua_isnil(L, -1) ? -1 : lua_tointeger(L, -1);
				/* s: tmm, tmm[i1], tmm[i1][n], tmm[i3], tmm[i3][n] */
				
				if (ss)
				{
					if (maxs < 0 || maxs > mins)
						numfree++;
					else
						remainder += mins;
					m[i1] = TMAX(m[i1], mins);
				}
				else
					m[i1] += mins;
				
				/**
				**	if maxs then
				**		maxs = max(maxs, mins)
				**		tmm[i3][n] = maxs
				**		m[i3] = (m[i3] or 0) + maxs
				**	elseif ori == i1 then
				**		m[i3] = (m[i3] or 0) + mins
				**	end
				**/

				if (maxs >= 0)
				{
					maxs = TMAX(maxs, mins);
					lua_pushinteger(L, maxs);
					/* s: tmm, tmm[i1], tmm[i1][n], tmm[i3], tmm[i3][n], val */
					lua_rawseti(L, -3, n);
					/* s: tmm, tmm[i1], tmm[i1][n], tmm[i3], tmm[i3][n] */
					m[i3] = TMAX(m[i3], 0) + maxs;
				}
				else if (ori == i1)
				{
					/* if on primary axis, we must reserve at least min: */
					m[i3] = TMAX(m[i3], 0) + mins;
				}
				
				lua_pop(L, 4);
			}

			/**
			**	if ss then
			**		if numfree == 0 then
			**			m[i1] = m[i1] * gs
			**		else
			**			m[i1] = m[i1] * numfree + remainder
			**		if m[i3] and m[i1] > m[i3] then
			**			m[i3] = m[i1]
			**		end
			**	end
			**	gs = gh
			**/

			if (ss)
			{
				if (numfree == 0)
					m[i1] *= gs;
				else
					m[i1] = m[i1] * numfree + remainder;
				if (m[i3] >= 0 && m[i1] > m[i3])
					m[i3] = m[i1];
			}
			gs = gh;
		}

		/* s: tmm */
		lua_pop(L, 1);
	}

	lua_pushinteger(L, m[1]);
	lua_pushinteger(L, m[2]);
	lua_pushinteger(L, m[3]);
	lua_pushinteger(L, m[4]);

	return 4;
}

/*****************************************************************************/

static int layout_new(lua_State *L)
{
	lua_getglobal(L, "require");
	/* s: <require> */
	lua_pushliteral(L, DEFLAYOUT_SUPERCLASS_NAME);
	/* s: <require>, "superclass" */
	lua_call(L, 1, 1);
	/* s: Layout */
	lua_getfield(L, -1, "new");
	/* s: Layout, Layout.new */
	lua_remove(L, -2);
	/* s: Layout.new */
	lua_pushvalue(L, 1);
	/* s: Layout.new, class */
	lua_pushvalue(L, 2);
	/* s: Layout.new, class, self */
	lua_newtable(L);
	lua_setfield(L, -2, "TempMinMax");
	lua_newtable(L);
	lua_setfield(L, -2, "Weights");
	lua_call(L, 2, 1);
	/* s: self */
	return 1;
}

/*****************************************************************************/

static const luaL_Reg tek_ui_layout_default_funcs[] =
{
	{ "new", layout_new },
	{ "askMinMax", layout_askMinMax },
	{ "layoutAxis", layout_layoutaxis },
	{ "layout", layout_layout },
	{ NULL, NULL }
};

/*****************************************************************************/

TMODENTRY int luaopen_tek_ui_layout_default(lua_State *L)
{
	lua_getglobal(L, "require");
	lua_pushliteral(L, DEFLAYOUT_SUPERCLASS_NAME);
	lua_call(L, 1, 1);
	lua_getfield(L, -1, "newClass");
	lua_insert(L, -2);
	/* s: newClass(), superclass */

	/* we do this just to check the version: */	
	lua_getglobal(L, "require");
	lua_pushliteral(L, "tek.ui");
	lua_call(L, 1, 1);
	lua_getfield(L, -1, "checkVersion");
	lua_pushinteger(L, DEFLAYOUT_TEKUI_VERSION);
	lua_call(L, 1, 0);
	lua_pop(L, 1);
	
	/* pass superclass as upvalue: */
	lua_pushvalue(L, -1);
	/* s: newClass(), superclass, superclass */
	tek_lua_register(L, DEFLAYOUT_CLASS_NAME, tek_ui_layout_default_funcs, 1);
	/* s: newClass(), superclass, class */
	
	/* insert name and version: */
	lua_pushliteral(L, DEFLAYOUT_CLASS_NAME);
	lua_setfield(L, -2, "_NAME");
	lua_pushliteral(L, DEFLAYOUT_CLASS_VERSION);
	lua_setfield(L, -2, "_VERSION");
	
	/* inherit: class = superclass.newClass(superclass, class) */
	lua_call(L, 2, 1); 
	/* s: class */
	return 1;
}
