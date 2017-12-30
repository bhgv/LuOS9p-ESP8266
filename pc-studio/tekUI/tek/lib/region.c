/*-----------------------------------------------------------------------------
--
--	tek.lib.region
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		This library implements the management of regions, which are
--		collections of non-overlapping rectangles.
--
--	FUNCTIONS::
--		- region:andRect() - ''And''s a rectangle to a region
--		- region:andRegion() - ''And''s a region to a region
--		- region:checkIntersect() - Checks if a rectangle intersects a region
--		- region:forEach() - Calls a function for each rectangle in a region
--		- region:get() - Gets a region's min/max extents
--		- Region.intersect() - Returns the intersection of two rectangles
--		- region:isEmpty() - Checks if a Region is empty
--		- Region.new() - Creates a new Region
--		- region:orRect() - ''Or''s a rectangle to a region
--		- region:orRegion() - ''Or''s a region to a region
--		- region:setRect() - Resets a region to the given rectangle
--		- region:shift() - Displaces a region
--		- region:subRect() - Subtracts a rectangle from a region
--		- region:subRegion() - Subtracts a region from a region
--		- region:xorRect() - ''Exclusive Or''s a rectangle to a region
--
-------------------------------------------------------------------------------

module "tek.lib.region"
_VERSION = "Region 11.3"
local Region = _M

******************************************************************************/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/lib/tek_lua.h>
#include <tek/lib/tekui.h>
#include <tek/lib/region.h>
#include <tek/proto/exec.h>

#define TEK_LIB_REGION_VERSION "Region 11.3"
#define TEK_LIB_REGION_NAME "tek.lib.region"
#define TEK_LIB_REGION_POOL_NAME "tek.lib.pool*"


static void *tek_lib_region_check(lua_State *L, int n)
{
	return luaL_checkudata(L, n, TEK_LIB_REGION_NAME "*");
}

static void *tek_lib_region_opt(lua_State *L, int n)
{
	if (lua_type(L, n) == LUA_TUSERDATA)
		return luaL_checkudata(L, n, TEK_LIB_REGION_NAME "*");
	return TNULL;
}

static void tek_lib_region_checkrect(lua_State *L, int firstidx, TINT s[4])
{
	int i;
	for (i = 0; i < 4; ++i)
		s[i] = luaL_checkinteger(L, firstidx + i);

	/*
	** if (s[2] < s[0] || s[3] < s[1])
	**	luaL_error(L, "Illegal rectangle %d,%d,%d,%d",s[0],s[1],s[2],s[3]);
	*/
}

/*-----------------------------------------------------------------------------
--	x0, y0, x1, y1 = Region.intersect(d1, d2, d3, d4, s1, s2, s3, s4):
--	Returns the coordinates of a rectangle where a rectangle specified by
--	the coordinates s1, s2, s3, s4 overlaps with the rectangle specified
--	by the coordinates d1, d2, d3, d4. The return value is '''nil''' if
--	the rectangles do not overlap.
-----------------------------------------------------------------------------*/

static int tek_lib_region_intersect(lua_State *L)
{
	RECTINT d[4], s[4];
	tek_lib_region_checkrect(L, 1, d);
	tek_lib_region_checkrect(L, 5, s);
	if (region_intersect(d, s))
	{
		int i;
		for (i = 0; i < 4; ++i)
			lua_pushinteger(L, d[i]);
		return 4;
	}
	return 0;
}

/*-----------------------------------------------------------------------------
--	region = Region.new(r1, r2, r3, r4): Creates a new region from the given
--	coordinates.
-----------------------------------------------------------------------------*/

static int tek_lib_region_new(lua_State *L)
{
	struct Region *region = lua_newuserdata(L, sizeof(struct Region));
	/* s: udata */
	region_initrectlist(&region->rg_Rects);
	lua_getfield(L, LUA_REGISTRYINDEX, TEK_LIB_REGION_NAME "*");
	/* s: udata, metatable */
	lua_rawgeti(L, -1, 2);
	/* s: udata, metatable, pool */
	region->rg_Pool = lua_touserdata(L, -1);
	lua_pop(L, 1);
	/* s: udata, metatable */
	lua_setmetatable(L, -2);
	/* s: udata */
	
	if (lua_gettop(L) == 5)
	{
		TINT r[4];
		tek_lib_region_checkrect(L, 1, r);
		if (region_insertrect(region->rg_Pool,
			&region->rg_Rects, r[0], r[1], r[2], r[3]) == TFALSE)
			luaL_error(L, "out of memory");
	}
	else if (lua_gettop(L) != 1)
		luaL_error(L, "wrong number of arguments");

	return 1;
}

/*-----------------------------------------------------------------------------
--	self = region:setRect(r1, r2, r3, r4): Resets an existing region
--	to the specified rectangle.
-----------------------------------------------------------------------------*/

static int tek_lib_region_set(lua_State *L)
{
	struct Region *region = tek_lib_region_check(L, 1);
	TINT r[4];
	tek_lib_region_checkrect(L, 2, r);
	region_freerects(region->rg_Pool, &region->rg_Rects);
	if (region_insertrect(region->rg_Pool, &region->rg_Rects,
		r[0], r[1], r[2], r[3]) == TFALSE)
		luaL_error(L, "out of memory");
	lua_pushvalue(L, 1);
	return 1;
}

static int tek_lib_region_collect(lua_State *L)
{
	struct Region *region = tek_lib_region_check(L, 1);
	region_freerects(region->rg_Pool, &region->rg_Rects);
	return 0;
}

/*-----------------------------------------------------------------------------
--	region:orRect(r1, r2, r3, r4): Logical ''or''s a rectangle to a region
-----------------------------------------------------------------------------*/

static int tek_lib_region_orrect(lua_State *L)
{
	struct Region *region = tek_lib_region_check(L, 1);
	RECTINT s[4];
	tek_lib_region_checkrect(L, 2, s);
	if (!region_orrectlist(region->rg_Pool, &region->rg_Rects, s, TFALSE))
		luaL_error(L, "out of memory");
	return 0;
}

/*-----------------------------------------------------------------------------
--	region:xorRect(r1, r2, r3, r4): Logical ''xor''s a rectange to a region
-----------------------------------------------------------------------------*/

static int tek_lib_region_xorrect(lua_State *L)
{
	struct Region *region = tek_lib_region_check(L, 1);
	RECTINT s[4];
	tek_lib_region_checkrect(L, 2, s);
	if (!region_xorrect(region->rg_Pool, region, s))
		luaL_error(L, "out of memory");
	return 0;
}

/*-----------------------------------------------------------------------------
--	self = region:subRect(r1, r2, r3, r4): Subtracts a rectangle from a region
-----------------------------------------------------------------------------*/

static int tek_lib_region_subrect(lua_State *L)
{
	struct Region *region = tek_lib_region_check(L, 1);
	RECTINT s[4];
	tek_lib_region_checkrect(L, 2, s);
	if (!region_subrect(region->rg_Pool, region, s))
		luaL_error(L, "out of memory");
	lua_pushvalue(L, 1);
	return 1;
}

/*-----------------------------------------------------------------------------
--	success = region:checkIntersect(x0, y0, x1, y1): Returns a boolean
--	indicating whether a rectangle specified by its coordinates overlaps
--	with a region.
-----------------------------------------------------------------------------*/

static int tek_lib_region_checkintersect(lua_State *L)
{
	struct Region *region = tek_lib_region_check(L, 1);
	RECTINT s[4];
	tek_lib_region_checkrect(L, 2, s);
	lua_pushboolean(L, region_overlap(region->rg_Pool, region, s));
	return 1;
}

/*-----------------------------------------------------------------------------
--	region:subRegion(region2): Subtracts {{region2}} from {{region}}.
-----------------------------------------------------------------------------*/

static int tek_lib_region_subregion(lua_State *L)
{
	struct Region *self = tek_lib_region_check(L, 1);
	struct Region *region = tek_lib_region_opt(L, 2);
	if (region && !region_subregion(self->rg_Pool, self, region))
		luaL_error(L, "out of memory");
	return 0;
}

/*-----------------------------------------------------------------------------
--	region:andRect(r1, r2, r3, r4): Logical ''and''s a rectange to a region
-----------------------------------------------------------------------------*/

static int tek_lib_region_andrect(lua_State *L)
{
	struct Region *dregion = tek_lib_region_check(L, 1);
	RECTINT s[4];
	tek_lib_region_checkrect(L, 2, s);
	if (!region_andrect(dregion->rg_Pool, dregion, s, 0, 0))
		luaL_error(L, "out of memory");
	return 0;
}

/*-----------------------------------------------------------------------------
--	region:andRegion(r): Logically ''and''s a region to a region
-----------------------------------------------------------------------------*/

static int tek_lib_region_andregion(lua_State *L)
{
	struct Region *dregion = tek_lib_region_check(L, 1);
	struct Region *sregion = tek_lib_region_opt(L, 2);
	if (!region_andregion(dregion->rg_Pool, dregion, sregion))
		luaL_error(L, "out of memory");
	return 0;
}

/*-----------------------------------------------------------------------------
--	region:orRegion(r): Logically ''or''s a region to a region
-----------------------------------------------------------------------------*/

static int tek_lib_region_orregion(lua_State *L)
{
	struct Region *dregion = tek_lib_region_check(L, 1);
	struct Region *sregion = tek_lib_region_opt(L, 2);
	if (!region_orregion(dregion, &sregion->rg_Rects, TFALSE))
		luaL_error(L, "out of memory");
	return 0;
}

/*-----------------------------------------------------------------------------
--	region:forEach(func, obj, ...): For each rectangle in a region, calls the
--	specified function according the following scheme:
--			func(obj, x0, y0, x1, y1, ...)
--	Extra arguments are passed through to the function.
-----------------------------------------------------------------------------*/

static int tek_lib_region_foreach(lua_State *L)
{
	struct Region *region = tek_lib_region_check(L, 1);
	struct TNode *next, *node = region->rg_Rects.rl_List.tlh_Head.tln_Succ;
	int narg = lua_gettop(L) - 3;
	int i;
	for (; (next = node->tln_Succ); node = next)
	{
		struct RectNode *rn = (struct RectNode *) node;
		lua_pushvalue(L, 2); /* func */
		lua_pushvalue(L, 3); /* object */
		for (i = 0; i < 4; ++i)
			lua_pushinteger(L, rn->rn_Rect[i]);
		for (i = 0; i < narg; ++i)
			lua_pushvalue(L, 4 + i);
		lua_call(L, 5 + narg, 0);
	}
	return 0;
}

/*-----------------------------------------------------------------------------
--	region:shift(dx, dy): Shifts a region by delta x and y.
-----------------------------------------------------------------------------*/

static int tek_lib_region_shift(lua_State *L)
{
	struct Region *region = tek_lib_region_check(L, 1);
	lua_Number sx = luaL_checknumber(L, 2);
	lua_Number sy = luaL_checknumber(L, 3);	
	region_shift(region, sx, sy);
	return 0;
}

/*-----------------------------------------------------------------------------
--	region:isEmpty(): Returns '''true''' if a region is empty.
-----------------------------------------------------------------------------*/

static int tek_lib_region_isempty(lua_State *L)
{
	struct Region *region = tek_lib_region_check(L, 1);
	lua_pushboolean(L, region_isempty(region->rg_Pool, region));
	return 1;
}

/*-----------------------------------------------------------------------------
--	minx, miny, maxx, maxy = region:get(): Get region's min/max extents
-----------------------------------------------------------------------------*/

static int tek_lib_region_getminmax(lua_State *L)
{
	struct Region *region = tek_lib_region_check(L, 1);
	RECTINT minmax[4];
	if (region_getminmax(region->rg_Pool, region, minmax))
	{
		int i;
		for (i = 0; i < 4; ++i)
			lua_pushinteger(L, minmax[i]);
		return 4;
	}
	return 0;
}

/*****************************************************************************/

static int tek_lib_pool_collect(lua_State *L)
{
	struct RectPool *pool = luaL_checkudata(L, 1, TEK_LIB_REGION_POOL_NAME);
	region_destroypool(pool);
	return 0;
}

static const luaL_Reg tek_lib_region_funcs[] =
{
	{ "new", tek_lib_region_new },
	{ "intersect", tek_lib_region_intersect },
	{ NULL, NULL }
};

static const luaL_Reg tek_lib_region_methods[] =
{
	{ "__gc", tek_lib_region_collect },
	{ "setRect", tek_lib_region_set },
	{ "orRect", tek_lib_region_orrect },
	{ "orRegion", tek_lib_region_orregion },
	{ "xorRect", tek_lib_region_xorrect },
	{ "subRect", tek_lib_region_subrect },
	{ "checkIntersect", tek_lib_region_checkintersect },
	{ "subRegion", tek_lib_region_subregion },
	{ "andRect", tek_lib_region_andrect },
	{ "andRegion", tek_lib_region_andregion },
	{ "forEach", tek_lib_region_foreach },
	{ "shift", tek_lib_region_shift },
	{ "isEmpty", tek_lib_region_isempty },
	{ "get", tek_lib_region_getminmax },
	{ NULL, NULL }
};

static const luaL_Reg tek_lib_region_poolmethods[] =
{
	{ "__gc", tek_lib_pool_collect },
	{ NULL, NULL }
};

TMODENTRY int luaopen_tek_lib_region(lua_State *L)
{
	struct RectPool *pool;
	
	tek_lua_register(L, TEK_LIB_REGION_NAME, tek_lib_region_funcs, 0);
	lua_pushstring(L, TEK_LIB_REGION_VERSION);
	lua_setfield(L, -2, "_VERSION");

	/* require "tek.lib.exec": */
	lua_getglobal(L, "require");
	/* s: require() */
	lua_pushliteral(L, "tek.lib.exec");
	/* s: require(), "tek.lib.exec" */
	lua_call(L, 1, 1);
	/* s: exectab */
	lua_getfield(L, -1, "base");
	/* s: exectab, execbase */
	lua_remove(L, -2);
	/* s: execbase */
	luaL_newmetatable(L, TEK_LIB_REGION_NAME "*");
	/* s: execbase, metatable */
	tek_lua_register(L, NULL, tek_lib_region_methods, 0);
	/* s: execbase, metatable */
	lua_pushvalue(L, -1);
	/* s: execbase, metatable, metatable */
	lua_pushvalue(L, -3);
	/* s: execbase, metatable, metatable, execbase */
	lua_rawseti(L, -2, 1);
	/* s: execbase, metatable, metatable */
	
	pool = lua_newuserdata(L, sizeof(struct RectPool));
	region_initrectlist(&pool->p_Rects);
	pool->p_ExecBase = *(TAPTR *) lua_touserdata(L, -4);
	/* s: execbase, metatable, metatable, pool */
	luaL_newmetatable(L, TEK_LIB_REGION_POOL_NAME);
	/* s: execbase, metatable, metatable, pool, poolmt */
	tek_lua_register(L, NULL, tek_lib_region_poolmethods, 0);
	lua_setmetatable(L, -2);
	/* s: execbase, metatable, metatable, pool */
	lua_rawseti(L, -2, 2);
	/* s: execbase, metatable, metatable */
	
	lua_setfield(L, -2, "__index");
	/* s: execbase, metatable */
	lua_pop(L, 2);
	/* s: */

	return 1;
}
