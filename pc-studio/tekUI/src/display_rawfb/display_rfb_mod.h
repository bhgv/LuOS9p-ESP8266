#ifndef _TEK_DISPLAY_RFB_MOD_H
#define _TEK_DISPLAY_RFB_MOD_H

/*
**	display_rfb_mod.h - Raw framebuffer display driver
**	Written by Franciska Schulze <fschulze at schulze-mueller.de>
**	and Timm S. Mueller <tmueller at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/exec.h>
#include <tek/teklib.h>
#include <tek/mod/visual.h>
#include <tek/lib/region.h>
#include <tek/lib/utf8.h>
#include <tek/lib/pixconv.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_CACHE_H

#if defined(ENABLE_VNCSERVER)
#include <rfb/rfb.h>
#include <rfb/rfbregion.h>
#endif

/*****************************************************************************/

#define RFB_DISPLAY_VERSION     2
#define RFB_DISPLAY_REVISION    0
#define RFB_DISPLAY_NUMVECTORS  10

#ifndef LOCAL
#define LOCAL
#endif

#ifndef EXPORT
#define EXPORT TMODAPI
#endif

#define RFB_HUGE 1000000

/* display flags */
#define RFBFL_BUFFER_OWNER      0x0001
#define RFBFL_BUFFER_DEVICE     0x0002
#define RFBFL_BUFFER_CAN_RESIZE 0x0004
#define RFBFL_CANSHOWPTR        0x0008
#define RFBFL_SHOWPTR           0x0010
#define RFBFL_BACKBUFFER        0x0020
#define RFBFL_PTR_VISIBLE       0x0100
#define RFBFL_PTR_ALLOCATED     0x0200
#define RFBFL_PTRMASK           0x0300
#define RFBFL_WIN_BACKBUFFER    0x0400
#define RFBFL_DIRTY             0x0800

/* window flags */
#define RFBWFL_IS_POPUP         0x0001
#define RFBWFL_BORDERLESS       0x0002
#define RFBWFL_FULLSCREEN       0x0004
#define RFBWFL_IS_ROOT          0x0010
#define RFBWFL_USERCLIP         0x0020
#define RFBWFL_BACKBUFFER       0x0400
#define RFBWFL_DIRTY            0x0800

#ifndef RFB_DEF_WIDTH
#define RFB_DEF_WIDTH           640
#endif
#ifndef RFB_DEF_HEIGHT
#define RFB_DEF_HEIGHT          480
#endif

#define RFB_UTF8_BUFSIZE        4096

#define RFB_DIRTY_ALIGN         7

/*****************************************************************************/

#if defined(ENABLE_LINUXFB)

#include <linux/fb.h>
#include <linux/input.h>

#define RFB_RAWKEY_QUALS        5

struct RawKey
{
	TUINT16 qualifier;			/* qualifier activated */
	TUINT keycode;				/* keycode active independent of qualifier */
	struct
	{
		TUINT16 qualifier;		/* qualifier */
		TUINT keycode;			/* keycode activated dependent on qualifier */
	} qualkeys[RFB_RAWKEY_QUALS];
};

#endif /* defined(ENABLE_LINUXFB) */

struct rfb_BackBuffer
{
	TUINT8 *data;
	TINT rect[4];
};

/*****************************************************************************/

/*
**	Fonts
*/

#ifndef DEF_FONTDIR
#define	DEF_FONTDIR         "tek/ui/font"
#endif

#define FNT_DEFNAME         "VeraMono"
#define FNT_DEFPXSIZE       14

#define	FNT_WILDCARD        "*"

#define FNTQUERY_NUMATTR	(5+1)
#define	FNTQUERY_UNDEFINED	-1

#define FNT_ITALIC			0x1
#define	FNT_BOLD			0x2
#define FNT_UNDERLINE		0x4

#define FNT_MATCH_NAME		0x01
#define FNT_MATCH_SIZE		0x02
#define FNT_MATCH_SLANT		0x04
#define	FNT_MATCH_WEIGHT	0x08
#define	FNT_MATCH_SCALE		0x10

/* all mandatory properties: */
#define FNT_MATCH_ALL		0x0f

struct rfb_FontManager
{
	struct TList openfonts;
};

struct rfb_FontNode
{
	struct THandle handle;
	FT_Face face;
	TUINT pxsize;
	TINT ascent;
	TINT descent;
	TINT height;
	TSTRPTR name;
};

struct rfb_FontQueryNode
{
	struct TNode node;
	TTAGITEM tags[FNTQUERY_NUMATTR];
};

struct rfb_FontQueryHandle
{
	struct THandle handle;
	struct TList reslist;
	struct TNode **nptr;
};

/*****************************************************************************/

struct rfb_Display
{
	/* Module header: */
	struct TModule rfb_Module;
	/* Exec module base ptr: */
	struct TExecBase *rfb_ExecBase;
	/* Locking for module base: */
	struct TLock *rfb_Lock;
	/* Number of module opens: */
	TUINT rfb_RefCount;
	/* Task: */
	struct TTask *rfb_Task;
	/* Command message port: */
	struct TMsgPort *rfb_CmdPort;

	/* Sub rendering device (optional): */
	TAPTR rfb_RndDevice;
	/* Replyport for render requests: */
	struct TMsgPort *rfb_RndRPort;
	/* Render device instance: */
	TAPTR rfb_RndInstance;
	/* Render request: */
	struct TVRequest *rfb_RndRequest;
	/* Own input message port receiving input from sub device: */
	TAPTR rfb_RndIMsgPort;

	/* Device open tags: */
	TTAGITEM *rfb_OpenTags;

	/* Module global memory manager (thread safe): */
	struct TMemManager *rfb_MemMgr;

	/* Locking for instance data: */
	struct TLock *rfb_InstanceLock;

	/* pooled input messages: */
	struct TList rfb_IMsgPool;

	/* list of all visuals: */
	struct TList rfb_VisualList;

	struct RectPool rfb_RectPool;

	/* pixel buffer exposed to drawing functions: */
	struct TVPixBuf rfb_PixBuf;
	/* pixel buffer exposed to the device: */
	struct TVPixBuf rfb_DevBuf;

	/* Device width/height */
	TINT rfb_DevWidth, rfb_DevHeight;

	/* Actual width/height */
	TINT rfb_Width, rfb_Height;

	TUINT rfb_Flags;
	TINT rfb_NumInterval;

	/* font rendering */
	FT_Library rfb_FTLibrary;
	FTC_Manager rfb_FTCManager;
	FTC_CMapCache rfb_FTCCMapCache;
	FTC_SBitCache rfb_FTCSBitCache;
	struct rfb_FontManager rfb_FontManager;

	TINT rfb_MouseX;
	TINT rfb_MouseY;
	TINT rfb_KeyQual;

	TUINT32 rfb_unicodebuffer[RFB_UTF8_BUFSIZE];

	struct Region rfb_DirtyRegion;

	struct rfb_Window *rfb_FocusWindow;

	struct TVPixBuf rfb_PtrImage;
	TINT rfb_PtrWidth, rfb_PtrHeight;
	TINT rfb_MouseHotX, rfb_MouseHotY;
	struct rfb_BackBuffer rfb_PtrBackBuffer;

#if defined(ENABLE_VNCSERVER)
	rfbScreenInfoPtr rfb_RFBScreen;
	TAPTR rfb_VNCTask;
	int rfb_RFBPipeFD[2];
	TUINT rfb_RFBReadySignal;
	TAPTR rfb_RFBMainTask;
	TBOOL rfb_WaitSignal;
#endif

#if defined(ENABLE_LINUXFB)
	int rfb_fbhnd;
	struct fb_var_screeninfo rfb_orig_vinfo;
	struct fb_var_screeninfo rfb_vinfo;
	struct fb_fix_screeninfo rfb_finfo;
	int rfb_fd_input_mouse;
	int rfb_fd_input_kbd;
	int rfb_fd_sigpipe_read;
	int rfb_fd_sigpipe_write;
	int rfb_fd_max;
	struct input_absinfo rfb_absinfo[2];
	int rfb_button_touch;
	int rfb_abspos[2];
	int rfb_absstart[2];
	int rfb_startmouse[2];
	int rfb_ttyfd;
	int rfb_ttyoldmode;
	struct RawKey *rfb_RawKeys[256];
	int rfb_fd_inotify_input;
	int rfb_fd_watch_input;
#endif

};

struct rfb_Window
{
	struct TNode rfbw_Node;

	struct rfb_Display *rfbw_Display;

	/* Window extents on screen: */
	struct Rect rfbw_ScreenRect;
	/* Window extents for drawing: */
	struct Rect rfbw_WinRect;
	/* Clipping boundaries (user): */
	struct Rect rfbw_UserClipRect;
	/* Clipping boundaries (real): */
	struct Rect rfbw_ClipRect;
	/* Current pens: */
	TVPEN rfbw_BGPen, rfbw_FGPen;
	/* list of allocated pens: */
	struct TList rfbw_PenList;
	/* current active font */
	TAPTR rfbw_CurrentFont;
	/* Destination message port for input messages: */
	TAPTR rfbw_IMsgPort;
	/* mask of active events */
	TUINT rfbw_InputMask;
	/* userdata attached to this window, also propagated in messages: */
	TTAG rfbw_UserData;

	/* Pixel buffer referring to upper left edge of visual: */
	struct TVPixBuf rfbw_PixBuf;

	TUINT rfbw_Flags;

	TINT rfbw_MinWidth;
	TINT rfbw_MinHeight;
	TINT rfbw_MaxWidth;
	TINT rfbw_MaxHeight;

	struct Region rfbw_DirtyRegion;
};

struct rfb_Pen
{
	struct TNode node;
	TUINT32 rgb;
};

struct rfb_attrdata
{
	struct rfb_Display *mod;
	struct rfb_Window *v;
	TAPTR font;
	TINT num;
	TINT neww, newh, newx, newy;
	TSTRPTR hints;
};

/*****************************************************************************/

LOCAL void fbp_drawfrect(struct rfb_Display *mod, struct rfb_Window *v,
	TINT rect[4], struct rfb_Pen *pen);
LOCAL void fbp_drawrect(struct rfb_Display *mod, struct rfb_Window *v,
	TINT rect[4], struct rfb_Pen *pen);
LOCAL void fbp_drawline(struct rfb_Display *mod, struct rfb_Window *v,
	TINT rect[4], struct rfb_Pen *pen);
LOCAL void fbp_drawtriangle(struct rfb_Display *mod, struct rfb_Window *v,
	TINT x0, TINT y0, TINT x1, TINT y1, TINT x2, TINT y2, struct rfb_Pen *pen);
LOCAL void fbp_drawbuffer(struct rfb_Display *mod, struct rfb_Window *v,
	struct TVPixBuf *src, TINT rect[4], TBOOL alpha);
LOCAL void fbp_doexpose(struct rfb_Display *mod, struct rfb_Window *v,
	struct Region *L, struct THook *exposehook);
LOCAL TBOOL fbp_copyarea_int(struct rfb_Display *mod, struct rfb_Window *v,
	TINT dx, TINT dy, TINT *dr);
LOCAL TBOOL fbp_copyarea(struct rfb_Display *mod, struct rfb_Window *v,
	TINT dx, TINT dy, TINT d[4], struct THook *exposehook);

LOCAL TBOOL rfb_initpointer(struct rfb_Display *mod);
LOCAL TBOOL rfb_getimsg(struct rfb_Display *mod, struct rfb_Window *v,
	TIMSG ** msgptr, TUINT type);
LOCAL void rfb_putbackmsg(struct rfb_Display *mod, TIMSG *msg);
LOCAL struct rfb_Window *rfb_passevent_by_mousexy(struct rfb_Display *mod,
	TIMSG *omsg, TBOOL focus);
LOCAL void rfb_passevent_mousebutton(struct rfb_Display *mod, TIMSG *msg);
LOCAL void rfb_passevent_keyboard(struct rfb_Display *mod, TIMSG *msg);
LOCAL void rfb_passevent_mousemove(struct rfb_Display *mod, TIMSG *msg);
LOCAL TINT rfb_sendevent(struct rfb_Display *mod, TUINT type, TUINT code,
	TINT x, TINT y);
LOCAL TBOOL rfb_getlayers(struct rfb_Display *mod, struct Region *A,
	struct rfb_Window *v, TINT dx, TINT dy);
LOCAL TBOOL rfb_getlayermask(struct rfb_Display *mod, struct Region *A,
	TINT *crect, struct rfb_Window *v, TINT dx, TINT dy);
LOCAL TBOOL rfb_damage(struct rfb_Display *mod, TINT drect[],
	struct rfb_Window *v);
LOCAL void rfb_markdirty(struct rfb_Display *mod, struct rfb_Window *v,
	TINT *r);
LOCAL void rfb_setwinrect(struct rfb_Display *mod, struct rfb_Window *v);
LOCAL void rfb_setrealcliprect(struct rfb_Display *mod, struct rfb_Window *v);
LOCAL void rfb_focuswindow(struct rfb_Display *mod, struct rfb_Window *v);
LOCAL void rfb_flush_clients(struct rfb_Display *mod, TBOOL also_external);
LOCAL TBOOL rfb_resizewinbuffer(struct rfb_Display *mod, struct rfb_Window *v,
	TINT oldw, TINT oldh, TINT w, TINT h);
LOCAL void rfb_copyrect_sub(struct rfb_Display *mod, TINT *rect, TINT dx,
	TINT dy);

LOCAL FT_Error rfb_fontrequester(FTC_FaceID faceID, FT_Library lib,
	FT_Pointer reqData, FT_Face *face);
LOCAL TAPTR rfb_hostopenfont(struct rfb_Display *mod, TTAGITEM *tags);
LOCAL TAPTR rfb_hostqueryfonts(struct rfb_Display *mod, TTAGITEM *tags);
LOCAL void rfb_hostsetfont(struct rfb_Display *mod, struct rfb_Window *v,
	TAPTR font);
LOCAL TTAGITEM *rfb_hostgetnextfont(struct rfb_Display *mod, TAPTR fqhandle);
LOCAL void rfb_hostclosefont(struct rfb_Display *mod, TAPTR font);
LOCAL TINT rfb_hosttextsize(struct rfb_Display *mod, TAPTR font, TSTRPTR text,
	TINT len);
LOCAL TVOID rfb_hostdrawtext(struct rfb_Display *mod, struct rfb_Window *v,
	TSTRPTR text, TINT len, TINT posx, TINT posy, TVPEN fgpen);
LOCAL THOOKENTRY TTAG rfb_hostgetfattrfunc(struct THook *hook, TAPTR obj,
	TTAG msg);

LOCAL void rfb_docmd(struct rfb_Display *mod, struct TVRequest *req);

#if defined(ENABLE_VNCSERVER)
int rfb_vnc_init(struct rfb_Display *mod, int port);
void rfb_vnc_exit(struct rfb_Display *mod);
void rfb_vnc_flush(struct rfb_Display *mod, struct Region *D);
void rfb_vnc_copyrect(struct rfb_Display *mod, struct rfb_Window *v, int dx,
	int dy, int x0, int y0, int x1, int y1, int yinc);
#endif

#if defined(ENABLE_LINUXFB)
LOCAL TBOOL rfb_linux_init(struct rfb_Display *mod);
LOCAL void rfb_linux_exit(struct rfb_Display *mod);
LOCAL void rfb_linux_wait(struct rfb_Display *mod, TTIME *waitt);
LOCAL void rfb_linux_wake(struct rfb_Display *mod);
#endif

#endif /* _TEK_DISPLAY_RFB_MOD_H */
