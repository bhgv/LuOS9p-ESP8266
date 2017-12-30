
/*
**	teklib/src/display_x11/display_x11_all.c - X11 Display driver
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <string.h>
#include <unistd.h>
#include <tek/inline/exec.h>
#include "display_x11_mod.h"

static void x11_wake(struct X11Display *inst)
{
	char sig = 0;

	if (write(inst->x11_fd_sigpipe_write, &sig, 1) != 1)
		TDBPRINTF(TDB_ERROR, ("could not send wakeup signal\n"));
}

static TMODAPI struct TVRequest *x11_allocreq(struct X11Display *mod)
{
	TAPTR TExecBase = TGetExecBase(mod);
	struct TVRequest *req = TAllocMsg(sizeof(struct TVRequest));

	if (req)
		req->tvr_Req.io_Device = (struct TModule *) mod;
	return req;
}

static TMODAPI void x11_freereq(struct X11Display *mod, struct TVRequest *req)
{
	TAPTR TExecBase = TGetExecBase(mod);

	TFree(req);
}

static TMODAPI void x11_beginio(struct X11Display *mod, struct TVRequest *req)
{
	TAPTR TExecBase = TGetExecBase(mod);

	TPutMsg(mod->x11_CmdPort, req->tvr_Req.io_ReplyPort, req);
	x11_wake(mod);
}

static TMODAPI TINT x11_abortio(struct X11Display *mod, struct TVRequest *req)
{
	/* not supported: */
	return -1;
}

static const TMFPTR x11_vectors[X11DISPLAY_NUMVECTORS] = {
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) x11_beginio,
	(TMFPTR) x11_abortio,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,

	(TMFPTR) x11_allocreq,
	(TMFPTR) x11_freereq,
};

static void x11_destroy(struct X11Display *mod)
{
	TDestroy(mod->x11_Lock);
}

#define X11_IS_BIG_ENDIAN (*(TUINT16 *)"\0\xff" < 0x100)

static TBOOL x11_getprops(struct X11Display *inst)
{
	int major, minor, pixmap;
	int order = X11_IS_BIG_ENDIAN ? MSBFirst : LSBFirst;
	XWindowAttributes rootwa;

	inst->x11_ByteOrder = order;
	if (ImageByteOrder(inst->x11_Display) != order)
		inst->x11_Flags |= X11FL_SWAPBYTEORDER;
#if defined(ENABLE_XSHM)
	if (XShmQueryVersion(inst->x11_Display, &major, &minor, 
		&pixmap) == True && major > 0)
		inst->x11_Flags |= X11FL_SHMAVAIL;
	if (inst->x11_Flags & X11FL_SHMAVAIL)
		inst->x11_ShmEvent = XShmGetEventBase(inst->x11_Display) +
			ShmCompletion;
#endif
	inst->x11_DefaultDepth = DefaultDepth(inst->x11_Display, inst->x11_Screen);
	switch (inst->x11_DefaultDepth)
	{
		case 24:
		case 32:
			inst->x11_DefaultBPP = 4;
			break;
		case 15:
		case 16:
		default:
			inst->x11_DefaultBPP = 2;
			break;
	}
	TDBPRINTF(TDB_INFO, ("default depth: %d - bpp: %d\n",
			inst->x11_DefaultDepth, inst->x11_DefaultBPP));

	XGetWindowAttributes(inst->x11_Display,
		DefaultRootWindow(inst->x11_Display), &rootwa);
	inst->x11_ScreenWidth = WidthOfScreen(rootwa.screen);
	inst->x11_ScreenHeight = HeightOfScreen(rootwa.screen);

	return TTRUE;
}

static void x11_createnullcursor(struct X11Display *mod)
{
	Pixmap cursormask;
	XGCValues xgc;
	GC gc;
	XColor dummycolour;
	Display *display = mod->x11_Display;
	Window root = XRootWindow(mod->x11_Display, mod->x11_Screen);

	cursormask = XCreatePixmap(display, root, 1, 1, 1);
	xgc.function = GXclear;
	gc = XCreateGC(display, cursormask, GCFunction, &xgc);
	XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
	memset(&dummycolour, 0, sizeof(XColor));
	dummycolour.flags = 7;
	mod->x11_NullCursor = XCreatePixmapCursor(display, cursormask, cursormask,
		&dummycolour, &dummycolour, 0, 0);
#if defined(ENABLE_DEFAULTCURSOR)
	mod->x11_DefaultCursor = XCreateFontCursor(display, XC_left_ptr);
#endif
	XFreePixmap(display, cursormask);
	XFreeGC(display, gc);
}

static THOOKENTRY TTAG x11_ireplyhookfunc(struct THook *hook, TAPTR obj,
	TTAG msg)
{
	struct X11Display *mod = hook->thk_Data;

	x11_wake(mod);
	return 0;
}

static void x11_exitinstance(struct X11Display *inst)
{
	TAPTR TExecBase = TGetExecBase(inst);
	struct TNode *imsg, *node, *next;

	/* free pooled input messages: */
	while ((imsg = TRemHead(&inst->x11_imsgpool)))
		TFree(imsg);

	/* free queued input messages in all open visuals: */
	node = inst->x11_vlist.tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
	{
		struct X11Window *v = (struct X11Window *) node;

		/* unset active font in all open visuals */
		v->curfont = TNULL;

		while ((imsg = TRemHead(&v->imsgqueue)))
			TFree(imsg);
	}

	/* force closing of default font */
	inst->x11_fm.defref = 0;

	/* close all fonts */
	node = inst->x11_fm.openfonts.tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
		x11_hostclosefont(inst, (TAPTR) node);

	if (inst->x11_HugeRegion)
		XDestroyRegion(inst->x11_HugeRegion);

	if (inst->x11_fd_sigpipe_read != -1)
	{
		close(inst->x11_fd_sigpipe_read);
		close(inst->x11_fd_sigpipe_write);
	}

#if !defined(ENABLE_XFT) || !defined(ENABLE_LAZY_SINGLETON)
	/* Xft+fontconfig have problems with reinitialization */
	if (inst->x11_Display)
		XCloseDisplay(inst->x11_Display);
	x11_exitlibxft(inst);
#endif
	
	TDestroy(inst->x11_IReplyPort);	
}

static TBOOL x11_initinstance(struct TTask *task)
{
	TAPTR TExecBase = TGetExecBase(task);
	struct X11Display *inst = TGetTaskData(task);

	for (;;)
	{
		TTAGITEM ftags[3];
		int pipefd[2];
		XRectangle rectangle;
		TTAGITEM tags[2];

		TInitHook(&inst->x11_IReplyHook, x11_ireplyhookfunc, inst);
		tags[0].tti_Tag = TMsgPort_Hook;
		tags[0].tti_Value = (TTAG) &inst->x11_IReplyHook;
		tags[1].tti_Tag = TTAG_DONE;
		inst->x11_IReplyPort = TCreatePort(tags);
		if (inst->x11_IReplyPort == TNULL)
			break;

		/* list of free input messages: */
		TInitList(&inst->x11_imsgpool);

		/* list of all open visuals: */
		TInitList(&inst->x11_vlist);

		/* init fontmanager and default font */
		TInitList(&inst->x11_fm.openfonts);

		inst->x11_fd_sigpipe_read = -1;
		inst->x11_fd_sigpipe_write = -1;

		inst->x11_Display = XOpenDisplay(NULL);
		if (inst->x11_Display == TNULL)
			break;

		inst->x11_XA_TARGETS =
			XInternAtom(inst->x11_Display, "TARGETS", False);
		inst->x11_XA_PRIMARY =
			XInternAtom(inst->x11_Display, "PRIMARY", False);
		inst->x11_XA_CLIPBOARD =
			XInternAtom(inst->x11_Display, "CLIPBOARD", False);
		inst->x11_XA_UTF8_STRING =
			XInternAtom(inst->x11_Display, "UTF8_STRING", False);
		inst->x11_XA_STRING = XInternAtom(inst->x11_Display, "STRING", False);
		inst->x11_XA_COMPOUND_TEXT =
			XInternAtom(inst->x11_Display, "COMPOUND_TEXT", False);

		XkbSetDetectableAutoRepeat(inst->x11_Display, TTRUE, TNULL);

		inst->x11_fd_display = ConnectionNumber(inst->x11_Display);
		inst->x11_Screen = DefaultScreen(inst->x11_Display);
		inst->x11_Visual = DefaultVisual(inst->x11_Display, inst->x11_Screen);

		if (x11_getprops(inst) == TFALSE)
			break;

		if (pipe(pipefd) != 0)
			break;
		inst->x11_fd_sigpipe_read = pipefd[0];
		inst->x11_fd_sigpipe_write = pipefd[1];
		inst->x11_fd_max =
			TMAX(inst->x11_fd_sigpipe_read, inst->x11_fd_display) + 1;

		/* needed for unsetcliprect: */
		inst->x11_HugeRegion = XCreateRegion();
		rectangle.x = 0;
		rectangle.y = 0;
		rectangle.width = (unsigned short) 0xffff;
		rectangle.height = (unsigned short) 0xffff;
		XUnionRectWithRegion(&rectangle, inst->x11_HugeRegion,
			inst->x11_HugeRegion);

		x11_initlibxft(inst);

		ftags[0].tti_Tag = TVisual_FontName;
		ftags[0].tti_Value = (TTAG) X11FNT_DEFNAME;
		ftags[1].tti_Tag = TVisual_FontPxSize;
		ftags[1].tti_Value = (TTAG) X11FNT_DEFPXSIZE;
		ftags[2].tti_Tag = TTAG_DONE;

		inst->x11_fm.deffont = x11_hostopenfont(inst, ftags);
		/* if (inst->x11_fm.deffont == TNULL) break; */

		x11_createnullcursor(inst);

		inst->x11_IMsgPort = (struct TMsgPort *) TGetTag(inst->x11_InitTags,
			TVisual_IMsgPort, TNULL);

		TDBPRINTF(TDB_TRACE, ("instance init successful\n"));
		return TTRUE;
	}

	x11_exitinstance(inst);

	return TFALSE;
}

static void x11_exit(struct X11Display *mod)
{
	TAPTR TExecBase = TGetExecBase(mod);

	if (mod->x11_Task)
	{
		TSignal(mod->x11_Task, TTASK_SIG_ABORT);
		x11_wake(mod);
		TDestroy(mod->x11_Task);
	}
}

static TBOOL x11_init(struct X11Display *mod, TTAGITEM *tags)
{
	TAPTR TExecBase = TGetExecBase(mod);

	mod->x11_InitTags = tags;
	for (;;)
	{
		TTAGITEM tags[2];

		tags[0].tti_Tag = TTask_UserData;
		tags[0].tti_Value = (TTAG) mod;
		tags[1].tti_Tag = TTAG_DONE;
		mod->x11_Task =
			TCreateTask(&mod->x11_Module.tmd_Handle.thn_Hook, tags);
		if (mod->x11_Task == TNULL)
			break;

		mod->x11_CmdPort = TGetUserPort(mod->x11_Task);
		return TTRUE;
	}
	x11_exit(mod);
	return TFALSE;
}

static TAPTR x11_modopen(struct X11Display *mod, TTAGITEM *tags)
{
	TAPTR TExecBase = TGetExecBase(mod);
	TBOOL success = TTRUE;

	TLock(mod->x11_Lock);
	if (mod->x11_RefCount == 0)
		success = x11_init(mod, tags);
	if (success)
		mod->x11_RefCount++;
	TUnlock(mod->x11_Lock);
	if (success)
		return mod;
	return TNULL;
}

static void x11_modclose(struct X11Display *mod)
{
	TAPTR TExecBase = TGetExecBase(mod);

	TLock(mod->x11_Lock);
	if (--mod->x11_RefCount == 0)
		x11_exit(mod);
	TUnlock(mod->x11_Lock);
}

static THOOKENTRY TTAG x11_dispatch(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct X11Display *mod = (struct X11Display *) hook->thk_Data;

	switch (msg)
	{
		case TMSG_DESTROY:
			x11_destroy(mod);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) x11_modopen(mod, obj);
		case TMSG_CLOSEMODULE:
			x11_modclose(obj);
			break;
		case TMSG_INITTASK:
			return x11_initinstance(obj);
		case TMSG_RUNTASK:
			x11_taskfunc(obj);
	}
	return 0;
}

TMODENTRY TUINT tek_init_display_x11(struct TTask *task, struct TModule *vis,
	TUINT16 version, TTAGITEM *tags)
{
	struct X11Display *mod = (struct X11Display *) vis;

	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * X11DISPLAY_NUMVECTORS;

		if (version <= X11DISPLAY_VERSION)
			return sizeof(struct X11Display);

		return 0;
	}

	for (;;)
	{
		TAPTR TExecBase = TGetExecBase(task);

		mod->x11_Lock = TCreateLock(TNULL);
		if (mod->x11_Lock == TNULL)
			break;

		mod->x11_Module.tmd_Version = X11DISPLAY_VERSION;
		mod->x11_Module.tmd_Revision = X11DISPLAY_REVISION;
		mod->x11_Module.tmd_Handle.thn_Hook.thk_Entry = x11_dispatch;
		mod->x11_Module.tmd_Flags = TMODF_VECTORTABLE | TMODF_OPENCLOSE;
		TInitVectors(&mod->x11_Module, x11_vectors, X11DISPLAY_NUMVECTORS);
		return TTRUE;
	}

	x11_destroy(mod);
	return TFALSE;
}

LOCAL TSTRPTR x11_utf8tolatin(struct X11Display *mod, TSTRPTR utf8string,
	TINT utf8len, TINT *bytelen)
{
	TUINT8 *latin = mod->x11_utf8buffer;
	size_t len = utf8tolatin((const unsigned char *) utf8string, utf8len,
		latin, X11_UTF8_BUFSIZE, 0xbf);

	if (bytelen)
		*bytelen = len;
	return (TSTRPTR) latin;
}
