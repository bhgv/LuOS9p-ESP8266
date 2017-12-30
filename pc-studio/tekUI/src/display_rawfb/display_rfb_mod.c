
/*
**	display_rfb_mod.c - Raw framebuffer display driver
**	Written by Franciska Schulze <fschulze at schulze-mueller.de>
**	and Timm S. Mueller <tmueller at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <string.h>
#include <tek/inline/exec.h>
#include <tek/lib/imgload.h>
#include <tek/mod/display.h>

#include "display_rfb_mod.h"

#if defined(RFB_SUB_DEVICE)
#define SUBDEVICE_NAME "display_" DISPLAY_NAME(RFB_SUB_DEVICE)
#else
#define SUBDEVICE_NAME TNULL
#endif

static void rfb_processevent(struct rfb_Display *mod);
static TBOOL rfb_passevent_to_focus(struct rfb_Display *mod, TIMSG *omsg);

#define DEF_PTRWIDTH	8
#define DEF_PTRHEIGHT	8
#ifndef DEF_CURSORFILE
#define DEF_CURSORFILE	"tek/ui/cursor/cursor-black.png"
#endif

/*****************************************************************************/

#define _mpE 0x00000000
#define _mpW 0xffffffff
#define _mpB 0xff000000
#define _mpG 0xffaaaaaa

static const TUINT rfb_defptrimg[] =
{
	_mpB, _mpB, _mpB, _mpB, _mpB, _mpB, _mpB, _mpB,
	_mpB, _mpW, _mpW, _mpW, _mpW, _mpW, _mpG, _mpB,
	_mpB, _mpW, _mpW, _mpW, _mpW, _mpG, _mpB, _mpE,
	_mpB, _mpW, _mpW, _mpW, _mpG, _mpB, _mpE, _mpE,
	_mpB, _mpW, _mpW, _mpG, _mpW, _mpG, _mpB, _mpE,
	_mpB, _mpW, _mpG, _mpB, _mpG, _mpW, _mpG, _mpB,
	_mpB, _mpG, _mpB, _mpE, _mpB, _mpG, _mpG, _mpB,
	_mpB, _mpB, _mpE, _mpE, _mpE, _mpB, _mpB, _mpE,
};

static void rfb_initdefpointer(struct rfb_Display *mod)
{
	mod->rfb_PtrImage.tpb_Data = (TUINT8 *) rfb_defptrimg;
	mod->rfb_PtrImage.tpb_Format = TVPIXFMT_A8R8G8B8;
	mod->rfb_PtrImage.tpb_BytesPerLine = DEF_PTRWIDTH * 4;
	mod->rfb_PtrWidth = DEF_PTRWIDTH;
	mod->rfb_PtrHeight = DEF_PTRHEIGHT;
	mod->rfb_MouseHotX = 0;
	mod->rfb_MouseHotY = 0;
	mod->rfb_Flags &= ~(RFBFL_PTR_VISIBLE | RFBFL_PTR_ALLOCATED);
}

LOCAL TBOOL rfb_initpointer(struct rfb_Display *mod)
{
	TAPTR TExecBase = TGetExecBase(mod);
	FILE *f;

	if ((mod->rfb_Flags & (RFBFL_CANSHOWPTR | RFBFL_SHOWPTR)) !=
		(RFBFL_CANSHOWPTR | RFBFL_SHOWPTR))
		return TFALSE;
	rfb_initdefpointer(mod);
	f = fopen(DEF_CURSORFILE, "rb");
	if (f)
	{
		struct ImgLoader ld;

		if (imgload_init_file(&ld, TExecBase, f))
		{
			if (imgload_load(&ld))
			{
				mod->rfb_PtrImage = ld.iml_Image;
				mod->rfb_PtrWidth = ld.iml_Width;
				mod->rfb_PtrHeight = ld.iml_Height;
				mod->rfb_Flags |= RFBFL_PTR_ALLOCATED;
			}
		}
		fclose(f);
	}
	mod->rfb_PtrBackBuffer.data = TAlloc(mod->rfb_MemMgr,
		TVPIXFMT_BYTES_PER_PIXEL(mod->rfb_PixBuf.tpb_Format) *
		mod->rfb_PtrWidth * mod->rfb_PtrHeight);
	if (mod->rfb_PtrBackBuffer.data)
		return TTRUE;
	rfb_initdefpointer(mod);
	return TFALSE;
}

static void rfb_storebackbuf(struct rfb_Display *mod,
	struct rfb_BackBuffer *bbuf, TINT x0, TINT y0, TINT x1, TINT y1)
{
	TUINT8 *bkdst = bbuf->data;
	TUINT8 *bksrc = TVPB_GETADDRESS(&mod->rfb_DevBuf, x0, y0);
	TUINT bpl = mod->rfb_DevBuf.tpb_BytesPerLine;
	TUINT bpp = TVPIXFMT_BYTES_PER_PIXEL(mod->rfb_DevBuf.tpb_Format);
	TUINT srcbpl = mod->rfb_PtrWidth * bpp;
	TINT y;

	for (y = y0; y <= y1; ++y)
	{
		memcpy(bkdst, bksrc, srcbpl);
		bksrc += bpl;
		bkdst += srcbpl;
	}
	bbuf->rect[0] = x0;
	bbuf->rect[1] = y0;
	bbuf->rect[2] = x1;
	bbuf->rect[3] = y1;
}

static void rfb_restorebackbuf(struct rfb_Display *mod,
	struct rfb_BackBuffer *bbuf)
{
	TUINT8 *bksrc = bbuf->data;
	TUINT8 *bkdst = TVPB_GETADDRESS(&mod->rfb_DevBuf, bbuf->rect[0],
		bbuf->rect[1]);
	TUINT bpl = mod->rfb_DevBuf.tpb_BytesPerLine;
	TUINT bpp = TVPIXFMT_BYTES_PER_PIXEL(mod->rfb_DevBuf.tpb_Format);
	TUINT srcbpl = mod->rfb_PtrWidth * bpp;
	TINT y;

	for (y = bbuf->rect[1]; y <= bbuf->rect[3]; ++y)
	{
		memcpy(bkdst, bksrc, srcbpl);
		bkdst += bpl;
		bksrc += srcbpl;
	}
}

static void rfb_drawpointer(struct rfb_Display *mod)
{
	TINT x0 = mod->rfb_MouseX - mod->rfb_MouseHotX;
	TINT y0 = mod->rfb_MouseY - mod->rfb_MouseHotY;

	if (mod->rfb_Flags & RFBFL_PTR_VISIBLE)
	{
		if (x0 == mod->rfb_PtrBackBuffer.rect[0] &&
			y0 == mod->rfb_PtrBackBuffer.rect[1])
			return;
		rfb_restorebackbuf(mod, &mod->rfb_PtrBackBuffer);
	}

	TINT s0 = 0;
	TINT s1 = 0;
	TINT s2 = mod->rfb_Width - 1;
	TINT s3 = mod->rfb_Height - 1;
	TINT x1 = x0 + mod->rfb_PtrWidth - 1;
	TINT y1 = y0 + mod->rfb_PtrHeight - 1;

	if (REGION_OVERLAP(s0, s1, s2, s3, x0, y0, x1, y1))
	{
		struct TVPixBuf dst = mod->rfb_DevBuf;

		x0 = TMAX(x0, s0);
		y0 = TMAX(y0, s1);
		x1 = TMIN(x1, s2);
		y1 = TMIN(y1, s3);
		rfb_storebackbuf(mod, &mod->rfb_PtrBackBuffer, x0, y0, x1, y1);
		pixconv_convert(&mod->rfb_PtrImage, &dst, x0, y0, x1, y1, 0, 0,
			TTRUE, TFALSE);
	}

	mod->rfb_Flags |= RFBFL_PTR_VISIBLE;
}

static void rfb_restoreptrbg(struct rfb_Display *mod)
{
	if (!(mod->rfb_Flags & RFBFL_PTR_VISIBLE))
		return;
	rfb_restorebackbuf(mod, &mod->rfb_PtrBackBuffer);
	mod->rfb_Flags &= ~RFBFL_PTR_VISIBLE;
}

static TINT rfb_cmdrectaffected(struct rfb_Display *mod, struct TVRequest *req,
	TINT result[4], TBOOL source_affect)
{
	struct rfb_Window *v = TNULL;
	TINT *rect = TNULL;
	TINT *xywh = TNULL;
	TINT temprect[4];

	switch (req->tvr_Req.io_Command)
	{
		default:
			/* not affected, no rect */
			return 0;

		case TVCMD_FLUSH:
		case TVCMD_SETATTRS:
		case TVCMD_CLOSEWINDOW:
			/* yes, affected, but no rect */
			return -1;

			/* window rect: */
		case TVCMD_DRAWSTRIP:
			v = req->tvr_Op.Strip.Window;
			break;
		case TVCMD_DRAWFAN:
			v = req->tvr_Op.Fan.Window;
			break;
		case TVCMD_TEXT:
			v = req->tvr_Op.Text.Window;
			break;

			/* specific rect: */
		case TVCMD_RECT:
			v = req->tvr_Op.Rect.Window;
			xywh = req->tvr_Op.Rect.Rect;
			break;
		case TVCMD_FRECT:
			v = req->tvr_Op.FRect.Window;
			xywh = req->tvr_Op.FRect.Rect;
			break;
		case TVCMD_LINE:
			v = req->tvr_Op.Line.Window;
			xywh = req->tvr_Op.Line.Rect;
			break;
		case TVCMD_DRAWBUFFER:
			v = req->tvr_Op.DrawBuffer.Window;
			xywh = req->tvr_Op.DrawBuffer.RRect;
			break;
		case TVCMD_COPYAREA:
		{
			v = req->tvr_Op.CopyArea.Window;
			TINT *s = req->tvr_Op.CopyArea.Rect;
			TINT dx0 = req->tvr_Op.CopyArea.DestX;
			TINT dy0 = req->tvr_Op.CopyArea.DestY;
			TINT sx0 = s[0];
			TINT sy0 = s[1];
			TINT sx1 = s[0] + s[2] - 1;
			TINT sy1 = s[1] + s[3] - 1;
			TINT dx = dx0 - sx0;
			TINT dy = dy0 - sy0;
			TINT dx1 = sx1 + dx;
			TINT dy1 = sy1 + dy;

			rect = temprect;
			if (source_affect)
			{
				rect[0] = TMIN(sx0, dx0);
				rect[1] = TMIN(sy0, dy0);
				rect[2] = TMAX(sx1, dx1);
				rect[3] = TMAX(sy1, dy1);
				break;
			}
			rect[0] = dx0;
			rect[1] = dy0;
			rect[2] = dx1;
			rect[3] = dy1;
			break;
		}
		case TVCMD_PLOT:
			v = req->tvr_Op.Plot.Window;
			rect = temprect;
			rect[0] = req->tvr_Op.Plot.Rect[0];
			rect[1] = req->tvr_Op.Plot.Rect[1];
			rect[2] = req->tvr_Op.Plot.Rect[0];
			rect[3] = req->tvr_Op.Plot.Rect[1];
			break;
	}

	if (v->rfbw_ClipRect.r[0] == -1)
		return 0;

	if (xywh)
	{
		rect = temprect;
		rect[0] = xywh[0];
		rect[1] = xywh[1];
		rect[2] = xywh[0] + xywh[2] - 1;
		rect[3] = xywh[1] + xywh[3] - 1;
	}

	if (rect)
	{
		result[0] = rect[0] + v->rfbw_WinRect.r[0];
		result[1] = rect[1] + v->rfbw_WinRect.r[1];
		result[2] = rect[2] + v->rfbw_WinRect.r[0];
		result[3] = rect[3] + v->rfbw_WinRect.r[1];
	}
	else
		memcpy(result, v->rfbw_WinRect.r, 16);

	return region_intersect(result, v->rfbw_ClipRect.r);
}

/*****************************************************************************/
/*
**	AllocReq/FreeReq
*/

static TMODAPI struct TVRequest *rfb_allocreq(struct rfb_Display *mod)
{
	struct TVRequest *req = TExecAllocMsg(mod->rfb_ExecBase,
		sizeof(struct TVRequest));

	if (req)
		req->tvr_Req.io_Device = (struct TModule *) mod;
	return req;
}

static TMODAPI void rfb_freereq(struct rfb_Display *mod, struct TVRequest *req)
{
	TExecFree(mod->rfb_ExecBase, req);
}

/*****************************************************************************/
/*
**	BeginIO/AbortIO
*/

static TMODAPI void rfb_beginio(struct rfb_Display *mod, struct TVRequest *req)
{
	TExecPutMsg(mod->rfb_ExecBase, mod->rfb_CmdPort,
		req->tvr_Req.io_ReplyPort, req);
#if defined(ENABLE_LINUXFB)
	rfb_linux_wake(mod);
#endif
}

static TMODAPI TINT rfb_abortio(struct rfb_Display *mod, struct TVRequest *req)
{
	return -1;
}

/*****************************************************************************/

LOCAL TBOOL rfb_getimsg(struct rfb_Display *mod, struct rfb_Window *v,
	TIMSG **msgptr, TUINT type)
{
	TAPTR TExecBase = TGetExecBase(mod);
	TIMSG *msg;

	TLock(mod->rfb_InstanceLock);
	msg = (TIMSG *) TRemHead(&mod->rfb_IMsgPool);
	TUnlock(mod->rfb_InstanceLock);
	if (msg == TNULL)
		msg = TAllocMsg(sizeof(TIMSG));
	*msgptr = msg;
	if (msg)
	{
		memset(msg, 0, sizeof(TIMSG));
		msg->timsg_Instance = v;
		msg->timsg_UserData = v ? v->rfbw_UserData : TNULL;
		msg->timsg_Type = type;
		msg->timsg_Qualifier = mod->rfb_KeyQual;
		msg->timsg_MouseX = mod->rfb_MouseX;
		msg->timsg_MouseY = mod->rfb_MouseY;
		TGetSystemTime(&msg->timsg_TimeStamp);
		return TTRUE;
	}
	return TFALSE;
}

LOCAL void rfb_putbackmsg(struct rfb_Display *mod, TIMSG *msg)
{
	TAPTR TExecBase = TGetExecBase(mod);

	TLock(mod->rfb_InstanceLock);
	TAddTail(&mod->rfb_IMsgPool, &msg->timsg_Node);
	TUnlock(mod->rfb_InstanceLock);
}

/*****************************************************************************/

static void rfb_exittask(struct rfb_Display *mod)
{
	TAPTR TExecBase = TGetExecBase(mod);
	struct TNode *imsg, *node, *next;

#if defined(ENABLE_LINUXFB)
	rfb_linux_exit(mod);
#endif

	TFree(mod->rfb_PtrBackBuffer.data);
	if (mod->rfb_Flags & RFBFL_PTR_ALLOCATED)
		TFree(mod->rfb_PtrImage.tpb_Data);

	/* free pooled input messages: */
	while ((imsg = TRemHead(&mod->rfb_IMsgPool)))
		TFree(imsg);

	/* close all fonts */
	node = mod->rfb_FontManager.openfonts.tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
		rfb_hostclosefont(mod, (TAPTR) node);

	if (mod->rfb_Flags & RFBFL_BUFFER_OWNER)
		TFree(mod->rfb_PixBuf.tpb_Data);

	TDestroy(mod->rfb_RndIMsgPort);
	TFree(mod->rfb_RndRequest);
	TCloseModule(mod->rfb_RndDevice);
	TDestroy((struct THandle *) mod->rfb_RndRPort);
	TDestroy((struct THandle *) mod->rfb_InstanceLock);

	region_free(&mod->rfb_RectPool, &mod->rfb_DirtyRegion);

	region_destroypool(&mod->rfb_RectPool);
}

/*****************************************************************************/

static TBOOL rfb_inittask(struct TTask *task)
{
	TAPTR TExecBase = TGetExecBase(task);
	struct rfb_Display *mod = TGetTaskData(task);

	for (;;)
	{
		TTAGITEM *opentags = mod->rfb_OpenTags;
		TSTRPTR subname;

		/* Initialize rectangle pool */
		region_initpool(&mod->rfb_RectPool, TExecBase);

		/* list of free input messages: */
		TInitList(&mod->rfb_IMsgPool);

		/* list of all open visuals: */
		TInitList(&mod->rfb_VisualList);

		/* init fontmanager and default font */
		TInitList(&mod->rfb_FontManager.openfonts);

		region_init(&mod->rfb_RectPool, &mod->rfb_DirtyRegion, TNULL);

		mod->rfb_PixBuf.tpb_Format = TVPIXFMT_UNDEFINED;
		mod->rfb_DevWidth = RFB_DEF_WIDTH;
		mod->rfb_DevHeight = RFB_DEF_HEIGHT;

		mod->rfb_Flags = RFBFL_BUFFER_CAN_RESIZE;

#if defined(ENABLE_LINUXFB)
		if (!rfb_linux_init(mod))
			break;
#endif
		/* Instance lock (currently needed for async VNC) */
		mod->rfb_InstanceLock = TCreateLock(TNULL);
		if (mod->rfb_InstanceLock == TNULL)
			break;

		/* Open sub device, if one is requested: */
		subname = (TSTRPTR) TGetTag(opentags, TVisual_DriverName,
			(TTAG) SUBDEVICE_NAME);
		if (subname)
		{
			TTAGITEM subtags[2];

			subtags[0].tti_Tag = TVisual_IMsgPort;
			subtags[0].tti_Value = TGetTag(opentags, TVisual_IMsgPort, TNULL);
			subtags[1].tti_Tag = TTAG_DONE;

			mod->rfb_RndRPort = TCreatePort(TNULL);
			if (mod->rfb_RndRPort == TNULL)
				break;
			mod->rfb_RndDevice = TOpenModule(subname, 0, subtags);
			if (mod->rfb_RndDevice == TNULL)
				break;
			mod->rfb_RndRequest = TAllocMsg(sizeof(struct TVRequest));
			if (mod->rfb_RndRequest == TNULL)
				break;
			mod->rfb_RndIMsgPort = TCreatePort(TNULL);
			if (mod->rfb_RndIMsgPort == TNULL)
				break;
		}

		TDBPRINTF(TDB_TRACE, ("Instance init successful\n"));
		return TTRUE;
	}

	rfb_exittask(mod);
	return TFALSE;
}

static void rfb_runtask(struct TTask *task)
{
	TAPTR TExecBase = TGetExecBase(task);
	struct rfb_Display *mod = TGetTaskData(task);
	struct TVRequest *req;
	TUINT sig = 0;

	TTIME intt = { 20000 };
	/* next absolute time to send interval message: */
	TTIME nextt;
	TTIME waitt, nowt, *pwaitt;

	TAPTR cmdport = TGetUserPort(task);
	TUINT cmdportsignal = TGetPortSignal(cmdport);
	TUINT imsgportsignal = TGetPortSignal(mod->rfb_RndIMsgPort);
	TUINT waitsigs = cmdportsignal | imsgportsignal | TTASK_SIG_ABORT;

	TDBPRINTF(TDB_INFO, ("RawFB device context running\n"));

	TGetSystemTime(&nowt);
	nextt = nowt;
	TAddTime(&nextt, &intt);

	do
	{
		if (sig & cmdportsignal)
		{
			TBOOL checkrect = mod->rfb_Flags & RFBFL_PTR_VISIBLE;
			TINT arec[4];

			while ((req = TGetMsg(cmdport)))
			{
				if (checkrect)
				{
					TINT res = rfb_cmdrectaffected(mod, req, arec, TTRUE);

					if (res < 0 || (res > 0 &&
							REGION_OVERLAPRECT(mod->rfb_PtrBackBuffer.rect,
								arec)))
					{
						rfb_restoreptrbg(mod);
						checkrect = TFALSE;
					}
				}
				rfb_docmd(mod, req);
				TReplyMsg(req);
			}
		}

		if (mod->rfb_Flags & RFBFL_SHOWPTR)
			rfb_drawpointer(mod);

		/* check if time interval has expired: */
		TGetSystemTime(&nowt);

		/* do interval timers */
		if (mod->rfb_NumInterval > 0)
		{
			if (TCmpTime(&nowt, &nextt) >= 0)
			{
				/* expired; send intervals: */
				TLock(mod->rfb_InstanceLock);
				struct TNode *next, 
					*node = mod->rfb_VisualList.tlh_Head.tln_Succ;

				for (; (next = node->tln_Succ); node = next)
				{
					struct rfb_Window *v = (struct rfb_Window *) node;
					TIMSG *imsg;

					if ((v->rfbw_InputMask & TITYPE_INTERVAL) &&
						rfb_getimsg(mod, v, &imsg, TITYPE_INTERVAL))
						TPutMsg(v->rfbw_IMsgPort, TNULL, imsg);
				}
				TUnlock(mod->rfb_InstanceLock);
				TAddTime(&nextt, &intt);
			}

			/* calculate new wait time: */
			waitt = nextt;
			TSubTime(&waitt, &nowt);
			if (waitt.tdt_Int64 <= 0 || waitt.tdt_Int64 > 20000)
			{
				nextt = nowt;
				TAddTime(&nextt, &intt);
				waitt = nextt;
				TSubTime(&waitt, &nowt);
			}
			pwaitt = &waitt;
		}
		else
			pwaitt = TNULL;

#if defined(ENABLE_LINUXFB)
		rfb_linux_wait(mod, pwaitt);
		sig = TSetSignal(0, waitsigs);
#else
		sig = TWaitTime(pwaitt, waitsigs);
#endif

		if (sig & imsgportsignal)
			rfb_processevent(mod);

	} while (!(sig & TTASK_SIG_ABORT));

	TDBPRINTF(TDB_INFO, ("RawFB device context closedown\n"));

	rfb_exittask(mod);
}

/*****************************************************************************/

static struct rfb_Window *rfb_findcoord(struct rfb_Display *mod, TINT x,
	TINT y)
{
	struct TNode *next, *node = mod->rfb_VisualList.tlh_Head.tln_Succ;

	for (; (next = node->tln_Succ); node = next)
	{
		struct rfb_Window *v = (struct rfb_Window *) node;
		TINT *r = v->rfbw_ScreenRect.r;

		if (x >= r[0] && x <= r[2] && y >= r[1] && y <= r[3])
			return v;
	}
	return TNULL;
}

static TBOOL rfb_passevent(struct rfb_Display *mod, struct rfb_Window *v,
	TIMSG *omsg)
{
	TAPTR TExecBase = TGetExecBase(mod);
	TUINT type = omsg->timsg_Type;

	if (v && (v->rfbw_InputMask & type))
	{
		TIMSG *imsg;

		if (rfb_getimsg(mod, v, &imsg, omsg->timsg_Type))
		{
			/* note: screen coordinates */
			TINT x = omsg->timsg_MouseX;
			TINT y = omsg->timsg_MouseY;

			mod->rfb_MouseX = x;
			mod->rfb_MouseY = y;
			imsg->timsg_Code = omsg->timsg_Code;
			imsg->timsg_Qualifier = omsg->timsg_Qualifier;
			imsg->timsg_MouseX = x - v->rfbw_ScreenRect.r[0];
			imsg->timsg_MouseY = y - v->rfbw_ScreenRect.r[1];
			imsg->timsg_ScreenMouseX = x;
			imsg->timsg_ScreenMouseY = y;
			memcpy(imsg->timsg_KeyCode, omsg->timsg_KeyCode, 8);
			TPutMsg(v->rfbw_IMsgPort, TNULL, imsg);
			return TTRUE;
		}
	}
	return TFALSE;
}

LOCAL struct rfb_Window *rfb_passevent_by_mousexy(struct rfb_Display *mod,
	TIMSG *omsg, TBOOL focus)
{
	TAPTR TExecBase = TGetExecBase(mod);

	/* note: screen coordinates */
	TINT x = omsg->timsg_MouseX =
		TCLAMP(0, omsg->timsg_MouseX, mod->rfb_Width - 1);
	TINT y = omsg->timsg_MouseY =
		TCLAMP(0, omsg->timsg_MouseY, mod->rfb_Height - 1);
	TLock(mod->rfb_InstanceLock);
	struct rfb_Window *v = rfb_findcoord(mod, x, y);

	if (v && (omsg->timsg_Type != TITYPE_MOUSEMOVE ||
		(mod->rfb_FocusWindow == v || (v->rfbw_Flags & RFBWFL_IS_POPUP))))
	{
		if (focus)
			rfb_focuswindow(mod, v);
		rfb_passevent(mod, v, omsg);
	}
	TUnlock(mod->rfb_InstanceLock);
	return v;
}

static TBOOL rfb_passevent_to_focus(struct rfb_Display *mod, TIMSG *omsg)
{
	TAPTR TExecBase = TGetExecBase(mod);
	TBOOL sent = TFALSE;

	TLock(mod->rfb_InstanceLock);
	struct rfb_Window *v = mod->rfb_FocusWindow;

	if (v)
		sent = rfb_passevent(mod, v, omsg);
	TUnlock(mod->rfb_InstanceLock);
	return sent;
}

LOCAL void rfb_passevent_mousebutton(struct rfb_Display *mod, TIMSG *msg)
{
	TBOOL down = msg->timsg_Code & (TMBCODE_LEFTDOWN |
		TMBCODE_RIGHTDOWN | TMBCODE_MIDDLEDOWN);
	struct rfb_Window *v = rfb_passevent_by_mousexy(mod, msg, down);

	if (!down && v != mod->rfb_FocusWindow)
		rfb_passevent_to_focus(mod, msg);
}

LOCAL void rfb_passevent_keyboard(struct rfb_Display *mod, TIMSG *msg)
{
	/* pass keyboard events to focused window, else to the
	** hovered window (also setting the focus): */
	if (!rfb_passevent_to_focus(mod, msg))
		rfb_passevent_by_mousexy(mod, msg, TTRUE);
}

LOCAL void rfb_passevent_mousemove(struct rfb_Display *mod, TIMSG *msg)
{
	/* pass mouse movements to focused and hovered window: */
	if (rfb_passevent_by_mousexy(mod, msg, TFALSE) != mod->rfb_FocusWindow)
		rfb_passevent_to_focus(mod, msg);
}

LOCAL TINT rfb_sendevent(struct rfb_Display *mod, TUINT type, TUINT code,
	TINT x, TINT y)
{
	TIMSG *msg;

	if (!rfb_getimsg(mod, TNULL, &msg, type))
		return 0;
	msg->timsg_Code = code;
	msg->timsg_MouseX = x;
	msg->timsg_MouseY = y;
	TINT res = 1;

	switch (type)
	{
		case TITYPE_MOUSEMOVE:
			rfb_passevent_mousemove(mod, msg);
			break;
		case TITYPE_MOUSEBUTTON:
			rfb_passevent_mousebutton(mod, msg);
			break;
		case TITYPE_KEYUP:
		case TITYPE_KEYDOWN:
			rfb_passevent_keyboard(mod, msg);
			break;
		default:
			TDBPRINTF(TDB_ERROR, ("illegal message type\n"));
			res = 0;
	}
	/* put back prototype message */
	TAddTail(&mod->rfb_IMsgPool, &msg->timsg_Node);
	return res;
}

/*****************************************************************************/

static void rfb_processevent(struct rfb_Display *mod)
{
	TAPTR TExecBase = TGetExecBase(mod);
	TIMSG *msg;

	if (mod->rfb_RndIMsgPort == TNULL)
		return;

	while ((msg = TGetMsg(mod->rfb_RndIMsgPort)))
	{
		/*struct rfb_Window *v = (struct rfb_Window *) msg->timsg_Instance; */
		TIMSG *imsg;

		switch (msg->timsg_Type)
		{
			case TITYPE_INTERVAL:
				TDBPRINTF(TDB_WARN, ("unhandled event: INTERVAL\n"));
				break;
			case TITYPE_REFRESH:
			{
				TINT drect[4];

				drect[0] = msg->timsg_X;
				drect[1] = msg->timsg_Y;
				drect[2] = msg->timsg_X + msg->timsg_Width - 1;
				drect[3] = msg->timsg_Y + msg->timsg_Height - 1;
				rfb_damage(mod, drect, TNULL);
				rfb_flush_clients(mod, TTRUE);
				break;
			}
			case TITYPE_NEWSIZE:
				if ((mod->rfb_Flags & RFBFL_BUFFER_OWNER) &&
					(mod->rfb_Flags & RFBFL_BUFFER_CAN_RESIZE))
				{
					region_free(&mod->rfb_RectPool, &mod->rfb_DirtyRegion);
					mod->rfb_Flags &= ~RFBFL_DIRTY;

					TINT neww = mod->rfb_Width = msg->timsg_Width;
					TINT newh = mod->rfb_Height = msg->timsg_Height;

					TUINT pixfmt = mod->rfb_PixBuf.tpb_Format;
					TUINT bpp = TVPIXFMT_BYTES_PER_PIXEL(pixfmt);
					TUINT bpl = bpp * neww;

					struct TVPixBuf newbuf;

					newbuf.tpb_BytesPerLine = bpl;
					newbuf.tpb_Format = mod->rfb_PixBuf.tpb_Format;
					newbuf.tpb_Data = TAlloc(mod->rfb_MemMgr, bpl * newh);
					if (newbuf.tpb_Data == TNULL)
						break;

					struct TNode *next, 
						*node = mod->rfb_VisualList.tlh_Head.tln_Succ;

					for (; (next = node->tln_Succ); node = next)
					{
						struct rfb_Window *v = (struct rfb_Window *) node;

						rfb_setrealcliprect(mod, v);

						TINT w0 = REGION_RECT_WIDTH(&v->rfbw_ScreenRect);
						TINT h0 = REGION_RECT_HEIGHT(&v->rfbw_ScreenRect);

						if ((v->rfbw_Flags & RFBWFL_FULLSCREEN))
						{
							v->rfbw_ScreenRect.r[0] = 0;
							v->rfbw_ScreenRect.r[1] = 0;
							v->rfbw_ScreenRect.r[2] = neww - 1;
							v->rfbw_ScreenRect.r[3] = newh - 1;
						}

						TINT ww = REGION_RECT_WIDTH(&v->rfbw_ScreenRect);
						TINT wh = REGION_RECT_HEIGHT(&v->rfbw_ScreenRect);

						if (v->rfbw_MinWidth > 0 && ww < v->rfbw_MinWidth)
							v->rfbw_ScreenRect.r[0] =
								v->rfbw_ScreenRect.r[2] - v->rfbw_MinWidth;
						if (v->rfbw_MinHeight > 0 && wh < v->rfbw_MinHeight)
							v->rfbw_ScreenRect.r[1] =
								v->rfbw_ScreenRect.r[3] - v->rfbw_MinHeight;

						ww = REGION_RECT_WIDTH(&v->rfbw_ScreenRect);
						wh = REGION_RECT_HEIGHT(&v->rfbw_ScreenRect);

						if (v->rfbw_Flags & RFBWFL_BACKBUFFER)
							rfb_resizewinbuffer(mod, v, w0, h0, ww, wh);
						else
							v->rfbw_PixBuf = newbuf;

						rfb_setwinrect(mod, v);

						if (ww != w0 || wh != h0)
						{
							if (rfb_getimsg(mod, v, &imsg, TITYPE_NEWSIZE))
							{
								imsg->timsg_Width = ww;
								imsg->timsg_Height = wh;
								TPutMsg(v->rfbw_IMsgPort, TNULL, imsg);
							}
						}
					}

					TFree(mod->rfb_PixBuf.tpb_Data);
					mod->rfb_PixBuf = newbuf;

					struct Rect drect;

					REGION_RECT_SET(&drect, 0, 0, neww - 1, newh - 1);
					rfb_damage(mod, drect.r, TNULL);
				}
				else
					TDBPRINTF(TDB_WARN, ("Cannot resize this framebuffer\n"));
				break;

			case TITYPE_CLOSE:
			{
				/* send to root window */
				TLock(mod->rfb_InstanceLock);
				struct rfb_Window *v =
					(struct rfb_Window *) TLASTNODE(&mod->rfb_VisualList);
				if (rfb_getimsg(mod, v, &imsg, TITYPE_CLOSE))
					TPutMsg(v->rfbw_IMsgPort, TNULL, imsg);
				TUnlock(mod->rfb_InstanceLock);
				break;
			}
			case TITYPE_FOCUS:
				TDBPRINTF(TDB_INFO, ("unhandled event: FOCUS\n"));
				break;
			case TITYPE_MOUSEOVER:
				TDBPRINTF(TDB_INFO, ("unhandled event: MOUSEOVER\n"));
				break;

			case TITYPE_KEYUP:
			case TITYPE_KEYDOWN:
				rfb_passevent_keyboard(mod, msg);
				break;

			case TITYPE_MOUSEMOVE:
				rfb_passevent_mousemove(mod, msg);
				break;

			case TITYPE_MOUSEBUTTON:
				rfb_passevent_mousebutton(mod, msg);
				break;
		}
		TReplyMsg(msg);
	}
}

/*****************************************************************************/
/*
**	Module init/exit
*/

static void rfb_exit(struct rfb_Display *mod)
{
	TAPTR TExecBase = TGetExecBase(mod);

	if (mod->rfb_Task)
	{
		TSignal(mod->rfb_Task, TTASK_SIG_ABORT);
#if defined(ENABLE_LINUXFB)
		rfb_linux_wake(mod);
#endif
		TDestroy((struct THandle *) mod->rfb_Task);
	}
}

static TBOOL rfb_init(struct rfb_Display *mod, TTAGITEM *tags)
{
	TAPTR TExecBase = TGetExecBase(mod);

	mod->rfb_OpenTags = tags;
	for (;;)
	{
		TTAGITEM tags[2];

		tags[0].tti_Tag = TTask_UserData;
		tags[0].tti_Value = (TTAG) mod;
		tags[1].tti_Tag = TTAG_DONE;
		mod->rfb_Task =
			TCreateTask(&mod->rfb_Module.tmd_Handle.thn_Hook, tags);
		if (mod->rfb_Task == TNULL)
			break;
		mod->rfb_CmdPort = TGetUserPort(mod->rfb_Task);
		return TTRUE;
	}

	rfb_exit(mod);
	return TFALSE;
}

/*****************************************************************************/
/*
**	Module open/close
*/

static TAPTR rfb_modopen(struct rfb_Display *mod, TTAGITEM *tags)
{
	TBOOL success = TTRUE;

	TExecLock(mod->rfb_ExecBase, mod->rfb_Lock);
	if (mod->rfb_RefCount == 0)
		success = rfb_init(mod, tags);
	if (success)
		mod->rfb_RefCount++;
	TExecUnlock(mod->rfb_ExecBase, mod->rfb_Lock);
	if (success)
	{
		/* Attributes that can be queried during open: */
		TTAG p = TGetTag(tags, TVisual_HaveWindowManager, TNULL);

		if (p)
			*((TBOOL *) p) = TFALSE;
		return mod;
	}
	return TNULL;
}

static void rfb_modclose(struct rfb_Display *mod)
{
	TExecLock(mod->rfb_ExecBase, mod->rfb_Lock);
	if (--mod->rfb_RefCount == 0)
		rfb_exit(mod);
	TExecUnlock(mod->rfb_ExecBase, mod->rfb_Lock);
}

static const TMFPTR rfb_vectors[RFB_DISPLAY_NUMVECTORS] = {
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) rfb_beginio,
	(TMFPTR) rfb_abortio,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,

	(TMFPTR) rfb_allocreq,
	(TMFPTR) rfb_freereq,
};

static void rfb_destroy(struct rfb_Display *mod)
{
	TDestroy((struct THandle *) mod->rfb_Lock);
	if (mod->rfb_FTLibrary)
	{
		if (mod->rfb_FTCManager)
			FTC_Manager_Done(mod->rfb_FTCManager);
		FT_Done_FreeType(mod->rfb_FTLibrary);
	}
}

static THOOKENTRY TTAG rfb_dispatch(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct rfb_Display *mod = (struct rfb_Display *) hook->thk_Data;

	switch (msg)
	{
		case TMSG_DESTROY:
			rfb_destroy(mod);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) rfb_modopen(mod, obj);
		case TMSG_CLOSEMODULE:
			rfb_modclose(obj);
			break;
		case TMSG_INITTASK:
			return rfb_inittask(obj);
		case TMSG_RUNTASK:
			rfb_runtask(obj);
			break;
	}
	return 0;
}

TMODENTRY TUINT tek_init_display_rawfb(struct TTask *task,
	struct TModule *vis, TUINT16 version, TTAGITEM *tags)
{
	struct rfb_Display *mod = (struct rfb_Display *) vis;

	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * RFB_DISPLAY_NUMVECTORS;

		if (version <= RFB_DISPLAY_VERSION)
			return sizeof(struct rfb_Display);

		return 0;
	}

	for (;;)
	{
		TAPTR TExecBase = TGetExecBase(mod);

		if (FT_Init_FreeType(&mod->rfb_FTLibrary) != 0)
			break;
		if (FTC_Manager_New(mod->rfb_FTLibrary, 0, 0, 0, rfb_fontrequester,
				NULL, &mod->rfb_FTCManager) != 0)
			break;
		if (FTC_CMapCache_New(mod->rfb_FTCManager, &mod->rfb_FTCCMapCache)
			!= 0)
			break;
		if (FTC_SBitCache_New(mod->rfb_FTCManager, &mod->rfb_FTCSBitCache)
			!= 0)
			break;

		mod->rfb_ExecBase = TExecBase;
		mod->rfb_Lock = TCreateLock(TNULL);
		if (mod->rfb_Lock == TNULL)
			break;

		mod->rfb_Module.tmd_Version = RFB_DISPLAY_VERSION;
		mod->rfb_Module.tmd_Revision = RFB_DISPLAY_REVISION;
		mod->rfb_Module.tmd_Handle.thn_Hook.thk_Entry = rfb_dispatch;
		mod->rfb_Module.tmd_Flags = TMODF_VECTORTABLE | TMODF_OPENCLOSE;
		TInitVectors(&mod->rfb_Module, rfb_vectors, RFB_DISPLAY_NUMVECTORS);
		return TTRUE;
	}

	rfb_destroy(mod);
	return TFALSE;
}

/*****************************************************************************/

LOCAL TBOOL rfb_getlayers(struct rfb_Display *mod, struct Region *A,
	struct rfb_Window *v, TINT dx, TINT dy)
{
	struct RectPool *pool = &mod->rfb_RectPool;

	region_init(pool, A, TNULL);
	struct TNode *next, *node = mod->rfb_VisualList.tlh_Head.tln_Succ;

	for (; (next = node->tln_Succ); node = next)
	{
		struct rfb_Window *bv = (struct rfb_Window *) node;

		if (bv == v)
			break;
		if (!region_orrect(pool, A, bv->rfbw_ScreenRect.r, TFALSE))
		{
			region_free(pool, A);
			return TFALSE;
		}
	}
	if (dx || dy)
		region_shift(A, dx, dy);
	return TTRUE;
}

/*****************************************************************************/

LOCAL TBOOL rfb_getlayermask(struct rfb_Display *mod, struct Region *A,
	TINT *crect, struct rfb_Window *v, TINT dx, TINT dy)
{
	if (crect[0] < 0)
		return TFALSE;
	struct RectPool *pool = &mod->rfb_RectPool;

	if (!region_init(pool, A, crect))
		return TFALSE;
	if (v->rfbw_Flags & RFBWFL_BACKBUFFER)
		return TTRUE;
	TBOOL success = TFALSE;
	struct Region L;

	if (rfb_getlayers(mod, &L, v, dx, dy))
	{
		success = region_subregion(pool, A, &L);
		region_free(pool, &L);
	}
	if (!success)
		region_free(pool, A);
	return success;
}

/*****************************************************************************/
/*
**	damage window stack with rectangle; if window is not NULL, the damage
**	starts below that window in the stack
*/

LOCAL TBOOL rfb_damage(struct rfb_Display *mod, TINT drect[],
	struct rfb_Window *v)
{
	TAPTR TExecBase = TGetExecBase(mod);
	struct RectPool *pool = &mod->rfb_RectPool;
	struct Region A, B;

	if (!region_init(pool, &A, drect))
		return TFALSE;

	TDBPRINTF(TDB_TRACE, ("incoming damage: %d %d %d %d\n",
			drect[0], drect[1], drect[2], drect[3]));

	/* traverse window stack; refresh B where A and B overlap ; A = A - B */

	struct TNode *next, *node = mod->rfb_VisualList.tlh_Head.tln_Succ;
	TBOOL success = TTRUE;
	TBOOL below = TFALSE;

	TLock(mod->rfb_InstanceLock);

	for (; success && !region_isempty(pool, &A) &&
		(next = node->tln_Succ); node = next)
	{
		struct rfb_Window *bv = (struct rfb_Window *) node;

		if (v && !below)
		{
			if (bv == v)
				below = TTRUE;
			else
				/* above: subtract current from rect to be damaged: */
				success = region_subrect(pool, &A, bv->rfbw_ScreenRect.r);
			continue;
		}

		success = TFALSE;
		if (region_init(pool, &B, bv->rfbw_ScreenRect.r))
		{
			if (region_andregion(pool, &B, &A))
			{
				region_shift(&B,
					bv->rfbw_WinRect.r[0] - bv->rfbw_ScreenRect.r[0],
					bv->rfbw_WinRect.r[1] - bv->rfbw_ScreenRect.r[1]);

				struct TNode *next, 
					*node = B.rg_Rects.rl_List.tlh_Head.tln_Succ;

				for (; (next = node->tln_Succ); node = next)
				{
					struct RectNode *r = (struct RectNode *) node;

					if (bv->rfbw_Flags & RFBWFL_BACKBUFFER)
					{
						rfb_markdirty(mod, bv, r->rn_Rect);
					}
					else if (bv->rfbw_InputMask & TITYPE_REFRESH)
					{
						TIMSG *imsg;

						if (rfb_getimsg(mod, bv, &imsg, TITYPE_REFRESH))
						{
							TDBPRINTF(TDB_TRACE, ("send refresh %d %d %d %d\n",
									r->rn_Rect[0], r->rn_Rect[1],
									r->rn_Rect[2], r->rn_Rect[3]));
							imsg->timsg_X = r->rn_Rect[0];
							imsg->timsg_Y = r->rn_Rect[1];
							imsg->timsg_Width =
								r->rn_Rect[2] - r->rn_Rect[0] + 1;
							imsg->timsg_Height =
								r->rn_Rect[3] - r->rn_Rect[1] + 1;
							imsg->timsg_X -= bv->rfbw_ScreenRect.r[0];
							imsg->timsg_Y -= bv->rfbw_ScreenRect.r[1];
							TPutMsg(bv->rfbw_IMsgPort, TNULL, imsg);
						}
					}
				}
				success = TTRUE;
			}
			region_free(pool, &B);
		}

		if (success)
			success = region_subrect(pool, &A, bv->rfbw_ScreenRect.r);
	}

	TUnlock(mod->rfb_InstanceLock);

	region_free(pool, &A);

	return success;
}

/*****************************************************************************/

LOCAL void rfb_markdirty(struct rfb_Display *mod, struct rfb_Window *v,
	TINT *r)
{
	if (r[2] < r[0] || r[3] < r[1])
		return;

	TINT d[4];
	TUINT align = RFB_DIRTY_ALIGN;

	d[0] = r[0] & ~align;
	d[1] = r[1] & ~align;
	d[2] = (r[2] & ~align) + align;
	d[3] = (r[3] & ~align) + align;

	if (region_intersect(d, v->rfbw_WinRect.r))
	{
		TBOOL winbackbuffer = v->rfbw_Flags & RFBWFL_BACKBUFFER;
		TBOOL res = TTRUE;

		if (!winbackbuffer)
		{
			struct Rect s;

			REGION_RECT_SET(&s, 0, 0, mod->rfb_Width - 1, mod->rfb_Height - 1);
			res = region_intersect(d, s.r);
		}
		if (res)
		{
			region_orrect(&mod->rfb_RectPool,
				winbackbuffer ? &v->rfbw_DirtyRegion : &mod->rfb_DirtyRegion,
				d, TTRUE);
			if (winbackbuffer)
				v->rfbw_Flags |= RFBWFL_DIRTY;
			else
				mod->rfb_Flags |= RFBFL_DIRTY;
		}
	}
}

/*****************************************************************************/

LOCAL void rfb_setwinrect(struct rfb_Display *mod, struct rfb_Window *v)
{
	TINT x = 0, y = 0;
	TINT w = REGION_RECT_WIDTH(&v->rfbw_ScreenRect);
	TINT h = REGION_RECT_HEIGHT(&v->rfbw_ScreenRect);

	if (!(v->rfbw_Flags & RFBWFL_BACKBUFFER))
	{
		x = v->rfbw_ScreenRect.r[0];
		y = v->rfbw_ScreenRect.r[1];
	}
	REGION_RECT_SET(&v->rfbw_WinRect, x, y, x + w - 1, y + h - 1);
}

/*****************************************************************************/

LOCAL void rfb_setrealcliprect(struct rfb_Display *mod, struct rfb_Window *v)
{
	v->rfbw_ClipRect = v->rfbw_UserClipRect;
	TBOOL res = region_intersect(v->rfbw_ClipRect.r, v->rfbw_WinRect.r);

	if (!(v->rfbw_Flags & RFBWFL_BACKBUFFER))
	{
		struct Rect s;

		REGION_RECT_SET(&s, 0, 0, mod->rfb_Width - 1, mod->rfb_Height - 1);
		if (!region_intersect(v->rfbw_ClipRect.r, s.r))
			res = TFALSE;
	}
	if (!res)
		v->rfbw_ClipRect.r[0] = -1;
}

/*****************************************************************************/

LOCAL void rfb_focuswindow(struct rfb_Display *mod, struct rfb_Window *v)
{
	TAPTR TExecBase = TGetExecBase(mod);
	TIMSG *imsg;

	if (v == mod->rfb_FocusWindow || (v && (v->rfbw_Flags & RFBWFL_IS_POPUP)))
		return;

	if (mod->rfb_FocusWindow)
	{
		if (rfb_getimsg(mod, mod->rfb_FocusWindow, &imsg, TITYPE_FOCUS))
		{
			imsg->timsg_Code = 0;
			TPutMsg(mod->rfb_FocusWindow->rfbw_IMsgPort, TNULL, imsg);
		}
	}

	if (v)
	{
		if (rfb_getimsg(mod, v, &imsg, TITYPE_FOCUS))
		{
			imsg->timsg_Code = 1;
			TPutMsg(v->rfbw_IMsgPort, TNULL, imsg);
		}
	}

	mod->rfb_FocusWindow = v;
}

/*****************************************************************************/

LOCAL void rfb_flush_clients(struct rfb_Display *mod, TBOOL also_external)
{
	TAPTR TExecBase = mod->rfb_ExecBase;
	struct RectPool *pool = &mod->rfb_RectPool;

	/* flush windows to buffer */

	/* screen mask: */
	struct Rect s;

	REGION_RECT_SET(&s, 0, 0, mod->rfb_Width - 1, mod->rfb_Height - 1);
	struct Region S;

	if (region_init(pool, &S, s.r))
	{
		struct TNode *next, *node = mod->rfb_VisualList.tlh_Head.tln_Succ;

		for (; (next = node->tln_Succ); node = next)
		{
			struct rfb_Window *v = (struct rfb_Window *) node;

			if (v->rfbw_Flags & RFBWFL_DIRTY)
			{
				struct Region *R = &v->rfbw_DirtyRegion;
				TINT sx = v->rfbw_ScreenRect.r[0];
				TINT sy = v->rfbw_ScreenRect.r[1];

				region_shift(R, sx, sy);
				region_andregion(pool, R, &S);
				struct TNode *rnext, *rnode = 
					R->rg_Rects.rl_List.tlh_Head.tln_Succ;

				for (; (rnext = rnode->tln_Succ); rnode = rnext)
				{
					struct RectNode *r = (struct RectNode *) rnode;
					TINT x0 = r->rn_Rect[0];
					TINT y0 = r->rn_Rect[1];
					TINT x1 = r->rn_Rect[2];
					TINT y1 = r->rn_Rect[3];

					pixconv_convert(&v->rfbw_PixBuf, &mod->rfb_PixBuf,
						x0, y0, x1, y1, x0 - sx, y0 - sy, TFALSE, TFALSE);

					region_orrect(pool, &mod->rfb_DirtyRegion, r->rn_Rect,
						TTRUE);
					mod->rfb_Flags |= RFBFL_DIRTY;
				}
				region_free(pool, R);
				v->rfbw_Flags &= ~RFBWFL_DIRTY;
			}
			/* subtract this window from screenmask: */
			region_subrect(pool, &S, v->rfbw_ScreenRect.r);
		}
		region_free(pool, &S);
	}

	/* flush buffer to device(s) */
	if (mod->rfb_Flags & RFBFL_DIRTY)
	{
		struct Region *D = &mod->rfb_DirtyRegion;
		struct TNode *next, *node;

#if defined(ENABLE_VNCSERVER)
		if (also_external && mod->rfb_VNCTask)
			rfb_vnc_flush(mod, D);
#endif
		/* flush to sub pixbuf: */
		if (mod->rfb_Flags & RFBFL_BUFFER_DEVICE)
		{
			node = D->rg_Rects.rl_List.tlh_Head.tln_Succ;
			for (; (next = node->tln_Succ); node = next)
			{
				struct RectNode *r = (struct RectNode *) node;
				TINT x0 = r->rn_Rect[0];
				TINT y0 = r->rn_Rect[1];
				TINT x1 = r->rn_Rect[2];
				TINT y1 = r->rn_Rect[3];

				pixconv_convert(&mod->rfb_PixBuf, &mod->rfb_DevBuf,
					x0, y0, x1, y1, x0, y0, TFALSE, TFALSE);
			}
		}

		/* flush to sub device: */
		if (mod->rfb_RndDevice)
		{
			TTAGITEM tags[2];

			tags[0].tti_Tag = TVisual_PixelFormat;
			tags[0].tti_Value = mod->rfb_PixBuf.tpb_Format;
			tags[1].tti_Tag = TTAG_DONE;

			/* NOTE: do multiple flushes asynchronously? */

			struct TVRequest *req = mod->rfb_RndRequest;

			req->tvr_Req.io_Command = TVCMD_DRAWBUFFER;
			req->tvr_Op.DrawBuffer.Window = mod->rfb_RndInstance;
			req->tvr_Op.DrawBuffer.Tags = tags;
			req->tvr_Op.DrawBuffer.TotWidth = mod->rfb_Width;

			node = D->rg_Rects.rl_List.tlh_Head.tln_Succ;
			for (; (next = node->tln_Succ); node = next)
			{
				struct RectNode *r = (struct RectNode *) node;
				TINT x0 = r->rn_Rect[0];
				TINT y0 = r->rn_Rect[1];
				TINT x1 = r->rn_Rect[2];
				TINT y1 = r->rn_Rect[3];

				req->tvr_Op.DrawBuffer.RRect[0] = x0;
				req->tvr_Op.DrawBuffer.RRect[1] = y0;
				req->tvr_Op.DrawBuffer.RRect[2] = x1 - x0 + 1;
				req->tvr_Op.DrawBuffer.RRect[3] = y1 - y0 + 1;
				req->tvr_Op.DrawBuffer.Buf =
					TVPB_GETADDRESS(&mod->rfb_PixBuf, x0, y0);
				TDoIO(&req->tvr_Req);
			}

			req->tvr_Req.io_Command = TVCMD_FLUSH;
			req->tvr_Op.Flush.Window = mod->rfb_RndInstance;
			req->tvr_Op.Flush.Rect[0] = 0;
			req->tvr_Op.Flush.Rect[1] = 0;
			req->tvr_Op.Flush.Rect[2] = -1;
			req->tvr_Op.Flush.Rect[3] = -1;
			TDoIO(&req->tvr_Req);
		}

		region_free(&mod->rfb_RectPool, D);
		mod->rfb_Flags &= ~RFBFL_DIRTY;
	}
}

/*****************************************************************************/

LOCAL TBOOL rfb_resizewinbuffer(struct rfb_Display *mod, struct rfb_Window *v,
	TINT oldw, TINT oldh, TINT w, TINT h)
{
	if (oldw == w && oldh == h)
		return TTRUE;
	TAPTR TExecBase = TGetExecBase(mod);
	struct TVPixBuf newbuf = v->rfbw_PixBuf;
	TUINT pixfmt = v->rfbw_PixBuf.tpb_Format;

	newbuf.tpb_BytesPerLine = TVPIXFMT_BYTES_PER_PIXEL(pixfmt) * w;
	newbuf.tpb_Data = TAlloc(mod->rfb_MemMgr, newbuf.tpb_BytesPerLine * h);
	if (!newbuf.tpb_Data)
		return TFALSE;
	TINT cw = TMIN(oldw, w);
	TINT ch = TMIN(oldh, h);

	pixconv_convert(&v->rfbw_PixBuf, &newbuf,
		0, 0, cw - 1, ch - 1, 0, 0, TFALSE, TFALSE);
	TFree(v->rfbw_PixBuf.tpb_Data);
	v->rfbw_PixBuf = newbuf;
	return TTRUE;
}

/*****************************************************************************/

LOCAL void rfb_copyrect_sub(struct rfb_Display *mod, TINT *rect, TINT dx,
	TINT dy)
{
	if (mod->rfb_Flags & RFBFL_BUFFER_DEVICE)
	{
		/* NOTE: this is just a flush, not a copyrect */
		TINT x0 = rect[0];
		TINT y0 = rect[1];
		TINT x1 = rect[2];
		TINT y1 = rect[3];

		pixconv_convert(&mod->rfb_PixBuf, &mod->rfb_DevBuf,
			x0, y0, x1, y1, x0, y0, TFALSE, TFALSE);
	}

	if (mod->rfb_RndDevice)
	{
		TAPTR TExecBase = mod->rfb_ExecBase;
		struct TVRequest *req = mod->rfb_RndRequest;

		req->tvr_Req.io_Command = TVCMD_COPYAREA;
		TINT x0 = rect[0] - dx;
		TINT y0 = rect[1] - dy;
		TINT x1 = rect[2] - dx;
		TINT y1 = rect[3] - dy;

		req->tvr_Op.CopyArea.Window = mod->rfb_RndInstance;
		req->tvr_Op.CopyArea.Rect[0] = x0;
		req->tvr_Op.CopyArea.Rect[1] = y0;
		req->tvr_Op.CopyArea.Rect[2] = x1 - x0 + 1;
		req->tvr_Op.CopyArea.Rect[3] = y1 - y0 + 1;
		req->tvr_Op.CopyArea.DestX = x0 + dx;
		req->tvr_Op.CopyArea.DestY = y0 + dy;
		req->tvr_Op.CopyArea.Tags = TNULL;
		TDoIO(&req->tvr_Req);
	}
}
