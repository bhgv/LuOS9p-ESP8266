/*-----------------------------------------------------------------------------
--
--	tek.class.example
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	Skeleton class written in C
--
-------------------------------------------------------------------------------

module("tek.class.example", tek.class)
_VERSION = "Example 2.0"
local Example = _M

******************************************************************************/

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* Name of superclass: */
#define SUPERCLASS_NAME "tek.class"

/* Name of this class: */
#define CLASS_NAME "tek.class.example"

/* Upvalue of superclass: */
#define UPVALUE_SUPERCLASS	lua_upvalueindex(1)

/*****************************************************************************/

/*
**	your class implementation here
*/

static int tek_class_example_new(lua_State *L)
{
	int narg = lua_gettop(L);
	lua_pushvalue(L, UPVALUE_SUPERCLASS);
	lua_getfield(L, -1, "new");
	lua_remove(L, -2);
	lua_pushvalue(L, 1);
	
	/* s: Superclass.new(), class */
	
	if (narg >= 2)
		lua_pushvalue(L, 2);
	else
		lua_newtable(L);
	
	/* s: Superclass.new(), class, self */
	
	/*	self = Superclass.new(class, self) */
	lua_call(L, 2, 1);
	
	return 1;
}

static const luaL_Reg classfuncs[] =
{
	{ "new", tek_class_example_new },
	/* insert methods here */
	{ NULL, NULL }
};

/*****************************************************************************/

int luaopen_tek_class_example(lua_State *L)
{
	lua_getglobal(L, "require");
	lua_pushliteral(L, SUPERCLASS_NAME);
	lua_call(L, 1, 1);
	lua_getfield(L, -1, "newClass");
	lua_insert(L, -2);
	/* s: newClass(), superclass */

	/* pass superclass as upvalue: */
	lua_pushvalue(L, -1);
	/* s: newClass(), superclass, superclass */
	tek_register_class(L, CLASS_NAME, classfuncs, 1);
	/* s: newClass(), superclass, class */
	
	/* insert name and version: */
	lua_pushliteral(L, CLASS_NAME);
	lua_setfield(L, -2, "_NAME");
	lua_pushliteral(L, CLASS_VERSION);
	lua_setfield(L, -2, "_VERSION");
	
	/* inherit: class = superclass.newClass(superclass, class) */
	lua_call(L, 2, 1); 
	/* s: class */
	return 1;
}
