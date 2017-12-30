
/*
**	tek.lib.visual - binding of TEKlib 'Visual' module to Lua
**	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
**	See copyright notice in COPYRIGHT
*/

#include <string.h>
#include <tek/lib/tek_lua.h>
#include <tek/mod/exec.h>
#include <tek/proto/hal.h>
#include "visual_lua.h"
#if defined(ENABLE_PIXMAP_CACHE)
#include <tek/lib/cachemanager.h>
#endif

#if !defined(DISPLAY_DRIVER)
#define DISPLAY_DRIVER "x11"
#endif

/*****************************************************************************/

static const struct TInitModule tek_lib_visual_initmodules[] =
{
	{ "visual", tek_init_visual, TNULL, 0 },
	{ TNULL, TNULL, TNULL, 0 }
};

static const luaL_Reg tek_lib_visual_funcs[] =
{
	{ "open", tek_lib_visual_open },
	{ "close", tek_lib_visual_close },
	{ "sleep", tek_lib_visual_sleep },
	{ "openFont", tek_lib_visual_openfont },
	{ "closeFont", tek_lib_visual_closefont },
	{ "getFontAttrs", tek_lib_visual_getfontattrs },
	{ "getTextSize", tek_lib_visual_textsize_font },
	{ "getTime", tek_lib_visual_gettime },
	{ "wait", tek_lib_visual_wait },
	{ "getMsg", tek_lib_visual_getmsg },
	{ "createPixmap", tek_lib_visual_createpixmap },
	{ "createGradient", tek_lib_visual_creategradient },
	{ "getDisplayAttrs", tek_lib_visual_getdisplayattrs },
	{ TNULL, TNULL }
};

static const luaL_Reg tek_lib_visual_methods[] =
{
	{ "__gc", tek_lib_visual_close },
	{ "setInput", tek_lib_visual_setinput },
	{ "clearInput", tek_lib_visual_clearinput },
	{ "close", tek_lib_visual_close },
	{ "allocPen", tek_lib_visual_allocpen },
	{ "freePen", tek_lib_visual_freepen },
	{ "fillRect", tek_lib_visual_frect },
	{ "drawRect", tek_lib_visual_rect },
	{ "drawLine", tek_lib_visual_line },
	{ "drawPoint", tek_lib_visual_plot },
	{ "drawText", tek_lib_visual_text },
	{ "drawImage", tek_lib_visual_drawimage },
	{ "getAttrs", tek_lib_visual_getattrs },
	{ "setAttrs", tek_lib_visual_setattrs },
	{ "textSize", tek_lib_visual_textsize_visual },
	{ "setFont", tek_lib_visual_setfont },
	{ "blitRect", tek_lib_visual_copyarea },
	{ "setClipRect", tek_lib_visual_setcliprect },
	{ "unsetClipRect", tek_lib_visual_unsetcliprect },
	{ "setShift", tek_lib_visual_setshift },
	{ "drawRGB", tek_lib_visual_drawrgb },
	{ "drawPixmap", tek_lib_visual_drawpixmap },
	{ "getUserdata", tek_lib_visual_getuserdata },
	{ "flush", tek_lib_visual_flush },
	{ "setTextureOrigin", tek_lib_visual_settextureorigin },
	{ "pushClipRect", tek_lib_visual_pushcliprect },
	{ "popClipRect", tek_lib_visual_popcliprect },
	{ "getClipRect", tek_lib_visual_getcliprect },
	{ "setBGPen", tek_lib_visual_setbgpen },
	{ "getSelection", tek_lib_visual_getselection },
	{ "getPaintInfo", tek_lib_visual_getpaintinfo },
	{ "setSelection", tek_lib_visual_setselection },
	{ TNULL, TNULL }
};

static const luaL_Reg tek_lib_visual_fontmethods[] =
{
	{ "__gc", tek_lib_visual_closefont },
	{ "getFontAttrs", tek_lib_visual_getfontattrs },
	{ "close", tek_lib_visual_closefont },
	{ "getTextSize", tek_lib_visual_textsize_font },
	{ TNULL, TNULL }
};

static const luaL_Reg tek_lib_visual_pixmapmethods[] =
{
	{ "__gc", tek_lib_visual_freepixmap },
	{ "free", tek_lib_visual_freepixmap },
	{ "getPixel", tek_lib_visual_getpixmap },
	{ "setPixel", tek_lib_visual_setpixmap },
	{ "getAttrs", tek_lib_visual_getpixmapattr },
	{ TNULL, TNULL }
};

/*****************************************************************************/

LOCAL LUACFUNC TINT
tek_lib_visual_open(lua_State *L)
{
	TTAGITEM tags[22], *tp = tags;
	TEKVisual *visbase, *vis;

	vis = lua_newuserdata(L, sizeof(TEKVisual));
	/* s: visinst */

	vis->vis_Visual = TNULL;
	vis->vis_isBase = TFALSE;
	vis->vis_refBase = -1;
	vis->vis_refFont = -1;
	vis->vis_refBGPen = -1;
	vis->vis_BGPen = TNULL;

	/* get and attach metatable: */
	luaL_newmetatable(L, TEK_LIB_VISUAL_CLASSNAME);
	/* s: visinst, vismeta */
	lua_pushvalue(L, -1);
	/* s: visinst, vismeta, vismeta */
	lua_setmetatable(L, -3);
	/* s: visinst, vismeta */

	lua_getfield(L, LUA_REGISTRYINDEX, TEK_LIB_VISUAL_BASECLASSNAME);
	/* s: visinst, vismeta, visbase */

	/* get visual module base: */
	visbase = (TEKVisual *) lua_touserdata(L, -1);
	if (visbase == TNULL)
	{
		lua_pop(L, 3);
		return 0;
	}

	vis->vis_VisBase = visbase;
	vis->vis_Base = visbase->vis_Base;
	vis->vis_ExecBase = visbase->vis_ExecBase;
	vis->vis_Font = visbase->vis_Font;
	vis->vis_FontHeight = visbase->vis_FontHeight;
#if defined(ENABLE_PIXMAP_CACHE)
	vis->vis_CacheManager = visbase->vis_CacheManager;
#endif
	vis->vis_Drawdata.nump = 0;
	vis->vis_Drawdata.points = TNULL;
	vis->vis_Drawdata.pens = TNULL;
	vis->vis_RectBuffer = TNULL;
	vis->vis_ShiftX = 0;
	vis->vis_ShiftY = 0;
	vis->vis_TextureX = 0;
	vis->vis_TextureY = 0;
	vis->vis_Display = visbase->vis_Display;
	vis->vis_HaveClipRect = TFALSE;
	TINITLIST(&vis->vis_FreeRects);
	TINITLIST(&vis->vis_ClipStack);
	
	/* place ref to base in metatable: */
	vis->vis_refBase = luaL_ref(L, -2);
	/* s: visinst, vismeta */

	/* place ref to self in metatable: */
	lua_pushvalue(L, -2);
	/* s: visinst, vismeta, visinst */
	vis->vis_refSelf = luaL_ref(L, -2);
	/* s: visinst, vismeta */

	/* place reference to userdata in metatable: */
	lua_getfield(L, 1, "UserData");
	if (!lua_isnil(L, -1))
		vis->vis_refUserData = luaL_ref(L, -2);
	else
	{
		vis->vis_refUserData = -1;
		lua_pop(L, 1);
	}

	/* place reference to pens in metatable: */
	lua_getfield(L, 1, "Pens");
	if (!lua_isnil(L, -1))
		vis->vis_refPens = luaL_ref(L, -2);
	else
	{
		vis->vis_refPens = -1;
		lua_pop(L, 1);
	}

	tp->tti_Tag = TVisual_Title;
	lua_getfield(L, 1, "Title");
	if (lua_isstring(L, -1))
		tp++->tti_Value = (TTAG) lua_tostring(L, -1);
	lua_pop(L, 1);

	tp->tti_Tag = TVisual_Borderless;
	lua_getfield(L, 1, "Borderless");
	if (lua_isboolean(L, -1))
		tp++->tti_Value = (TTAG) lua_toboolean(L, -1);
	lua_pop(L, 1);
	
	tp->tti_Tag = TVisual_PopupWindow;
	lua_getfield(L, 1, "PopupWindow");
	if (lua_isboolean(L, -1))
		tp++->tti_Value = (TTAG) lua_toboolean(L, -1);
	lua_pop(L, 1);

	tp->tti_Tag = TVisual_Center;
	lua_getfield(L, 1, "Center");
	if (lua_isboolean(L, -1))
		tp++->tti_Value = (TTAG) lua_toboolean(L, -1);
	lua_pop(L, 1);

	tp->tti_Tag = TVisual_FullScreen;
	lua_getfield(L, 1, "FullScreen");
	if (lua_isboolean(L, -1))
		tp++->tti_Value = (TTAG) lua_toboolean(L, -1);
	lua_pop(L, 1);

	tp->tti_Tag = TVisual_Width;
	lua_getfield(L, 1, "Width");
	if (lua_isnumber(L, -1))
		tp++->tti_Value = (TTAG) lua_tointeger(L, -1);
	lua_pop(L, 1);

	tp->tti_Tag = TVisual_Height;
	lua_getfield(L, 1, "Height");
	if (lua_isnumber(L, -1))
		tp++->tti_Value = (TTAG) lua_tointeger(L, -1);
	lua_pop(L, 1);

	tp->tti_Tag = TVisual_WinLeft;
	lua_getfield(L, 1, "Left");
	if (lua_isnumber(L, -1))
		tp++->tti_Value = (TTAG) lua_tointeger(L, -1);
	lua_pop(L, 1);

	tp->tti_Tag = TVisual_WinTop;
	lua_getfield(L, 1, "Top");
	if (lua_isnumber(L, -1))
		tp++->tti_Value = (TTAG) lua_tointeger(L, -1);
	lua_pop(L, 1);

	tp->tti_Tag = TVisual_MinWidth;
	lua_getfield(L, 1, "MinWidth");
	if (lua_isnumber(L, -1))
		tp++->tti_Value = (TTAG) lua_tointeger(L, -1);
	lua_pop(L, 1);

	tp->tti_Tag = TVisual_MinHeight;
	lua_getfield(L, 1, "MinHeight");
	if (lua_isnumber(L, -1))
		tp++->tti_Value = (TTAG) lua_tointeger(L, -1);
	lua_pop(L, 1);

	tp->tti_Tag = TVisual_MaxWidth;
	lua_getfield(L, 1, "MaxWidth");
	if (lua_isnumber(L, -1))
		tp++->tti_Value = (TTAG) lua_tointeger(L, -1);
	lua_pop(L, 1);

	tp->tti_Tag = TVisual_MaxHeight;
	lua_getfield(L, 1, "MaxHeight");
	if (lua_isnumber(L, -1))
		tp++->tti_Value = (TTAG) lua_tointeger(L, -1);
	lua_pop(L, 1);

	tp->tti_Tag = TVisual_EventMask;
	lua_getfield(L, 1, "EventMask");
	if (lua_isnumber(L, -1))
		tp++->tti_Value = (TTAG) lua_tointeger(L, -1);
	lua_pop(L, 1);

	tp->tti_Tag = TVisual_BlankCursor;
	lua_getfield(L, 1, "BlankCursor");
	if (lua_isboolean(L, -1))
		tp++->tti_Value = (TTAG) lua_toboolean(L, -1);
	lua_pop(L, 1);
	
	tp->tti_Tag = TVisual_ExtraArgs;
	lua_getfield(L, 1, "ExtraArgs");
	if (lua_isstring(L, -1))
		tp++->tti_Value = (TTAG) lua_tostring(L, -1);
	lua_pop(L, 1);

	
	lua_getfield(L, 1, "MsgFileNo");
	if (lua_isnumber(L, -1))
		visbase->vis_IOFileNo = lua_tointeger(L, -1);
	lua_pop(L, 1);

	tp->tti_Tag = TVisual_Display;
	tp++->tti_Value = (TTAG) vis->vis_Display;

	tp->tti_Tag = TVisual_UserData;
	tp++->tti_Value = (TTAG) vis->vis_refSelf;

	tp->tti_Tag = TVisual_CmdRPort;
	tp++->tti_Value = (TTAG) visbase->vis_CmdRPort;

	tp->tti_Tag = TVisual_IMsgPort;
	tp++->tti_Value = (TTAG) visbase->vis_IMsgPort;

	
	tp->tti_Tag = TTAG_DONE;

	vis->vis_Visual = TVisualOpen(visbase->vis_Base, tags);
	if (vis->vis_Visual)
	{
		TTAGITEM attrs[3];
		struct TVRequest *req;
		TAPTR window;
		TBOOL success = TTRUE;
		
		vis->vis_Device = TNULL;
		vis->vis_FlushReq = TNULL;
		
		attrs[0].tti_Tag = TVisual_Device;
		attrs[0].tti_Value = (TTAG) &vis->vis_Device;
		attrs[1].tti_Tag = TVisual_Window;
		attrs[1].tti_Value = (TTAG) &window;
		attrs[2].tti_Tag = TTAG_DONE;
		TVisualGetAttrs(vis->vis_Visual, attrs);
		
		if (vis->vis_Device)
		{
			success = TFALSE;
			req = (struct TVRequest *) TDisplayAllocReq(vis->vis_Device);
			if (req)
			{
				req->tvr_Req.io_Command = TVCMD_FLUSH;
				req->tvr_Op.Flush.Window = window;
				req->tvr_Op.Flush.Rect[0] = 0;
				req->tvr_Op.Flush.Rect[1] = 0;
				req->tvr_Op.Flush.Rect[2] = -1;
				req->tvr_Op.Flush.Rect[3] = -1;
				vis->vis_FlushReq = req;
				success = TTRUE;
			}
		}
			
		if (success)
		{
			#if defined(TEK_VISUAL_DEBUG)
			vis->vis_DebugPen1 = TVisualAllocPen(vis->vis_Visual, 0x88ff00);
			vis->vis_DebugPen2 = TVisualAllocPen(vis->vis_Visual, 0x000000);
			#endif
			TVisualSetFont(vis->vis_Visual, vis->vis_Font);
			lua_pop(L, 1);

			if (!visbase->vis_IOTask)
				tek_lib_visual_io_open(visbase);
			return 1;
		}
		
		TVisualClose(visbase->vis_Base, vis->vis_Visual);
		vis->vis_Visual = TNULL;
	}
	
	TDBPRINTF(TDB_ERROR,("Failed to open visual\n"));
	lua_pop(L, 2);
	lua_pushnil(L);
	return 1;
}

/*****************************************************************************/

LOCAL LUACFUNC TINT
tek_lib_visual_close(lua_State *L)
{
	TEKVisual *vis = luaL_checkudata(L, 1, TEK_LIB_VISUAL_CLASSNAME);
	struct TExecBase *TExecBase = vis->vis_ExecBase;

	TDBPRINTF(TDB_TRACE,("visual %08x closing\n", vis));

	TFree(vis->vis_Drawdata.pens);
	vis->vis_Drawdata.pens = TNULL;

	TFree(vis->vis_Drawdata.points);
	vis->vis_Drawdata.points = TNULL;

	TFree(vis->vis_RectBuffer);
	vis->vis_RectBuffer = TNULL;

	if (vis->vis_Visual)
	{
		TEKVisual *visbase;
		struct TNode *next, *node;
		struct TList tmplist;
		struct TMsgPort *port;

		lua_getfield(L, LUA_REGISTRYINDEX, TEK_LIB_VISUAL_BASECLASSNAME);
		/* s: visbase */
		visbase = lua_touserdata(L, -1);
		lua_pop(L, 1);
		port = visbase->vis_IMsgPort;

		/* stop sending messages: */
		TVisualSetInput(vis->vis_Visual, TITYPE_ALL, TITYPE_NONE);

		/* clean up message port: */
		TInitList(&tmplist);
		THALLock(visbase->vis_ExecBase->texb_HALBase, &port->tmp_Lock);
		node = port->tmp_MsgList.tlh_Head.tln_Succ;
		for (; (next = node->tln_Succ); node = next)
		{
			TIMSG *imsg = TGETMSGBODY(node);
			if (imsg->timsg_UserData == vis->vis_refSelf)
			{
				TRemove(node);
				TAddTail(&tmplist, node);
			}
		}
		THALUnlock(visbase->vis_ExecBase->texb_HALBase, &port->tmp_Lock);

		while ((node = TRemHead(&tmplist)))
			TAckMsg(TGETMSGBODY(node));
	}

	if (vis->vis_refBase >= 0)
	{
		int ui = lua_upvalueindex(1);
		if (vis->vis_refPens >= 0)
			luaL_unref(L, ui, vis->vis_refPens);
		if (vis->vis_refUserData >= 0)
			luaL_unref(L, ui, vis->vis_refUserData);
		if (vis->vis_refBGPen >= 0)
			luaL_unref(L, ui, vis->vis_refBGPen);
		luaL_unref(L, ui, vis->vis_refSelf);
		luaL_unref(L, ui, vis->vis_refBase);
		vis->vis_refBase = -1;
		TDBPRINTF(TDB_TRACE,("visual %08x unref'd\n", vis));
	}

	if (vis->vis_Visual)
	{
		struct TNode *node;
		while ((node = TRemHead(&vis->vis_ClipStack)))
			TFree(node);
		while ((node = TRemHead(&vis->vis_FreeRects)))
			TFree(node);
		
		if (vis->vis_Device)
			TDisplayFreeReq(vis->vis_Device, 
				(struct TVRequest *) vis->vis_FlushReq);
	
		TVisualClose(vis->vis_Base, vis->vis_Visual);
		vis->vis_Visual = TNULL;
		TDBPRINTF(TDB_INFO,("visual instance %08x closed\n", vis));
	}

	if (vis->vis_isBase)
	{
		tek_lib_visual_io_close(vis);
		
		TDestroy((struct THandle *) vis->vis_IMsgPort);
		TDestroy((struct THandle *) vis->vis_CmdRPort);
		if (vis->vis_Base)
		{
#if defined(ENABLE_PIXMAP_CACHE)
			TDestroy(vis->vis_CacheManager);
#endif
			TVisualCloseFont(vis->vis_Base, vis->vis_Font);
			TCloseModule(vis->vis_Base);
		}
		/* collected visual base; remove TEKlib module: */
		TRemModules((struct TModInitNode *) &vis->vis_InitModules, 0);
		TDBPRINTF(TDB_INFO,("visual module removed\n"));
		
		TFree(vis->vis_DrawBuffer);
	}

	return 0;
}

/*****************************************************************************/

TMODENTRY int luaopen_tek_lib_visual(lua_State *L)
{
	TEKVisual *vis;
	struct TExecBase *TExecBase;
	
	/* register input message */
	luaL_newmetatable(L, "tek_msg*");
	lua_pushcfunction(L, tek_msg_reply);
	lua_setfield(L, -2, "__gc");
	lua_pushcfunction(L, tek_msg_index);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, tek_msg_len);
	lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	/* require "tek.lib.display.x11": */
	lua_getglobal(L, "require");
	/* s: "require" */
	lua_pushliteral(L, "tek.lib.display." DISPLAY_DRIVER);
	/* s: "require", "tek.lib.display.x11" */
	lua_call(L, 1, 1);
	/* s: displaytab */

	/* require "tek.lib.exec": */
	lua_getglobal(L, "require");
	/* s: displaytab, "require" */
	lua_pushliteral(L, "tek.lib.exec");
	/* s: displaytab, "require", "tek.lib.exec" */
	lua_call(L, 1, 1);
	/* s: displaytab, exectab */
	lua_getfield(L, -1, "base");
	/* s: displaytab, exectab, execbase */
	TExecBase = *(TAPTR *) lua_touserdata(L, -1);

	/* register functions: */
	tek_lua_register(L, "tek.lib.visual", tek_lib_visual_funcs, 0);
	/* s: displaytab, exectab, execbase, vistab */

	lua_pushvalue(L, -1);
	/* s: displaytab, exectab, execbase, vistab, vistab */
	lua_insert(L, -5);
	/* s: vistab, displaytab, exectab, execbase, vistab */

	lua_pushstring(L, TEK_LIB_VISUAL_VERSION);
	lua_setfield(L, -2, "_VERSION");

	/* create userdata: */
	vis = lua_newuserdata(L, sizeof(TEKVisual));
	memset(vis, 0, sizeof(TEKVisual));
	/* s: displaytab, exectab, execbase, vistab, visbase */

	vis->vis_Base = TNULL;
	vis->vis_ExecBase = TExecBase;
	vis->vis_Visual = TNULL;
	vis->vis_refBase = -1;
	vis->vis_isBase = TTRUE;
	vis->vis_DrawBuffer = TNULL;
	vis->vis_IOFileNo = -1; /* default */

	/* register base: */
	lua_pushvalue(L, -1);
	/* s: displaytab, exectab, execbase, vistab, visbase, visbase */
	lua_setfield(L, LUA_REGISTRYINDEX, TEK_LIB_VISUAL_BASECLASSNAME);
	/* s: displaytab, exectab, execbase, vistab, visbase */

	/* create metatable for userdata, register methods: */
	luaL_newmetatable(L, TEK_LIB_VISUAL_CLASSNAME);
	/* s: displaytab, exectab, execbase, vistab, visbase, vismeta */
	lua_pushvalue(L, -1);
	/* s: displaytab, exectab, execbase, vistab, visbase, vismeta, vismeta */
	lua_setfield(L, -2, "__index");
	/* s: displaytab, exectab, execbase, vistab, visbase, vismeta */
	lua_pushvalue(L, -1);
	luaL_newmetatable(L, TEK_LIB_VISUALPEN_CLASSNAME);
	luaL_newmetatable(L, TEK_LIB_VISUALPIXMAP_CLASSNAME);
	luaL_newmetatable(L, TEK_LIB_VISUALGRADIENT_CLASSNAME);
	
	tek_lua_register(L, NULL, tek_lib_visual_methods, 4);
	
	/* s: displaytab, exectab, execbase, vistab, visbase, vismeta */
	lua_setmetatable(L, -2);
	/* s: displaytab, exectab, execbase, vistab, visbase */

	/* place exec reference in metatable: */
	lua_getmetatable(L, -1);
	/* s: displaytab, exectab, execbase, vistab, visbase, vismeta */
	lua_pushvalue(L, -4);
	/* s: displaytab, exectab, execbase, vistab, visbase, vismeta, execbase */
	luaL_ref(L, -2); /* index returned is always 1 */
	/* s: displaytab, exectab, execbase, vistab, visbase, vismeta */
 	lua_pop(L, 6);

	/* prepare pixmap metatable and store reference in metatable: */
	luaL_newmetatable(L, TEK_LIB_VISUALPIXMAP_CLASSNAME);
	/* s: meta */
	lua_pushvalue(L, -1);
	/* s: meta, meta */
	lua_setfield(L, -2, "__index");
	/* s: meta */
	tek_lua_register(L, NULL, tek_lib_visual_pixmapmethods, 0);
	lua_pop(L, 1);
	
	/* prepare font metatable and store reference in metatable: */
	luaL_newmetatable(L, TEK_LIB_VISUALFONT_CLASSNAME);
	/* s: fontmeta */
	lua_pushvalue(L, -1);
	/* s: fontmeta, fontmeta */
	lua_setfield(L, -2, "__index");
	/* s: fontmeta */
	tek_lua_register(L, NULL, tek_lib_visual_fontmethods, 0);
	lua_pop(L, 1);

	/* Add visual module to TEKlib's internal module list: */
	memset(&vis->vis_InitModules, 0, sizeof vis->vis_InitModules);
	vis->vis_InitModules.tmin_Modules = tek_lib_visual_initmodules;
	TAddModules(&vis->vis_InitModules, 0);

	for (;;)
	{
		TTAGITEM ftags[2];
		TTAGITEM dtags[4];

		/* Open the Visual module: */
		vis->vis_Base = TOpenModule("visual", 0, TNULL);
		if (vis->vis_Base == TNULL)
			break;
		vis->vis_CmdRPort = TCreatePort(TNULL);
		if (vis->vis_CmdRPort == TNULL) 
			break;
		vis->vis_IMsgPort = TCreatePort(TNULL);
		if (vis->vis_IMsgPort == TNULL) 
			break;
#if defined(ENABLE_PIXMAP_CACHE)
		vis->vis_CacheManager = cachemanager_create(TExecBase);
		if (!vis->vis_CacheManager)
			break;
#endif
		
		vis->vis_HaveWindowManager = TTRUE;
		/* Open a display: */
		dtags[0].tti_Tag = TVisual_DisplayName;
		dtags[0].tti_Value = (TTAG) "display_" DISPLAY_DRIVER;
		dtags[1].tti_Tag = TVisual_IMsgPort;
		dtags[1].tti_Value = (TTAG) vis->vis_IMsgPort;
		dtags[2].tti_Tag = TVisual_HaveWindowManager;
		dtags[2].tti_Value = (TTAG) &vis->vis_HaveWindowManager;
		dtags[3].tti_Tag = TTAG_DONE;
		vis->vis_Display = TVisualOpenDisplay(vis->vis_Base, dtags);
		if (vis->vis_Display == TNULL)
		{
			TDBPRINTF(TDB_ERROR,("Failed to open driver 'display_%s'\n",
				DISPLAY_DRIVER));
			break;
		}

		/* try to obtain default font: */
		vis->vis_Font = TNULL;
		vis->vis_FontHeight = 0;
		dtags[0].tti_Tag = TVisual_Display;
		dtags[0].tti_Value = (TTAG) vis->vis_Display;
		dtags[1].tti_Tag = TTAG_DONE;
		vis->vis_Font = TVisualOpenFont(vis->vis_Base, dtags);
		if (vis->vis_Font)
		{
			ftags[0].tti_Tag = TVisual_FontHeight;
			ftags[0].tti_Value = (TTAG) &vis->vis_FontHeight;
			ftags[1].tti_Tag = TTAG_DONE;
			TVisualGetFontAttrs(vis->vis_Base, vis->vis_Font, ftags);
			TDBPRINTF(TDB_INFO,("Driver '%s' supplied default font, size %dpx\n", 
				DISPLAY_DRIVER, vis->vis_FontHeight));
		}
		else
			TDBPRINTF(TDB_INFO,("Driver '%s' supplied no default font\n", 
				DISPLAY_DRIVER));

		/* success: */
		return 1;
	}

	TDestroy((struct THandle *) vis->vis_IMsgPort);
	TDestroy((struct THandle *) vis->vis_CmdRPort);
	vis->vis_IMsgPort = TNULL;
	vis->vis_CmdRPort = TNULL;

	luaL_error(L, "Visual initialization failure");
	return 0;
}
