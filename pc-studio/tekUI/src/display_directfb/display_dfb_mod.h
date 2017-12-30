#ifndef _TEK_DISPLAY_DIRECTFB_MOD_H
#define _TEK_DISPLAY_DIRECTFB_MOD_H

/*
**	teklib/src/display_dfb/display_dfb_mod.h - DirectFB Display Driver
**	Written by Franciska Schulze <fschulze at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <string.h>
#include <directfb.h>
#include <directfb_keynames.h>

#include <tek/debug.h>
#include <tek/exec.h>
#include <tek/teklib.h>
#include <tek/lib/utf8.h>

#include <tek/proto/exec.h>
#include <tek/mod/visual.h>

/*****************************************************************************/

#define DFBDISPLAY_VERSION		1
#define DFBDISPLAY_REVISION		0
#define DFBDISPLAY_NUMVECTORS	10

#define DEF_WINWIDTH			600
#define DEF_WINHEIGHT			400

#ifndef LOCAL
#define LOCAL
#endif

#ifndef EXPORT
#define EXPORT TMODAPI
#endif

/*****************************************************************************/

#ifndef DEF_CURSORFILE
#define DEF_CURSORFILE			TEKHOST_SYSDIR"cursors/cursor-green.png"
#endif

#ifndef DEF_FONTDIR
#define	DEF_FONTDIR			TEKHOST_SYSDIR"fonts/"
#endif

#define FNT_DEFNAME			"VeraMono"
#define FNT_DEFPXSIZE		14
#define	FNT_WILDCARD		"*"

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

struct FontMan
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
	IDirectFBFont *font;
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
};

/* internal structures */

struct fnt_node
{
	struct TNode node;
	TSTRPTR fname;
};

struct fnt_attr
{
	/* list of fontnames */
	struct TList fnlist;
	TSTRPTR fname;
	TINT  fpxsize;
	TBOOL fitalic;
	TBOOL fbold;
	TBOOL fscale;
	TINT  fnum;
};

/*****************************************************************************/

typedef struct
{
	/* Module header: */
	struct TModule dfb_Module;
	/* Exec module base ptr: */
	struct TExecBase *dfb_ExecBase;
	/* Locking for module base structure: */
	struct TLock *dfb_Lock;
	/* Number of module opens: */
	TUINT dfb_RefCount;
	/* Task: */
	struct TTask *dfb_Task;
	/* Command message port: */
	struct TMsgPort *dfb_CmdPort;
	/* Command message port signal: */
	TUINT dfb_CmdPortSignal;

	/* DirectFB super interface: */
	IDirectFB *dfb_DFB;
	/* DirectFB primary Layer */
	IDirectFBDisplayLayer *dfb_Layer;
	/* input interfaces: device and its buffer: */
	IDirectFBEventBuffer *dfb_Events;
	/* DirectFB root window, used capture background events */
	IDirectFBWindow *dfb_RootWindow;

	DFBDisplayLayerConfig dfb_LayerConfig;
	DFBGraphicsDeviceDescription dfb_DevDsc;

	/* cursor image */
	IDirectFBSurface *dfb_CursorSurface;

	TINT dfb_ScrWidth;
	TINT dfb_ScrHeight;
	TINT dfb_MouseX;
	TINT dfb_MouseY;
	TINT dfb_KeyQual;

	TAPTR dfb_Focused;
	TAPTR dfb_Active;

	/* Input file descriptor: */
	int dfb_FDInput;

	/* Self-pipe: */
	int dfb_FDSigPipeRead;
	int dfb_FDSigPipeWrite;
	int dfb_FDMax;

	/* pooled input messages: */
	struct TList dfb_imsgpool;
	/* list of all visuals: */
	struct TList dfb_vlist;
	/* Module global memory manager (thread safe): */
	struct TMemManager *dfb_MemMgr;

	struct FontMan dfb_fm;

} DFBDISPLAY;

typedef struct
{
	struct TNode node;

	TINT winwidth, winheight;
	TINT winleft, wintop;
	TINT wminwidth, wminheight;
	TINT wmaxwidth, wmaxheight;
	TBOOL borderless;
	TSTRPTR title;

	TUINT base_mask;
	TUINT eventmask;

	TVPEN bgpen, fgpen;

	struct TList imsgqueue;
	TAPTR imsgport;

	/* list of allocated pens: */
	struct TList penlist;

	IDirectFBWindow *window;
	IDirectFBSurface *winsurface;
	DFBWindowID winid;

	/* current active font */
	TAPTR curfont;

	/* userdata attached to this window, also propagated in messages: */
	TTAG userdata;

} DFBWINDOW;

struct DFBPen
{
	struct TNode node;
	TUINT8 r;
	TUINT8 g;
	TUINT8 b;
	TUINT8 a;
};

struct attrdata
{
	DFBDISPLAY *mod;
	DFBWINDOW *v;
	TAPTR font;
	TINT num;
	TBOOL sizechanged;
};

/*****************************************************************************/

LOCAL TBOOL dfb_init(DFBDISPLAY *mod, TTAGITEM *tags);
LOCAL void dfb_exit(DFBDISPLAY *mod);
LOCAL void dfb_sendimessages(DFBDISPLAY *mod, TBOOL do_interval);
LOCAL void dfb_wake(DFBDISPLAY *inst);
LOCAL void dfb_openvisual(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_closevisual(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_setinput(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_allocpen(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_freepen(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_frect(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_rect(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_line(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_plot(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_drawstrip(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_clear(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_getattrs(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_setattrs(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_drawtext(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_openfont(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_getfontattrs(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_textsize(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_setfont(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_closefont(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_queryfonts(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_getnextfont(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_drawtags(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_drawfan(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_drawarc(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_copyarea(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_setcliprect(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_drawfarc(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_unsetcliprect(DFBDISPLAY *mod, struct TVRequest *req);
LOCAL void dfb_drawbuffer(DFBDISPLAY *mod, struct TVRequest *req);

LOCAL TAPTR dfb_hostopenfont(DFBDISPLAY *mod, TTAGITEM *tags);
LOCAL void dfb_hostclosefont(DFBDISPLAY *mod, TAPTR font);
LOCAL void dfb_hostsetfont(DFBDISPLAY *mod, DFBWINDOW *v, TAPTR font);
LOCAL TTAGITEM *dfb_hostgetnextfont(DFBDISPLAY *mod, TAPTR fqhandle);
LOCAL TINT dfb_hosttextsize(DFBDISPLAY *mod, TAPTR font, TSTRPTR text, size_t len);
LOCAL THOOKENTRY TTAG dfb_hostgetfattrfunc(struct THook *hook, TAPTR obj, TTAG msg);
LOCAL TAPTR dfb_hostqueryfonts(DFBDISPLAY *mod, TTAGITEM *tags);

#endif /* _TEK_DISPLAY_DIRECTFB_MOD_H */
