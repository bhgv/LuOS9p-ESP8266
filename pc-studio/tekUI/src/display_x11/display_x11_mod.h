#ifndef _TEK_DISPLAY_X11_MOD_H
#define _TEK_DISPLAY_X11_MOD_H

/*
**  teklib/src/visual/display_x11_mod.h - X11 Display Driver
**  Written by Timm S. Mueller <tmueller at neoscientists.org>
**  See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/mod/visual.h>
#include <tek/lib/region.h>
#include <tek/lib/utf8.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>

#if defined(ENABLE_XFT)
#include <X11/Xft/Xft.h>
#endif

#if defined(ENABLE_XSHM)
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

#if defined(ENABLE_XVID)
#include <X11/extensions/xf86vmode.h>
#endif

/*****************************************************************************/

#define X11DISPLAY_VERSION      1
#define X11DISPLAY_REVISION     1
#define X11DISPLAY_NUMVECTORS   10

#define X11_UTF8_BUFSIZE 4096

#ifndef LOCAL
#define LOCAL
#endif

#ifndef EXPORT
#define EXPORT TMODAPI
#endif

#define X11_DEF_WINWIDTH 600
#define X11_DEF_WINHEIGHT 400

/*****************************************************************************/

#define X11FNT_LENGTH           41
#define X11FNT_DEFNAME          "fixed"
#define X11FNT_WGHT_MEDIUM      "medium"
#define X11FNT_WGHT_BOLD        "bold"
#define X11FNT_SLANT_R          "r"
#define X11FNT_SLANT_I          "i"
#define X11FNT_DEFPXSIZE        14
#define X11FNT_DEFREGENC        "iso8859-1"
#define X11FNT_WILDCARD         "*"

#define X11FNTQUERY_NUMATTR (5+1)
#define X11FNTQUERY_UNDEFINED   0xffffffff

#define X11FNT_ITALIC           0x1
#define X11FNT_BOLD             0x2
#define X11FNT_UNDERLINE        0x4

#define X11FNT_MATCH_NAME       0x01
#define X11FNT_MATCH_SIZE       0x02
#define X11FNT_MATCH_SLANT      0x04
#define X11FNT_MATCH_WEIGHT     0x08
#define X11FNT_MATCH_SCALE      0x10

/* all mandatory properties: */
#define X11FNT_MATCH_ALL        0x0f

struct X11FontMan
{
    struct TList openfonts;     /* list of opened fonts */
    TAPTR deffont;              /* pointer to default font */
    TINT defref;                /* count of references to default font */
};

struct X11FontHandle
{
    struct THandle handle;
    XFontStruct *font;
#if defined(ENABLE_XFT)
    XftFont *xftfont;
#endif
    TUINT attr;
    TUINT pxsize;
};

struct X11FontQueryNode
{
    struct TNode node;
    TTAGITEM tags[X11FNTQUERY_NUMATTR];
};

struct X11FontQueryHandle
{
    struct THandle handle;
    struct TList reslist;
    struct TNode **nptr;
};

/* internal structures */

struct X11FontNode
{
    struct TNode node;
    TSTRPTR fname;
};

struct X11FontAttr
{
    struct TList fnlist;        /* list of fontnames */
    TSTRPTR fname;
    TUINT fpxsize;
    TUINT flags;
    TBOOL fitalic;
    TBOOL fbold;
    TBOOL fscale;
    TINT fnum;
};

#define X11FL_SWAPBYTEORDER     0x0001
#define X11FL_USE_XFT           0x0002
#define X11FL_SHMAVAIL          0x0004
#define X11FL_FULLSCREEN        0x0008
#define X11WFL_IMG_SHM          0x0010
#define X11WFL_WAIT_EXPOSE      0x0020
#define X11WFL_WAIT_RESIZE      0x0040
#define X11WFL_CHANGE_VIDMODE   0x0080
#define X11WFL_IS_ROOTWINDOW    0x0100

/*****************************************************************************/

struct X11Display
{
    /* Module header: */
    struct TModule x11_Module;
    /* Module global memory manager (thread safe): */
    TAPTR x11_MemMgr;
    /* Locking for module base structure: */
    TAPTR x11_Lock;
    /* Number of module opens: */
    TUINT x11_RefCount;
    /* Task: */
    TAPTR x11_Task;
    /* Command message port: */
    TAPTR x11_CmdPort;

    /* X11 display: */
    Display *x11_Display;
    /* default X11 screen number: */
    int x11_Screen;
    /* default X11 visual: */
    Visual *x11_Visual;

    TAPTR x11_IReplyPort;
    struct THook x11_IReplyHook;

    TUINT x11_Flags;
    
    TINT x11_DefaultBPP;
    TINT x11_DefaultDepth;
    TINT x11_ByteOrder;

    int x11_fd_display;
    int x11_fd_sigpipe_read;
    int x11_fd_sigpipe_write;
    int x11_fd_max;

    struct X11FontMan x11_fm;

    /* list of all visuals: */
    struct TList x11_vlist;

    struct TList x11_imsgpool;

    struct TVRequest *x11_RequestInProgress;
    struct THook *x11_CopyExposeHook;

    Region x11_HugeRegion;
    TINT x11_ShmEvent;

    TINT x11_KeyQual;
    TINT x11_ScreenMouseX, x11_ScreenMouseY;

    TUINT8 x11_utf8buffer[X11_UTF8_BUFSIZE];

    Cursor x11_NullCursor;
#if defined(ENABLE_DEFAULTCURSOR)
    Cursor x11_DefaultCursor;
#endif

    TTAGITEM *x11_InitTags;
    struct TMsgPort *x11_IMsgPort;

    TINT x11_ScreenWidth;
    TINT x11_ScreenHeight;

    /* vidmode screensize: */
    TINT x11_FullScreenWidth;
    TINT x11_FullScreenHeight;

    Atom x11_XA_TARGETS;
    Atom x11_XA_PRIMARY;
    Atom x11_XA_CLIPBOARD;
    Atom x11_XA_UTF8_STRING;
    Atom x11_XA_STRING;
    Atom x11_XA_COMPOUND_TEXT;

#if defined(ENABLE_XVID)
    XF86VidModeModeInfo x11_OldMode;
    XF86VidModeModeInfo x11_VidMode;
#endif

    TINT x11_NumWindows;
    TINT x11_NumInterval;
};

struct X11Pen
{
    struct TNode node;
    XColor color;
#if defined(ENABLE_XFT)
    XftColor xftcolor;
#endif
};

struct X11Window
{
    struct TNode node;

    TINT winwidth, winheight;
    TINT winleft, wintop;
    TSTRPTR title;

    Window window;

    Colormap colormap;
    GC gc;

#if defined(ENABLE_XFT)
    XftDraw *draw;
#endif
    TAPTR curfont;              /* current active font */

    Atom atom_wm_delete_win;

    TUINT base_mask;
    TUINT eventmask;
    
    TUINT flags;

    TVPEN bgpen, fgpen;

    XImage *image;
    char *tempbuf;
    int imw, imh;

    XSizeHints *sizehints;

    XImage *imgCursorBG;
    int old_crsr_x, old_crsr_y, old_crsr_w, old_crsr_h;

    struct TList imsgqueue;
    TAPTR imsgport;

    /* list of allocated pens: */
    struct TList penlist;

#if defined(ENABLE_XSHM)
    XShmSegmentInfo shminfo;
    size_t shmsize;
#endif

    /* userdata attached to this window, also propagated in messages: */
    TTAG userdata;

    TUINT pixfmt;
    TUINT bpp;

    TINT mousex, mousey;
};

struct attrdata
{
    struct X11Display *mod;
    struct X11Window *v;
    TAPTR font;
    TINT num;
    TBOOL sizechanged;
    TINT neww, newh, newx, newy;
};

/*****************************************************************************/

LOCAL TSTRPTR x11_utf8tolatin(struct X11Display *mod, TSTRPTR utf8string,
    TINT len, TINT *bytelen);

LOCAL void x11_docmd(struct X11Display *inst, struct TVRequest *req);

LOCAL void x11_sendimessages(struct X11Display *mod);
LOCAL TTASKENTRY void x11_taskfunc(struct TTask *task);

LOCAL TBOOL x11_initlibxft(struct X11Display *mod);
LOCAL void x11_exitlibxft(struct X11Display *mod);
LOCAL void x11_hostsetfont(struct X11Display *mod, struct X11Window *v,
    TAPTR font);
LOCAL TAPTR x11_hostopenfont(struct X11Display *mod, TTAGITEM *tags);
LOCAL TAPTR x11_hostqueryfonts(struct X11Display *mod, TTAGITEM *tags);
LOCAL void x11_hostclosefont(struct X11Display *mod, TAPTR font);
LOCAL TINT x11_hosttextsize(struct X11Display *mod, TAPTR font, TSTRPTR text,
    TINT len);
LOCAL THOOKENTRY TTAG x11_hostgetfattrfunc(struct THook *hook, TAPTR obj,
    TTAG msg);
LOCAL TTAGITEM *x11_hostgetnextfont(struct X11Display *mod, TAPTR fqhandle);

#endif /* _TEK_DISPLAY_X11_MOD_H */
