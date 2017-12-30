
/*
**	tek.lib.support - C support library
*/

#include <tek/lib/tek_lua.h>
#include <tek/lib/tekui.h>

#define TEK_LIB_SUPPORT_NAME    "tek.lib.support"
#define TEK_LIB_SUPPORT_VERSION "Support Library 5.0"

static const int srcidx = -1;
static const int dstidx = -2;

static void tek_lib_support_copytable_r(lua_State *L)
{
	lua_pushnil(L);
	/* s: nil, src, dst */
	while (lua_next(L, srcidx - 1) != 0)
	{
		/* s: val, key, src, dst */
		lua_pushvalue(L, -2);
		/* s: key, val, key, src, dst */
		lua_insert(L, -2);
		/* s: val, key, key, src, dst */
		
		if (lua_type(L, -1) == LUA_TTABLE)
		{
			lua_newtable(L);
			/* s: dst2, val=src2, key, key, src, dst */
			lua_insert(L, -2);
			/* s: val=src2, dst2, key, key, src, dst */
			tek_lib_support_copytable_r(L);
			/* s: src2, dst2, key, key, src, dst */
			lua_pop(L, 1);
			/* s: val=dst2, key, key, src, dst */
		}
		
		/* s: val, key, key, src, dst */
		lua_rawset(L, dstidx - 3);
		/* s: key, src, dst */
	}
	/* s: src, dst */
}

static int tek_lib_support_copytable(lua_State *L)
{
	lua_pushvalue(L, 2);
	lua_pushvalue(L, 1);
	tek_lib_support_copytable_r(L);
	lua_pop(L, 1);
	return 1;
}

static int tek_lib_support_band(lua_State *L)
{
	lua_pushinteger(L, lua_tointeger(L, 1) & lua_tointeger(L, 2));
	return 1;
}

static int tek_lib_support_bor(lua_State *L)
{
	lua_pushinteger(L, lua_tointeger(L, 1) | lua_tointeger(L, 2));
	return 1;
}

static int tek_lib_support_bnot(lua_State *L)
{
	lua_pushinteger(L, ~lua_tointeger(L, 1));
	return 1;
}

static int tek_lib_support_bxor(lua_State *L)
{
	lua_pushinteger(L, lua_tointeger(L, 1) ^ lua_tointeger(L, 2));
	return 1;
}

static const luaL_Reg tek_lib_support_funcs[] =
{
	{ "band", tek_lib_support_band },
	{ "bnot", tek_lib_support_bnot },
	{ "bor", tek_lib_support_bor },
	{ "bxor", tek_lib_support_bxor },
	{ "copyTable", tek_lib_support_copytable },
	{ NULL, NULL }
};

int luaopen_tek_lib_support(lua_State *L)
{
	tek_lua_register(L, TEK_LIB_SUPPORT_NAME, tek_lib_support_funcs, 0);
	lua_pushstring(L, TEK_LIB_SUPPORT_VERSION);
	lua_setfield(L, -2, "_VERSION");
	return 1;
}
