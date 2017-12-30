/*-----------------------------------------------------------------------------
--
--	tek.ui.class.frame
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		[[#ClassOverview]] :
--		[[#tek.class : Class]] /
--		[[#tek.class.object : Object]] /
--		[[#tek.ui.class.element : Element]] /
--		[[#tek.ui.class.area : Area]] /
--		Frame ${subclasses(Frame)}
--
--		This class implements an element's borders. The ''default'' border
--		class handles up to three sub borders:
--			* The ''main'' border is the innermost of the three sub borders.
--			It is used to render the primary border style, which can be
--			''inset'', ''outset'', ''ridge'', ''groove'', or
--			''solid''. This border has priority over the other two.
--			* The ''rim'' border separates the two other borders and
--			may give the composition a more contrastful look. This border
--			has the lowest priority.
--			* The ''focus'' border (in addition to the element's focus
--			highlighting style) can be used to visualize that the element is
--			currently receiving the input. This is the outermost of the three
--			sub borders. When the element is in unfocused state, this border
--			often appears in the same color as the surrounding group, making
--			it indistinguishable from the surrounding background.
--		Border classes do not have to implement all sub borders; these
--		properties are all handled by the ''default'' border class internally,
--		and more (and other) sub borders and properties may be defined and
--		implemented in the future (or in other border classes). As the Frame
--		class has no notion of sub borders, their respective widths are
--		subtracted from the Element's total border width, leaving only the
--		remaining width for the ''main'' border.
--
--	ATTRIBUTES::
--		- {{BorderRegion [G]}} ([[#tek.lib.region : Region]])
--			Region object holding the outline of the element's border
--		- {{Legend [IG]}} (string)
--			Border legend text
--
--	STYLE PROPERTIES::
--		''border-bottom-color'' || controls the ''default'' border class
--		''border-bottom-width'' || controls {{Frame.Border}}
--		''border-class'' || controls {{Frame.BorderClass}}
--		''border-color'' || controls the ''default'' border class
--		''border-focus-color'' || controls the ''default'' border class
--		''border-focus-width'' || controls the ''default'' border class
--		''border-left-color'' || controls the ''default'' border class
--		''border-left-width'' || controls {{Frame.Border}}
--		''border-legend-font'' || controls the ''default'' border class
--		''border-legend-align'' || controls the ''default'' border class
--		''border-right-color'' || controls the ''default'' border class
--		''border-right-width'' || controls {{Frame.Border}}
--		''border-rim-color'' || controls the ''default'' border class
--		''border-rim-width'' || controls the ''default'' border class
--		''border-style'' || controls the ''default'' border class
--		''border-top-color'' || controls the ''default'' border class
--		''border-top-width'' || controls {{Frame.Border}}
--		''border-width'' || controls {{Frame.Border}}
--
--	IMPLEMENTS::
--		- Frame:drawBorder() - Draws the element's border
--		- Frame:getBorder() - Queries the element's border
--
--	OVERRIDES::
--		- Element:cleanup()
--		- Area:damage()
--		- Area:draw()
--		- Area:getByXY()
--		- Area:getMargin()
--		- Area:layout()
--		- Class.new()
--		- Element:onSetStyle()
--		- Area:punch()
--		- Area:setState()
--		- Element:setup()
--
-------------------------------------------------------------------------------

module("tek.ui.class.frame", tek.ui.class.area)
_VERSION = "Frame 24.2"
local Frame = _M
Area:newClass(Frame)

-----------------------------------------------------------------------------*/

#include <stdint.h>
#include <tek/lib/tek_lua.h>
#include <tek/lib/tekui.h>


/* Name of superclass: */
#define FRAME_SUPERCLASS_NAME "tek.ui.class.area"

/* Name of this class: */
#define FRAME_CLASS_NAME "tek.ui.class.frame"

/* Version string: */
#define FRAME_CLASS_VERSION "Frame 24.2"

/* Required tekui version: */
#define FRAME_TEKUI_VERSION 112

/* Required major version of the Region library: */
#define FRAME_REGION_VERSION	10


/* Stack index of self argument: */
#define ISELF	1

/* Stack index of superclass: */
#define ISUPERCLASS	lua_upvalueindex(1)

/* Stack index of tek.ui: */
#define ITEKUI	lua_upvalueindex(2)

/* Stack index of Region library: */
#define IREGION	lua_upvalueindex(3)


/*-----------------------------------------------------------------------------
--	new: overrides
-----------------------------------------------------------------------------*/

static int tek_ui_class_frame_new(lua_State *L)
{
	int narg = lua_gettop(L);
	lua_settop(L, 2);
	lua_getfield(L, ISUPERCLASS, "new");
	lua_pushvalue(L, ISELF);
	
	if (narg >= 2)
		lua_pushvalue(L, 2);
	else
		lua_newtable(L);
	
	/* s: SuperClass.new(), class, self */
	
	lua_pushboolean(L, 0);
	lua_setfield(L, -2, "BorderObject");
	
	lua_pushboolean(L, 0);
	lua_setfield(L, -2, "BorderRegion");
	
	lua_getfield(L, -1, "Legend");
	int have_legend = lua_toboolean(L, -1);
	lua_pop(L, 1);
	if (!have_legend)
	{
		lua_pushboolean(L, 0);
		lua_setfield(L, -2, "Legend");
	}

	/*	self = Superclass.new(class, self) */
	lua_call(L, 2, 1);
	
	/*	if self.Legend then self:addStyleClass("button") end */
	if (have_legend)
	{
		lua_getfield(L, -1, "addStyleClass");
		lua_pushvalue(L, -2);
		lua_pushliteral(L, "legend");
		lua_call(L, 2, 0);
	}
	
	return 1;
}

/*-----------------------------------------------------------------------------
--	newBorderObject: internal
-----------------------------------------------------------------------------*/

static void newBorderObject(lua_State *L)
{
	lua_Integer b1, b2, b3, b4;
	lua_getfield(L, ISELF, "Properties");
	lua_getfield(L, -1, "border-left-width");
	lua_getfield(L, -2, "border-top-width");
	lua_getfield(L, -3, "border-right-width");
	lua_getfield(L, -4, "border-bottom-width");
	b1 = lua_tointeger(L, -4);
	b2 = lua_tointeger(L, -3);
	b3 = lua_tointeger(L, -2);
	b4 = lua_tointeger(L, -1);
	lua_pop(L, 4);
	if (b1 > 0 || b2 > 0 || b3 > 0 || b4 > 0)
	{
		lua_getfield(L, ITEKUI, "createHook");
		lua_pushliteral(L, "border");
		lua_getfield(L, -3, "border-class");
		/* s: Properties, createHook(), "border", props["border-class"] */
		if (!lua_toboolean(L, -1))
		{
			lua_pop(L, 1);
			lua_pushliteral(L, "default");
		}
		lua_pushvalue(L, ISELF);
		/* s: Properties, createHook(), "border", props["border-class"] or "default", self */
		lua_newtable(L);
		lua_newtable(L);
		lua_pushinteger(L, b1);
		lua_rawseti(L, -2, 1);
		lua_pushinteger(L, b2);
		lua_rawseti(L, -2, 2);
		lua_pushinteger(L, b3);
		lua_rawseti(L, -2, 3);
		lua_pushinteger(L, b4);
		lua_rawseti(L, -2, 4);
		lua_setfield(L, -2, "Border");
		/* { Border = { b1, b2, b3, b4 } } */
		lua_getfield(L, 1, "Legend");
		lua_setfield(L, -2, "Legend");
		lua_getfield(L, 1, "Style");
		lua_setfield(L, -2, "Style");
		/* { Border = { b1, b2, b3, b4 }, Legend = self.Legend, Style = self.Style } */
		/* s: Properties, createHook(), "border", props["border-class"] or "default", self, {} */
		lua_call(L, 4, 1);
		lua_setfield(L, 1, "BorderObject");
	}
	else
	{
		lua_pushboolean(L, TFALSE);
		lua_setfield(L, ISELF, "BorderObject");
	}
	lua_pop(L, 1);
}

/*-----------------------------------------------------------------------------
--	setup: overrides
-----------------------------------------------------------------------------*/

static int tek_ui_class_frame_setup(lua_State *L)
{
	lua_settop(L, 3);
	lua_getfield(L, ISUPERCLASS, "setup");
	lua_pushvalue(L, ISELF);
	lua_pushvalue(L, 2);
	lua_pushvalue(L, 3);
	lua_call(L, 3, 0);
	newBorderObject(L);
	return 0;
}
	
/*-----------------------------------------------------------------------------
--	cleanup: overrides
-----------------------------------------------------------------------------*/
	
static int tek_ui_class_frame_cleanup(lua_State *L)
{
	lua_pushboolean(L, 0);
	lua_setfield(L, ISELF, "BorderRegion");
	lua_getfield(L, ITEKUI, "destroyHook");
	lua_getfield(L, ISELF, "BorderObject");
	lua_call(L, 1, 0);
	lua_pushboolean(L, 0);
	lua_setfield(L, ISELF, "BorderObject");
	lua_getfield(L, ISUPERCLASS, "cleanup");
	lua_pushvalue(L, ISELF);
	lua_call(L, 1, 0);
	return 0;
}

/*
**	b1, b2, b3, b4 = Frame:getBorder(): Returns an element's border widths in
**	the order left, top, right, bottom.
*/

static int tek_ui_class_frame_getborder(lua_State *L)
{
	lua_getfield(L, ISELF, "BorderObject");
	if (lua_toboolean(L, -1))
	{
		lua_getfield(L, -1, "getBorder");
		lua_insert(L, -2);
		lua_call(L, 1, 4);
	}
	else
	{
		lua_pop(L, 1);
		lua_pushinteger(L, 0);
		lua_pushinteger(L, 0);
		lua_pushinteger(L, 0);
		lua_pushinteger(L, 0);
	}
	return 4;
}

/*-----------------------------------------------------------------------------
--	damage: overrides
-----------------------------------------------------------------------------*/

static int tek_ui_class_frame_damage(lua_State *L)
{
	lua_settop(L, 5);
	lua_getfield(L, ISUPERCLASS, "damage");
	lua_pushvalue(L, ISELF);
	lua_Integer r1 = lua_tointeger(L, 2);
	lua_Integer r2 = lua_tointeger(L, 3);
	lua_Integer r3 = lua_tointeger(L, 4);
	lua_Integer r4 = lua_tointeger(L, 5);
	lua_pushinteger(L, r1);
	lua_pushinteger(L, r2);
	lua_pushinteger(L, r3);
	lua_pushinteger(L, r4);
	lua_call(L, 5, 0);
	lua_getfield(L, ISELF, "BorderRegion");
	if (lua_toboolean(L, -1))
	{
		lua_getfield(L, -1, "checkIntersect");
		lua_insert(L, -2);
		/* checkIntersect(), BorderRegion */
		lua_pushinteger(L, r1);
		lua_pushinteger(L, r2);
		lua_pushinteger(L, r3);
		lua_pushinteger(L, r4);
		lua_call(L, 5, 1);
		/* result */
		if (lua_toboolean(L, -1))
		{
			lua_getfield(L, ISELF, "setFlags");
			lua_pushvalue(L, ISELF);
			lua_pushinteger(L, 0x0004); /* FL_REDRAWBORDER */
			lua_call(L, 2, 0);
		}
	}
	lua_pop(L, 1);
	return 0;
}

/*-----------------------------------------------------------------------------
--	layoutBorder: internal
-----------------------------------------------------------------------------*/

static int layoutborder(lua_State *L)
{
	int result = 0;
	lua_getfield(L, ISELF, "BorderObject");
	if (lua_toboolean(L, -1))
	{
		lua_getfield(L, -1, "getRegion");
		lua_pushvalue(L, -2);
		/* BorderObject, getRegion(), BorderObject */
		lua_getfield(L, ISELF, "BorderRegion");
		if (!lua_toboolean(L, -1))
		{
			lua_pop(L, 1);
			lua_getfield(L, IREGION, "new");
			lua_call(L, 0, 1);
		}
		/* BorderObject, getRegion(), BorderObject, BorderRegion or Region.new() */
		lua_call(L, 2, 1);
		lua_setfield(L, ISELF, "BorderRegion");
		result = 1;
	}
	lua_pop(L, 1);
	return result;
}

/*-----------------------------------------------------------------------------
--	layout: overrides. Additionally maintains a border region.
-----------------------------------------------------------------------------*/

static int tek_ui_class_frame_layout(lua_State *L)
{
	lua_settop(L, 6);
	lua_getfield(L, ISUPERCLASS, "layout");
	lua_pushvalue(L, ISELF);
	lua_pushvalue(L, 2);
	lua_pushvalue(L, 3);
	lua_pushvalue(L, 4);
	lua_pushvalue(L, 5);
	lua_pushvalue(L, 6);
	lua_call(L, 6, 2);
	int border_ok = lua_toboolean(L, -1);
	lua_pop(L, 1);
	if (lua_toboolean(L, -1))
	{
		if (layoutborder(L) && !border_ok && 
			!(lua_isboolean(L, 6) && !lua_toboolean(L, 6)))
		{
			lua_getfield(L, ISELF, "setFlags");
			lua_pushvalue(L, ISELF);
			lua_pushinteger(L, 0x0004); /* FL_REDRAWBORDER */
			lua_call(L, 2, 0);
		}
	}
	return 1;
}

/*-----------------------------------------------------------------------------
--	punch: overrides
-----------------------------------------------------------------------------*/

static int tek_ui_class_frame_punch(lua_State *L)
{
	lua_settop(L, 2);
	lua_getfield(L, ISUPERCLASS, "punch");
	lua_pushvalue(L, ISELF);
	lua_pushvalue(L, 2);
	lua_call(L, 2, 0);
	lua_getfield(L, 2, "subRegion");
	lua_pushvalue(L, 2);
	lua_getfield(L, ISELF, "BorderRegion");
	lua_call(L, 2, 0);
	return 0;
}

/*
**	Frame:drawBorder(): Draws an element's border.
*/

static int tek_ui_class_frame_drawborder(lua_State *L)
{
	lua_getfield(L, ISELF, "BorderObject");
	if (lua_toboolean(L, -1))
	{
		/* bo */
		lua_getfield(L, ISELF, "checkFlags");
		lua_pushvalue(L, ISELF);
		lua_pushinteger(L, 0x0010); /* FL_SHOW */
		/* bo, self, FL_SHOW */
		lua_call(L, 2, 1);
		/* bo, res */
		if (lua_toboolean(L, -1))
		{
			lua_getfield(L, -2, "draw");
			lua_pushvalue(L, -3);
			lua_getfield(L, ISELF, "Window");
			lua_getfield(L, -1, "Drawable");
			lua_remove(L, -2);
			/* bo, res, draw, bo, self.Window.Drawable */
			lua_call(L, 2, 0);
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return 0;
}

/*-----------------------------------------------------------------------------
--	draw: overrides
-----------------------------------------------------------------------------*/

static int tek_ui_class_frame_draw(lua_State *L)
{
	lua_getfield(L, ISUPERCLASS, "draw");
	lua_pushvalue(L, ISELF);
	lua_call(L, 1, 1);
	lua_getfield(L, ISELF, "checkClearFlags");
	lua_pushvalue(L, ISELF);
	lua_pushinteger(L, 0x0004); /* FL_REDRAWBORDER */
	lua_call(L, 2, 1);
	if (lua_toboolean(L, -1))
		tek_ui_class_frame_drawborder(L);
	lua_pop(L, 1);
	return 1;
}

/*-----------------------------------------------------------------------------
--	getByXY: overrides
-----------------------------------------------------------------------------*/

static int tek_ui_class_frame_getbyxy(lua_State *L)
{
	lua_settop(L, 3);
	lua_getfield(L, ISELF, "getRect");
	lua_pushvalue(L, ISELF);
	lua_call(L, 1, 4);
	if (lua_toboolean(L, -1))
	{
		lua_Integer r1 = lua_tointeger(L, -4);
		lua_Integer r2 = lua_tointeger(L, -3);
		lua_Integer r3 = lua_tointeger(L, -2);
		lua_Integer r4 = lua_tointeger(L, -1);
		lua_pop(L, 4);
		lua_getfield(L, ISELF, "getBorder");
		lua_pushvalue(L, ISELF);
		lua_call(L, 1, 4);
		lua_Integer b1 = lua_tointeger(L, -4);
		lua_Integer b2 = lua_tointeger(L, -3);
		lua_Integer b3 = lua_tointeger(L, -2);
		lua_Integer b4 = lua_tointeger(L, -1);
		lua_pop(L, 4);
		lua_Integer x = lua_tointeger(L, 2);
		lua_Integer y = lua_tointeger(L, 3);
		if (x >= r1 - b1 && x <= r3 + b3 && y >= r2 - b2 && y <= r4 + b4)
		{
			lua_pushvalue(L, ISELF);
			return 1;
		}
	}
	else
		lua_pop(L, 4);
	return 0;
}

/*-----------------------------------------------------------------------------
--	getMargin: overrides
-----------------------------------------------------------------------------*/

static int tek_ui_class_frame_getmargin(lua_State *L)
{
	lua_getfield(L, ISUPERCLASS, "getMargin");
	lua_pushvalue(L, ISELF);
	lua_call(L, 1, 4);
	lua_Integer m1 = lua_tointeger(L, -4);
	lua_Integer m2 = lua_tointeger(L, -3);
	lua_Integer m3 = lua_tointeger(L, -2);
	lua_Integer m4 = lua_tointeger(L, -1);
	lua_pop(L, 4);
	lua_getfield(L, ISELF, "getBorder");
	lua_pushvalue(L, ISELF);
	lua_call(L, 1, 4);
	lua_Integer b1 = lua_tointeger(L, -4);
	lua_Integer b2 = lua_tointeger(L, -3);
	lua_Integer b3 = lua_tointeger(L, -2);
	lua_Integer b4 = lua_tointeger(L, -1);
	lua_pop(L, 4);
	lua_pushinteger(L, m1 + b1);
	lua_pushinteger(L, m2 + b2);
	lua_pushinteger(L, m3 + b3);
	lua_pushinteger(L, m4 + b4);
	return 4;
}

/*-----------------------------------------------------------------------------
--	onSetStyle: overrides
-----------------------------------------------------------------------------*/

static int tek_ui_class_frame_onsetstyle(lua_State *L)
{
	lua_getfield(L, ISUPERCLASS, "onSetStyle");
	lua_pushvalue(L, ISELF);
	lua_call(L, 1, 0);
	lua_getfield(L, ISELF, "checkFlags");
	lua_pushvalue(L, ISELF);
	lua_pushinteger(L, 0x0009); /* FL_SETUP | FL_LAYOUT */
	lua_call(L, 2, 1);
	if (lua_toboolean(L, -1))
	{
		newBorderObject(L);
		layoutborder(L);
		lua_getfield(L, ISELF, "setFlags");
		lua_pushvalue(L, ISELF);
		lua_pushinteger(L, 0x0004); /* FL_REDRAWBORDER */
		lua_call(L, 2, 0);
	}
	lua_pop(L, 1);
	return 0;
}

/*-----------------------------------------------------------------------------
--	setState: overrides
-----------------------------------------------------------------------------*/

static int tek_ui_class_frame_setstate(lua_State *L)
{
	lua_settop(L, 2);
	lua_getfield(L, ISUPERCLASS, "setState");
	lua_pushvalue(L, ISELF);
	lua_pushvalue(L, 2);
	lua_call(L, 2, 0);
	lua_getfield(L, ISELF, "setFlags");
	lua_pushvalue(L, ISELF);
	lua_pushinteger(L, 0x0004); /* FL_REDRAWBORDER */
	lua_call(L, 2, 0);
	return 0;
}

/*-----------------------------------------------------------------------------
--	reconfigure: overrides
-----------------------------------------------------------------------------*/

static int tek_ui_class_frame_reconfigure(lua_State *L)
{
	lua_getfield(L, ISUPERCLASS, "reconfigure");
	lua_pushvalue(L, ISELF);
	lua_call(L, 1, 0);
	newBorderObject(L);
	lua_pushboolean(L, TFALSE);
	lua_setfield(L, ISELF, "BorderRegion");
	return 0;
}

/*****************************************************************************/

static const luaL_Reg tek_ui_class_frame_classfuncs[] =
{
	{ "new", tek_ui_class_frame_new },
	{ "setup", tek_ui_class_frame_setup },
	{ "cleanup", tek_ui_class_frame_cleanup },
	{ "getBorder", tek_ui_class_frame_getborder },
	{ "damage", tek_ui_class_frame_damage },
	{ "layout", tek_ui_class_frame_layout },
	{ "punch", tek_ui_class_frame_punch },
	{ "draw", tek_ui_class_frame_draw },
	{ "getByXY", tek_ui_class_frame_getbyxy },
	{ "getMargin", tek_ui_class_frame_getmargin },
	{ "onSetStyle", tek_ui_class_frame_onsetstyle },
	{ "setState", tek_ui_class_frame_setstate },
	{ "reconfigure", tek_ui_class_frame_reconfigure },
	{ NULL, NULL }
};

/*****************************************************************************/

int luaopen_tek_ui_class_frame(lua_State *L)
{
	lua_getglobal(L, "require");
	lua_pushliteral(L, FRAME_SUPERCLASS_NAME);
	lua_call(L, 1, 1);
	lua_getfield(L, -1, "newClass");
	lua_insert(L, -2);
	/* s: newClass(), superclass */
	
	/* pass superclass, tek.ui and required libraries as upvalues: */
	lua_pushvalue(L, -1);
	lua_getglobal(L, "require");
	lua_pushliteral(L, "tek.ui");
	lua_call(L, 1, 1);
	lua_getfield(L, -1, "checkVersion");
	lua_pushinteger(L, FRAME_TEKUI_VERSION);
	lua_call(L, 1, 0);
	
	lua_getfield(L, -1, "loadLibrary");
	lua_pushliteral(L, "region");	
	lua_pushinteger(L, FRAME_REGION_VERSION);
	lua_call(L, 2, 1);
	/* s: newClass(), superclass, superclass, tek.ui, Region */
	tek_lua_register(L, FRAME_CLASS_NAME, tek_ui_class_frame_classfuncs, 3);
	/* s: newClass(), superclass, class */
	
	/* insert name and version: */
	lua_pushliteral(L, FRAME_CLASS_NAME);
	lua_setfield(L, -2, "_NAME");
	lua_pushliteral(L, FRAME_CLASS_VERSION);
	lua_setfield(L, -2, "_VERSION");

	/* inherit: class = superclass.newClass(superclass, class) */
	lua_call(L, 2, 1); 
	/* s: class */
	return 1;
}
