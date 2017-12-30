
/*
**	display_win_mod.c - Windows display driver
**	Written by Timm S. Mueller <tmueller@schulze-mueller.de>
**	contributions by Tobias Schwinger <tschwinger@isonews2.com>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>
#include <tek/mod/exec.h>
#include <tek/mod/hal.h>
#include <tek/mod/winnt/hal.h>
#include "../hal/hal_mod.h"

#include "display_win_mod.h"

static void fb_runinstance(TAPTR task);
static TBOOL fb_initinstance(TAPTR task);
static void fb_exitinstance(WINDISPLAY *inst);
static TAPTR fb_modopen(WINDISPLAY *mod, TTAGITEM *tags);
static void fb_modclose(WINDISPLAY *mod);
static TMODAPI void fb_beginio(WINDISPLAY *mod, struct TVRequest *req);
static TMODAPI TINT fb_abortio(WINDISPLAY *mod, struct TVRequest *req);
static TMODAPI struct TVRequest *fb_allocreq(WINDISPLAY *mod);
static TMODAPI void fb_freereq(WINDISPLAY *mod, struct TVRequest *req);
static void fb_docmd(WINDISPLAY *mod, struct TVRequest *req);
static void fb_forwardreq(WINDISPLAY *mod, struct TVRequest *req);
static void fb_dointaskcmd(WINDISPLAY *mod, struct TVRequest *req);
static void fb_intaskflush(WINDISPLAY *mod);
LOCAL void fb_sendinterval(WINDISPLAY *mod);
static VOID CALLBACK
dev_timerproc(HWND hDevWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
static LRESULT CALLBACK
win_wndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static THOOKENTRY TTAG
fb_intask(struct THook *hook, TAPTR obj, TTAG msg);

static const TMFPTR
fb_vectors[FB_DISPLAY_NUMVECTORS] =
{
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) fb_beginio,
	(TMFPTR) fb_abortio,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) fb_allocreq,
	(TMFPTR) fb_freereq,
};

static void
fb_destroy(WINDISPLAY *mod)
{
	TDBPRINTF(TDB_TRACE,("Module destroy...\n"));
	TDestroy((struct THandle *) mod->fbd_Lock);
}

static THOOKENTRY TTAG
fb_dispatch(struct THook *hook, TAPTR obj, TTAG msg)
{
	WINDISPLAY *mod = (WINDISPLAY *) hook->thk_Data;
	switch (msg)
	{
		case TMSG_DESTROY:
			fb_destroy(mod);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) fb_modopen(mod, obj);
		case TMSG_CLOSEMODULE:
			fb_modclose(obj);
			break;
		case TMSG_INITTASK:
			return fb_initinstance(obj);
		case TMSG_RUNTASK:
			fb_runinstance(obj);
			break;
	}
	return 0;
}

TMODENTRY TUINT
tek_init_display_windows(struct TTask *task, struct TModule *vis, TUINT16 version,
	TTAGITEM *tags)
{
	WINDISPLAY *mod = (WINDISPLAY *) vis;

	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * FB_DISPLAY_NUMVECTORS;

		if (version <= FB_DISPLAY_VERSION)
			return sizeof(WINDISPLAY);

		return 0;
	}

	TDBPRINTF(TDB_TRACE,("Module init...\n"));

	for (;;)
	{
		struct TExecBase *TExecBase = TGetExecBase(mod);
		mod->fbd_ExecBase = TExecBase;
		mod->fbd_Lock = TCreateLock(TNULL);
		if (mod->fbd_Lock == TNULL) break;

		mod->fbd_Module.tmd_Version = FB_DISPLAY_VERSION;
		mod->fbd_Module.tmd_Revision = FB_DISPLAY_REVISION;
		mod->fbd_Module.tmd_Handle.thn_Hook.thk_Entry = fb_dispatch;
		mod->fbd_Module.tmd_Flags = TMODF_VECTORTABLE | TMODF_OPENCLOSE;
		TInitVectors(&mod->fbd_Module, fb_vectors, FB_DISPLAY_NUMVECTORS);
		return TTRUE;
	}
	fb_destroy(mod);

	return TFALSE;
}

/*****************************************************************************/
/*
**	Module open/close
*/

static TAPTR fb_modopen(WINDISPLAY *mod, TTAGITEM *tags)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	TBOOL success = TTRUE;
	TLock(mod->fbd_Lock);
	if (mod->fbd_RefCount == 0)
		success = fb_init(mod, tags);
	if (success)
		mod->fbd_RefCount++;
	TUnlock(mod->fbd_Lock);
	if (success)
		return mod;
	return TNULL;
}

static void
fb_modclose(WINDISPLAY *mod)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	TDBPRINTF(TDB_TRACE,("Device close\n"));
	TLock(mod->fbd_Lock);
	if (--mod->fbd_RefCount == 0)
		fb_exit(mod);
	TUnlock(mod->fbd_Lock);
}

/*****************************************************************************/
/*
**	BeginIO/AbortIO
*/

static TMODAPI void
fb_beginio(WINDISPLAY *mod, struct TVRequest *req)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	TPutMsg(mod->fbd_CmdPort, req->tvr_Req.io_ReplyPort, req);
}

static TMODAPI TINT
fb_abortio(WINDISPLAY *mod, struct TVRequest *req)
{
	return -1;
}

/*****************************************************************************/
/*
**	AllocReq/FreeReq
*/

static TMODAPI struct TVRequest *
fb_allocreq(WINDISPLAY *mod)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	struct TVRequest *req = TAllocMsg(sizeof(struct TVRequest));
	if (req)
		req->tvr_Req.io_Device = (struct TModule *) mod;
	return req;
}

static TMODAPI void
fb_freereq(WINDISPLAY *mod, struct TVRequest *req)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	TFree(req);
}

/*****************************************************************************/
/*
**	Module init/exit
*/

LOCAL TBOOL
fb_init(WINDISPLAY *mod, TTAGITEM *tags)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);

	for (;;)
	{
		TTAGITEM tags[2];

		tags[0].tti_Tag = TTask_UserData;
		tags[0].tti_Value = (TTAG) mod;
		tags[1].tti_Tag = TTAG_DONE;

		mod->fbd_Task = TCreateTask(&mod->fbd_Module.tmd_Handle.thn_Hook,
			tags);
		if (mod->fbd_Task == TNULL) break;

		return TTRUE;
	}

	fb_exit(mod);
	return TFALSE;
}

LOCAL void
fb_exit(WINDISPLAY *mod)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	if (mod->fbd_Task)
	{
		TSignal(mod->fbd_Task, TTASK_SIG_ABORT);
		TDestroy((struct THandle *) mod->fbd_Task);
	}
}

/*****************************************************************************/
/*
**	Device instance init/exit
*/

static TBOOL fb_initinstance(TAPTR task)
{
	struct TExecBase *TExecBase = TGetExecBase(task);
	WINDISPLAY *mod = TGetTaskData(task);

	for (;;)
	{
		struct THook ithook;
		TTAGITEM ftags[3];
		WNDCLASSEX wclass, pclass;
		RECT rect;

		mod->fbd_InputTask = NULL;

		mod->fbd_HInst = GetModuleHandle(NULL);
		if (mod->fbd_HInst == TNULL)
			break;

		wclass.cbSize = sizeof(wclass);
		wclass.style = CS_GLOBALCLASS;
		wclass.lpfnWndProc = win_wndproc;
		wclass.cbClsExtra = 0;
		wclass.cbWndExtra = 0;
		wclass.hInstance = mod->fbd_HInst;
		wclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wclass.hbrBackground = NULL;
		wclass.lpszMenuName = NULL;
		wclass.lpszClassName = FB_DISPLAY_CLASSNAME;
		wclass.hIconSm = NULL;
		mod->fbd_ClassAtom = RegisterClassEx(&wclass);
		if (mod->fbd_ClassAtom == 0)
			break;

		pclass.cbSize = sizeof(pclass);
		pclass.style = CS_GLOBALCLASS | CS_NOCLOSE;
		pclass.lpfnWndProc = win_wndproc;
		pclass.cbClsExtra = 0;
		pclass.cbWndExtra = 0;
		pclass.hInstance = mod->fbd_HInst;
		pclass.hIcon = NULL;
		pclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		pclass.hbrBackground = NULL;
		pclass.lpszMenuName = NULL;
		pclass.lpszClassName = FB_DISPLAY_CLASSNAME_POPUP;
		pclass.hIconSm = NULL;
		mod->fbd_ClassAtomPopup = RegisterClassEx(&pclass);
		if (mod->fbd_ClassAtomPopup == 0)
			break;

		mod->fbd_WindowFocussedApp = NULL;
		mod->fbd_WindowUnderCursor = NULL;
		mod->fbd_WindowActivePopup = NULL;
		mod->fbd_NumInterval = 0;

		/* Create invisible window for this device: */
		mod->fbd_DeviceHWnd = CreateWindowEx(0, FB_DISPLAY_CLASSNAME, NULL,
			0, 0, 0, 0, 0, (HWND) NULL, (HMENU) NULL, mod->fbd_HInst,
			(LPVOID) NULL);
		if (mod->fbd_DeviceHWnd == NULL)
			break;

		mod->fbd_DeviceHDC = GetDC(mod->fbd_DeviceHWnd);

		/* list of all open visuals: */
		TInitList(&mod->fbd_VisualList);

		/* init fontmanager and default font */
		TInitList(&mod->fbd_FontManager.openfonts);

		ftags[0].tti_Tag = TVisual_FontName;
		ftags[0].tti_Value = (TTAG) FNT_DEFNAME;
		ftags[1].tti_Tag = TVisual_FontPxSize;
		ftags[1].tti_Value = (TTAG) FNT_DEFPXSIZE;
		ftags[2].tti_Tag = TTAG_DONE;
		mod->fbd_FontManager.deffont = fb_hostopenfont(mod, ftags);
		/*
		if (mod->fbd_FontManager.deffont == TNULL) 
			break;
		*/
		GetWindowRect(GetDesktopWindow(), &rect);
		mod->fbd_ScreenWidth = rect.right;
		mod->fbd_ScreenHeight = rect.bottom;

		TInitHook(&ithook, &fb_intask, mod);
		mod->fbd_InputTask = TCreateTask(&ithook, NULL);
		if (! mod->fbd_InputTask)
			break;

		mod->fbd_CmdPort = TGetUserPort(mod->fbd_Task);
		mod->fbd_CmdPortSignal = TGetPortSignal(mod->fbd_CmdPort);

		TDBPRINTF(TDB_TRACE,("Instance init successful\n"));
		return TTRUE;
	}

	fb_exitinstance(mod);
	return TFALSE;
}

static void
fb_exitinstance(WINDISPLAY *mod)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	struct TNode *imsg, *node, *next;

	/* signal and wait for termination of input task: */
	if (mod->fbd_InputTask != NULL)
	{
		TSignal(mod->fbd_InputTask, TTASK_SIG_ABORT);
		TDestroy((struct THandle *) mod->fbd_InputTask);
	}

	/* free queued input messages in all open visuals: */
	node = mod->fbd_VisualList.tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
	{
		WINWINDOW *v = (WINWINDOW *) node;

		/* unset active font in all open visuals */
		v->fbv_CurrentFont = TNULL;

		while ((imsg = TRemHead(&v->fbv_IMsgQueue)))
			TFree(imsg);
	}

	/* force closing of default font */
	mod->fbd_FontManager.defref = 0;

	/* close all fonts */
	node = mod->fbd_FontManager.openfonts.tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
		fb_hostclosefont(mod, (TAPTR) node);

	if (mod->fbd_DeviceHWnd)
		DestroyWindow(mod->fbd_DeviceHWnd);

	if (mod->fbd_ClassAtom)
		UnregisterClass(FB_DISPLAY_CLASSNAME, mod->fbd_HInst);

	if (mod->fbd_ClassAtomPopup)
		UnregisterClass(FB_DISPLAY_CLASSNAME_POPUP, mod->fbd_HInst);
}

static void fb_runinstance(TAPTR task)
{
	struct TExecBase *TExecBase = TGetExecBase(task);
	WINDISPLAY *mod = TGetTaskData(task);

	struct TVRequest *req;
	TUINT sig;

	TDBPRINTF(TDB_INFO,("Device instance running\n"));

	/* interval time: 1/50s: */
	TTIME intt = { 20000 };
	/* next absolute time to send interval message: */
	TTIME nextt;
	TTIME waitt, nowt;
	TGetSystemTime(&nextt);
	TAddTime(&nextt, &intt);

	do
	{
		TBOOL reqServiced = TFALSE;
		TTIME *timeout = TNULL;

		/* process commands */
		while ((req = TGetMsg(mod->fbd_CmdPort)))
		{
			fb_docmd(mod, req);
			TReplyMsg(req);

			reqServiced = TTRUE;

			/* promote bulk processing */
			#ifdef YieldProcessor
			YieldProcessor();
			#endif
			Sleep(0);
		}

		/* check if time interval has expired: */
		TGetSystemTime(&nowt);
		if (TCmpTime(&nowt, &nextt) > 0)
		{
			/* expired; schedule next interval message: */
			TAddTime(&nextt, &intt);
			if (TCmpTime(&nowt, &nextt) >= 0)
			{
				/* nexttime expired already; create new time from now: */
				nextt = nowt;
				TAddTime(&nextt, &intt);
			}
			/* send interval messages to subscribers: */
			if (mod->fbd_NumInterval > 0)
				fb_sendinterval(mod);
		}

		/* calculate timeout when subscribers for interval messages: */
		if (mod->fbd_NumInterval > 0)
		{
			waitt = nextt;
			TSubTime(&waitt, &nowt);
			timeout = &waitt;
		}

		/* wake up input thread (to process invalidation?): */
		if (reqServiced)
			TSignal(mod->fbd_InputTask, TTASK_SIG_USER);

		/* sleep until incoming messages or timeout: */
		sig = TWaitTime(timeout, mod->fbd_CmdPortSignal | TTASK_SIG_ABORT);

	} while (!(sig & TTASK_SIG_ABORT));

	TDBPRINTF(TDB_INFO,("Device instance closedown\n"));
	fb_exitinstance(mod);
}

/*****************************************************************************/

static THOOKENTRY TTAG
fb_intask(struct THook *hook, TAPTR obj, TTAG msg)
{
	WINDISPLAY *mod = (WINDISPLAY *) hook->thk_Data;
	struct TExecBase *TExecBase = TGetExecBase(mod);
	struct THALBase *hal = TGetHALBase();
	struct HALSpecific *hws = hal->hmb_Specific;
	struct HALThread *wth = TlsGetValue(hws->hsp_TLSIndex);
	struct TMsgPort* cmdPort;
	TUINT cmdPortSignal;
	TUINT sig = 0;

	if (msg != TMSG_RUNTASK)
		return TTRUE;

	cmdPort = TGetUserPort(TNULL);
	cmdPortSignal = TGetPortSignal(cmdPort);

	do
	{
		MSG msg;

		/* queue input messages: */
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE | PM_NOYIELD))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		fb_intaskflush(mod);

		/* consume and eventually process pending signals: */
		sig = TSetSignal(0, cmdPortSignal | TTASK_SIG_ABORT);
		if (!(sig & (cmdPortSignal | TTASK_SIG_ABORT)))
		{
			/* wait for the arrival of messages or signals: */
			MsgWaitForMultipleObjects(1, & wth->hth_SigEvent, FALSE,
									  INFINITE, QS_ALLEVENTS);

			sig = TSetSignal(0, TTASK_SIG_ABORT);
		}

	} while (!(sig & TTASK_SIG_ABORT));

	return 0;
}

static void fb_intaskflush(WINDISPLAY *mod)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	struct TMsgPort* cmdPort = TGetUserPort(TNULL);
	struct TVRequest *req;

	/* flush queue: */
	fb_sendimessages(mod);

	/* process certain commands in this thread: */
	while ((req = TGetMsg(cmdPort)))
	{
		fb_dointaskcmd(mod, req);
		TReplyMsg(req);
	}
}

/*****************************************************************************/

static void fb_docmd(WINDISPLAY *mod, struct TVRequest *req)
{
	/*TDBPRINTF(20,("Command: %08x\n", req->tvr_Req.io_Command));*/
	switch (req->tvr_Req.io_Command)
	{
		case TVCMD_OPENWINDOW:
		case TVCMD_CLOSEWINDOW:
		case TVCMD_SETINPUT:
		case TVCMD_SETATTRS:
			fb_forwardreq(mod, req);
			break;
		case TVCMD_OPENFONT:
			fb_openfont(mod, req);
			break;
		case TVCMD_CLOSEFONT:
			fb_closefont(mod, req);
			break;
		case TVCMD_GETFONTATTRS:
			fb_getfontattrs(mod, req);
			break;
		case TVCMD_TEXTSIZE:
			fb_textsize(mod, req);
			break;
		case TVCMD_QUERYFONTS:
			fb_queryfonts(mod, req);
			break;
		case TVCMD_GETNEXTFONT:
			fb_getnextfont(mod, req);
			break;
		case TVCMD_GETATTRS:
			fb_getattrs(mod, req);
			break;
		case TVCMD_ALLOCPEN:
			fb_allocpen(mod, req);
			break;
		case TVCMD_FREEPEN:
			fb_freepen(mod, req);
			break;
		case TVCMD_SETFONT:
			fb_setfont(mod, req);
			break;
		case TVCMD_CLEAR:
			fb_clear(mod, req);
			break;
		case TVCMD_RECT:
			fb_rect(mod, req);
			break;
		case TVCMD_FRECT:
			fb_frect(mod, req);
			break;
		case TVCMD_LINE:
			fb_line(mod, req);
			break;
		case TVCMD_PLOT:
			fb_plot(mod, req);
			break;
		case TVCMD_TEXT:
			fb_drawtext(mod, req);
			break;
		case TVCMD_DRAWSTRIP:
			fb_drawstrip(mod, req);
			break;
		case TVCMD_DRAWTAGS:
			fb_drawtags(mod, req);
			break;
		case TVCMD_DRAWFAN:
			fb_drawfan(mod, req);
			break;
		case TVCMD_COPYAREA:
			fb_copyarea(mod, req);
			break;
		case TVCMD_SETCLIPRECT:
			fb_setcliprect(mod, req);
			break;
		case TVCMD_UNSETCLIPRECT:
			fb_unsetcliprect(mod, req);
			break;
		case TVCMD_DRAWBUFFER:
			fb_drawbuffer(mod, req);
			break;
		case TVCMD_FLUSH:
			GdiFlush();
			break;
		case TVCMD_GETSELECTION:
			fb_getselection(mod, req);
			break;
		case TVCMD_SETSELECTION:
			fb_setselection(mod, req);
			break;
		default:
			TDBPRINTF(TDB_INFO,("Unknown command code: %08x\n",
			req->tvr_Req.io_Command));
	}
}

static void fb_forwardreq(WINDISPLAY *mod, struct TVRequest *req)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	struct TMsgPort* port = TGetUserPort(mod->fbd_InputTask);
	struct TVRequest *fwd = TAllocMsg(sizeof(struct TVRequest));
	TCopyMem(req, fwd, sizeof(struct TVRequest));
	TSendMsg(port, fwd);
	switch (req->tvr_Req.io_Command)
	{
		case TVCMD_OPENWINDOW:
			req->tvr_Op.OpenWindow.Window = fwd->tvr_Op.OpenWindow.Window;
			break;
		case TVCMD_CLOSEWINDOW:
			break;
		case TVCMD_SETINPUT:
			req->tvr_Op.SetInput.OldMask = fwd->tvr_Op.SetInput.OldMask;
			break;
		case TVCMD_SETATTRS:
			req->tvr_Op.SetAttrs.Num = fwd->tvr_Op.SetAttrs.Num;
			break;
	}
	TFree(fwd);
}

static void fb_dointaskcmd(WINDISPLAY *mod, struct TVRequest *req)
{
	switch (req->tvr_Req.io_Command)
	{
		case TVCMD_OPENWINDOW:
			fb_openwindow(mod, req);
			break;
		case TVCMD_CLOSEWINDOW:
			fb_closewindow(mod, req);
			break;
		case TVCMD_SETINPUT:
			fb_setinput(mod, req);
			break;
		case TVCMD_SETATTRS:
			fb_setattrs(mod, req);
			break;
	}
}

/*****************************************************************************/

LOCAL TBOOL
fb_getimsg(WINDISPLAY *mod, WINWINDOW *win, TIMSG **msgptr, TUINT type)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	TIMSG *msg = TAllocMsg0(sizeof(TIMSG));
	if (!msg)
	{
		*msgptr = TNULL;
		return TFALSE;
	}
	msg->timsg_Instance = win;
	msg->timsg_UserData = win->fbv_UserData;
	msg->timsg_Type = type;
	msg->timsg_Qualifier = mod->fbd_KeyQual;
	msg->timsg_MouseX = win->fbv_MouseX;
	msg->timsg_MouseY = win->fbv_MouseY;
	TGetSystemTime(&msg->timsg_TimeStamp);
	*msgptr = msg;
	return  TTRUE;
}

static WINWINDOW *fb_getwindowptr(WINDISPLAY *mod, HWND hwnd)
{
	if (hwnd == NULL)
		return TNULL;
	if ((HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE) != mod->fbd_HInst)
		return TNULL;
	return (WINWINDOW *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

LOCAL WINWINDOW*
fb_getcrossimsg(WINDISPLAY *mod, HWND hwnd, TIMSG **msgptr, TUINT type)
{
	WINWINDOW *win = fb_getwindowptr(mod, hwnd);
	if (!win || !(win->fbv_InputMask & type)
			|| !fb_getimsg(mod, win, msgptr, type))
		return NULL;
	return win;
}

LOCAL void fb_sendimessages(WINDISPLAY *mod)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	struct TNode *next, *node = mod->fbd_VisualList.tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
	{
		WINWINDOW *v = (WINWINDOW *) node;
		TIMSG *imsg;
		while ((imsg = (TIMSG *) TRemHead(&v->fbv_IMsgQueue)))
			TPutMsg(v->fbv_IMsgPort, TNULL, imsg);
	}
}

LOCAL void fb_sendinterval(WINDISPLAY *mod)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	struct TNode *next, *node = mod->fbd_VisualList.tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
	{
		WINWINDOW *v = (WINWINDOW *) node;
		TIMSG *imsg;
		if ((v->fbv_InputMask & TITYPE_INTERVAL)
				&& fb_getimsg(mod, v, &imsg, TITYPE_INTERVAL))
			TPutMsg(v->fbv_IMsgPort, TNULL, imsg);
	}
}

/*****************************************************************************/

static TBOOL getqualifier(WINDISPLAY *mod)
{
	TUINT quali = TKEYQ_NONE;
	TBOOL newquali;
	BYTE *keystate = mod->fbd_KeyState;
	GetKeyboardState(keystate);
	if (keystate[VK_LSHIFT] & 0x80) quali |= TKEYQ_LSHIFT;
	if (keystate[VK_RSHIFT] & 0x80) quali |= TKEYQ_RSHIFT;
	if (keystate[VK_LCONTROL] & 0x80) quali |= TKEYQ_LCTRL;
	if (keystate[VK_RCONTROL] & 0x80) quali |= TKEYQ_RCTRL;
	if (keystate[VK_LMENU] & 0x80) quali |= TKEYQ_LALT;
	if (keystate[VK_RMENU] & 0x80) quali |= TKEYQ_RALT;
	if (keystate[VK_LWIN] & 0x80) quali |= TKEYQ_LPROP;
	if (keystate[VK_RWIN] & 0x80) quali |= TKEYQ_RPROP;
	/*if (keystate[VK_NUMLOCK] & 1) quali |= TKEYQ_NUMBLOCK;*/
	newquali = (mod->fbd_KeyQual != quali);
	mod->fbd_KeyQual = quali;
	return newquali;
}

static void processkey(WINDISPLAY *mod, WINWINDOW *win, TUINT type, 
	TINT code, TUINT code2)
{
	TIMSG *imsg;
	getqualifier(mod);

#if 0
	/* send key events to the active application window */
	if (mod->fbd_WindowFocussedApp != NULL)
		win = (WINWINDOW *) GetWindowLongPtr(mod->fbd_WindowFocussedApp, 
											 GWLP_USERDATA);
	TDBPRINTF(TDB_TRACE,("KEY EVENT -> %p\n", win));
#endif

	switch (code)
	{
		case VK_LEFT:
			code = TKEYC_CRSRLEFT;
			break;
		case VK_UP:
			code = TKEYC_CRSRUP;
			break;
		case VK_RIGHT:
			code = TKEYC_CRSRRIGHT;
			break;
		case VK_DOWN:
			code = TKEYC_CRSRDOWN;
			break;

		case VK_ESCAPE:
			code = TKEYC_ESC;
			break;
		case VK_DELETE:
			code = TKEYC_DEL;
			break;
		case VK_BACK:
			code = TKEYC_BCKSPC;
			break;
		case VK_TAB:
			code = TKEYC_TAB;
			break;
		case VK_RETURN:
			code = TKEYC_RETURN;
			break;

		case VK_HELP:
			code = TKEYC_HELP;
			break;
		case VK_INSERT:
			code = TKEYC_INSERT;
			break;
		case VK_PRIOR:
			code = TKEYC_PAGEUP;
			break;
		case VK_NEXT:
			code = TKEYC_PAGEDOWN;
			break;
		case VK_HOME:
			code = TKEYC_POSONE;
			break;
		case VK_END:
			code = TKEYC_POSEND;
			break;
		case VK_PRINT:
			code = TKEYC_PRINT;
			break;
		case VK_SCROLL:
			code = TKEYC_SCROLL;
			break;
		case VK_PAUSE:
			code = TKEYC_PAUSE;
			break;
		case VK_F1: case VK_F2: case VK_F3: case VK_F4:
		case VK_F5: case VK_F6: case VK_F7: case VK_F8:
		case VK_F9: case VK_F10: case VK_F11: case VK_F12:
			code = (TUINT) (code - VK_F1) + TKEYC_F1;
			break;
		
		case VK_NUMLOCK: /* numlock */
		case 233: /* altgr */
			code = 0;
			
		default:
		{
			WCHAR buff[2];
			TINT numchars = ToUnicode(code, 0, mod->fbd_KeyState, buff, 2, 0);
			TINT mapcode = MapVirtualKey(code, 2);
			TBOOL shift = mod->fbd_KeyQual & (TKEYQ_LSHIFT | TKEYQ_RSHIFT);
			TUINT qualis = mod->fbd_KeyQual & ~(TKEYQ_LSHIFT | TKEYQ_RSHIFT);
			if (qualis == 0x24)
			{
				code = buff[0];
				mod->fbd_KeyQual = 0;
			}
			else if (qualis != 0 && code == mapcode)
			{
				if (code >= 'A' && code <= 'Z' && !shift)
					code += 32;
			}
			else if (numchars > 0)
				code = buff[0];
		}
	}
	
	if ((win->fbv_InputMask & type) &&
		fb_getimsg(mod, win, &imsg, type))
	{
		ptrdiff_t len;
		imsg->timsg_Code = code;
		len = (ptrdiff_t)
			utf8encode(imsg->timsg_KeyCode, imsg->timsg_Code) -
			(ptrdiff_t) imsg->timsg_KeyCode;
		imsg->timsg_KeyCode[len] = 0;
		TAddTail(&win->fbv_IMsgQueue, &imsg->timsg_Node);
	}
}

LOCAL void win_getminmax(WINWINDOW *win, TINT *pm1, TINT *pm2, TINT *pm3,
	TINT *pm4, TBOOL windowsize)
{
	TINT m1 = win->fbv_MinWidth;
	TINT m2 = win->fbv_MinHeight;
	TINT m3 = win->fbv_MaxWidth;
	TINT m4 = win->fbv_MaxHeight;
	m1 = TMAX(0, m1);
	m2 = TMAX(0, m2);
	m3 = m3 < 0 ? 1000000 : m3;
	m4 = m4 < 0 ? 1000000 : m4;
	m3 = TMAX(m3, m1);
	m4 = TMAX(m4, m2);
	if (windowsize)
	{
		m1 += win->fbv_BorderWidth;
		m2 += win->fbv_BorderHeight;
		m3 += win->fbv_BorderWidth;
		m4 += win->fbv_BorderHeight;
	}
	*pm1 = m1;
	*pm2 = m2;
	*pm3 = m3;
	*pm4 = m4;
}

static LRESULT CALLBACK
win_wndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WINWINDOW *win = (WINWINDOW *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
	WINDISPLAY *mod = win ? win->fbv_Display : TNULL;
	if (mod)
	{
		TIMSG *imsg;
		switch (uMsg)
		{
			default:
				/*TDBPRINTF(TDB_TRACE,("uMsg: %08x\n", uMsg));*/
				break;

			case WM_CLOSE:
				if ((win->fbv_InputMask & TITYPE_CLOSE) &&
					(fb_getimsg(mod, win, &imsg, TITYPE_CLOSE)))
					TAddTail(&win->fbv_IMsgQueue, &imsg->timsg_Node);
				return 0;

			case WM_ERASEBKGND:
				return 0;

			case WM_GETMINMAXINFO:
			{
				LPMINMAXINFO mm = (LPMINMAXINFO) lParam;
				TINT m1, m2, m3, m4;
				EnterCriticalSection(&win->fbv_LockExtents);
				win_getminmax(win, &m1, &m2, &m3, &m4, TTRUE);
				LeaveCriticalSection(&win->fbv_LockExtents);
				mm->ptMinTrackSize.x = m1;
				mm->ptMinTrackSize.y = m2;
				mm->ptMaxTrackSize.x = m3;
				mm->ptMaxTrackSize.y = m4;
				return 0;
			}

			case WM_PAINT:
			{
				PAINTSTRUCT ps;
				if (BeginPaint(win->fbv_HWnd, &ps))
				{
					if ((win->fbv_InputMask & TITYPE_REFRESH) &&
						(fb_getimsg(mod, win, &imsg, TITYPE_REFRESH)))
					{
						imsg->timsg_X = ps.rcPaint.left;
						imsg->timsg_Y = ps.rcPaint.top;
						imsg->timsg_Width = ps.rcPaint.right - ps.rcPaint.left;
						imsg->timsg_Height = ps.rcPaint.bottom - ps.rcPaint.top;
						TDBPRINTF(TDB_TRACE,("dirty: %d %d %d %d\n",
							imsg->timsg_X, imsg->timsg_Y, imsg->timsg_Width,
							imsg->timsg_Height));
						TAddTail(&win->fbv_IMsgQueue, &imsg->timsg_Node);
					}
					EndPaint(win->fbv_HWnd, &ps);
				}
				fb_intaskflush(mod);
				return 0;
			}

			case WM_SYSCOMMAND:
				if (wParam == SC_MAXIMIZE || wParam == SC_RESTORE)
					win_wndproc(hwnd, WM_ENTERSIZEMOVE, 0, 0);
				break;

			case WM_ACTIVATE:
				if (LOWORD(wParam) != WA_INACTIVE)
				{
					if (win->fbv_Borderless
							&& hwnd != mod->fbd_WindowActivePopup)
					{
						mod->fbd_WindowActivePopup = hwnd;
						TDBPRINTF(TDB_TRACE, ("INPUT -> %p\n", win));
					}
				}
				else 
				{
					if (mod->fbd_WindowActivePopup == hwnd)
					{
						mod->fbd_WindowActivePopup = NULL;
						if (GetCapture() == hwnd)
							ReleaseCapture();

						TDBPRINTF(TDB_TRACE, ("INPUT -> NULL\n"));
					}
					return 0;
				}
				/* fall through for activate */

			case WM_NCLBUTTONDOWN:
			case WM_NCRBUTTONDOWN:
			case WM_NCMBUTTONDOWN:
				if (uMsg != WM_ACTIVATE && mod->fbd_WindowActivePopup == NULL)
					break;
			case WM_ENTERSIZEMOVE:
				wParam = 1; /* pretend activation for refocus */

			case WM_ACTIVATEAPP:

				/* deactivation only via WM_ACTIVATEAPP */
				if (uMsg == WM_ACTIVATEAPP && wParam)
					break;

				if (!win->fbv_Borderless)
				{
					WINWINDOW *winpf, *winnf;
					HWND hwndpf = mod->fbd_WindowFocussedApp;
					HWND hwndnf = wParam ? hwnd : NULL;

					mod->fbd_WindowFocussedApp = hwndnf;

					/* done unless focus changed or refocus enforced */
					if (hwndnf == hwndpf && uMsg == WM_ACTIVATE)
						return 0;

					winpf = fb_getcrossimsg(mod, hwndpf, &imsg, TITYPE_FOCUS);
					if (winpf != NULL)
					{
						imsg->timsg_Code = TFALSE;
						TDBPRINTF(TDB_TRACE, ("FOCUS 0 -> %p\n", winpf));
						TAddTail(&winpf->fbv_IMsgQueue, &imsg->timsg_Node);
					}
					winnf = fb_getcrossimsg(mod, hwndnf, &imsg, TITYPE_FOCUS);
					if (winnf != NULL)
					{
						imsg->timsg_Code = TTRUE;
						TDBPRINTF(TDB_TRACE, ("FOCUS 1 -> %p\n", winnf));
						TAddTail(&winnf->fbv_IMsgQueue, &imsg->timsg_Node);
					}
				}
				return 0;

			case WM_MOVE:
			{
				TUINT w, h;
				EnterCriticalSection(&win->fbv_LockExtents);
				win->fbv_Left = LOWORD(lParam);
				win->fbv_Top = HIWORD(lParam);
				w = win->fbv_Width;
				h = win->fbv_Height;
				LeaveCriticalSection(&win->fbv_LockExtents);

				if ((win->fbv_InputMask & TITYPE_REFRESH) &&
					(fb_getimsg(mod, win, &imsg, TITYPE_REFRESH)))
				{
					imsg->timsg_X = 0;
					imsg->timsg_Y = 0;
					imsg->timsg_Width = w;
					imsg->timsg_Height = h;
					TDBPRINTF(TDB_TRACE,("dirty: %d %d %d %d\n",
						imsg->timsg_X, imsg->timsg_Y, imsg->timsg_Width,
						imsg->timsg_Height));
					TAddTail(&win->fbv_IMsgQueue, &imsg->timsg_Node);
				}
				fb_intaskflush(mod);
				return 0;
			}

			case WM_SIZE:
				EnterCriticalSection(&win->fbv_LockExtents);
				win->fbv_Width = LOWORD(lParam);
				win->fbv_Height = HIWORD(lParam);
				LeaveCriticalSection(&win->fbv_LockExtents);

				if ((win->fbv_InputMask & TITYPE_NEWSIZE) &&
					(fb_getimsg(mod, win, &imsg, TITYPE_NEWSIZE)))
				{
					imsg->timsg_Width = LOWORD(lParam);
					imsg->timsg_Height = HIWORD(lParam);
					TAddTail(&win->fbv_IMsgQueue, &imsg->timsg_Node);
				}
				fb_intaskflush(mod);
				return 0;

			case WM_CAPTURECHANGED:
				TDBPRINTF(TDB_TRACE,("Capture changed %p\n", win));
				return 0;

			case WM_MOUSEMOVE:
			{
				POINT scrpos;
				HWND  hwndcp, hwndcc = mod->fbd_WindowActivePopup;
				WINWINDOW *wincp = NULL;
				TINT x = LOWORD(lParam);
				TINT y = HIWORD(lParam);
				win->fbv_MouseX = x;
				win->fbv_MouseY = y;

				/* get screen position of cursor and window underneath */
				GetCursorPos(&scrpos);
				hwndcp = WindowFromPoint(scrpos);
				wincp = fb_getwindowptr(mod, hwndcp);

				/* dispatch to current window */
				if ((win->fbv_InputMask & TITYPE_MOUSEMOVE) &&
					(fb_getimsg(mod, win, &imsg, TITYPE_MOUSEMOVE)))
				{
					/* Copied from win in fb_getimsg:
					** - imsg->timsg_MouseX = x;
					** - imsg->timsg_MouseY = y;
					*/
					TAddTail(&win->fbv_IMsgQueue, &imsg->timsg_Node);
				}

				/* also send move events to active popup */
				if (hwnd != hwndcc)
				{
					WINWINDOW *wincc = fb_getwindowptr(mod, hwndcc);
					if (wincc != NULL)
					{
						POINT p = scrpos;
						ScreenToClient(hwndcc, &p);
						wincc->fbv_MouseX = p.x;
						wincc->fbv_MouseY = p.y;

						if ((wincc->fbv_InputMask & TITYPE_MOUSEMOVE)
								&& fb_getimsg(mod, wincc, &imsg, 
											  TITYPE_MOUSEMOVE))
						{
							/* Copied from wincc in fb_getimsg:
							** - imsg->timsg_MouseX = p.x;
							** - imsg->timsg_MouseY = p.y;
							**/
							TAddTail(&wincc->fbv_IMsgQueue, &imsg->timsg_Node);
						}
					}
				}

				/* handle hover state change */
				if (hwndcp != mod->fbd_WindowUnderCursor)
				{
					WINWINDOW *winpv;
					HWND hwndpv = mod->fbd_WindowUnderCursor;
					mod->fbd_WindowUnderCursor = hwndcp;

					winpv = fb_getcrossimsg(mod, hwndpv, 
											&imsg, TITYPE_MOUSEOVER);
					if (winpv != NULL)
					{
						imsg->timsg_Code = TFALSE;
						TDBPRINTF(TDB_TRACE, ("MOUSEOVER 0 -> %p\n", winpv));
						TAddTail(&winpv->fbv_IMsgQueue, &imsg->timsg_Node);
					}
					if (wincp != NULL
							&& wincp->fbv_InputMask & TITYPE_MOUSEOVER
							&& fb_getimsg(mod, wincp, 
										  &imsg, TITYPE_MOUSEOVER))
					{
						imsg->timsg_Code = TTRUE;
						TDBPRINTF(TDB_TRACE, ("MOUSEOVER 1 -> %p\n", wincp));
						TAddTail(&wincp->fbv_IMsgQueue, &imsg->timsg_Node);
					}
				}
				return 0;
			}

			case WM_LBUTTONDOWN:
				if ((win->fbv_InputMask & TITYPE_MOUSEBUTTON) &&
					fb_getimsg(mod, win, &imsg, TITYPE_MOUSEBUTTON))
				{
					SetCapture(hwnd);
					imsg->timsg_Code = TMBCODE_LEFTDOWN;
					TAddTail(&win->fbv_IMsgQueue, &imsg->timsg_Node);
				}
				return 0;
			case WM_LBUTTONUP:
				if ((win->fbv_InputMask & TITYPE_MOUSEBUTTON) &&
					fb_getimsg(mod, win, &imsg, TITYPE_MOUSEBUTTON))
				{
					ReleaseCapture();
					imsg->timsg_Code = TMBCODE_LEFTUP;
					TAddTail(&win->fbv_IMsgQueue, &imsg->timsg_Node);
				}
				return 0;
			case WM_RBUTTONDOWN:
				if ((win->fbv_InputMask & TITYPE_MOUSEBUTTON) &&
					fb_getimsg(mod, win, &imsg, TITYPE_MOUSEBUTTON))
				{
					SetCapture(hwnd);
					imsg->timsg_Code = TMBCODE_RIGHTDOWN;
					TAddTail(&win->fbv_IMsgQueue, &imsg->timsg_Node);
				}
				return 0;
			case WM_RBUTTONUP:
				if ((win->fbv_InputMask & TITYPE_MOUSEBUTTON) &&
					fb_getimsg(mod, win, &imsg, TITYPE_MOUSEBUTTON))
				{
					ReleaseCapture();
					imsg->timsg_Code = TMBCODE_RIGHTUP;
					TAddTail(&win->fbv_IMsgQueue, &imsg->timsg_Node);
				}
				return 0;
			case WM_MBUTTONDOWN:
				if ((win->fbv_InputMask & TITYPE_MOUSEBUTTON) &&
					fb_getimsg(mod, win, &imsg, TITYPE_MOUSEBUTTON))
				{
					SetCapture(hwnd);
					imsg->timsg_Code = TMBCODE_MIDDLEDOWN;
					TAddTail(&win->fbv_IMsgQueue, &imsg->timsg_Node);
				}
				return 0;
			case WM_MBUTTONUP:
				if ((win->fbv_InputMask & TITYPE_MOUSEBUTTON) &&
					fb_getimsg(mod, win, &imsg, TITYPE_MOUSEBUTTON))
				{
					ReleaseCapture();
					imsg->timsg_Code = TMBCODE_MIDDLEUP;
					TAddTail(&win->fbv_IMsgQueue, &imsg->timsg_Node);
				}
				return 0;

			case WM_SYSKEYDOWN:
				wParam = 0;
			case WM_KEYDOWN:
				processkey(mod, win, TITYPE_KEYDOWN, wParam, lParam);
				return 0;

			case WM_SYSKEYUP:
				wParam = 0;
			case WM_KEYUP:
				processkey(mod, win, TITYPE_KEYUP, wParam, lParam);
				return 0;
				
			case 0x020a:
				if (win->fbv_InputMask & TITYPE_MOUSEBUTTON)
				{
					TINT16 zdelta = (TINT16) HIWORD(wParam);
					if (zdelta != 0 &&
						fb_getimsg(mod, win, &imsg, TITYPE_MOUSEBUTTON))
					{
						imsg->timsg_Code = zdelta > 0 ?
							TMBCODE_WHEELUP : TMBCODE_WHEELDOWN;
						TAddTail(&win->fbv_IMsgQueue, &imsg->timsg_Node);
					}
				}
				return 0;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

