
/*
**	tek.lib.display.rawfb - binding of TEKlib's RawFB driver to Lua
**	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
**	See copyright notice in COPYRIGHT
*/

#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/mod/display.h>

/*****************************************************************************/

#if defined(RFB_SUB_DEVICE)

#define NAMEHELP(fun, suffix) fun ## suffix
#define NAME(fun, suffix) NAMEHELP(fun, suffix)
#define SUBDEVICE_ENTRY NAME(tek_init_display_, RFB_SUB_DEVICE)

extern TMODENTRY TUINT SUBDEVICE_ENTRY(struct TTask *, struct TModule *, TUINT16, TTAGITEM *);

#endif

/*****************************************************************************/

#define TEK_LIB_DISPLAY_RAWFB_CLASSNAME "tek.lib.display.rawfb*"
#define TEK_LIB_DISPLAY_RAWFB_BASECLASSNAME "tek.lib.display.rawfb.base*"

extern TMODENTRY TUINT
tek_init_display_rawfb(struct TTask *, struct TModule *, TUINT16, TTAGITEM *);
static TCALLBACK TINT tek_lib_display_rawfb_close(lua_State *L);

static const struct TInitModule tek_lib_display_rawfb_initmodules[] =
{
	{ "display_rawfb", tek_init_display_rawfb, TNULL, 0 },
#if defined(RFB_SUB_DEVICE)
	{ "display_" DISPLAY_NAME(RFB_SUB_DEVICE), SUBDEVICE_ENTRY, TNULL, 0 },
#endif
	{ TNULL, TNULL, TNULL, 0 }
};

static const luaL_Reg tek_lib_display_rawfb_funcs[] =
{
	{ TNULL, TNULL }
};

static const luaL_Reg tek_lib_display_rawfb_methods[] =
{
	{ "__gc", tek_lib_display_rawfb_close },
	{ TNULL, TNULL }
};

/*****************************************************************************/

static TCALLBACK TINT
tek_lib_display_rawfb_close(lua_State *L)
{
	TEKDisplay *display = luaL_checkudata(L, 1, TEK_LIB_DISPLAY_RAWFB_CLASSNAME);
	TDBPRINTF(TDB_TRACE,("display %08x closing\n", display));
	if (display->IsBase)
	{
		/* collected base; remove TEKlib module: */
		TExecRemModules(display->ExecBase, &display->InitModules, 0);
		TDBPRINTF(TDB_TRACE,("display module removed\n"));
	}
	return 0;
}

/*****************************************************************************/

int luaopen_tek_lib_display_rawfb(lua_State *L)
{
	TAPTR exec;
	TEKDisplay *display;

	/* require "tek.lib.exec": */
	lua_getglobal(L, "require");
	/* s: "require" */
	lua_pushliteral(L, "tek.lib.exec");
	/* s: "require", "tek.lib.exec" */
	lua_call(L, 1, 1);
	/* s: exectab */
	lua_getfield(L, -1, "base");
	/* s: exectab, execbase */
	exec = *(TAPTR *) lua_touserdata(L, -1);

	/* register functions: */
#if LUA_VERSION_NUM < 502
	luaL_register(L, "tek.lib.display.rawfb", tek_lib_display_rawfb_funcs);
#else
	luaL_newlib(L, tek_lib_display_rawfb_funcs);
#endif
	/* s: exectab, execbase, libtab */

	lua_pushvalue(L, -1);
	/* s: exectab, execbase, libtab, libtab */
	lua_insert(L, -4);
	/* s: libtab, exectab, execbase, libtab */

	/* create userdata: */
	display = lua_newuserdata(L, sizeof(TEKDisplay));
	/* s: exectab, execbase, libtab, libbase */

	display->Base = TNULL;
	display->ExecBase = exec;
	display->IsBase = TTRUE;

	/* register base: */
	lua_pushvalue(L, -1);
	/* s: exectab, execbase, libtab, libbase, libbase */
	lua_setfield(L, LUA_REGISTRYINDEX, TEK_LIB_DISPLAY_RAWFB_BASECLASSNAME);
	/* s: exectab, execbase, libtab, libbase */

	/* create metatable for userdata, register methods: */
	luaL_newmetatable(L, TEK_LIB_DISPLAY_RAWFB_CLASSNAME);
	/* s: exectab, execbase, libtab, libbase, libmeta */
	lua_pushvalue(L, -1);
	/* s: exectab, execbase, libtab, libbase, libmeta, libmeta */
	lua_setfield(L, -2, "__index");
	/* s: exectab, execbase, libtab, libbase, libmeta */
#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, tek_lib_display_rawfb_methods);
#else
	luaL_setfuncs(L, tek_lib_display_rawfb_methods, 0);	
#endif
	/* s: exectab, execbase, libtab, libbase, libmeta */
	lua_setmetatable(L, -2);
	/* s: exectab, execbase, libtab, libbase */

	/* place exec reference in metatable: */
	lua_getmetatable(L, -1);
	/* s: exectab, execbase, libtab, libbase, libmeta */
	lua_pushvalue(L, -4);
	/* s: exectab, execbase, libtab, libbase, libmeta, execbase */
	luaL_ref(L, -2); /* index returned is always 1 */
	/* s: exectab, execbase, libtab, libbase, libmeta */
	lua_pop(L, 5);

	/* Add visual module to TEKlib's internal module list: */
	memset(&display->InitModules, 0, sizeof display->InitModules);
	display->InitModules.tmin_Modules = tek_lib_display_rawfb_initmodules;
	TExecAddModules(exec, &display->InitModules, 0);

	return 1;
}
