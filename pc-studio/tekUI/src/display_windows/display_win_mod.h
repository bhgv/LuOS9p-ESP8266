#ifndef _TEK_DISPLAY_WINDOWS_MOD_H
#define _TEK_DISPLAY_WINDOWS_MOD_H

/*
**	display_win_mod.h - Windows display driver
**	Written by Timm S. Mueller <tmueller@schulze-mueller.de>
**	contributions by Tobias Schwinger <tschwinger@isonews2.com>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/exec.h>
#include <tek/teklib.h>

#include <tek/proto/exec.h>
#include <tek/inline/exec.h>
#include <tek/mod/visual.h>
#include <tek/lib/utf8.h>

#include <windows.h>

/*****************************************************************************/

#define FB_DISPLAY_CLASSNAME "tek_display_windows_class"
#define FB_DISPLAY_CLASSNAME_POPUP "tek_display_windows_popup_class"

#define FB_DISPLAY_VERSION		1
#define FB_DISPLAY_REVISION		0
#define FB_DISPLAY_NUMVECTORS	10

#define FB_DEF_WIDTH			600
#define FB_DEF_HEIGHT			400

#ifndef LOCAL
#define LOCAL
#endif

#ifndef EXPORT
#define EXPORT TMODAPI
#endif

/*****************************************************************************/
/*
**	Fonts
*/

#define FNT_DEFNAME			""
#define FNT_DEFPXSIZE		10

#define FNT_WILDCARD		"*"

#define FNTQUERY_NUMATTR	(5+1)
#define FNTQUERY_UNDEFINED	-1

struct FontManager
{
	/* list of opened fonts */
	struct TList openfonts;
	/* pointer to default font */
	TAPTR deffont;
	/* count of references to default font */
	TINT defref;
};

struct FontNode
{
	struct THandle handle;
	TAPTR font;
	TUINT attr;
	TUINT pxsize;
	TINT ascent;
	TINT descent;
	TINT height;
};

struct FontQueryNode
{
	struct TNode node;
	TTAGITEM tags[FNTQUERY_NUMATTR];
};

struct FontQueryHandle
{
	struct THandle handle;
	struct TList reslist;
	struct TNode **nptr;
	TUINT fpxsize;
	TBOOL fitalic;
	TBOOL fbold;
	TBOOL fscale;
	TBOOL fnum;
	TBOOL success;
	TINT match_depth;	/* 0: find font, 1: find style */
};

/*****************************************************************************/

typedef struct
{
	/* Module header: */
	struct TModule fbd_Module;
	/* Exec module base ptr: */
	struct TExecBase *fbd_ExecBase;
	/* Locking for module base: */
	struct TLock *fbd_Lock;
	/* Number of module opens: */
	TUINT fbd_RefCount;
	/* Task: */
	struct TTask *fbd_Task;
	/* Command message port: */
	struct TMsgPort *fbd_CmdPort;
	/* Command message port signal: */
	TUINT fbd_CmdPortSignal;

	/* screen size: */
	TINT fbd_ScreenWidth, fbd_ScreenHeight;
	
	/* list of all visuals: */
	struct TList fbd_VisualList;
	/* Module global memory manager (thread safe): */
	struct TMemManager *fbd_MemMgr;

	struct FontManager fbd_FontManager;

	struct TTask *fbd_InputTask;

	HWND fbd_DeviceHWnd;
	HDC fbd_DeviceHDC;

	HINSTANCE fbd_HInst;
	ATOM fbd_ClassAtom;
	ATOM fbd_ClassAtomPopup;

	HWND fbd_WindowFocussedApp;
	HWND fbd_WindowUnderCursor;
	HWND fbd_WindowActivePopup;

	TINT fbd_NumInterval;

	TUINT fbd_KeyQual;
	BYTE fbd_KeyState[256];

} WINDISPLAY;

typedef struct
{
	struct TNode fbv_Node;
	TINT fbv_Modulo;

	WINDISPLAY *fbv_Display;

	/* Window extents: */
	CRITICAL_SECTION fbv_LockExtents;
	TUINT fbv_Left, fbv_Top;
	TUINT fbv_Width, fbv_Height;

	/* list of allocated pens: */
	struct TList penlist;

	/* current active font */
	TAPTR fbv_CurrentFont;

	/* List of queued input messages to be sent: */
	struct TList fbv_IMsgQueue;
	/* Destination message port for input messages: */
	TAPTR fbv_IMsgPort;

	TUINT fbv_InputMask;

	TSTRPTR fbv_Title;

	HWND fbv_HWnd;
	HDC fbv_HDC;

	TINT fbv_MouseX, fbv_MouseY;

	BITMAPINFOHEADER fbv_DrawBitMap;

	RECT fbv_ClipRect;

	char fbv_RegionData[1024];

	TBOOL fbv_Borderless;

	/* userdata attached to this window, also propagated in messages: */
	TTAG fbv_UserData;

	TINT fbv_BorderWidth, fbv_BorderHeight;
	TINT fbv_BorderLeft, fbv_BorderTop;

	TINT fbv_MinWidth, fbv_MinHeight;
	TINT fbv_MaxWidth, fbv_MaxHeight;

} WINWINDOW;

struct FBPen
{
	struct TNode node;
	TUINT32 rgb;
	COLORREF col;
	HPEN pen;
	HBRUSH brush;
};

struct attrdata
{
	WINDISPLAY *mod;
	WINWINDOW *v;
	TEXTMETRIC textmetric;
	TINT neww, newh;
	TINT num;
};

/*****************************************************************************/
/*
**	UTF8 support
*/

#define WIN_UTF8_BUFSIZE 4096

struct UTF8Reader
{
	/* character reader callback: */
	int (*readchar)(struct UTF8Reader *);
	/* reader state: */
	int accu, numa, min, bufc;
	/* userdata to reader */
	void *udata;
};

LOCAL int readutf8(struct UTF8Reader *rd);
LOCAL unsigned char *encodeutf8(unsigned char *buf, int c);

/*****************************************************************************/

LOCAL TBOOL fb_init(WINDISPLAY *mod, TTAGITEM *tags);
LOCAL TBOOL fb_getimsg(WINDISPLAY *mod, WINWINDOW *v, TIMSG **msgptr,
	TUINT type);
LOCAL void fb_sendimessages(WINDISPLAY *mod);

LOCAL void win_getminmax(WINWINDOW *win, TINT *pm1, TINT *pm2, TINT *pm3,
	TINT *pm4, TBOOL windowsize);

LOCAL void fb_exit(WINDISPLAY *mod);
LOCAL void fb_wake(WINDISPLAY *inst);
LOCAL void fb_openwindow(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_closewindow(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_setinput(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_allocpen(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_freepen(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_frect(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_rect(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_line(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_plot(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_drawstrip(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_clear(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_getattrs(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_setattrs(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_drawtext(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_openfont(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_getfontattrs(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_textsize(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_setfont(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_closefont(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_queryfonts(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_getnextfont(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_drawtags(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_drawfan(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_copyarea(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_setcliprect(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_unsetcliprect(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_drawbuffer(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_getselection(WINDISPLAY *mod, struct TVRequest *req);
LOCAL void fb_setselection(WINDISPLAY *mod, struct TVRequest *req);

LOCAL TAPTR fb_hostopenfont(WINDISPLAY *mod, TTAGITEM *tags);
LOCAL void fb_hostclosefont(WINDISPLAY *mod, TAPTR font);
LOCAL void fb_hostsetfont(WINDISPLAY *mod, WINWINDOW *v, TAPTR font);
LOCAL TTAGITEM *fb_hostgetnextfont(WINDISPLAY *mod, struct FontQueryHandle *fqh);
LOCAL TINT fb_hosttextsize(WINDISPLAY *mod, TAPTR font, TSTRPTR text, TSIZE len);
LOCAL THOOKENTRY TTAG fb_hostgetfattrfunc(struct THook *hook, TAPTR obj,
	TTAG msg);
LOCAL TAPTR fb_hostqueryfonts(WINDISPLAY *mod, TTAGITEM *tags);

#endif /* _TEK_DISPLAY_WINDOWS_MOD_H */
