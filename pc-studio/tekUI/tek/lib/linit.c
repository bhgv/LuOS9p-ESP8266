
/*
**	Lua library initialization
*/

#define linit_c
#define LUA_LIB

#include "lua.h"

#include "lualib.h"
#include "lauxlib.h"

#if defined(LUA_TEKUI_INCLUDE_CLASS_LIBRARY)
#include "tekui_classlib.c"
#endif

static const luaL_Reg lualibs[] = {
#if LUA_VERSION_NUM < 502
  {"", luaopen_base},
#else
  {"_G", luaopen_base},
#endif
  {LUA_LOADLIBNAME, luaopen_package},
#if LUA_VERSION_NUM >= 502
  {LUA_COLIBNAME, luaopen_coroutine},
#endif
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_IOLIBNAME, luaopen_io},
  {LUA_OSLIBNAME, luaopen_os},
  {LUA_STRLIBNAME, luaopen_string},
#if LUA_VERSION_NUM == 502 || (LUA_VERSION_NUM > 502 && defined(LUA_COMPAT_BITLIB))
  {LUA_BITLIBNAME, luaopen_bit32},
#endif
  {LUA_MATHLIBNAME, luaopen_math},
#if LUA_VERSION_NUM > 502
  {LUA_UTF8LIBNAME, luaopen_utf8},
#endif
  {LUA_DBLIBNAME, luaopen_debug},
  {NULL, NULL}
};

#if defined(LUA_TEKUI_INCLUDE_CLASS_LIBRARY)

static const luaL_Reg lualibs2[] = {
  { "tek.lib.exec", luaopen_tek_lib_exec },
  { "tek.lib.region", luaopen_tek_lib_region },
  { "tek.lib.string", luaopen_tek_lib_string },
#if defined(TSYS_POSIX)
  { "tek.lib.display.x11", luaopen_tek_lib_display_x11 },
/*{ "tek.lib.display.rawfb", luaopen_tek_lib_display_rawfb },*/
#elif defined(TSYS_WINNT)
  { "tek.lib.display.windows", luaopen_tek_lib_display_windows },
#endif
  { "tek.lib.visual", luaopen_tek_lib_visual },
  { "tek.lib.support", luaopen_tek_lib_support },
  { "tek.ui.layout.default", luaopen_tek_ui_layout_default },
  { "tek.ui.class.area", luaopen_tek_ui_class_area },
  { "tek.ui.class.frame", luaopen_tek_ui_class_frame },
  
/**
* 
* 	place for additional C libraries
* 
**/
  
  {NULL, NULL}
};

#endif

static void luali_openlibs(lua_State *L, const luaL_Reg *lib)
{
	for (; lib->func; lib++) {
#if LUA_VERSION_NUM < 502
		lua_pushcfunction(L, lib->func);
		lua_pushstring(L, lib->name);
		lua_call(L, 1, 0);
#else
	    luaL_requiref(L, lib->name, lib->func, 1);
    	lua_pop(L, 1);  /* remove lib */
#endif
	}
}

LUALIB_API void luaL_openlibs (lua_State *L) 
{
	luali_openlibs(L, lualibs);
#if defined(LUA_TEKUI_INCLUDE_CLASS_LIBRARY)
	luaL_loadbuffer(L, (const char *) bytecode, sizeof(bytecode),
  		"tekUI class library");
	lua_call(L, 0, 0);
	luali_openlibs(L, lualibs2);
#endif
}
