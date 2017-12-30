#ifndef LUA_TEK_LIB_VISUAL_H
#define LUA_TEK_LIB_VISUAL_H

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/proto/visual.h>
#include <tek/proto/display.h>
#include <tek/lib/tekui.h>
#include <tek/lib/region.h>

/*****************************************************************************/

#define TEK_VISUAL_DEBUG

#define TEK_LIB_VISUAL_VERSION "Visual 4.3"
#define TEK_LIB_VISUAL_BASECLASSNAME "tek.lib.visual.base*"
#define TEK_LIB_VISUAL_CLASSNAME "tek.lib.visual*"
#define TEK_LIB_VISUALPEN_CLASSNAME "tek.lib.visual.pen*"
#define TEK_LIB_VISUALFONT_CLASSNAME "tek.lib.visual.font*"
#define TEK_LIB_VISUALPIXMAP_CLASSNAME "tek.lib.visual.pixmap*"
#define TEK_LIB_VISUALGRADIENT_CLASSNAME "tek.lib.visual.gradient*"

/*****************************************************************************/

#ifndef LUACFUNC
#define LUACFUNC TCALLBACK
#endif

#ifndef EXPORT
#define EXPORT TMODAPI
#endif

#ifndef LOCAL
#define LOCAL
#endif

/*****************************************************************************/

typedef struct
{
	TINT nump;
	TINT *points;
	TVPEN *pens;

} TEKDrawdata;

typedef struct TEKVisual
{
	/* Visual module base: */
	TAPTR vis_Base;
	/* VisBase: */
	struct TEKVisual *vis_VisBase;
	/* Execbase: */
	struct TExecBase *vis_ExecBase;
	/* Reference to base (stored in metatable): */
	int vis_refBase;
	/* Reference to self (stored in metatable): */
	int vis_refSelf;
	/* Reference to userdata (stored in metatable): */
	int vis_refUserData;
	/* Reference to pens (stored in metatable): */
	int vis_refPens;
	/* Is base instance: */
	TBOOL vis_isBase;

	/* Reference to font (stored in metatable): */
	int vis_refFont;
	/* Font (always default font in base): */
	TAPTR vis_Font;
	/* FontHeight (always default font height in base): */
	TINT vis_FontHeight;

	/* Visual Display ptr: */
	TAPTR vis_Display;

	/* Visual instance ptr: */
	TAPTR vis_Visual;

	TEKDrawdata vis_Drawdata;

	/* ShiftX/Y: */
	TINT vis_ShiftX, vis_ShiftY;
	/* Texture Origin X/Y */
	TINT vis_TextureX, vis_TextureY;

	TINT vis_RectBufferNum;
	TINT *vis_RectBuffer;

	struct TMsgPort *vis_CmdRPort;
	struct TMsgPort *vis_IMsgPort;
	
	TINT *vis_DrawBuffer;
	
	TBOOL vis_Dirty;
	TAPTR vis_Device;
	struct TVRequest *vis_FlushReq;
	
	struct TList vis_FreeRects;
	struct TList vis_ClipStack;
	RECTINT vis_ClipRect[4];
	TBOOL vis_HaveClipRect;
	
	struct TEKPen *vis_BGPen;
	int vis_refBGPen;
	TINT vis_BGPenType;
	
	struct TTask *vis_IOTask;
	TAPTR vis_IOData;
	int vis_IOFileNo;
	
	TBOOL vis_HaveWindowManager;
	TUINT vis_SignalsPending;
	
	struct TModInitNode vis_InitModules;
	
#if defined(ENABLE_PIXMAP_CACHE)
	struct THandle *vis_CacheManager;
#endif
	
#if defined(TEK_VISUAL_DEBUG)
	TBOOL vis_Debug;
	TVPEN vis_DebugPen1;
	TVPEN vis_DebugPen2;
#endif

} TEKVisual;

typedef struct TEKPen
{
	/* Pen object: */
	TVPEN pen_Pen;
	/* Visual: */
	TEKVisual *pen_Visual;

} TEKPen;

typedef struct
{
	struct TVPixBuf pxm_Image;
	TINT pxm_Width, pxm_Height;
	TUINT pxm_Flags;
	TEKVisual *pxm_VisualBase;
} TEKPixmap;

typedef struct
{
	/* Visualbase: */
	TEKVisual *font_VisBase;
	/* Font object: */
	TAPTR font_Font;
	/* Font height: */
	TUINT font_Height;
	/* underline position: */
	TINT font_UlPosition;
	/* underline thickness: */
	TINT font_UlThickness;

} TEKFont;


#if defined(ENABLE_GRADIENT)

typedef struct { float x, y; } vec;

typedef struct {
	vec vec;
	float r, g, b;
} rgbpt;

typedef struct
{
	rgbpt A, B;
} TEKGradient;

#endif

/*****************************************************************************/

LOCAL TBOOL tek_lib_visual_io_open(TEKVisual *vis);
LOCAL void tek_lib_visual_io_close(TEKVisual *vis);

LOCAL LUACFUNC TINT tek_lib_visual_open(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_close(lua_State *L);
LOCAL LUACFUNC int tek_lib_visual_getuserdata(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_getdisplayattrs(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_wait(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_sleep(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_openfont(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_closefont(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_textsize_font(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_gettime(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_setinput(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_clearinput(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_getmsg(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_allocpen(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_freepen(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_frect(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_rect(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_line(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_plot(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_text(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_drawimage(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_drawpixmap(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_getattrs(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_setattrs(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_textsize_visual(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_setfont(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_copyarea(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_setcliprect(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_unsetcliprect(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_setshift(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_drawrgb(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_getfontattrs(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_drawppm(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_createpixmap(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_freepixmap(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_getpixmap(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_setpixmap(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_flush(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_settextureorigin(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_pushcliprect(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_popcliprect(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_getcliprect(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_setbgpen(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_getselection(lua_State *L);
LOCAL LUACFUNC TINT tek_msg_reply(lua_State *L);
LOCAL LUACFUNC TINT tek_msg_index(lua_State *L);
LOCAL LUACFUNC TINT tek_msg_len(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_creategradient(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_getpaintinfo(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_getpixmapattr(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_scalepixmap(lua_State *L);
LOCAL LUACFUNC TINT tek_lib_visual_setselection(lua_State *L);

#endif
