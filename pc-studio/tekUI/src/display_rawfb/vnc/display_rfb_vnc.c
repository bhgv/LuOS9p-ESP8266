
/*
**	display_rfb_vnc.c - VNC extension to the raw buffer display driver
**	(C) 2013 by Timm S. Mueller <tmueller at schulze-mueller.de>
**
**	This module is written against LibVNCServer, which is a GPLv2 licensed
**	work. Distributing binary versions of your software based on tekUI with
**	VNC support makes your software a combined work with LibVNCServer, which
**	means that your software must comply to the terms of not only tekUI's
**	license, but also to the GPLv2. (Scripts being run on this combined work
**	should not be affected.)
*/

#include <rfb/keysym.h>
#include <tek/lib/utf8.h>
#include "display_rfb_mod.h"
#if defined(ENABLE_VNCSERVER_COPYRECT)
#include <unistd.h>
#include <fcntl.h>
#endif

#define VNCSERVER_COPYRECT_MINPIXELS	10000

static struct rfb_Display *g_mod;

/*****************************************************************************/

typedef struct
{
	int oldbutton;
	int oldx, oldy;
} ClientData;

static void rfb_clientgone(rfbClientPtr cl)
{
	free(cl->clientData);
}

static enum rfbNewClientAction rfb_newclient(rfbClientPtr cl)
{
	cl->clientData = (void *) calloc(sizeof(ClientData), 1);
	cl->clientGoneHook = rfb_clientgone;
	return RFB_CLIENT_ACCEPT;
}

static void rfb_doremoteptr(int buttonMask, int x, int y, rfbClientPtr cl)
{
	ClientData *cd = cl->clientData;
	struct rfb_Display *mod = g_mod;
	TAPTR TExecBase = TGetExecBase(mod);

	TLock(mod->rfb_InstanceLock);
	int sent = 0;

	if (!(cd->oldbutton & 0x01) && (buttonMask & 0x01))
		sent += rfb_sendevent(mod, TITYPE_MOUSEBUTTON, TMBCODE_LEFTDOWN, x, y);
	else if ((cd->oldbutton & 0x01) && !(buttonMask & 0x01))
		sent += rfb_sendevent(mod, TITYPE_MOUSEBUTTON, TMBCODE_LEFTUP, x, y);
	if (!(cd->oldbutton & 0x02) && (buttonMask & 0x02))
		sent +=
			rfb_sendevent(mod, TITYPE_MOUSEBUTTON, TMBCODE_MIDDLEDOWN, x, y);
	else if ((cd->oldbutton & 0x02) && !(buttonMask & 0x02))
		sent += rfb_sendevent(mod, TITYPE_MOUSEBUTTON, TMBCODE_MIDDLEUP, x, y);
	if (!(cd->oldbutton & 0x04) && (buttonMask & 0x04))
		sent +=
			rfb_sendevent(mod, TITYPE_MOUSEBUTTON, TMBCODE_RIGHTDOWN, x, y);
	else if ((cd->oldbutton & 0x04) && !(buttonMask & 0x04))
		sent += rfb_sendevent(mod, TITYPE_MOUSEBUTTON, TMBCODE_RIGHTUP, x, y);
	if (buttonMask & 0x10)
		sent +=
			rfb_sendevent(mod, TITYPE_MOUSEBUTTON, TMBCODE_WHEELDOWN, x, y);
	if (buttonMask & 0x08)
		sent += rfb_sendevent(mod, TITYPE_MOUSEBUTTON, TMBCODE_WHEELUP, x, y);
	if (sent == 0)
		rfb_sendevent(mod, TITYPE_MOUSEMOVE, 0, x, y);
	TUnlock(mod->rfb_InstanceLock);
	cd->oldbutton = buttonMask;
	rfbDefaultPtrAddEvent(buttonMask, x, y, cl);
}

static void rfb_doremotekey(rfbBool keydown, rfbKeySym keysym, rfbClientPtr cl)
{
	/*ClientData *cd = cl->clientData; */
	struct rfb_Display *mod = g_mod;
	TUINT evtype = 0;
	TUINT newqual;
	TUINT evmask = TITYPE_KEYDOWN | TITYPE_KEYUP;
	TBOOL newkey = TFALSE;

	switch (keysym)
	{
		case XK_Shift_L:
			newqual = TKEYQ_LSHIFT;
			break;
		case XK_Shift_R:
			newqual = TKEYQ_RSHIFT;
			break;
		case XK_Control_L:
			newqual = TKEYQ_LCTRL;
			break;
		case XK_Control_R:
			newqual = TKEYQ_RCTRL;
			break;
		case XK_Alt_L:
			newqual = TKEYQ_LALT;
			break;
		case XK_Alt_R:
			newqual = TKEYQ_RALT;
			break;
		default:
			newqual = 0;
	}

	if (newqual != 0)
	{
		if (keydown)
			mod->rfb_KeyQual |= newqual;
		else
			mod->rfb_KeyQual &= ~newqual;
	}

	if (keydown && (evmask & TITYPE_KEYDOWN))
		evtype = TITYPE_KEYDOWN;
	else if (!keydown && (evmask & TITYPE_KEYUP))
		evtype = TITYPE_KEYUP;

	if (evtype)
	{
		TUINT code;
		TUINT qual = mod->rfb_KeyQual;

		if (keysym >= XK_F1 && keysym <= XK_F12)
		{
			code = (TUINT) (keysym - XK_F1) + TKEYC_F1;
			newkey = TTRUE;
		}
		else if (keysym < 256)
		{
			/* cooked ASCII/Latin-1 code */
			code = keysym;
			newkey = TTRUE;
		}
		else if (keysym >= XK_KP_0 && keysym <= XK_KP_9)
		{
			code = (TUINT) (keysym - XK_KP_0) + 48;
			qual |= TKEYQ_NUMBLOCK;
			newkey = TTRUE;
		}
		else
		{
			newkey = TTRUE;
			switch (keysym)
			{
				case XK_Left:
					code = TKEYC_CRSRLEFT;
					break;
				case XK_Right:
					code = TKEYC_CRSRRIGHT;
					break;
				case XK_Up:
					code = TKEYC_CRSRUP;
					break;
				case XK_Down:
					code = TKEYC_CRSRDOWN;
					break;

				case XK_Escape:
					code = TKEYC_ESC;
					break;
				case XK_Delete:
					code = TKEYC_DEL;
					break;
				case XK_BackSpace:
					code = TKEYC_BCKSPC;
					break;
				case XK_ISO_Left_Tab:
				case XK_Tab:
					code = TKEYC_TAB;
					break;
				case XK_Return:
					code = TKEYC_RETURN;
					break;

				case XK_Help:
					code = TKEYC_HELP;
					break;
				case XK_Insert:
					code = TKEYC_INSERT;
					break;
				case XK_Page_Up:
					code = TKEYC_PAGEUP;
					break;
				case XK_Page_Down:
					code = TKEYC_PAGEDOWN;
					break;
				case XK_Home:
					code = TKEYC_POSONE;
					break;
				case XK_End:
					code = TKEYC_POSEND;
					break;
				case XK_Print:
					code = TKEYC_PRINT;
					break;
				case XK_Scroll_Lock:
					code = TKEYC_SCROLL;
					break;
				case XK_Pause:
					code = TKEYC_PAUSE;
					break;
				case XK_KP_Enter:
					code = TKEYC_RETURN;
					qual |= TKEYQ_NUMBLOCK;
					break;
				case XK_KP_Decimal:
					code = '.';
					qual |= TKEYQ_NUMBLOCK;
					break;
				case XK_KP_Add:
					code = '+';
					qual |= TKEYQ_NUMBLOCK;
					break;
				case XK_KP_Subtract:
					code = '-';
					qual |= TKEYQ_NUMBLOCK;
					break;
				case XK_KP_Multiply:
					code = '*';
					qual |= TKEYQ_NUMBLOCK;
					break;
				case XK_KP_Divide:
					code = '/';
					qual |= TKEYQ_NUMBLOCK;
					break;
				default:
					if (keysym > 31 && keysym <= 0x20ff)
						code = keysym;
					else if (keysym >= 0x01000100 && keysym <= 0x0110ffff)
						code = keysym - 0x01000000;
					else
						newkey = TFALSE;
					break;
			}
		}

		if (!newkey && newqual)
		{
			code = TKEYC_NONE;
			newkey = TTRUE;
		}

		if (newkey)
		{
			TAPTR TExecBase = TGetExecBase(mod);
			TIMSG *imsg;
			struct rfb_Window *v;

			TLock(mod->rfb_InstanceLock);
			v = (struct rfb_Window *) TFIRSTNODE(&mod->rfb_VisualList);
			if (rfb_getimsg(mod, v, &imsg, evtype))
			{
				ptrdiff_t len =
					(ptrdiff_t) utf8encode(imsg->timsg_KeyCode, code) -
					(ptrdiff_t) imsg->timsg_KeyCode;
				imsg->timsg_KeyCode[len] = 0;
				imsg->timsg_Code = code;
				imsg->timsg_Qualifier = qual;
				imsg->timsg_MouseX = mod->rfb_MouseX;
				imsg->timsg_MouseY = mod->rfb_MouseY;
				TPutMsg(v->rfbw_IMsgPort, TNULL, imsg);
			}
			TUnlock(mod->rfb_InstanceLock);
		}
	}
}

static void rfb_vnc_task(struct TTask *task)
{
	TAPTR TExecBase = TGetExecBase(task);
	struct rfb_Display *mod = TGetTaskData(task);
	TBOOL waitsig = TFALSE;

#if defined(ENABLE_VNCSERVER_COPYRECT)
	int extra_fd = mod->rfb_RFBPipeFD[0];

	FD_SET(extra_fd, &mod->rfb_RFBScreen->allFds);
	fcntl(extra_fd, F_SETFL, O_NONBLOCK);
#endif

	while (!(TSetSignal(0, 0) & TTASK_SIG_ABORT))
	{
#if defined(ENABLE_VNCSERVER_COPYRECT)
		if (extra_fd > mod->rfb_RFBScreen->maxFd)
			mod->rfb_RFBScreen->maxFd = extra_fd;
		int res = rfbProcessEvents(mod->rfb_RFBScreen, 10000);
		char rdbuf[16];

		if (read(extra_fd, rdbuf, 16) > 0)
			waitsig = TTRUE;
		if (res == 0 && waitsig)
		{
			TSignal(mod->rfb_RFBMainTask, mod->rfb_RFBReadySignal);
			waitsig = TFALSE;
		}
#else
		rfbProcessEvents(mod->rfb_RFBScreen, 20000);
#endif
	}
}

static THOOKENTRY TTAG rfb_vnc_dispatch(struct THook *hook,
	TAPTR obj, TTAG msg)
{
	switch (msg)
	{
		case TMSG_INITTASK:
			return TTRUE;
		case TMSG_RUNTASK:
			rfb_vnc_task(obj);
			break;
	}
	return 0;
}

/*****************************************************************************/

int rfb_vnc_init(struct rfb_Display *mod, int port)
{
	TAPTR TExecBase = TGetExecBase(mod);
	TTAGITEM tags[2];
	struct THook dispatch;
	rfbScreenInfoPtr rfbScreen;

	mod->rfb_RFBPipeFD[0] = -1;

	for (;;)
	{
		if (pipe(mod->rfb_RFBPipeFD) == -1)
			break;
		mod->rfb_RFBReadySignal = TAllocSignal(0);
		if (mod->rfb_RFBReadySignal == 0)
			break;
		mod->rfb_RFBMainTask = TFindTask(TNULL);
		if (mod->rfb_RFBMainTask == TNULL)
			break;

		TUINT fmt = mod->rfb_PixBuf.tpb_Format;

		rfbScreen = rfbGetScreen(0, NULL, mod->rfb_Width,
			mod->rfb_Height, TVPIXFMT_BITS_RED(fmt), 3,
			TVPIXFMT_BYTES_PER_PIXEL(fmt));

		rfbScreen->paddedWidthInBytes = mod->rfb_PixBuf.tpb_BytesPerLine;

		if (rfbScreen == TNULL)
			break;

		g_mod = mod;
		mod->rfb_RFBScreen = rfbScreen;
		rfbScreen->alwaysShared = TRUE;
		rfbScreen->frameBuffer = (char *) mod->rfb_PixBuf.tpb_Data;
		rfbScreen->ptrAddEvent = rfb_doremoteptr;
		rfbScreen->kbdAddEvent = rfb_doremotekey;
		rfbScreen->newClientHook = rfb_newclient;
		mod->rfb_Flags &= ~RFBFL_BUFFER_CAN_RESIZE;

		if (port == 0)
		{
			const char *s = getenv("VNC_PORTNUMBER");

			if (s)
				port = atoi(s);
		}
		if (port > 0)
			rfbScreen->port = port;
		else if (port < 0)
			rfbScreen->autoPort = TRUE;

#if defined(VNCSERVER_HTTP_PATH)
		rfbScreen->httpDir = VNCSERVER_HTTP_PATH;
#endif
		rfbInitServer(rfbScreen);
		rfbScreen->cursor = rfbMakeXCursor(1, 1, " ", " ");

		tags[0].tti_Tag = TTask_UserData;
		tags[0].tti_Value = (TTAG) mod;
		tags[1].tti_Tag = TTAG_DONE;
		TInitHook(&dispatch, rfb_vnc_dispatch, TNULL);
		mod->rfb_VNCTask = TCreateTask(&dispatch, tags);
		if (mod->rfb_VNCTask == TNULL)
			break;
		return 1;
	}

	rfb_vnc_exit(mod);
	return 0;
}

void rfb_vnc_exit(struct rfb_Display *mod)
{
	TAPTR TExecBase = TGetExecBase(mod);

	if (mod->rfb_VNCTask)
	{
		TSignal(mod->rfb_VNCTask, TTASK_SIG_ABORT);
		TDestroy(mod->rfb_VNCTask);
	}
	if (mod->rfb_RFBScreen)
	{
		rfbShutdownServer(mod->rfb_RFBScreen, TRUE);
		rfbScreenCleanup(mod->rfb_RFBScreen);
		mod->rfb_RFBScreen = TNULL;
	}
	if (mod->rfb_RFBReadySignal)
	{
		TFreeSignal(mod->rfb_RFBReadySignal);
		mod->rfb_RFBReadySignal = 0;
	}
	if (mod->rfb_RFBPipeFD[0])
	{
		close(mod->rfb_RFBPipeFD[0]);
		close(mod->rfb_RFBPipeFD[1]);
		mod->rfb_RFBPipeFD[0] = -1;
	}
}

void rfb_vnc_flush(struct rfb_Display *mod, struct Region *D)
{
	struct TNode *next, *node;
	sraRegionPtr region = sraRgnCreate();

	node = D->rg_Rects.rl_List.tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
	{
		struct RectNode *rn = (struct RectNode *) node;
		sraRegionPtr rect = sraRgnCreateRect(rn->rn_Rect[0], rn->rn_Rect[1],
			rn->rn_Rect[2] + 1, rn->rn_Rect[3] + 1);

		sraRgnOr(region, rect);
		sraRgnDestroy(rect);
	}
	rfbMarkRegionAsModified(mod->rfb_RFBScreen, region);
	sraRgnDestroy(region);
}

void rfb_vnc_copyrect(struct rfb_Display *mod, struct rfb_Window *v, int dx,
	int dy, int x0, int y0, int x1, int y1, int yinc)
{
	int i, y;
	int w = x1 - x0 + 1;
	int h = y1 - y0 + 1;
	int dy0 = y0;
	int dy1 = y1;

	if (yinc > 0)
	{
		int t = dy0;

		dy0 = dy1;
		dy1 = t;
	}

	TUINT bpl = w * TVPIXFMT_BYTES_PER_PIXEL(mod->rfb_PixBuf.tpb_Format);

#if defined(ENABLE_VNCSERVER_COPYRECT)
	if (w * h > VNCSERVER_COPYRECT_MINPIXELS)
	{
		char wrbuf = 0;

		/* flush dirty rects */
		rfb_flush_clients(mod, TTRUE);
		/* break rfbProcessEvents */
		if (write(mod->rfb_RFBPipeFD[1], &wrbuf, 1) != 1)
			TDBPRINTF(TDB_ERROR, ("error writing to signalfd\n"));
		/* wait for completion of rfbProcessEvents */
		TExecWait(mod->rfb_ExecBase, mod->rfb_RFBReadySignal);
		/* update own buffer */
		for (i = 0, y = dy0; i < h; ++i, y -= yinc)
			CopyLineOver(v, x0 - dx, y - dy, x0, y, bpl);
		/* schedule copyrect */
		rfbScheduleCopyRect(mod->rfb_RFBScreen, x0, y0, x1 + 1, y1 + 1,
			dx, dy);
	}
	else
#endif
	{
		/* update own buffer */
		for (i = 0, y = dy0; i < h; ++i, y -= yinc)
			CopyLineOver(v, x0 - dx, y - dy, x0, y, bpl);
		/* mark dirty */
		rfbMarkRectAsModified(mod->rfb_RFBScreen, x0, y0, x1 + 1, y1 + 1);
	}
}
