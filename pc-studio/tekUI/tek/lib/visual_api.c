/*-----------------------------------------------------------------------------
--
--  tek.lib.visual
--  Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--  See copyright notice in COPYRIGHT
--
--  FUNCTIONS::
--      - Visual:allocPen() - Obtain a colored pen
--      - Visual:blitRect() - Move rectangular area
--      - Visual:clearInput() - Remove input sources
--      - Visual.close() - Close a visual
--      - Visual.closeFont() - Close font
--      - Visual.createGradient() - Create gradient
--      - Visual.createPixmap() - Create pixmap from file or table
--      - Visual:drawImage() - Draw simple vector image
--      - Visual:drawLine() - Draw line
--      - Visual:drawPixmap() - Draw pixmap
--      - Visual:drawPoint() - Draw pixel
--      - Visual:drawRect() - Draw rectangle
--      - Visual:drawRGB() - Draw table as RGB
--      - Visual:drawText() - Draw text
--      - Visual:fillRect() - Fill rectangle
--      - Visual:flush() - Flush changes to display
--      - Visual:freePen() - Release a colored pen
--      - Visual:getAttrs() - Retrieve attributes from a visual
--      - Visual:getClipRect() - Get active clipping rectangle
--      - Visual.getDisplayAttrs() - Get attributes from the display
--      - Visual.getFontAttrs() - Get font attributes
--      - Visual.getMsg() - Get next input message
--      - Visual:getPaintInfo() - Get type of the background paint
--      - Visual.getTextSize() - Get size of a text when rendered with a font
--      - Visual.getTime() - Get system time
--      - Visual:getUserdata() - Get a visual's userdata
--      - Visual.open() - Open a visual
--      - Visual.openFont() - Open a font
--      - Visual:popClipRect() - Pop rectangle from stack of clip rects
--      - Visual:pushClipRect() - Push rectangle on stack of clip rects
--      - Visual:setAttrs() - Set attributes in visual
--      - Visual:setBGPen() - Set visual's background pen, pixmap or gradient
--      - Visual:setClipRect() - Set clipping rectangle
--      - Visual:setFont() - Set the visual's current font
--      - Visual:setInput() - Add input sources
--      - Visuak:getSelection() - Get visual's selection or clipboard
--      - Visual:setSelection() - Set visual's selection or clipboard
--      - Visual:setShift() - Set coordinate displacement
--      - Visual:setTextureOrigin() - Set texture origin
--      - Visual.sleep() - Wait for number of milliseconds
--      - Visual:textSize() - Get size of text when rendered with current font
--      - Visual:unsetClipRect() - Unset clipping rectangle
--      - Visual.wait() - Wait for any event from any window
--  
-------------------------------------------------------------------------------

module "tek.lib.visual"
_VERSION = "Visual 1.2"
local Visual = _M

-----------------------------------------------------------------------------*/

#include <string.h>
#include "visual_lua.h"
#include <tek/lib/pixconv.h>
#include <tek/lib/imgload.h>
#include <tek/lib/tek_lua.h>

/*****************************************************************************/
/*
**  check userdata with classname from registry
*/

static TAPTR checkinstptr(lua_State *L, int n, const char *classname)
{
    TAPTR p = luaL_checkudata(L, n, classname);
    if (p == TNULL) luaL_argerror(L, n, "Closed handle");
    return p;
}

#define getpenptr(L, n) luaL_checkudata(L, n, TEK_LIB_VISUALPEN_CLASSNAME)
#define getpixmapptr(L, n) \
luaL_checkudata(L, n, TEK_LIB_VISUALPIXMAP_CLASSNAME)
#define checkfontptr(L, n) checkinstptr(L, n, TEK_LIB_VISUALFONT_CLASSNAME)

/*****************************************************************************/

static int my_typerror (lua_State *L, int narg, const char *tname) {
  const char *msg = lua_pushfstring(L, "%s expected, got %s",
                                    tname, luaL_typename(L, narg));
  return luaL_argerror(L, narg, msg);
}

/*****************************************************************************/
/*
**  check userdata with metatable from upvalue
*/

static void *checkudataupval(lua_State *L, int n, int upidx, const char *tname)
{
    void *p = lua_touserdata(L, n);
    if (p)
    {
        if (lua_getmetatable(L, n))
        {
            if (lua_rawequal(L, -1, lua_upvalueindex(upidx)))
            {
                lua_pop(L, 1);
                return p;
            }
        }
        my_typerror(L, n, tname);
    }
    return NULL;
}

static void *checkinstupval(lua_State *L, int n, int upidx, const char *tname)
{
    TAPTR p = checkudataupval(L, n, upidx, tname);
    if (p == TNULL)
        my_typerror(L, n, tname);
    return p;
}

#define checkvisptr(L, n) checkinstupval(L, n, 1, "visual")
#define checkpenptr(L, n) checkinstupval(L, n, 2, "pen")
#define checkpixmapptr(L, n) checkinstupval(L, n, 3, "pixmap")

/*****************************************************************************/

#define PAINT_UNDEFINED 0
#define PAINT_PEN       1
#define PAINT_PIXMAP    2
#define PAINT_GRADIENT  3

static int checkpaint(lua_State *L, int uidx, void **udata, TBOOL alsopm)
{
    *udata = lua_touserdata(L, uidx);
    if (*udata)
    {
        if (lua_getmetatable(L, uidx))
        {
            int res = PAINT_UNDEFINED;
            if (lua_rawequal(L, -1, lua_upvalueindex(2)))
                res = PAINT_PEN;
            if (res == 0 && alsopm && lua_rawequal(L, -1, lua_upvalueindex(4)))
                res = PAINT_GRADIENT;
            if (res == 0 && alsopm && lua_rawequal(L, -1, lua_upvalueindex(3)))
                res = PAINT_PIXMAP;
            lua_pop(L, 1);
            if (res != PAINT_UNDEFINED)
                return res;
        }
        /* userdata, but not of requested type */
        my_typerror(L, uidx, "paint");
    }
    /* other type */
    return 0;
}

static int lookuppaint(lua_State *L, int refpen, int uidx,
    void **udata, TBOOL alsopm)
{
    int res = checkpaint(L, uidx, udata, alsopm);
    if (res != PAINT_UNDEFINED)
        return res;
    if (!lua_isnoneornil(L, uidx))
    {
        if (refpen >= 0)
        {
            lua_rawgeti(L, lua_upvalueindex(1), refpen); /* VIS */
            lua_pushvalue(L, uidx);
            lua_gettable(L, -2);
            res = checkpaint(L, -1, udata, alsopm);
            lua_pop(L, 2);
        }
    }
    return res;
}

static void *lookuppen(lua_State *L, int refpen, int uidx)
{
    void *udata;
    lookuppaint(L, refpen, uidx, &udata, TFALSE);
    return udata;
}

static void *checklookuppen(lua_State *L, int refpen, int uidx)
{
    void *udata = lookuppen(L, refpen, uidx);
    if (udata == NULL)
        my_typerror(L, uidx, "pen");
    return udata;
}

static int getbgpaint(lua_State *L, TEKVisual *vis, int uidx,
    void **udata)
{
    int res = lookuppaint(L, vis->vis_refPens, uidx, udata, TTRUE);
    if (res != PAINT_UNDEFINED)
        return res;
    if (vis->vis_BGPen)
    {
        *udata = vis->vis_BGPen;
        return vis->vis_BGPenType;
    }
    return PAINT_UNDEFINED;
}

/*-----------------------------------------------------------------------------
--  Visual.wait(): Suspends the caller waiting for any event from any window.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_wait(lua_State *L)
{
    TEKVisual *vis;
    struct TExecBase *TExecBase;
    lua_getfield(L, LUA_REGISTRYINDEX, TEK_LIB_VISUAL_BASECLASSNAME);
    vis = lua_touserdata(L, -1);
    TExecBase = vis->vis_ExecBase;
    vis->vis_SignalsPending |= TWait(TGetPortSignal(vis->vis_IMsgPort) | 
        TTASK_SIG_ABORT | TTASK_SIG_TERM | TTASK_SIG_CHLD);
    if (vis->vis_SignalsPending & TTASK_SIG_ABORT)
        luaL_error(L, "received abort signal");
    lua_pop(L, 1);
    return 0;
}

/*-----------------------------------------------------------------------------
--  Visual.sleep(ms): Suspends the caller waiting for the specified number of
--  1/1000th seconds
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_sleep(lua_State *L)
{
    struct TExecBase *TExecBase;
    TEKVisual *vis;
    TTIME dt;
    lua_getfield(L, LUA_REGISTRYINDEX, TEK_LIB_VISUAL_BASECLASSNAME);
    vis = lua_touserdata(L, -1);
    TExecBase = vis->vis_ExecBase;
    dt.tdt_Int64 = luaL_checknumber(L, 1) * 1000;
    TWaitTime(&dt, 0);
    lua_pop(L, 1);
    return 0;
}

/*-----------------------------------------------------------------------------
--  attr1, ... = Visual.getDisplayAttrs(attrs): Queries attributes from the
--  display. Returns one value for each character specified in the {{attrs}}
--  string. Attributes:
--      - {{"M"}} - The display provides a window manager (boolean)
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_getdisplayattrs(lua_State *L)
{
    size_t narg = 0, i;
    lua_getfield(L, LUA_REGISTRYINDEX, TEK_LIB_VISUAL_BASECLASSNAME);
    TEKVisual *vis = lua_touserdata(L, -1);
    const char *opts = lua_tolstring(L, 1, &narg);
    for (i = 0; i < narg; ++i)
    {
        switch (opts[i])
        {
            case 'M':
                lua_pushboolean(L, vis->vis_HaveWindowManager);
                break;
            default:
                luaL_error(L, "unknown attribute");
        }
    }
    lua_remove(L, -narg - 1);
    return narg;
}

/*-----------------------------------------------------------------------------
--  sec, millis = Visual.getTime(): Returns the system time in seconds and
--  milliseconds.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_gettime(lua_State *L)
{
    struct TExecBase *TExecBase;
    TEKVisual *vis;
    TTIME dt;
    lua_getfield(L, LUA_REGISTRYINDEX, TEK_LIB_VISUAL_BASECLASSNAME);
    vis = lua_touserdata(L, -1);
    TExecBase = vis->vis_ExecBase;
    TGetSystemTime(&dt);
    lua_pushinteger(L, dt.tdt_Int64 / 1000000);
    lua_pushinteger(L, dt.tdt_Int64 % 1000000);
    return 2;
}

/*-----------------------------------------------------------------------------
--  font = Visual.openFont([name[, size[, attrs]]]): Opens a named font and,
--  if successful, returns a handle on it. The size is measured in pixels.
--  {{attrs}} is a string with each character representing a hint:
--      - {{"s"}} - the font should be scalable
--      - {{"b"}} - the font should be bold
--      - {{"i"}} - the font should be slanted to the right
--  It depends on the underlying font resource whether name, size, and
--  attributes must match exactly for this function to succeed, and which of
--  the attributes can and will be taken into account. In the case of the TTF
--  freetype library, for example, it may be more promising to use names of
--  fonts specifically crafted to be of the desired type, e.g.
--  {{"DejaVuSans-BoldOblique"}}, rather than {{"DejaVuSans"}} with the
--  attributes {{"ib"}}.
-----------------------------------------------------------------------------*/

static TBOOL iTStrChr(TSTRPTR s, TINT c)
{
    TINT d;
    while ((d = *s))
    {
        if (d == c)
            return TTRUE;
        s++;
    }
    return TFALSE;
}


LOCAL LUACFUNC TINT
tek_lib_visual_openfont(lua_State *L)
{
    TTAGITEM ftags[8], *tp = ftags;
    TEKVisual *vis;
    TEKFont *font;
    TSTRPTR name = (TSTRPTR) luaL_optstring(L, 1, "");
    TINT size = luaL_optinteger(L, 2, -1);
    TSTRPTR attr = (TSTRPTR) luaL_optstring(L, 3, "");

    lua_getfield(L, LUA_REGISTRYINDEX, TEK_LIB_VISUAL_BASECLASSNAME);
    vis = lua_touserdata(L, -1);
    lua_pop(L, 1);

    tp->tti_Tag = TVisual_FontBold;
    tp++->tti_Value = (TTAG) !!iTStrChr(attr, 'b');
    tp->tti_Tag = TVisual_FontItalic;
    tp++->tti_Value = (TTAG) !!iTStrChr(attr, 'i');
    tp->tti_Tag = TVisual_FontScaleable;
    tp++->tti_Value = (TTAG) !!iTStrChr(attr, 's');
    
    if (name && name[0] != 0)
    {
        tp->tti_Tag = TVisual_FontName;
        tp++->tti_Value = (TTAG) name;
    }

    if (size > 0)
    {
        tp->tti_Tag = TVisual_FontPxSize;
        tp++->tti_Value = (TTAG) size;
    }

    tp->tti_Tag = TVisual_Display;
    tp++->tti_Value = (TTAG) vis->vis_Display;

    tp->tti_Tag = TTAG_DONE;

    /* reserve userdata for a collectable object: */
    font = lua_newuserdata(L, sizeof(TEKFont));
    /* s: fontdata */
    font->font_Font = TVisualOpenFont(vis->vis_Base, ftags);
    if (font->font_Font)
    {
        font->font_VisBase = vis->vis_Base;

        ftags[0].tti_Tag = TVisual_FontHeight;
        ftags[0].tti_Value = (TTAG) &font->font_Height;
        ftags[1].tti_Tag = TVisual_FontUlPosition;
        ftags[1].tti_Value = (TTAG) &font->font_UlPosition;
        ftags[2].tti_Tag = TVisual_FontUlThickness;
        ftags[2].tti_Value = (TTAG) &font->font_UlThickness;
        ftags[3].tti_Tag = TTAG_DONE;

        if (TVisualGetFontAttrs(vis->vis_Base, font->font_Font, ftags) == 3)
        {
            TDBPRINTF(TDB_TRACE,("Height: %d - Pos: %d - Thick: %d\n",
                font->font_Height, font->font_UlPosition,
                font->font_UlThickness));

            /* attach class metatable to userdata object: */
            luaL_newmetatable(L, TEK_LIB_VISUALFONT_CLASSNAME);
            /* s: fontdata, meta */
            lua_setmetatable(L, -2);
            /* s: fontdata */
            lua_pushinteger(L, font->font_Height);
            /* s: fontdata, height */
            return 2;
        }

        TDestroy(font->font_Font);
    }

    lua_pop(L, 1);
    lua_pushnil(L);
    return 1;
}

/*-----------------------------------------------------------------------------
--  Visual.closeFont(font): Free font and associated resources.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_closefont(lua_State *L)
{
    TEKFont *font = checkfontptr(L, 1);
    if (font->font_Font)
    {
        TVisualCloseFont(font->font_VisBase, font->font_Font);
        font->font_Font = TNULL;
    }
    return 0;
}

/*-----------------------------------------------------------------------------
--  width, height = Visual.getTextSize(font, text): Returns the width and
--  height of the specified text when rendered with the given font. See also
--  Visual:textSize().
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_textsize_font(lua_State *L)
{
    TEKFont *font = checkfontptr(L, 1);
    size_t len;
    TSTRPTR s = (TSTRPTR) luaL_checklstring(L, 2, &len);
    lua_pushinteger(L,
        TVisualTextSize(font->font_VisBase, font->font_Font, s, (TINT) len));
    lua_pushinteger(L, font->font_Height);
    return 2;
}

/*-----------------------------------------------------------------------------
--  table = Visual.getFontAttrs(font[, table]): Returns font attributes in
--  a table. Field keys as currently defined:
--      - {{"Height"}} - Height in pixels
--      - {{"UlPosition"}} - Position of an underline, in pixels
--      - {{"UlThickness"}} - Thickness of an underline, in pixels
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_getfontattrs(lua_State *L)
{
    TEKFont *font = checkfontptr(L, 1);
    if (lua_type(L, 2) == LUA_TTABLE)
        lua_pushvalue(L, 2);
    else
        lua_newtable(L);
    lua_pushinteger(L, font->font_Height);
    lua_setfield(L, -2, "Height");
    lua_pushinteger(L, font->font_Height - font->font_UlPosition);
    lua_setfield(L, -2, "UlPosition");
    lua_pushinteger(L, font->font_UlThickness);
    lua_setfield(L, -2, "UlThickness");
    return 1;
}

/*-----------------------------------------------------------------------------
--  Visual:setInput(inputmask): For a visual, specifies a set of input
--  message types the caller wishes to add to the set of messages to be
--  received. For the valid types, see Visual.getMsg().
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_setinput(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TUINT mask = (TUINT) lua_tointeger(L, 2);
    TVisualSetInput(vis->vis_Visual, 0, mask);
    return 0;
}

/*-----------------------------------------------------------------------------
--  Visual:clearInput(inputmask): For a visual, specifies a set of input
--  message types the caller wishes to remove from the set of messages to be
--  received. For the valid types, see Visual.getMsg().
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_clearinput(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TUINT mask = (TUINT) lua_tointeger(L, 2);
    TVisualSetInput(vis->vis_Visual, mask, 0);
    return 0;
}

/*-----------------------------------------------------------------------------
--  Visual:flush(): Flush all changes to the visual to the display.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_flush(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    if (vis->vis_FlushReq && (vis->vis_Dirty || lua_toboolean(L, 2)))
    {
        struct TExecBase *TExecBase = vis->vis_ExecBase;
        struct TVRequest *req = vis->vis_FlushReq;
        req->tvr_Req.io_ReplyPort = TGetSyncPort(TNULL);
        TDoIO(&req->tvr_Req);
        vis->vis_Dirty = TFALSE;
    }
    lua_pop(L, 1);
    return 0;
}

/*****************************************************************************/

typedef struct
{
    struct TExecBase *execbase;
    TIMSG *imsg;
    int numfields;
} tek_msg;

LOCAL LUACFUNC TINT tek_msg_reply(lua_State *L)
{
    tek_msg *msg = luaL_checkudata(L, 1, "tek_msg*");
    if (msg->imsg)
    {
        struct TExecBase *TExecBase = msg->execbase;
        TIMSG *imsg = msg->imsg;
        if (imsg->timsg_Type == TITYPE_REQSELECTION)
        {
            if (lua_type(L, 2) == LUA_TTABLE)
            {
                size_t len;
                TUINT8 *s, *selection;
                lua_getfield(L, 2, "UTF8Selection");
                s = (TUINT8 *) lua_tolstring(L, -1, &len);
                selection = TAlloc(TNULL, len);
                if (selection)
                {
                    TCopyMem(s, selection, len);
                    imsg->timsg_Tags[0].tti_Tag = TIMsgReply_UTF8Selection;
                    imsg->timsg_Tags[0].tti_Value = (TTAG) selection;
                    imsg->timsg_Tags[1].tti_Tag = TIMsgReply_UTF8SelectionLen;
                    imsg->timsg_Tags[1].tti_Value = len;
                    imsg->timsg_Tags[2].tti_Tag = TTAG_DONE;
                    imsg->timsg_ReplyData = (TTAG) imsg->timsg_Tags;
                }
            }
        }
        TReplyMsg(msg->imsg);
        msg->imsg = NULL;
    }
    return 0;
}

LOCAL LUACFUNC TINT tek_msg_len(lua_State *L)
{
    tek_msg *msg = luaL_checkudata(L, 1, "tek_msg*");
    lua_pushinteger(L, msg->numfields);
    return 1;
}

LOCAL LUACFUNC TINT tek_msg_index(lua_State *L)
{
    tek_msg *msg = luaL_checkudata(L, 1, "tek_msg*");
    TIMSG *imsg = msg->imsg;
    
    if (lua_type(L, 2) == LUA_TSTRING)
    {
        const char *s = lua_tostring(L, 2);
        if (s && s[0] == 'r') /* "reply" */
            lua_pushcfunction(L, tek_msg_reply);
        else
            lua_pushnil(L);
        return 1;
    }
    
    if (!imsg)
    {
        TDBPRINTF(TDB_WARN,("Message invalid - already replied\n"));
        lua_pushnil(L);
        return 1;
    }
    
    switch (lua_tointeger(L, 2))
    {
        default:
            lua_pushnil(L);
            break;
        case -1:
            if (imsg->timsg_UserData > 0)
            {
                TEKVisual *refvis;
                lua_getfield(L, LUA_REGISTRYINDEX,
                    TEK_LIB_VISUAL_BASECLASSNAME);
                /* s: visbase */
                lua_getmetatable(L, -1);
                /* s: visbase, reftab */
                /* If we have a userdata, we regard it as an index into the
                visual metatable, referencing a visual: */
                lua_rawgeti(L, -1, (int) imsg->timsg_UserData);
                /* s: visbase, reftab, visual */
                refvis = lua_touserdata(L, -1);
                /* from there, we retrieve the visual's userdata, which
                is stored as a reference in the same table: */
                lua_rawgeti(L, -2, refvis->vis_refUserData);
                /* s: visbase, reftab, visual, userdata */
                lua_remove(L, -2);
                lua_remove(L, -2);
                lua_remove(L, -2);
            }
            else
            {
                /* otherwise, we retrieve a "raw" user data package: */
                lua_pushlstring(L, (void *) (imsg + 1), imsg->timsg_ExtraSize);
            }
            break;
        case 0:
            lua_pushinteger(L, imsg->timsg_TimeStamp.tdt_Int64 % 1000000);
            break;
        case 1:
            lua_pushinteger(L, imsg->timsg_TimeStamp.tdt_Int64 / 1000000);
            break;
        case 2:
            lua_pushinteger(L, imsg->timsg_Type);
            break;
        case 3:
            lua_pushinteger(L, imsg->timsg_Code);
            break;
        case 4:
            lua_pushinteger(L, imsg->timsg_MouseX);
            break;
        case 5:
            lua_pushinteger(L, imsg->timsg_MouseY);
            break;
        case 6:
            lua_pushinteger(L, imsg->timsg_Qualifier);
            break;
        case 7:
            if (imsg->timsg_Type != TITYPE_REFRESH)
            {
                /* UTF-8 representation of keycode: */
                lua_pushstring(L, (const char *) imsg->timsg_KeyCode);
                break;
            }
            lua_pushinteger(L, imsg->timsg_X);
            break;
        case 8:
            lua_pushinteger(L, imsg->timsg_Y);
            break;
        case 9:
            lua_pushinteger(L, imsg->timsg_X + imsg->timsg_Width - 1);
            break;
        case 10:
            lua_pushinteger(L, imsg->timsg_Y + imsg->timsg_Height - 1);
            break;
        case 11:
            lua_pushinteger(L, imsg->timsg_ScreenMouseX);
            break;
        case 12:
            lua_pushinteger(L, imsg->timsg_ScreenMouseY);
            break;
    }
    return 1;
}

/*-----------------------------------------------------------------------------
--  msg = Visual.getMsg(): Get next input message. Fields in a message are
--  indexed numerically:
--      - {{-1}} - Userdata, e.g. the window object from which the message
--      originates, or the user message body in case of {{ui.MSG_USER}}
--      - {{0}} - Timestamp of the message, milliseconds
--      - {{1}} - Timestamp of the message, seconds
--      - {{2}} - Message type. Types:
--          - {{ui.MSG_CLOSE}} - the user wishes to close the window
--          - {{ui.MSG_FOCUS}} - the window received/lost the input focus
--          - {{ui.MSG_NEWSIZE}} - the window has been resized
--          - {{ui.MSG_REFRESH}} - a part of the window needs a repaint
--          - {{ui.MSG_MOUSEOVER}} - the mouse moved over/out of the window
--          - {{ui.MSG_KEYDOWN}} - a key was pressed down
--          - {{ui.MSG_MOUSEMOVE}} - the mouse was moved
--          - {{ui.MSG_MOUSEBUTTON}} - a mouse button was pressed
--          - {{ui.MSG_INTERVAL}} - a timer interval has passed
--          - {{ui.MSG_KEYUP}} - a key was released
--          - {{ui.MSG_USER}} - a user message was send to the application
--          - {{ui.MSG_REQSELECTION}} - selection or clipboard requested
--      - {{3}} - Message code - depending on the message type, indicates
--      focus on/off, keycode, mouse button number, etc.
--      - {{4}} - Mouse X position on window
--      - {{5}} - Mouse Y position on window
--      - {{6}} - Keyboard Qualifier
--      - {{7}} - Keyboard message: UTF-8 representation of key code, or
--      - {{7}} - Refresh message: X position of damaged area
--      - {{8}} - Refresh message: Y position of damaged area
--      - {{9}} - Refresh message: Width of damaged area
--      - {{10}} - Refresh message: Height of damaged area
--      - {{11}} - Mouse X position on screen
--      - {{12}} - Mouse Y position on screen
--  A message will be acknowledged implicitely by garbage collection, or
--  by invoking the {{msg:reply([table])}} method on it, in which case extra
--  data can be returned to the sender. Valid fields in {{table}} are:
--      - {{UTF8Selection}} - a string containing the selection buffer, in
--      reply to messages of the type {{MSG_REQSELECTION}}
-----------------------------------------------------------------------------*/

static TIMSG *tek_lib_visual_getsigmsg(TEKVisual *vis, TUINT sig)
{
    TIMSG *imsg = TNULL;
    if (vis->vis_SignalsPending & sig)
    {
        struct TExecBase *TExecBase = vis->vis_ExecBase;
        vis->vis_SignalsPending &= ~sig;
        imsg = TAllocMsg0(sizeof(TIMSG));
        if (imsg)
        {
            imsg->timsg_Code = sig;
            imsg->timsg_Type = TITYPE_SIGNAL;
            imsg->timsg_MouseX = -1;
            imsg->timsg_MouseY = -1;
            TGetSystemTime(&imsg->timsg_TimeStamp);
        }
    }
    return imsg;
}

LOCAL LUACFUNC TINT
tek_lib_visual_getmsg(lua_State *L)
{
    struct TExecBase *TExecBase;
    TEKVisual *vis;
    TIMSG *imsg;
    tek_msg *tmsg;
    
    lua_getfield(L, LUA_REGISTRYINDEX, TEK_LIB_VISUAL_BASECLASSNAME);
    /* s: visbase */
    vis = lua_touserdata(L, -1);
    TExecBase = vis->vis_ExecBase;
    
    if (/*!(imsg = tek_lib_visual_getsigmsg(vis, TTASK_SIG_ABORT)) &&*/
        !(imsg = tek_lib_visual_getsigmsg(vis, TTASK_SIG_CHLD)) &&
        !(imsg = tek_lib_visual_getsigmsg(vis, TTASK_SIG_TERM)))
        imsg = (TIMSG *) TGetMsg(vis->vis_IMsgPort);

    if (imsg == TNULL)
    {
        lua_pop(L, 1);
        return 0;
    }
    tmsg = lua_newuserdata(L, sizeof(tek_msg));
    tmsg->execbase = TExecBase;
    lua_getfield(L, LUA_REGISTRYINDEX, "tek_msg*");
    lua_setmetatable(L, -2);
    /* s: tekmsg */
    tmsg->imsg = imsg;
    tmsg->numfields = 8;
    if (imsg->timsg_Type == TITYPE_REFRESH)
        tmsg->numfields += 4;
    else if (imsg->timsg_Type == TITYPE_KEYUP ||
        imsg->timsg_Type == TITYPE_KEYDOWN)
        tmsg->numfields++;
    return 1;
}

/*****************************************************************************/

static TINT tek_lib_visual_createpixmap_from_img(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, TEK_LIB_VISUAL_BASECLASSNAME);
    TEKVisual *vis = lua_touserdata(L, -1);
    lua_pop(L, 1);
    struct TExecBase *TExecBase = vis->vis_ExecBase;
    struct ImgLoader ld;
    size_t len;
    const char *src = lua_tolstring(L, 1, &len);
    if (src)
    {
        if (!(imgload_init_memory(&ld, TExecBase, src, len) &&
            imgload_load(&ld)))
            return 0;
    }
    else
    {
        FILE **f = luaL_checkudata(L, 1, LUA_FILEHANDLE);
        if (*f == NULL)
            luaL_error(L, "attempt to use a closed file");
        if (!(imgload_init_file(&ld, TExecBase, *f) &&
            imgload_load(&ld)))
            return 0;
    }
    
    TEKPixmap *pm = lua_newuserdata(L, sizeof(TEKPixmap));
    luaL_newmetatable(L, TEK_LIB_VISUALPIXMAP_CLASSNAME);
    lua_setmetatable(L, -2);
    pm->pxm_Image = ld.iml_Image;
    pm->pxm_Width = ld.iml_Width;
    pm->pxm_Height = ld.iml_Height;
    pm->pxm_Flags = ld.iml_Flags;
    if (lua_isboolean(L, 4))
    {
        if (lua_toboolean(L, 4))
            ld.iml_Flags |= IMLFL_HAS_ALPHA;
        else
            ld.iml_Flags &= ~IMLFL_HAS_ALPHA;
    }
    pm->pxm_VisualBase = vis;
    
    lua_pushinteger(L, pm->pxm_Width);
    lua_pushinteger(L, pm->pxm_Height);
    lua_pushboolean(L, ld.iml_Flags & IMLFL_HAS_ALPHA);
    return 4;
}

static TINT
tek_lib_visual_createpixmap_from_table(lua_State *L)
{
    TEKVisual *vis;
    struct TExecBase *TExecBase;
    TUINT *buf;
    TEKPixmap *bm;
    int x, y;
    
    int tw = luaL_checkinteger(L, 2);
    int th = luaL_checkinteger(L, 3);
    TBOOL has_alpha = lua_toboolean(L, 4);
    int i = luaL_optinteger(L, 5, 0);
    int lw = luaL_optinteger(L, 6, tw);
    
    lua_getfield(L, LUA_REGISTRYINDEX, TEK_LIB_VISUAL_BASECLASSNAME);
    vis = lua_touserdata(L, -1);
    lua_pop(L, 1);
    TExecBase = vis->vis_ExecBase;
    
    bm = lua_newuserdata(L, sizeof(TEKPixmap));
    luaL_newmetatable(L, TEK_LIB_VISUALPIXMAP_CLASSNAME);
    lua_setmetatable(L, -2);
    
    buf = TAlloc(TNULL, tw * th * sizeof(TUINT));
    if (buf == TNULL)
    {
        lua_pushstring(L, "out of memory");
        lua_error(L);
    }

    bm->pxm_Image.tpb_Data = (TUINT8 *) buf;
    bm->pxm_Image.tpb_Format = TVPIXFMT_A8R8G8B8;
    bm->pxm_Image.tpb_BytesPerLine = tw * 4;
    
    bm->pxm_Width = tw;
    bm->pxm_Height = th;
    bm->pxm_Flags = has_alpha ? IMLFL_HAS_ALPHA : 0;
    bm->pxm_VisualBase = vis;
    
    for (y = 0; y < th; ++y)
    {
        for (x = 0; x < tw; ++x)
        {
            lua_rawgeti(L, 1, i + x);
            *buf++ = lua_tointeger(L, -1);
            lua_pop(L, 1);
        }
        i += lw;
    }
    return 1;
}

/*-----------------------------------------------------------------------------
--  pixmap = Visual.createPixmap(src[, width[, height[, alpha[, idx]]]]):
--  Creates a pixmap object from some source. The {{src}} argument can be a
--  string, containing a loaded image in some file format (PPM or PNG), an open
--  file, or a table. In case of a string or a file, width and height determine
--  an optional size to scale the image to. If only one of width and height are
--  given, the image is scaled proprtionally. In case of a table, the width and
--  height arguments are mandatory, and the table is expected to contain RGB
--  values starting at table index {{0}}, unless another index is given.
--  The {{alpha}} argument can be used to override presence or absence of an
--  alpha channel; the default is determined by the picture, or none in the
--  case of a table source.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_createpixmap(lua_State *L)
{
    if (lua_istable(L, 1))
        return tek_lib_visual_createpixmap_from_table(L);
    return tek_lib_visual_createpixmap_from_img(L);
}

LOCAL LUACFUNC TINT
tek_lib_visual_freepixmap(lua_State *L)
{
    TEKPixmap *bm = getpixmapptr(L, 1);
    if (bm->pxm_Image.tpb_Data)
    {
        TEKVisual *vis = bm->pxm_VisualBase;
        struct TExecBase *TExecBase = vis->vis_ExecBase;
        TFree(bm->pxm_Image.tpb_Data);
        bm->pxm_Image.tpb_Data = TNULL;
    }
    return 0;
}

LOCAL LUACFUNC TINT
tek_lib_visual_getpixmap(lua_State *L)
{
    TEKPixmap *bm = getpixmapptr(L, 1);
    if (bm->pxm_Image.tpb_Data)
    {
        lua_Integer x = luaL_checkinteger(L, 2);
        lua_Integer y = luaL_checkinteger(L, 3);
        if (x < 0 || x >= bm->pxm_Width)
            luaL_argerror(L, 2, "Invalid position");
        if (y < 0 || y >= bm->pxm_Height)
            luaL_argerror(L, 3, "Invalid position");
        lua_pushinteger(L, pixconv_getpixelbuf(&bm->pxm_Image, x, y));
        return 1;
    }
    return 0;
}

LOCAL LUACFUNC TINT
tek_lib_visual_getpixmapattr(lua_State *L)
{
    TEKPixmap *bm = getpixmapptr(L, 1);
    if (bm->pxm_Image.tpb_Data)
    {
        lua_pushinteger(L, bm->pxm_Width);
        lua_pushinteger(L, bm->pxm_Height);
        lua_pushboolean(L, bm->pxm_Flags & IMLFL_HAS_ALPHA);
        return 3;
    }
    return 0;
}


LOCAL LUACFUNC TINT
tek_lib_visual_setpixmap(lua_State *L)
{
    TEKPixmap *bm = getpixmapptr(L, 1);
    if (bm->pxm_Image.tpb_Data)
    {
        lua_Integer x = luaL_checkinteger(L, 2);
        lua_Integer y = luaL_checkinteger(L, 3);
        lua_Integer val = luaL_checkinteger(L, 4);
        if (x < 0 || x >= bm->pxm_Width)
            luaL_argerror(L, 2, "Invalid position");
        if (y < 0 || y >= bm->pxm_Height)
            luaL_argerror(L, 3, "Invalid position");
        pixconv_setpixelbuf(&bm->pxm_Image, x, y, val);
    }
    return 0;
}

/*-----------------------------------------------------------------------------
--  pen = Visual:allocPen(a, r, g, b): Obtain a colored pen.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_allocpen(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TINT a = luaL_checkinteger(L, 2);
    TINT r = luaL_checkinteger(L, 3);
    TINT g = luaL_checkinteger(L, 4);
    TINT b = luaL_checkinteger(L, 5);
    TUINT rgb;
    TEKPen *pen = lua_newuserdata(L, sizeof(TEKPen));
    /* s: pendata */
    pen->pen_Pen = TVPEN_UNDEFINED;
    /* attach class metatable to userdata object: */
    luaL_newmetatable(L, TEK_LIB_VISUALPEN_CLASSNAME);
    /* s: pendata, meta */
    lua_setmetatable(L, -2);
    /* s: pendata */
    a = TCLAMP(0, a, 255);
    r = TCLAMP(0, r, 255);
    g = TCLAMP(0, g, 255);
    b = TCLAMP(0, b, 255);
    rgb = ((TUINT)(a) << 24) | ((TUINT)(r) << 16) | ((TUINT)(g) << 8) | 
        (TUINT)(b);
    pen->pen_Pen = TVisualAllocPen(vis->vis_Visual, rgb);
    pen->pen_Visual = vis;
    return 1;
}

/*-----------------------------------------------------------------------------
--  Visual:freePen(pen): Release a colored pen
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_freepen(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TEKPen *pen = getpenptr(L, 2);
    if (pen->pen_Pen != TVPEN_UNDEFINED)
    {
        if (vis != pen->pen_Visual)
            luaL_argerror(L, 2, "Pen not from visual");
        TVisualFreePen(vis->vis_Visual, pen->pen_Pen);
        pen->pen_Pen = TVPEN_UNDEFINED;
    }
    return 0;
}

/*****************************************************************************/

#if defined(TEK_VISUAL_DEBUG)
static void tek_lib_visual_debugwait(TEKVisual *vis)
{
    struct TExecBase *TExecBase = vis->vis_ExecBase;
    TTIME dt = { 1800 };
    TWaitTime(&dt, 0);
}
#endif

/*-----------------------------------------------------------------------------
--  Visual:drawRect(x0, y0, x1, y1, pen): Draw a rectangle
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_rect(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TINT sx = vis->vis_ShiftX, sy = vis->vis_ShiftY;
    TINT x0 = luaL_checkinteger(L, 2) + sx;
    TINT y0 = luaL_checkinteger(L, 3) + sy;
    TINT x1 = luaL_checkinteger(L, 4) + sx;
    TINT y1 = luaL_checkinteger(L, 5) + sy;
    TEKPen *pen = checklookuppen(L, vis->vis_refPens, 6);
    #if defined(TEK_VISUAL_DEBUG)
    if (vis->vis_VisBase->vis_Debug)
    {
        TVisualRect(vis->vis_Visual, x0, y0, x1 - x0 + 1, y1 - y0 + 1,
            vis->vis_DebugPen1);
        tek_lib_visual_debugwait(vis);
        TVisualRect(vis->vis_Visual, x0, y0, x1 - x0 + 1, y1 - y0 + 1,
            vis->vis_DebugPen2);
        tek_lib_visual_debugwait(vis);
    }
    #endif
    TVisualRect(vis->vis_Visual, x0, y0, x1 - x0 + 1, y1 - y0 + 1,
        pen->pen_Pen);
    vis->vis_Dirty = TTRUE;
    return 0;
}

static void
tek_lib_visual_frectpixmap(lua_State *L, TEKVisual *vis, TEKPixmap *pm,
    TINT x0, TINT y0, TINT w, TINT h, TINT ox, TINT oy)
{
    TINT tw = pm->pxm_Width;
    TINT th = pm->pxm_Height;
    TUINT *buf = (TUINT *) pm->pxm_Image.tpb_Data;
    TINT th0, yo;
    TINT y = y0;
    TTAGITEM tags[2];
    tags[0].tti_Tag = TVisual_AlphaChannel;
    tags[0].tti_Value = pm->pxm_Flags & IMLFL_HAS_ALPHA;
    tags[1].tti_Tag = TTAG_DONE;
    
    if (vis->vis_HaveClipRect)
    {
        TINT c0 = vis->vis_ClipRect[0];
        TINT c1 = vis->vis_ClipRect[1];
        TINT c2 = vis->vis_ClipRect[2];
        TINT c3 = vis->vis_ClipRect[3];
        TINT x1 = x0 + w - 1;
        TINT y1 = y0 + h - 1;
        if (!TEK_UI_OVERLAP(x0, y0, x1, y1, c0, c1, c2, c3))
            return;
        x0 = TMAX(x0, c0);
        y0 = TMAX(y0, c1);
        x1 = TMIN(x1, c2);
        y1 = TMIN(y1, c3);
        w = x1 - x0 + 1;
        h = y1 - y0 + 1;
        y = y0;
    }
    
    yo = (y0 - oy) % th;
    if (yo < 0) yo += th;
    th0 = th - yo;
    
    while (h > 0)
    {
        int tw0;
        int x = x0;
        int ww = w;
        int dh = TMIN(h, th0);
        int xo = (x0 - ox) % tw;
        if (xo < 0) xo += tw;
        tw0 = tw - xo;
        
        while (ww > 0)
        {
            int dw = TMIN(ww, tw0);
            TVisualDrawBuffer(vis->vis_Visual, x, y, 
                buf + xo + yo * tw, dw, dh, tw, tags);
            ww -= dw;
            x += dw;
            tw0 = tw;
            xo = 0;
        }
        h -= dh;
        y += dh;
        th0 = th;
        yo = 0;
    }
}

/*****************************************************************************/
/*
**  Gradient support
*/

#if defined(ENABLE_GRADIENT)

static float mulVec(vec a, vec b)
{
    return a.x * b.x + a.y * b.y;
}

static vec subVec(vec a, vec b)
{
    vec c;
    c.x = a.x - b.x;
    c.y = a.y - b.y;
    return c;
}

static vec addVec(vec a, vec b)
{
    vec c;
    c.x = a.x + b.x;
    c.y = a.y + b.y;
    return c;
}

static vec scaleVec(vec a, float b)
{
    vec c;
    c.x = a.x * b;
    c.y = a.y * b;
    return c;
}

static void getS2(vec r0, vec u, int Ar, int Ag, int Ab, int Dr, int Dg, 
    int Db, float *pr, float *pg, float *pb, float *pf, int x, int y)
{
    vec z = { x, y };
    float s = mulVec(subVec(z, r0), u) / mulVec(u, u);
    vec F = subVec(addVec(r0, scaleVec(u, s)), r0);
    float f = TABS(u.y) > TABS(u.x) ? F.y / u.y : F.x / u.x;
    *pr = Ar + Dr * f;
    *pg = Ag + Dg * f;
    *pb = Ab + Db * f;
    *pf = f * 256;
}

static void
tek_lib_visual_frectgradient(lua_State *L, TEKVisual *vis, TEKGradient *gr,
    TINT x0, TINT y0, TINT w, TINT h, TINT ox, TINT oy)
{
    if (x0 < -32768 || x0 + w > 32768 || y0 < -32768 || y0 + h > 32768 ||
        w < 1 || w > 10000 || h < 1 || h > 10000 || w * h > 2000000)
    {
        TDBPRINTF(TDB_WARN,("illegal gradient rect %d,%d,%d,%d\n", 
            x0, y0, x0 + w - 1, y0 + h - 1));
        return;
    }
    
    TTAGITEM tags[2];
    tags[0].tti_Tag = TTAG_DONE;

#if defined(ENABLE_PIXMAP_CACHE)
    struct 
    { 
        TINT16 x0, y0, x1, y1;
        TUINT8 r0, g0, b0, r1, g1, b1;
    } ckey = 
    { 
        gr->A.vec.x, gr->A.vec.y, gr->B.vec.x, gr->B.vec.y, 
        gr->A.r, gr->A.g, gr->A.b, gr->B.r, gr->B.g, gr->B.b
    };
    
    struct TVImageCacheRequest creq;
    creq.tvc_CacheManager = vis->vis_CacheManager;
    creq.tvc_Key = (TUINT8 *) &ckey;
    creq.tvc_KeyLen = sizeof(ckey);
    creq.tvc_OrigX = ox;
    creq.tvc_OrigY = oy;
    creq.tvc_Result = TVIMGCACHE_NOTFOUND;
    
    tags[0].tti_Tag = TVisual_CacheRequest;
    tags[0].tti_Value = (TTAG) &creq;
    tags[1].tti_Tag = TTAG_DONE;
    
    TVisualDrawBuffer(vis->vis_Visual, x0, y0, TNULL, w, h, w, tags);
    if (creq.tvc_Result == TVIMGCACHE_FOUND)
        return; /* cache used, painted successfully */
#endif

    int y, x;
    int Ar = gr->A.r;
    int Ag = gr->A.g;
    int Ab = gr->A.b;
    int Br = gr->B.r;
    int Bg = gr->B.g;
    int Bb = gr->B.b;
    int Dr = Br - Ar;
    int Dg = Bg - Ag;
    int Db = Bb - Ab;
    
    vec r0 = gr->A.vec;
    vec u = subVec(gr->B.vec, r0);
    
    x = x0 - ox;
    y = y0 - oy;

    float cr0, cg0, cb0, cx0;
    float cr1, cg1, cb1, cx1;
    float cr2, cg2, cb2, cx2;
    getS2(r0, u, Ar, Ag, Ab, Dr, Dg, Db, &cr0, &cg0, &cb0, &cx0, x, y);
    getS2(r0, u, Ar, Ag, Ab, Dr, Dg, Db, &cr1, &cg1, &cb1, &cx1, x + w - 1, y);
    getS2(r0, u, Ar, Ag, Ab, Dr, Dg, Db, &cr2, &cg2, &cb2, &cx2, x, y + h - 1);
    float dr0 = (cr2 - cr0) / (h - 1);
    float dg0 = (cg2 - cg0) / (h - 1);
    float db0 = (cb2 - cb0) / (h - 1);
    float dx0 = (cx2 - cx0) / (h - 1);
    float dr = (cr1 - cr0) / (w - 1);
    float dg = (cg1 - cg0) / (w - 1);
    float db = (cb1 - cb0) / (w - 1);
    float dx = (cx1 - cx0) / (w - 1);
    
    TUINT *buf = TExecAlloc(vis->vis_ExecBase, TNULL, w * h * sizeof(TUINT));
    if (!buf)
        return;
    
    for (y = 0; y < h; ++y)
    {
        float cr = cr0;
        float cg = cg0;
        float cb = cb0;
        float cx = cx0;
        cr0 += dr0;
        cg0 += dg0;
        cb0 += db0;
        cx0 += dx0;

        float ce = cx + dx * w;
        TUINT r, g, b;
        if (cx < 1 && ce < 1)
        {
            r = Ar;
            g = Ag;
            b = Ab;
        }
        else if (cx > 255 && ce > 255)
        {
            r = Br;
            g = Bg;
            b = Bb;
        }
        else if (cx >= 1 && cx <= 255 && ce >= 1 && ce <= 255)
        {
            if (cx == ce)
            {
                r = cr;
                g = cg;
                b = cb;
            }
            else
            {
                for (x = 0; x < w; ++x)
                {
                    *buf++ = 
                        ((TUINT) cr << 16) | ((TUINT) cg << 8) | (TUINT) cb;
                    cr += dr;
                    cg += dg;
                    cb += db;
                }
                continue;
            }
        }
        else if ((cx < 1 && ce <= 255) || (cx <= 255 && ce < 1))
        {
            for (x = 0; x < w; ++x)
            {
                if (cx < 1)
                    *buf++ = 
                        ((TUINT) Ar << 16) | ((TUINT) Ag << 8) | (TUINT) Ab;
                else
                    *buf++ =
                        ((TUINT) cr << 16) | ((TUINT) cg << 8) | (TUINT) cb;
                cr += dr;
                cg += dg;
                cb += db;
                cx += dx;
            }
            continue;
        }
        else if ((cx >= 1 && ce > 255) || (cx > 255 && ce >= 1))
        {
            for (x = 0; x < w; ++x)
            {
                if (cx > 255)
                    *buf++ =
                        ((TUINT) Br << 16) | ((TUINT) Bg << 8) | (TUINT) Bb;
                else
                    *buf++ =
                        ((TUINT) cr << 16) | ((TUINT) cg << 8) | (TUINT) cb;
                cr += dr;
                cg += dg;
                cb += db;
                cx += dx;
            }
            continue;
        }
        else
        {
            for (x = 0; x < w; ++x)
            {
                if (cx < 1)
                    *buf++ =
                        ((TUINT) Ar << 16) | ((TUINT) Ag << 8) | (TUINT) Ab;
                else if (cx > 255)
                    *buf++ =
                        ((TUINT) Br << 16) | ((TUINT) Bg << 8) | (TUINT) Bb;
                else
                    *buf++ =
                        ((TUINT) cr << 16) | ((TUINT) cg << 8) | (TUINT) cb;
                cr += dr;
                cg += dg;
                cb += db;
                cx += dx;
            }
            continue;
        }
        
        for (x = 0; x < w; ++x)
            *buf++ = ((TUINT) r << 16) | ((TUINT) g << 8) | (TUINT) b;
    }

    buf -= w * h;
    
    /* paint and possibly store in cache: */
    TVisualDrawBuffer(vis->vis_Visual, x0, y0, buf, w, h, w, tags);

    TExecFree(vis->vis_ExecBase, buf);
}

#endif

/*-----------------------------------------------------------------------------
--  Visual:fillRect(x0, y0, x1, y1[, paint]): Draw a filled rectangle.
--  {{paint}} may refer to a pen, a pixmap, or a gradient. For drawing with
--  a pixmap or gradient, see also Visual:setTextureOrigin(). If no paint
--  argument is specified, the visual's current background paint is used,
--  see also Visual:setBGPen().
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_frect(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TINT sx = vis->vis_ShiftX, sy = vis->vis_ShiftY;
    TINT x0 = luaL_checkinteger(L, 2) + sx;
    TINT y0 = luaL_checkinteger(L, 3) + sy;
    TINT x1 = luaL_checkinteger(L, 4) + sx;
    TINT y1 = luaL_checkinteger(L, 5) + sy;
    TINT w = x1 - x0 + 1;
    TINT h = y1 - y0 + 1;
    void *udata;
    
    if (x0 > x1 || y0 > y1)
        return 0;
    
    #if defined(TEK_VISUAL_DEBUG)
    if (vis->vis_VisBase->vis_Debug)
    {
        TVisualFRect(vis->vis_Visual, x0, y0, w, h, vis->vis_DebugPen1);
        tek_lib_visual_debugwait(vis);
        TVisualFRect(vis->vis_Visual, x0, y0, w, h, vis->vis_DebugPen2);
        tek_lib_visual_debugwait(vis);
    }
    #endif
    
    switch (getbgpaint(L, vis, 6, &udata))
    {
        case PAINT_PEN:
        {
            TEKPen *pen = udata;
            TVisualFRect(vis->vis_Visual, x0, y0, w, h, pen->pen_Pen);
            break;
        }
        case PAINT_PIXMAP:
        {
            TEKPixmap *pm = udata;
            TINT ox = vis->vis_TextureX + sx;
            TINT oy = vis->vis_TextureY + sy;
            tek_lib_visual_frectpixmap(L, vis, pm, x0, y0, w, h, ox, oy);
            break;
        }
#if defined(ENABLE_GRADIENT)
        case PAINT_GRADIENT:
        {
            TEKGradient *grad = udata;
            TINT ox = vis->vis_TextureX + sx;
            TINT oy = vis->vis_TextureY + sy;
            tek_lib_visual_frectgradient(L, vis, grad, x0, y0, w, h, ox, oy);
            break;
        }
#endif
    }
    
    vis->vis_Dirty = TTRUE;
    return 0;
}

/*-----------------------------------------------------------------------------
--  Visual:drawLine(x0, y0, x1, y1, pen): Draw a line.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_line(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TINT sx = vis->vis_ShiftX, sy = vis->vis_ShiftY;
    TINT x0 = luaL_checkinteger(L, 2) + sx;
    TINT y0 = luaL_checkinteger(L, 3) + sy;
    TINT x1 = luaL_checkinteger(L, 4) + sx;
    TINT y1 = luaL_checkinteger(L, 5) + sy;
    TEKPen *pen = checklookuppen(L, vis->vis_refPens, 6);
    #if defined(TEK_VISUAL_DEBUG)
    if (vis->vis_VisBase->vis_Debug)
    {
        TVisualLine(vis->vis_Visual, x0, y0, x1, y1, vis->vis_DebugPen1);
        tek_lib_visual_debugwait(vis);
        TVisualLine(vis->vis_Visual, x0, y0, x1, y1, vis->vis_DebugPen2);
        tek_lib_visual_debugwait(vis);
    }
    #endif
    TVisualLine(vis->vis_Visual, x0, y0, x1, y1, pen->pen_Pen);
    vis->vis_Dirty = TTRUE;
    return 0;
}

/*-----------------------------------------------------------------------------
--  Visual:drawPoint(x, y, pen): Draw a pixel.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_plot(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TINT sx = vis->vis_ShiftX, sy = vis->vis_ShiftY;
    TINT x0 = luaL_checkinteger(L, 2) + sx;
    TINT y0 = luaL_checkinteger(L, 3) + sy;
    TEKPen *pen = checklookuppen(L, vis->vis_refPens, 4);
    #if defined(TEK_VISUAL_DEBUG)
    if (vis->vis_VisBase->vis_Debug)
    {
        TVisualPlot(vis->vis_Visual, x0, y0, vis->vis_DebugPen1);
        tek_lib_visual_debugwait(vis);
        TVisualPlot(vis->vis_Visual, x0, y0, vis->vis_DebugPen2);
        tek_lib_visual_debugwait(vis);
    }
    #endif
    TVisualPlot(vis->vis_Visual, x0, y0, pen->pen_Pen);
    vis->vis_Dirty = TTRUE;
    return 0;
}

/*-----------------------------------------------------------------------------
--  Visual:drawText(x0, y0, x1, y1, text, fgpen[, bgpaint]): Draw the text into
--  the specified rectangle. {{fgpen}} must be a pen, bgpaint is optional and
--  may be a pen, a pixmap, or a gradient. For drawing background with a pixmap
--  or gradient, see also Visual:setTextureOrigin().
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_text(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TINT sx = vis->vis_ShiftX, sy = vis->vis_ShiftY;
    TINT x0 = luaL_checkinteger(L, 2) + sx;
    TINT y0 = luaL_checkinteger(L, 3) + sy;
    TINT x1 = luaL_checkinteger(L, 4) + sx;
    TINT y1 = luaL_checkinteger(L, 5) + sy;
    size_t tlen;
    TSTRPTR text = (TSTRPTR) luaL_checklstring(L, 6, &tlen);
    TEKPen *fpen = checklookuppen(L, vis->vis_refPens, 7);
    void *udata;

    switch (lookuppaint(L, vis->vis_refPens, 8, &udata, TTRUE))
    {
        case PAINT_PEN:
        {
            TEKPen *pen = udata;
            TVisualFRect(vis->vis_Visual, x0, y0, x1 - x0 + 1, y1 - y0 + 1,
                pen->pen_Pen);
            break;
        }
        case PAINT_PIXMAP:
        {
            TEKPixmap *pm = udata;
            TINT ox = vis->vis_TextureX + sx;
            TINT oy = vis->vis_TextureY + sy;
            tek_lib_visual_frectpixmap(L, vis, pm, x0, y0, 
                x1 - x0 + 1, y1 - y0 + 1, ox, oy);
            break;
        }
#if defined(ENABLE_GRADIENT)
        case PAINT_GRADIENT:
        {
            TEKGradient *grad = udata;
            TINT ox = vis->vis_TextureX + sx;
            TINT oy = vis->vis_TextureY + sy;
            tek_lib_visual_frectgradient(L, vis, grad, x0, y0, 
                x1 - x0 + 1, y1 - y0 + 1, ox, oy);
            break;
        }
#endif
    }
    
#if defined(TEK_VISUAL_DEBUG)
    if (vis->vis_VisBase->vis_Debug)
    {
        TVisualText(vis->vis_Visual, x0, y0, text, tlen, vis->vis_DebugPen1);
        tek_lib_visual_debugwait(vis);
        TVisualText(vis->vis_Visual, x0, y0, text, tlen, vis->vis_DebugPen2);
        tek_lib_visual_debugwait(vis);
    }
#endif
    TVisualText(vis->vis_Visual, x0, y0, text, tlen, fpen->pen_Pen);
        
    vis->vis_Dirty = TTRUE;
    return 0;
}

/*-----------------------------------------------------------------------------
--  Visual:drawImage(table, x0, y0, x1, y1, pen_or_pentab): Draw a table
--  containing a simple vector image into the specified rectangle, with the
--  specified table of pens, or a single pen. Image table format:
--          {
--            [1] = { x0, y0, x1, y1, ... }, -- coordinates table
--            [4] = boolean, -- is_transparent
--            [5] = { -- table of primitive records
--              { [1]=fmtcode, [2]=nmpts, [3]={ indices }, [4]=pen_or_table },
--              ...
--            }
--          }
--  {{fmtcode}} can be {{0x1000}} for a strip, {{0x2000}} for a fan.
--  The coordinates are in the range from 0 to 0xffff.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_drawimage(lua_State *L)
{
    TEKPen *pen_override = TNULL;
    TEKVisual *vis = checkvisptr(L, 1);
    struct TExecBase *TExecBase = vis->vis_ExecBase;
    TINT sx = vis->vis_ShiftX, sy = vis->vis_ShiftY;
    lua_Integer rect[4], scalex, scaley;
    size_t primcount, i, j;
    TTAGITEM tags[2];
    tags[1].tti_Tag = TTAG_DONE;
    
    if (lua_type(L, 7) != LUA_TTABLE)
        pen_override = lookuppen(L, vis->vis_refPens, 7);
    
    rect[0] = luaL_checkinteger(L, 3);
    rect[1] = luaL_checkinteger(L, 4);
    rect[2] = luaL_checkinteger(L, 5);
    rect[3] = luaL_checkinteger(L, 6);
    scalex = rect[2] - rect[0];
    scaley = rect[1] - rect[3];
    
    lua_getmetatable(L, 1);
    /* vismeta */
    lua_rawgeti(L, -1, vis->vis_refPens);
    /* vismeta, pentab */
    lua_remove(L, -2);
    /* pentab */
    
    lua_pushinteger(L, 1);
    lua_gettable(L, 2);
    /* s: pentab, coords */
    lua_pushinteger(L, 5);
    lua_gettable(L, 2);
    /* s: pentab, coords, primitives */
#if LUA_VERSION_NUM < 502
    primcount = lua_objlen(L, -1);
#else
    primcount = lua_rawlen(L, -1);
#endif

    for (i = 0; i < primcount; i++)
    {
        lua_Integer nump, fmt;
        size_t bufsize;
        void *buf;
        TINT *coord;
        TINT *pentab;
        
        lua_rawgeti(L, -1, i + 1);
        /* s: pentab, coords, primitives, prim[i] */
        lua_rawgeti(L, -1, 1);
        /* s: pentab, coords, primitives, prim[i], fmtcode */
        fmt = luaL_checkinteger(L, -1);
        lua_rawgeti(L, -2, 2);
        /* s: pentab, coords, primitives, prim[i], fmtcode, nump */
        nump = luaL_checkinteger(L, -1);
        
        bufsize = sizeof(TINT) * 3 * nump;
        buf = vis->vis_VisBase->vis_DrawBuffer;
        if (buf && TGetSize(buf) < bufsize)
        {
            TFree(buf);
            buf = TNULL;
        }
        if (buf == TNULL)
            buf = TAlloc(TNULL, bufsize);
        vis->vis_VisBase->vis_DrawBuffer = buf;
        if (buf == TNULL)
        {
            lua_pushstring(L, "out of memory");
            lua_error(L);
        }
        coord = buf;
        
        lua_rawgeti(L, -3, 3);
        /* s: pentab, coords, primitives, prim[i], fmtcode, nump, indices */
        lua_rawgeti(L, -4, 4);
        /* s: pentab, coords, primitives, prim[i], fmtcode, nump, indices, p */
        
        pentab = lua_type(L, -1) == LUA_TTABLE ? coord + 2 * nump : TNULL;
        if (pentab)
            tags[0].tti_Tag = TVisual_PenArray;
        else
        {
            tags[0].tti_Tag = TVisual_Pen;
            if (pen_override)
                tags[0].tti_Value = pen_override->pen_Pen;
            else
            {
                lua_gettable(L, -8);
                tags[0].tti_Value = ((TEKPen *) checkpenptr(L, -1))->pen_Pen;
            }
        }
        
        for (j = 0; j < (size_t) nump; ++j)
        {
            lua_Integer idx;
            lua_rawgeti(L, -2, j + 1);
            idx = lua_tointeger(L, -1);
            lua_rawgeti(L, -8, idx * 2 - 1);
            lua_rawgeti(L, -9, idx * 2);
            /* index, x, y */
            coord[j * 2] = rect[0] + sx + 
                (lua_tointeger(L, -2) * scalex) / 0x10000;
            coord[j * 2 + 1] = rect[3] + sy +
                (lua_tointeger(L, -1) * scaley) / 0x10000;
            if (pentab)
            {
                lua_rawgeti(L, -7, idx + 1);
                pentab[j] = ((TEKPen *) checkpenptr(L, -1))->pen_Pen;
                lua_pop(L, 4);
            }
            else
                lua_pop(L, 3);
        }
        
        switch (fmt & 0xf000)
        {
            case 0x1000:
            case 0x4000:
                TVisualDrawStrip(vis->vis_Visual, coord, nump, rect[0], rect[1], 
                    scalex, scaley<0 ? -scaley : scaley, 0, 
                tags);
                break;
            case 0x2000:
                TVisualDrawFan(vis->vis_Visual, coord, nump, rect[0], rect[1], 
                    scalex, scaley<0 ? -scaley : scaley, 0, 
                tags);
            case 0xa000:
                TVisualDrawStrip(vis->vis_Visual, coord, nump, rect[0], rect[1], 
                    scalex, scaley<0 ? -scaley : scaley, 1, 
                tags);
                break;
            case 0x8000:
                TVisualDrawFan(vis->vis_Visual, coord, nump, rect[0], rect[1], 
                    scalex, scaley<0 ? -scaley : scaley, 1, 
                tags);
                break;
        }
        
        lua_pop(L, 5);
    }
    
    vis->vis_Dirty = TTRUE;
    lua_pop(L, 3);
    return 0;
}

/*-----------------------------------------------------------------------------
--  attr1, ... = Visual:getAttrs(attributes): Retrieve attributes from a
--  visual. {{attributes}} is a string, in which individual characters
--  determine a property each, which are returned in the order of their
--  occurrence. Default: {{"whxy"}}. Attributes:
--      - {{"w"}} - number, visual's width in pixels
--      - {{"h"}} - number, visual's height in pixels
--      - {{"W"}} - number, screen width in pixels
--      - {{"H"}} - number, screen height in pixels
--      - {{"x"}} - number, visual's left edge on the screen
--      - {{"y"}} - number, visual's top edge on the screen
--      - {{"s"}} - boolean, whether the visual has a selection
--      - {{"c"}} - boolean, whether the visual has a clipboard
--      - {{"M"}} - boolean, whether the screen has a window manager
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_getattrs(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TTAG values[10];
    TTAGITEM tags[10];
    size_t narg = 0, i;
    const char *opts = lua_tolstring(L, 2, &narg);
    
    if (narg > 9)
        luaL_error(L, "too many attributes");
    
    if (narg == 0)
    {
        opts = "whxy";
        narg = 4;
    }
    
    memset(values, 0, sizeof values);
    for (i = 0; i < narg; ++i)
        tags[i].tti_Value = (TTAG) &values[i];
    tags[narg].tti_Tag = TTAG_DONE;
    
    for (i = 0; i < narg; ++i)
    {
        switch (opts[i])
        {
            case 'w':
                tags[i].tti_Tag = TVisual_Width;
                break;
            case 'h':
                tags[i].tti_Tag = TVisual_Height;
                break;
            case 'W':
                tags[i].tti_Tag = TVisual_ScreenWidth;
                break;
            case 'H':
                tags[i].tti_Tag = TVisual_ScreenHeight;
                break;
            case 'x':
                tags[i].tti_Tag = TVisual_WinLeft;
                break;
            case 'y':
                tags[i].tti_Tag = TVisual_WinTop;
                break;
            case 's':
                tags[i].tti_Tag = TVisual_HaveSelection;
                break;
            case 'c':
                tags[i].tti_Tag = TVisual_HaveClipboard;
                break;
            case 'M':
                tags[i].tti_Tag = TVisual_HaveWindowManager;
                break;
            default:
                luaL_error(L, "unknown attribute");
        }
    }
    TVisualGetAttrs(vis->vis_Visual, tags);
    
    for (i = 0; i < narg; ++i)
    {
        switch (opts[i])
        {
            case 'w':
            case 'h':
            case 'W':
            case 'H':
            case 'x':
            case 'y':
                lua_pushinteger(L, *((TINT *) &values[i]));
                break;
            case 's':
            case 'c':
            case 'M':
                lua_pushboolean(L, *((TBOOL *) &values[i]));
                break;
        }
    }
    return narg;
}

/*-----------------------------------------------------------------------------
--  data = Visual:getUserData(): Get the user data attached to a visual.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_getuserdata(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    if (vis->vis_refUserData >= 0)
    {
        lua_getmetatable(L, 1);
        /* s: metatable */
        lua_rawgeti(L, -1, vis->vis_refUserData);
        /* s: metatable, userdata */
        lua_remove(L, -2);
    }
    else
        lua_pushnil(L);
    return 1;
}

/*****************************************************************************/

static TTAGITEM *getminmax(lua_State *L, TTAGITEM *tp, const char *keyname,
    TTAG tag)
{
    TBOOL isfalse;
    lua_getfield(L, 2, keyname);
    isfalse = lua_isboolean(L, -1) && !lua_toboolean(L, -1);
    if (lua_isnumber(L, -1) || isfalse)
    {
        TINT val = isfalse ? -1 : lua_tointeger(L, -1);
        tp->tti_Tag = tag;
        tp->tti_Value = val;
        tp++;
    }
    lua_pop(L, 1);
    return tp;
}

/*-----------------------------------------------------------------------------
--  num = Visual:setAttrs(table): Set attributes in a visual. Possible fields
--  in the {{table}}, as currently defined:
--      - {{"MinWidth"}} - number, minimum width the visual may shrink to
--      - {{"MinHeight"}} - number, minimum height the visual may shrink to
--      - {{"MaxWidth"}} - number, minimum width the visual may grow to
--      - {{"MaxHeight"}} - number, minimum width the visual may grow to
--      - {{"HaveSelection"}} - boolean, indicates that the visual has the
--      selection
--      - {{"HaveClipboard"}} - boolean, indicates that the visual has the
--      clipboard
--      - {{"Left"}} - number, left edge of the visual on the screen, in pixels
--      - {{"Top"}} - number, top edge of the visual on the screen, in pixels
--      - {{"Width"}} - number, width of the visual on the screen, in pixels
--      - {{"Height"}} - number, height of the visual on the screen, in pixels
--      - {{"WindowHints"}} - string, with each character acting as a hint
--      to window management. Currently defined:
--          - {{"t"}} - ''top'', to raise the window on top of all others
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_setattrs(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TTAGITEM tags[12], *tp = tags;
    tp = getminmax(L, tp, "MinWidth", TVisual_MinWidth);
    tp = getminmax(L, tp, "MinHeight", TVisual_MinHeight);
    tp = getminmax(L, tp, "MaxWidth", TVisual_MaxWidth);
    tp = getminmax(L, tp, "MaxHeight", TVisual_MaxHeight);
    lua_getfield(L, 2, "HaveSelection");
    if (!lua_isnoneornil(L, -1))
    {
        tp->tti_Tag = TVisual_HaveSelection;
        tp++->tti_Value = lua_toboolean(L, -1);
        lua_pop(L, 1);
    }
    lua_getfield(L, 2, "HaveClipboard");
    if (!lua_isnoneornil(L, -1))
    {
        tp->tti_Tag = TVisual_HaveClipboard;
        tp++->tti_Value = lua_toboolean(L, -1);
        lua_pop(L, 1);
    }
    
    lua_getfield(L, 2, "Left");
    if (lua_isnumber(L, -1))
    {
        tp->tti_Tag = TVisual_WinLeft;
        tp++->tti_Value = lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
    lua_getfield(L, 2, "Top");
    if (lua_isnumber(L, -1))
    {
        tp->tti_Tag = TVisual_WinTop;
        tp++->tti_Value = lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
    lua_getfield(L, 2, "Width");
    if (lua_isnumber(L, -1))
    {
        tp->tti_Tag = TVisual_Width;
        tp++->tti_Value = lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
    lua_getfield(L, 2, "Height");
    if (lua_isnumber(L, -1))
    {
        tp->tti_Tag = TVisual_Height;
        tp++->tti_Value = lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
    lua_getfield(L, 2, "WindowHints");
    if (lua_isstring(L, -1))
    {
        tp->tti_Tag = TVisual_WindowHints;
        tp++->tti_Value = (TTAG) lua_tostring(L, -1);
    }
    lua_pop(L, 1);
    
    tp->tti_Tag = TTAG_DONE;
    
    #if defined(TEK_VISUAL_DEBUG)
    lua_getfield(L, 2, "Debug");
    if (lua_isboolean(L, -1))
        vis->vis_VisBase->vis_Debug = lua_toboolean(L, -1);
    lua_pop(L, 1);
    #endif
    
    lua_pushnumber(L, TVisualSetAttrs(vis->vis_Visual, tags));
    return 1;
}

/*-----------------------------------------------------------------------------
--  width, height = Visual:textSize(text): Get width and height of the given
--  text, if rendered with the visual's current font. See also
--  Visual.getTextSize().
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_textsize_visual(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    size_t len;
    TSTRPTR s = (TSTRPTR) luaL_checklstring(L, 2, &len);
    lua_pushinteger(L,
        TVisualTextSize(vis->vis_Base, vis->vis_Font, s, (TINT) len));
    lua_pushinteger(L, vis->vis_FontHeight);
    return 2;
}

/*-----------------------------------------------------------------------------
--  Visual:setFont(font): Set the visual's current font.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_setfont(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TEKFont *font = checkfontptr(L, 2);
    if (font->font_Font && vis->vis_Font != font->font_Font)
    {
        lua_getmetatable(L, 1);
        /* s: vismeta */

        if (vis->vis_refFont != -1)
        {
            /* unreference old current font: */
            luaL_unref(L, -1, vis->vis_refFont);
            vis->vis_refFont = -1;
        }

        TVisualSetFont(vis->vis_Visual, font->font_Font);
        vis->vis_Font = font->font_Font;
        vis->vis_FontHeight = font->font_Height;

        /* reference new font: */
        lua_pushvalue(L, 2);
        /* s: vismeta, font */
        vis->vis_refFont = luaL_ref(L, -2);
        /* s: vismeta */
        lua_pop(L, 1);
    }
    return 0;
}

/*****************************************************************************/

static TTAG hookfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
    TEKVisual *vis = hook->thk_Data;
    struct TExecBase *TExecBase = vis->vis_ExecBase;
    TINT *rect = (TINT *) msg;
    TINT *newbuf = vis->vis_RectBuffer ?
        TRealloc(vis->vis_RectBuffer,
            (vis->vis_RectBufferNum + 4) * sizeof(TINT)) :
        TAlloc(TNULL, sizeof(TINT) * 4);

    if (newbuf)
    {
        vis->vis_RectBuffer = newbuf;
        newbuf += vis->vis_RectBufferNum;
        vis->vis_RectBufferNum += 4;
        newbuf[0] = rect[0];
        newbuf[1] = rect[1];
        newbuf[2] = rect[2];
        newbuf[3] = rect[3];
    }

    return 0;
}

/*-----------------------------------------------------------------------------
--  Visual:blitRect(x0, y0, x1, y1, dx, dy[, exposetable]): Blits the given
--  rectangle to the destination upper left position {{dx}}/{{dy}}. Source
--  areas of the blit that were previously obscured but are getting exposed by
--  blitting them into visibility show up as coordinate quartets (x0, y0, x1,
--  y1 each) in the optional {{exposetable}}.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_copyarea(lua_State *L)
{
    TTAGITEM tags[2], *tp = TNULL;
    struct THook hook;
    TEKVisual *vis = checkvisptr(L, 1);
    struct TExecBase *TExecBase = vis->vis_ExecBase;
    TINT sx = vis->vis_ShiftX;
    TINT sy = vis->vis_ShiftY;
    TINT x = luaL_checkinteger(L, 2);
    TINT y = luaL_checkinteger(L, 3);
    TINT w = luaL_checkinteger(L, 4) - x + 1;
    TINT h = luaL_checkinteger(L, 5) - y + 1;
    TINT dx = luaL_checkinteger(L, 6) + sx;
    TINT dy = luaL_checkinteger(L, 7) + sy;
    x += sx;
    y += sy;

    if (lua_istable(L, 8))
    {
        vis->vis_RectBuffer = TNULL;
        vis->vis_RectBufferNum = 0;
        TInitHook(&hook, hookfunc, vis);
        tags[0].tti_Tag = TVisual_ExposeHook;
        tags[0].tti_Value = (TTAG) &hook;
        tags[1].tti_Tag = TTAG_DONE;
        tp = tags;
    }

    TVisualCopyArea(vis->vis_Visual, x, y, w, h, dx, dy, tp);

    if (tp)
    {
        TINT i;
        for (i = 0; i < vis->vis_RectBufferNum; ++i)
        {
            lua_pushinteger(L, vis->vis_RectBuffer[i]);
            lua_rawseti(L, 8, i + 1);
        }
        TFree(vis->vis_RectBuffer);
        vis->vis_RectBuffer = TNULL;
    }

    vis->vis_Dirty = TTRUE;
    return 0;
}

/*-----------------------------------------------------------------------------
--  Visual:setClipRect(x0, y0, x1, y1): Set clipping rectangle
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_setcliprect(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TINT x = luaL_checkinteger(L, 2);
    TINT y = luaL_checkinteger(L, 3);
    TINT w = luaL_checkinteger(L, 4);
    TINT h = luaL_checkinteger(L, 5);
    
    TVisualSetClipRect(vis->vis_Visual, x, y, w, h, TNULL);
    
    vis->vis_ClipRect[0] = x;
    vis->vis_ClipRect[1] = y;
    vis->vis_ClipRect[2] = x + w - 1;
    vis->vis_ClipRect[3] = y + h - 1;
    vis->vis_HaveClipRect = TTRUE;
    
    return 0;
}

/*-----------------------------------------------------------------------------
--  Visual:unsetClipRect(): Unset clipping rectangle
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_unsetcliprect(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TVisualUnsetClipRect(vis->vis_Visual);
    vis->vis_HaveClipRect = TFALSE;
    return 0;
}

/*-----------------------------------------------------------------------------
--  Visual:setShift(sx, sy): Set coordinate displacement in visual
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_setshift(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TINT sx = vis->vis_ShiftX;
    TINT sy = vis->vis_ShiftY;
    vis->vis_ShiftX = sx + (TINT) luaL_optinteger(L, 2, 0);
    vis->vis_ShiftY = sy + (TINT) luaL_optinteger(L, 3, 0);
    lua_pushinteger(L, sx);
    lua_pushinteger(L, sy);
    return 2;
}

/*-----------------------------------------------------------------------------
--  Visual:drawRGB(table, x, y, w, h[, pw[, ph[, has_alpha[, x0]]]): Draw a
--  table of RGB values as pixels. The table starts at index {{x0}}, default
--  {{0}}. {{pw}} and {{ph}} are the "thickness" of pixels, whih allows to
--  stretch the output by the given factor. The default is {{1}} respectively.
--  The boolean {{has_alpha}} determines whether the pixel values are to be
--  interpreted as ARGB and be rendered with alpha channel.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_drawrgb(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    struct TExecBase *TExecBase = vis->vis_ExecBase;
    TINT sx = vis->vis_ShiftX, sy = vis->vis_ShiftY;
    TINT x0 = luaL_checkinteger(L, 2) + sx;
    TINT y0 = luaL_checkinteger(L, 3) + sy;
    TINT w = luaL_checkinteger(L, 5);
    TINT h = luaL_checkinteger(L, 6);
    TINT pw = luaL_optinteger(L, 7, 1);
    TINT ph = luaL_optinteger(L, 8, 1);
    TBOOL has_alpha = lua_toboolean(L, 9);
    TINT i0 = luaL_optinteger(L, 10, 0);

    TUINT *buf;
    TINT bw = w * pw;
    TINT bh = h * ph;

    luaL_checktype(L, 4, LUA_TTABLE);

    buf = TAlloc(TNULL, bw * bh * sizeof(TUINT));
    if (buf)
    {
        TUINT rgb;
        TUINT *p = buf;
        TINT i = i0;
        TINT xx, yy, x, y;

        for (y = 0; y < h; ++y)
        {
            TUINT *lp = p;
            for (x = 0; x < w; ++x)
            {
                lua_rawgeti(L, 4, i++);
                rgb = lua_tointeger(L, -1);
                lua_pop(L, 1);
                for (xx = 0; xx < pw; ++xx)
                    *p++ = rgb;
            }

            for (yy = 0; yy < ph - 1; ++yy)
            {
                TCopyMem(lp, p, bw * sizeof(TUINT));
                p += bw;
            }
        }

        TTAGITEM tags[2];
        tags[0].tti_Tag = TVisual_AlphaChannel;
        tags[0].tti_Value = has_alpha;
        tags[1].tti_Tag = TTAG_DONE;
        TVisualDrawBuffer(vis->vis_Visual, x0, y0, buf, bw, bh, bw, tags);

        TFree(buf);
    }

    vis->vis_Dirty = TTRUE;
    return 0;
}

/*-----------------------------------------------------------------------------
--  Visual:drawPixmap(pm, x0, y0, x1, y1): Draw pixmap.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT
tek_lib_visual_drawpixmap(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TEKPixmap *img = checkpixmapptr(L, 2);
    TINT sx = vis->vis_ShiftX;
    TINT sy = vis->vis_ShiftY;
    TINT x0 = luaL_checkinteger(L, 3);
    TINT y0 = luaL_checkinteger(L, 4);
    TINT w = luaL_optinteger(L, 5, x0 + img->pxm_Width - 1) - x0 + 1;
    TINT h = luaL_optinteger(L, 6, y0 + img->pxm_Height - 1) - y0 + 1;
    TTAGITEM tags[2];
    tags[0].tti_Tag = TVisual_AlphaChannel;
    tags[0].tti_Value = img->pxm_Flags & IMLFL_HAS_ALPHA;
    tags[1].tti_Tag = TTAG_DONE;
    TVisualDrawBuffer(vis->vis_Visual, x0 + sx, y0 + sy,
        img->pxm_Image.tpb_Data, w, h, img->pxm_Width, tags);
    vis->vis_Dirty = TTRUE;
    return 0;
}

/*-----------------------------------------------------------------------------
--  otx, oty = Visual:setTextureOrigin(tx, ty): Sets the texture origin for
--  the drawing operations Visual:fillRect() and Visual:drawText(), and
--  returns the old texture origin.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT 
tek_lib_visual_settextureorigin(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TINT tx = vis->vis_TextureX;
    TINT ty = vis->vis_TextureY;
    vis->vis_TextureX = lua_tointeger(L, 2);
    vis->vis_TextureY = lua_tointeger(L, 3);
    lua_pushinteger(L, tx);
    lua_pushinteger(L, ty);
    return 2;
}

/*-----------------------------------------------------------------------------
--  Visual:pushClipRect(x0, y0, x1, y1): Push a rectangle on top of the stack
--  of clipping rectangles.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT 
tek_lib_visual_pushcliprect(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    struct TExecBase *TExecBase = vis->vis_ExecBase;
    TINT sx = vis->vis_ShiftX;
    TINT sy = vis->vis_ShiftY;
    TINT x0 = luaL_checkinteger(L, 2) + sx;
    TINT y0 = luaL_checkinteger(L, 3) + sy;
    TINT x1 = luaL_checkinteger(L, 4) + sx;
    TINT y1 = luaL_checkinteger(L, 5) + sy;
    struct RectNode *clipnode = 
        (struct RectNode *) TRemHead(&vis->vis_FreeRects);
    if (clipnode == TNULL)
    {
        clipnode = TAlloc(TNULL, sizeof(struct RectNode));
        if (clipnode == TNULL)
            luaL_error(L, "Out of memory");
    }
    clipnode->rn_Rect[0] = x0;
    clipnode->rn_Rect[1] = y0;
    clipnode->rn_Rect[2] = x1;
    clipnode->rn_Rect[3] = y1;
    TAddTail(&vis->vis_ClipStack, &clipnode->rn_Node);
    
    if (vis->vis_HaveClipRect)
    {
        TINT c0 = vis->vis_ClipRect[0];
        TINT c1 = vis->vis_ClipRect[1];
        TINT c2 = vis->vis_ClipRect[2];
        TINT c3 = vis->vis_ClipRect[3];
        if (TEK_UI_OVERLAP(x0, y0, x1, y1, c0, c1, c2, c3))
        {
            x0 = TMAX(x0, c0);
            y0 = TMAX(y0, c1);
            x1 = TMIN(x1, c2);
            y1 = TMIN(y1, c3);
        }
        else
        {
            x0 = y0 = x1 = y1 = -1;
        }
    }
    
    vis->vis_ClipRect[0] = x0;
    vis->vis_ClipRect[1] = y0;
    vis->vis_ClipRect[2] = x1;
    vis->vis_ClipRect[3] = y1;
    vis->vis_HaveClipRect = TTRUE;
    TVisualSetClipRect(vis->vis_Visual, x0, y0, 
        x1 - x0 + 1, y1 - y0 + 1, TNULL);

    return 0;
}

/*-----------------------------------------------------------------------------
--  Visual:popClipRect(): Pop a rectangle from the top of the stack
--  of clipping rectangles.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT 
tek_lib_visual_popcliprect(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    TINT x0 = -1;
    TINT y0 = -1;
    TINT x1 = -1;
    TINT y1 = -1;
    TINT have_cliprect = TFALSE;
    TAddHead(&vis->vis_FreeRects, TRemTail(&vis->vis_ClipStack));
    if (!TISLISTEMPTY(&vis->vis_ClipStack))
    {
        struct TNode *next, *node = vis->vis_ClipStack.tlh_Head.tln_Succ;
        x0 = 0;
        y0 = 0;
        x1 = TEKUI_HUGE;
        y1 = TEKUI_HUGE;
        have_cliprect = TTRUE;
        for (; (next = node->tln_Succ); node = next)
        {
            struct RectNode *rn = (struct RectNode *) node;
            TINT c0 = rn->rn_Rect[0];
            TINT c1 = rn->rn_Rect[1];
            TINT c2 = rn->rn_Rect[2];
            TINT c3 = rn->rn_Rect[3];
            if (!TEK_UI_OVERLAP(x0, y0, x1, y1, c0, c1, c2, c3))
            {
                x0 = y0 = x1 = y1 = -1;
                break;
            }
            x0 = TMAX(x0, c0);
            y0 = TMAX(y0, c1);
            x1 = TMIN(x1, c2);
            y1 = TMIN(y1, c3);
        }
    }
    vis->vis_HaveClipRect = have_cliprect;
    vis->vis_ClipRect[0] = x0;
    vis->vis_ClipRect[1] = y0;
    vis->vis_ClipRect[2] = x1;
    vis->vis_ClipRect[3] = y1;
    if (have_cliprect)
        TVisualSetClipRect(vis->vis_Visual, x0, y0, x1 - x0 + 1, y1 - y0 + 1,
            TNULL);
    else
        TVisualUnsetClipRect(vis->vis_Visual);
    return 0;
}

/*-----------------------------------------------------------------------------
--  x0, y0, x1, y1 = Visual:getClipRect(): Get the visual's clipping rectangle
--  that is in effect currently.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT 
tek_lib_visual_getcliprect(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    if (vis->vis_HaveClipRect)
    {
        lua_pushinteger(L, vis->vis_ClipRect[0]);
        lua_pushinteger(L, vis->vis_ClipRect[1]);
        lua_pushinteger(L, vis->vis_ClipRect[2]);
        lua_pushinteger(L, vis->vis_ClipRect[3]);
        return 4;
    }
    return 0;
}

/*-----------------------------------------------------------------------------
--  Visual:setBGPen(paint[, tx[, ty]]): Set the visual's pen, pixmap or
--  gradient for rendering backgrounds. Also, optionally, sets a pixmap's or
--  gradient's texture origin.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT 
tek_lib_visual_setbgpen(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    if (vis->vis_refBGPen >= 0)
    {
        luaL_unref(L, lua_upvalueindex(1), vis->vis_refBGPen);
        vis->vis_refBGPen = -1;
    }
    vis->vis_BGPenType = lookuppaint(L, vis->vis_refPens, 2, 
        (void **) &vis->vis_BGPen, TTRUE);
    if (vis->vis_BGPen)
    {
        lua_pushvalue(L, 2);
        vis->vis_refBGPen = luaL_ref(L, lua_upvalueindex(1));
    }
    vis->vis_TextureX = lua_tointeger(L, 3);
    vis->vis_TextureY = lua_tointeger(L, 4);
    return 0;
}

/*-----------------------------------------------------------------------------
--  string = Visual:getSelection([type]): Gets the visual's selection of the
--  specified type:
--      - {{1}} - the selection (default)
--      - {{2}} - the clipboard
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT 
tek_lib_visual_getselection(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    struct TExecBase *TExecBase = vis->vis_ExecBase;
    struct TTagItem tags[3];
    TAPTR data;
    TSIZE len = 0;
    tags[0].tti_Tag = TVisual_SelectionType;
    tags[0].tti_Value = luaL_optinteger(L, 2, 1);
    tags[1].tti_Tag = TVisual_SelectionLength;
    tags[1].tti_Value = (TTAG) &len;
    tags[2].tti_Tag = TTAG_DONE;
    data = TVisualGetSelection(vis->vis_Visual, tags);
    if (data && len > 0)
    {
        lua_pushlstring(L, data, len);
        TFree(data);
        return 1;       
    }
    return 0;
}

/*-----------------------------------------------------------------------------
--  Visual:setSelection(string, [type]): Sets the visual's selection
--  of the specified type:
--      - {{1}} - the selection (default)
--      - {{2}} - the clipboard
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT 
tek_lib_visual_setselection(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    struct TTagItem tags[2];
    size_t len;
    const char *selection = lua_tolstring(L, 2, &len);
    tags[0].tti_Tag = TVisual_SelectionType;
    tags[0].tti_Value = luaL_optinteger(L, 3, 1);
    tags[1].tti_Tag = TTAG_DONE;
    TVisualSetSelection(vis->vis_Visual, (TSTRPTR) selection, len, tags);
    return 0;
}

/*-----------------------------------------------------------------------------
--  gradient = Visual.createGradient(x0, y0, x1, y1, rgb0, rgb1): If gradient
--  support is available, creates a gradient object with a gradient from the
--  coordinates x0,y0, with the color rgb0 to x1,y1, with the color rgb1.
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT tek_lib_visual_creategradient(lua_State *L)
{
#if defined(ENABLE_GRADIENT)
    TUINT rgb0 = luaL_checkinteger(L, 5);
    TUINT rgb1 = luaL_checkinteger(L, 6);
    TEKGradient *gr = lua_newuserdata(L, sizeof(TEKGradient));
    gr->A.vec.x = luaL_checkinteger(L, 1);
    gr->A.vec.y = luaL_checkinteger(L, 2);
    gr->B.vec.x = luaL_checkinteger(L, 3);
    gr->B.vec.y = luaL_checkinteger(L, 4);
    gr->A.r = (rgb0 & 0xff0000) >> 16;
    gr->A.g = (rgb0 & 0xff00) >> 8;
    gr->A.b = rgb0 & 0xff;
    gr->B.r = (rgb1 & 0xff0000) >> 16;
    gr->B.g = (rgb1 & 0xff00) >> 8;
    gr->B.b = rgb1 & 0xff;
    luaL_newmetatable(L, TEK_LIB_VISUALGRADIENT_CLASSNAME);
    lua_setmetatable(L, -2);
#else
    lua_pushnil(L);
#endif
    return 1;
}

/*-----------------------------------------------------------------------------
--  type = Visual:getPaintInfo(): Returns the type of background paint that
--  is currently effective:
--      - {{0}} - undefined
--      - {{1}} - single-color pen
--      - {{2}} - pixmap
--      - {{3}} - gradient
-----------------------------------------------------------------------------*/

LOCAL LUACFUNC TINT tek_lib_visual_getpaintinfo(lua_State *L)
{
    TEKVisual *vis = checkvisptr(L, 1);
    void *udata;
    lua_pushinteger(L, getbgpaint(L, vis, 2, &udata));
    return 1;
}

/*-----------------------------------------------------------------------------
--  visual = Visual.open([args]): Opens a visual. Possible keys and their
--  values in the {{args}} table:
--      - {{"UserData"}} - Lua userdata attached to the visual
--      - {{"Pens"}} - table, for looking up pens, pixmaps, and gradients
--      - {{"Title"}} - string, window title (if applicable)
--      - {{"Borderless"}} - bool, open window borderless (if applicable)
--      - {{"PopupWindow"}} - bool, the window is used for a popup
--      - {{"Center"}} - bool, try to open the window centered on the screen
--      - {{"FullScreen"}} - bool, try to open in fullscreen mode
--      - {{"Width"}} - number, width in pixels
--      - {{"Height"}} - number, height in pixels
--      - {{"Left"}} - number, left edge in pixels on the screen
--      - {{"Top"}} - number, top edge in pixels on the screen
--      - {{"MinWidth"}} - number, min. width for the window to shrink to
--      - {{"MinHeight"}} - number, min. height for the window to shrink to
--      - {{"MaxWidth"}} - number, max. width for the window to grow to
--      - {{"MaxHeight"}} - number, max. height for the window to grow to
--      - {{"EventMask"}} - number, mask of input message types, see also
--      Visual.getMsg() for details
--      - {{"BlankCursor"}} - bool, clear the mouse pointer over the window
--      - {{"ExtraArgs"}} - string, extra arugments containing hints for the
--      underlying display driver
--      - {{"MsgFileNo"}} - number, of a file descriptor to read messages of
--      type {{MSG_USER}} from. Default: the file number of {{stdin}}
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
--  Visual.close(): Close visual.
-----------------------------------------------------------------------------*/
