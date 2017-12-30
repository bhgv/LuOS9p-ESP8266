
/*
**	display_rfb_api.c - Raw framebuffer display driver
**	Written by Franciska Schulze <fschulze at schulze-mueller.de>
**	and Timm S. Mueller <tmueller at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <assert.h>
#include "display_rfb_mod.h"
#include <tek/inline/exec.h>
#if defined(RFB_PIXMAP_CACHE)
#include <tek/lib/imgcache.h>
#endif

struct extraopenargs
{
	TUINT pixfmt;
	TUINT vncportnr;
	TBOOL backbuffer;
	TBOOL winbackbuffer;
};

static TBOOL string2bool(const char *s)
{
	return !strcmp(s, "1") || !strcasecmp(s, "on") || !strcasecmp(s, "yes") ||
		!strcasecmp(s, "true");
}

static void getextraopenargs(TTAGITEM *tags, struct extraopenargs *args)
{
	TSTRPTR extraarg = (TSTRPTR) TGetTag(tags, TVisual_ExtraArgs, TNULL);

	memset(args, 0, sizeof *args);
	if (extraarg)
	{
		char buf[2048];
		char *s = buf, *ss, *t;
		char temp[5];

		strncpy(buf, extraarg, sizeof(buf) - 1);
		buf[sizeof(buf) - 1] = 0;
		while ((t = strtok_r(s, "\n", &ss)))
		{
			s = NULL;
			sscanf(t, " fb_pixfmt = %08x ", &args->pixfmt);
			sscanf(t, " vnc_portnumber = %d ", &args->vncportnr);
			if (sscanf(t, " fb_backbuffer = %4s ", temp))
				args->backbuffer = string2bool(temp);
			if (sscanf(t, " fb_winbackbuffer = %4s ", temp))
				args->winbackbuffer = string2bool(temp);
		}
	}
}

static void rfb_openvisual(struct rfb_Display *mod, struct TVRequest *req)
{
	TAPTR TExecBase = TGetExecBase(mod);
	TTAGITEM *tags = req->tvr_Op.OpenWindow.Tags;
	TINT width, height;
	struct rfb_Window *v;
	TIMSG *imsg;

	req->tvr_Op.OpenWindow.Window = TNULL;

	v = TAlloc0(mod->rfb_MemMgr, sizeof(struct rfb_Window));
	if (v == TNULL)
		return;

	TINT minw = (TINT) TGetTag(tags, TVisual_MinWidth, -1);
	TINT minh = (TINT) TGetTag(tags, TVisual_MinHeight, -1);
	TINT maxw = (TINT) TGetTag(tags, TVisual_MaxWidth, RFB_HUGE);
	TINT maxh = (TINT) TGetTag(tags, TVisual_MaxHeight, RFB_HUGE);
	TUINT flags = 0;

	if (TGetTag(tags, TVisual_Borderless, TFALSE))
		flags |= RFBWFL_BORDERLESS;
	if (TGetTag(tags, TVisual_PopupWindow, TFALSE))
		flags |= RFBWFL_IS_POPUP;

	if (TISLISTEMPTY(&mod->rfb_VisualList))
	{
		/* Open root window */
		flags |= RFBWFL_IS_ROOT;
		struct extraopenargs args;

		getextraopenargs(tags, &args);

#if !defined(ENABLE_WINBACKBUFFER)
		if (args.winbackbuffer)
#endif
		{
			TDBPRINTF(TDB_WARN, ("per-window backbuffers enabled\n"));
			mod->rfb_Flags |= RFBFL_WIN_BACKBUFFER;
		}

		if (TGetTag(tags, TVisual_BlankCursor, TFALSE))
			mod->rfb_Flags &= ~RFBFL_SHOWPTR;

		/* dimensions */
		width = (TINT) TGetTag(tags, TVisual_Width,
			minw > 0 ? TMAX(minw, mod->rfb_DevWidth) :
			TMAX(mod->rfb_Width, mod->rfb_DevWidth));
		height = (TINT) TGetTag(tags, TVisual_Height,
			minh > 0 ? TMAX(minh, mod->rfb_DevHeight) :
			TMAX(mod->rfb_Height, mod->rfb_DevHeight));
		mod->rfb_Width = width;
		mod->rfb_Height = height;

		/* determine pixel format */
		TUINT pixfmt = TVPIXFMT_08R8G8B8;

		if (mod->rfb_PixBuf.tpb_Format)
			pixfmt = mod->rfb_PixBuf.tpb_Format;
#if defined(RFBPIXFMT)
		pixfmt = RFBPIXFMT;
#endif
#if defined(ENABLE_VNCSERVER)
		if (pixfmt != TVPIXFMT_08B8G8R8 && pixfmt != TVPIXFMT_0B5G5R5)
			pixfmt = TVPIXFMT_BYTES_PER_PIXEL(pixfmt) == 4 ?
				TVPIXFMT_08B8G8R8 : TVPIXFMT_0B5G5R5;
#endif
		if (args.pixfmt)
			pixfmt = args.pixfmt;
		pixfmt = TGetTag(tags, TVisual_PixelFormat, pixfmt);

		if (mod->rfb_PixBuf.tpb_Data == TNULL)
		{
			/* no buffer allocated yet; get one */
			TUINT modulo = TGetTag(tags, TVisual_Modulo, 0);
			TUINT bpp = TVPIXFMT_BYTES_PER_PIXEL(pixfmt);

			mod->rfb_PixBuf.tpb_Format = pixfmt;
			mod->rfb_PixBuf.tpb_BytesPerLine = (width + modulo) * bpp;
			mod->rfb_PixBuf.tpb_Data = (TUINT8 *) TGetTag(tags, TVisual_BufPtr,
				TNULL);
			if (mod->rfb_PixBuf.tpb_Data == TNULL)
			{
				mod->rfb_PixBuf.tpb_Data = TAlloc0(mod->rfb_MemMgr,
					mod->rfb_PixBuf.tpb_BytesPerLine * mod->rfb_Height);
				if (mod->rfb_PixBuf.tpb_Data == TNULL)
					return;
				/* we own the buffer: */
				mod->rfb_Flags |= RFBFL_BUFFER_OWNER;
			}
		}
		else
#if !defined(ENABLE_BACKBUFFER)
		if (pixfmt != mod->rfb_PixBuf.tpb_Format || args.backbuffer)
#endif
		{
			/* there is a buffer already, but an extra backbuffer was
			   requested, or the formats do not match */
			TDBPRINTF(TDB_WARN, ("Using backbuffer, format %08x\n", pixfmt));
			TUINT bpp = TVPIXFMT_BYTES_PER_PIXEL(pixfmt);

			mod->rfb_PixBuf.tpb_Format = pixfmt;
			mod->rfb_PixBuf.tpb_BytesPerLine = bpp * mod->rfb_Width;
			mod->rfb_PixBuf.tpb_Data = TAlloc0(mod->rfb_MemMgr,
				mod->rfb_Width * mod->rfb_Height * bpp);
			if (mod->rfb_PixBuf.tpb_Data == TNULL)
				return;
			mod->rfb_Flags |= RFBFL_BUFFER_OWNER | RFBFL_BUFFER_DEVICE;
		}

		rfb_initpointer(mod);

#if defined(ENABLE_VNCSERVER)
		rfb_vnc_init(mod, args.vncportnr);
#endif

		/* Open rendering instance: */
		if (mod->rfb_RndDevice)
		{
			TTAGITEM subtags[4];

			/* we are proxy, want all input, except intervals: */
			subtags[0].tti_Tag = TVisual_EventMask;
			subtags[0].tti_Value = TITYPE_ALL & ~TITYPE_INTERVAL;
			subtags[1].tti_Tag = TVisual_Width;
			subtags[1].tti_Value = width;
			subtags[2].tti_Tag = TVisual_Height;
			subtags[2].tti_Value = height;
			subtags[3].tti_Tag = TTAG_MORE;
			subtags[3].tti_Value = (TTAG) tags;
			mod->rfb_RndRequest->tvr_Req.io_Device = mod->rfb_RndDevice;
			mod->rfb_RndRequest->tvr_Req.io_Command = TVCMD_OPENWINDOW;
			mod->rfb_RndRequest->tvr_Req.io_ReplyPort = mod->rfb_RndRPort;
			mod->rfb_RndRequest->tvr_Op.OpenWindow.Window = TNULL;
			mod->rfb_RndRequest->tvr_Op.OpenWindow.Tags = subtags;
			/* place our own ("proxy") messageport inbetween */
			mod->rfb_RndRequest->tvr_Op.OpenWindow.IMsgPort =
				mod->rfb_RndIMsgPort;
			TDoIO(&mod->rfb_RndRequest->tvr_Req);
			mod->rfb_RndInstance =
				mod->rfb_RndRequest->tvr_Op.OpenWindow.Window;
			flags |= RFBWFL_FULLSCREEN;
		}
	}
	else
	{
		/* Not root window: */

		width = (TINT) TGetTag(tags, TVisual_Width, RFB_DEF_WIDTH);
		height = (TINT) TGetTag(tags, TVisual_Height, RFB_DEF_HEIGHT);

		width = TCLAMP(minw, width, maxw);
		height = TCLAMP(minh, height, maxh);

		if (TGetTag(tags, TVisual_FullScreen, TFALSE))
		{
			flags |= RFBWFL_FULLSCREEN;
			width = mod->rfb_Width;
			height = mod->rfb_Height;
		}
		else
		{
			TINT wx = (TINT) TGetTag(tags, TVisual_WinLeft, -1);
			TINT wy = (TINT) TGetTag(tags, TVisual_WinTop, -1);

			if (wx == -1)
				wx = (mod->rfb_Width - width) / 2;
			if (wy == -1)
				wy = (mod->rfb_Height - height) / 2;

			v->rfbw_ScreenRect.r[0] = wx;
			v->rfbw_ScreenRect.r[1] = wy;
		}
	}

	if (mod->rfb_Flags & RFBFL_WIN_BACKBUFFER)
		flags |= RFBWFL_BACKBUFFER;

	v->rfbw_Display = mod;
	v->rfbw_Flags = flags;

	v->rfbw_ScreenRect.r[2] = v->rfbw_ScreenRect.r[0] + width - 1;
	v->rfbw_ScreenRect.r[3] = v->rfbw_ScreenRect.r[1] + height - 1;
	rfb_setwinrect(mod, v);

	v->rfbw_UserClipRect = v->rfbw_WinRect;
	rfb_setrealcliprect(mod, v);

	v->rfbw_MinWidth = minw;
	v->rfbw_MinHeight = minh;
	v->rfbw_MaxWidth = maxw;
	v->rfbw_MaxHeight = maxh;

	v->rfbw_PixBuf.tpb_Format = mod->rfb_PixBuf.tpb_Format;
	if (flags & RFBWFL_BACKBUFFER)
	{
		TINT bpl = width * TVPIXFMT_BYTES_PER_PIXEL(v->rfbw_PixBuf.tpb_Format);
		v->rfbw_PixBuf.tpb_BytesPerLine = bpl;
		v->rfbw_PixBuf.tpb_Data = TAlloc(mod->rfb_MemMgr, bpl * height);
	}
	else
	{
		v->rfbw_PixBuf.tpb_BytesPerLine = mod->rfb_PixBuf.tpb_BytesPerLine;
		v->rfbw_PixBuf.tpb_Data = mod->rfb_PixBuf.tpb_Data;
	}

	v->rfbw_InputMask = (TUINT) TGetTag(tags, TVisual_EventMask, 0);
	v->rfbw_UserData = TGetTag(tags, TVisual_UserData, TNULL);

	v->rfbw_IMsgPort = req->tvr_Op.OpenWindow.IMsgPort;

	TInitList(&v->rfbw_PenList);
	v->rfbw_BGPen = TVPEN_UNDEFINED;
	v->rfbw_FGPen = TVPEN_UNDEFINED;

	region_init(&mod->rfb_RectPool, &v->rfbw_DirtyRegion, TNULL);

	/* add window on top of window stack: */
	TLock(mod->rfb_Lock);
	TAddHead(&mod->rfb_VisualList, &v->rfbw_Node);
	rfb_focuswindow(mod, v);
	if (v->rfbw_InputMask & TITYPE_INTERVAL)
		mod->rfb_NumInterval++;
	TUnlock(mod->rfb_Lock);

	/* Reply instance: */
	req->tvr_Op.OpenWindow.Window = v;

	/* send refresh message: */
	if (rfb_getimsg(mod, v, &imsg, TITYPE_REFRESH))
	{
		imsg->timsg_X = 0;
		imsg->timsg_Y = 0;
		imsg->timsg_Width = width;
		imsg->timsg_Height = height;
		TPutMsg(v->rfbw_IMsgPort, TNULL, imsg);
	}
}

/*****************************************************************************/

static void rfb_closevisual(struct rfb_Display *mod, struct TVRequest *req)
{
	TAPTR TExecBase = TGetExecBase(mod);
	struct rfb_Pen *pen;
	struct rfb_Window *v = req->tvr_Op.CloseWindow.Window;

	if (v == TNULL)
		return;

	/*rfb_focuswindow(mod, v, TFALSE); */

	rfb_damage(mod, v->rfbw_ScreenRect.r, v);

	TLock(mod->rfb_Lock);

	TBOOL had_focus = mod->rfb_FocusWindow == v;

	if (had_focus)
		mod->rfb_FocusWindow = TNULL;

	TRemove(&v->rfbw_Node);

	if (v->rfbw_InputMask & TITYPE_INTERVAL)
		mod->rfb_NumInterval--;

	if (v->rfbw_Flags & RFBWFL_BACKBUFFER)
		TFree(v->rfbw_PixBuf.tpb_Data);

	while ((pen = (struct rfb_Pen *) TRemHead(&v->rfbw_PenList)))
	{
		/* free pens */
		TRemove(&pen->node);
		TFree(pen);
	}

	if (TISLISTEMPTY(&mod->rfb_VisualList))
	{
		/* last window: */
		if (mod->rfb_RndDevice)
		{
			mod->rfb_RndRequest->tvr_Req.io_Command = TVCMD_CLOSEWINDOW;
			mod->rfb_RndRequest->tvr_Op.CloseWindow.Window =
				mod->rfb_RndInstance;
			TDoIO(&mod->rfb_RndRequest->tvr_Req);
		}
#if defined(ENABLE_VNCSERVER)
		rfb_vnc_exit(mod);
#endif
	}
	else if (had_focus)
		rfb_focuswindow(mod,
			(struct rfb_Window *) TFIRSTNODE(&mod->rfb_VisualList));

	TUnlock(mod->rfb_Lock);

	region_free(&mod->rfb_RectPool, &v->rfbw_DirtyRegion);

	TFree(v);

	rfb_flush_clients(mod, TTRUE);
}

/*****************************************************************************/

static void rfb_setinput(struct rfb_Display *mod, struct TVRequest *req)
{
	struct rfb_Window *v = req->tvr_Op.SetInput.Window;
	TUINT oldmask = v->rfbw_InputMask;
	TUINT newmask = req->tvr_Op.SetInput.Mask;

	if (oldmask & TITYPE_INTERVAL)
		mod->rfb_NumInterval--;
	if (newmask & TITYPE_INTERVAL)
		mod->rfb_NumInterval++;
	req->tvr_Op.SetInput.OldMask = oldmask;
	v->rfbw_InputMask = newmask;
}

/*****************************************************************************/

static void rfb_flush(struct rfb_Display *mod, struct TVRequest *req)
{
	rfb_flush_clients(mod, TTRUE);
}

/*****************************************************************************/

static void rfb_setbgpen(struct rfb_Display *mod, struct rfb_Window *v,
	TVPEN pen)
{
	if (pen != v->rfbw_BGPen && pen != TVPEN_UNDEFINED)
		v->rfbw_BGPen = pen;
}

static TVPEN rfb_setfgpen(struct rfb_Display *mod, struct rfb_Window *v,
	TVPEN pen)
{
	TVPEN oldpen = v->rfbw_FGPen;

	if (pen != oldpen && pen != TVPEN_UNDEFINED)
	{
		v->rfbw_FGPen = pen;
		if (oldpen == TVPEN_UNDEFINED)
			oldpen = pen;
	}
	return oldpen;
}

/*****************************************************************************/

static void rfb_allocpen(struct rfb_Display *mod, struct TVRequest *req)
{
	TAPTR TExecBase = TGetExecBase(mod);
	struct rfb_Window *v = req->tvr_Op.AllocPen.Window;
	TUINT rgb = req->tvr_Op.AllocPen.RGB & 0xffffff;
	struct rfb_Pen *pen = TAlloc(mod->rfb_MemMgr, sizeof(struct rfb_Pen));

	if (pen)
	{
		pen->rgb = rgb;
		TAddTail(&v->rfbw_PenList, &pen->node);
		req->tvr_Op.AllocPen.Pen = (TVPEN) pen;
		return;
	}
	req->tvr_Op.AllocPen.Pen = TVPEN_UNDEFINED;
}

/*****************************************************************************/

static void rfb_freepen(struct rfb_Display *mod, struct TVRequest *req)
{
	TAPTR TExecBase = TGetExecBase(mod);
	struct rfb_Pen *pen = (struct rfb_Pen *) req->tvr_Op.FreePen.Pen;

	TRemove(&pen->node);
	TFree(pen);
}

/*****************************************************************************/

static void rfb_frect(struct rfb_Display *mod, struct TVRequest *req)
{
	struct rfb_Window *v = req->tvr_Op.FRect.Window;
	TINT rect[4];
	struct rfb_Pen *pen = (struct rfb_Pen *) req->tvr_Op.FRect.Pen;

	rfb_setfgpen(mod, v, req->tvr_Op.FRect.Pen);
	rect[0] = req->tvr_Op.FRect.Rect[0] + v->rfbw_WinRect.r[0];
	rect[1] = req->tvr_Op.FRect.Rect[1] + v->rfbw_WinRect.r[1];
	rect[2] = rect[0] + req->tvr_Op.FRect.Rect[2] - 1;
	rect[3] = rect[1] + req->tvr_Op.FRect.Rect[3] - 1;
	fbp_drawfrect(mod, v, rect, pen);
}

/*****************************************************************************/

static void rfb_line(struct rfb_Display *mod, struct TVRequest *req)
{
	struct rfb_Window *v = req->tvr_Op.Line.Window;
	struct rfb_Pen *pen = (struct rfb_Pen *) req->tvr_Op.Line.Pen;

	rfb_setfgpen(mod, v, req->tvr_Op.Line.Pen);
	TINT rect[4];

	rect[0] = req->tvr_Op.Line.Rect[0] + v->rfbw_WinRect.r[0];
	rect[1] = req->tvr_Op.Line.Rect[1] + v->rfbw_WinRect.r[1];
	rect[2] = req->tvr_Op.Line.Rect[2] + v->rfbw_WinRect.r[0];
	rect[3] = req->tvr_Op.Line.Rect[3] + v->rfbw_WinRect.r[1];
	fbp_drawline(mod, v, rect, pen);
}

/*****************************************************************************/

static void rfb_rect(struct rfb_Display *mod, struct TVRequest *req)
{
	struct rfb_Window *v = req->tvr_Op.Rect.Window;
	struct rfb_Pen *pen = (struct rfb_Pen *) req->tvr_Op.Rect.Pen;

	rfb_setfgpen(mod, v, req->tvr_Op.Rect.Pen);
	TINT rect[4];

	rect[0] = req->tvr_Op.Rect.Rect[0] + v->rfbw_WinRect.r[0];
	rect[1] = req->tvr_Op.Rect.Rect[1] + v->rfbw_WinRect.r[1];
	rect[2] = rect[0] + req->tvr_Op.Rect.Rect[2] - 1;
	rect[3] = rect[1] + req->tvr_Op.Rect.Rect[3] - 1;
	fbp_drawrect(mod, v, rect, pen);
}

/*****************************************************************************/

static void rfb_plot(struct rfb_Display *mod, struct TVRequest *req)
{
	struct rfb_Window *v = req->tvr_Op.Plot.Window;
	struct rfb_Pen *pen = (struct rfb_Pen *) req->tvr_Op.Rect.Pen;

	rfb_setfgpen(mod, v, req->tvr_Op.Plot.Pen);
	TINT rect[4];

	rect[0] = req->tvr_Op.Rect.Rect[0] + v->rfbw_WinRect.r[0];
	rect[1] = req->tvr_Op.Rect.Rect[1] + v->rfbw_WinRect.r[1];
	rect[2] = rect[0];
	rect[3] = rect[1];
	fbp_drawrect(mod, v, rect, pen);
}

/*****************************************************************************/

static void rfb_drawstrip(struct rfb_Display *mod, struct TVRequest *req)
{
	TINT i, x0, y0, x1, y1, x2, y2;
	struct rfb_Window *v = req->tvr_Op.Strip.Window;
	TINT *array = req->tvr_Op.Strip.Array;
	TINT num = req->tvr_Op.Strip.Num;
	TTAGITEM *tags = req->tvr_Op.Strip.Tags;
	TVPEN pen = (TVPEN) TGetTag(tags, TVisual_Pen, TVPEN_UNDEFINED);
	TVPEN *penarray = (TVPEN *) TGetTag(tags, TVisual_PenArray, TNULL);
	TINT wx = v->rfbw_WinRect.r[0];
	TINT wy = v->rfbw_WinRect.r[1];

	if (num < 3)
		return;

	if (penarray)
		rfb_setfgpen(mod, v, penarray[2]);
	else
		rfb_setfgpen(mod, v, pen);

	x0 = array[0] + wx;
	y0 = array[1] + wy;
	x1 = array[2] + wx;
	y1 = array[3] + wy;
	x2 = array[4] + wx;
	y2 = array[5] + wy;

	fbp_drawtriangle(mod, v, x0, y0, x1, y1, x2, y2,
		(struct rfb_Pen *) v->rfbw_FGPen);

	for (i = 3; i < num; i++)
	{
		x0 = x1;
		y0 = y1;
		x1 = x2;
		y1 = y2;
		x2 = array[i * 2] + wx;
		y2 = array[i * 2 + 1] + wy;

		if (penarray)
			rfb_setfgpen(mod, v, penarray[i]);

		fbp_drawtriangle(mod, v, x0, y0, x1, y1, x2, y2,
			(struct rfb_Pen *) v->rfbw_FGPen);
	}
}

/*****************************************************************************/

static void rfb_drawfan(struct rfb_Display *mod, struct TVRequest *req)
{
	TINT i, x0, y0, x1, y1, x2, y2;
	struct rfb_Window *v = req->tvr_Op.Fan.Window;
	TINT *array = req->tvr_Op.Fan.Array;
	TINT num = req->tvr_Op.Fan.Num;
	TTAGITEM *tags = req->tvr_Op.Fan.Tags;
	TVPEN pen = (TVPEN) TGetTag(tags, TVisual_Pen, TVPEN_UNDEFINED);
	TVPEN *penarray = (TVPEN *) TGetTag(tags, TVisual_PenArray, TNULL);
	TINT wx = v->rfbw_WinRect.r[0];
	TINT wy = v->rfbw_WinRect.r[1];

	if (num < 3)
		return;

	if (penarray)
		rfb_setfgpen(mod, v, penarray[2]);
	else
		rfb_setfgpen(mod, v, pen);

	x0 = array[0] + wx;
	y0 = array[1] + wy;
	x1 = array[2] + wx;
	y1 = array[3] + wy;
	x2 = array[4] + wx;
	y2 = array[5] + wy;

	fbp_drawtriangle(mod, v, x0, y0, x1, y1, x2, y2,
		(struct rfb_Pen *) v->rfbw_FGPen);

	for (i = 3; i < num; i++)
	{
		x1 = x2;
		y1 = y2;
		x2 = array[i * 2] + wx;
		y2 = array[i * 2 + 1] + wy;

		if (penarray)
			rfb_setfgpen(mod, v, penarray[i]);

		fbp_drawtriangle(mod, v, x0, y0, x1, y1, x2, y2,
			(struct rfb_Pen *) v->rfbw_FGPen);
	}
}

/*****************************************************************************/

static void rfb_copyarea(struct rfb_Display *mod, struct TVRequest *req)
{
	struct rfb_Window *v = req->tvr_Op.CopyArea.Window;
	TINT dx = req->tvr_Op.CopyArea.DestX;
	TINT dy = req->tvr_Op.CopyArea.DestY;
	struct THook *exposehook = (struct THook *)
		TGetTag(req->tvr_Op.CopyArea.Tags, TVisual_ExposeHook, TNULL);
	TINT s[4];
	TINT *w = v->rfbw_WinRect.r;
	TINT *c = req->tvr_Op.CopyArea.Rect;

	dx -= c[0];
	dy -= c[1];
	s[0] = c[0] + w[0];
	s[1] = c[1] + w[1];
	s[2] = c[2] + s[0] - 1;
	s[3] = c[3] + s[1] - 1;

	TINT old[4];

	memcpy(old, s, sizeof(s));

	if (region_intersect(s, v->rfbw_ClipRect.r))
	{
		s[0] += dx;
		s[1] += dy;
		s[2] += dx;
		s[3] += dy;
		if (fbp_copyarea(mod, v, dx, dy, s, exposehook) && exposehook)
		{
			/* also expose regions coming from outside the screen */
			struct RectPool *pool = &mod->rfb_RectPool;
			struct Region R2;

			if (region_init(pool, &R2, old))
			{
				struct Rect scr;

				REGION_RECT_SET(&scr, 0, 0, mod->rfb_Width - 1,
					mod->rfb_Height - 1);
				region_subrect(pool, &R2, scr.r);
				region_shift(&R2, dx, dy);
				region_andrect(pool, &R2, scr.r, 0, 0);
				fbp_doexpose(mod, v, &R2, exposehook);
				region_free(pool, &R2);
			}
		}
	}
}

/*****************************************************************************/

static void rfb_setcliprect(struct rfb_Display *mod, struct TVRequest *req)
{
	struct rfb_Window *v = req->tvr_Op.ClipRect.Window;
	TINT x0 = req->tvr_Op.ClipRect.Rect[0] + v->rfbw_WinRect.r[0];
	TINT y0 = req->tvr_Op.ClipRect.Rect[1] + v->rfbw_WinRect.r[1];
	TINT x1 = x0 + req->tvr_Op.ClipRect.Rect[2] - 1;
	TINT y1 = y0 + req->tvr_Op.ClipRect.Rect[3] - 1;

	REGION_RECT_SET(&v->rfbw_UserClipRect, x0, y0, x1, y1);
	v->rfbw_Flags |= RFBWFL_USERCLIP;
	rfb_setrealcliprect(mod, v);
}

/*****************************************************************************/

static void rfb_unsetcliprect(struct rfb_Display *mod, struct TVRequest *req)
{
	struct rfb_Window *v = req->tvr_Op.ClipRect.Window;

	v->rfbw_UserClipRect = v->rfbw_WinRect;
	v->rfbw_Flags &= ~RFBWFL_USERCLIP;
	rfb_setrealcliprect(mod, v);
}

/*****************************************************************************/

static void rfb_clear(struct rfb_Display *mod, struct TVRequest *req)
{
	struct rfb_Window *v = req->tvr_Op.Clear.Window;
	struct rfb_Pen *pen = (struct rfb_Pen *) req->tvr_Op.Clear.Pen;

	rfb_setfgpen(mod, v, req->tvr_Op.Clear.Pen);
	fbp_drawfrect(mod, v, v->rfbw_WinRect.r, pen);
}

/*****************************************************************************/

static void rfb_drawbuffer(struct rfb_Display *mod, struct TVRequest *req)
{
	struct TVPixBuf src;
	TTAGITEM *tags = req->tvr_Op.DrawBuffer.Tags;
	struct rfb_Window *v = req->tvr_Op.DrawBuffer.Window;
	TINT x = req->tvr_Op.DrawBuffer.RRect[0];
	TINT y = req->tvr_Op.DrawBuffer.RRect[1];
	TINT w = req->tvr_Op.DrawBuffer.RRect[2];
	TINT h = req->tvr_Op.DrawBuffer.RRect[3];

	src.tpb_Data = req->tvr_Op.DrawBuffer.Buf;
	src.tpb_Format = TGetTag(tags, TVisual_PixelFormat, TVPIXFMT_A8R8G8B8);
	src.tpb_BytesPerLine = req->tvr_Op.DrawBuffer.TotWidth *
		TVPIXFMT_BYTES_PER_PIXEL(src.tpb_Format);
	TBOOL alpha = TGetTag(tags, TVisual_AlphaChannel, TFALSE);

#if defined(RFB_PIXMAP_CACHE)
	struct TVImageCacheRequest *creq = (struct TVImageCacheRequest *)
		TGetTag(tags, TVisual_CacheRequest, TNULL);
	if (creq)
	{
		struct ImageCacheState cstate;

		cstate.src = src;
		cstate.dst.tpb_Format = alpha ?
			src.tpb_Format : mod->rfb_PixBuf.tpb_Format;
		cstate.convert = pixconv_convert;
		int res = imgcache_lookup(&cstate, creq, x, y, w, h);

		if (res != TVIMGCACHE_FOUND && src.tpb_Data != TNULL)
			res = imgcache_store(&cstate, creq);
		if (res == TVIMGCACHE_FOUND || res == TVIMGCACHE_STORED)
			src = cstate.dst;
	}
#endif

	if (!src.tpb_Data)
		return;

	TINT rect[4];

	rect[0] = x + v->rfbw_WinRect.r[0];
	rect[1] = y + v->rfbw_WinRect.r[1];
	rect[2] = rect[0] + w - 1;
	rect[3] = rect[1] + h - 1;
	fbp_drawbuffer(mod, v, &src, rect, alpha);
}

/*****************************************************************************/

static void rbp_move_expose(struct rfb_Display *mod, struct rfb_Window *v,
	struct rfb_Window *predv, TINT dx, TINT dy)
{
	struct RectPool *pool = &mod->rfb_RectPool;
	struct Region S, L;

	if (rfb_getlayers(mod, &S, v, 0, 0))
	{
		if (rfb_getlayers(mod, &L, v, dx, dy))
		{
			if (region_subregion(pool, &L, &S))
			{
				if (region_andrect(pool, &L, v->rfbw_ScreenRect.r, 0, 0))
				{
					struct TNode *next, *node = 
						L.rg_Rects.rl_List.tlh_Head.tln_Succ;

					for (; (next = node->tln_Succ); node = next)
					{
						struct RectNode *rn = (struct RectNode *) node;

						rfb_damage(mod, rn->rn_Rect, predv);
					}
				}
			}
			region_free(pool, &L);
		}
		region_free(pool, &S);
	}
}

static void rfb_movewindow(struct rfb_Display *mod, struct rfb_Window *v,
	TINT x, TINT y)
{
	TINT *sr = v->rfbw_ScreenRect.r;
	TINT dx = x - sr[0];
	TINT dy = y - sr[1];

	if (dx == 0 && dy == 0)
		return;
	if (&v->rfbw_Node == TLASTNODE(&mod->rfb_VisualList))
		return;

	struct Rect old = v->rfbw_ScreenRect;
	struct rfb_Window *succv = (struct rfb_Window *) v->rfbw_Node.tln_Succ;
	TBOOL is_top = TFIRSTNODE(&mod->rfb_VisualList) == &v->rfbw_Node;
	struct rfb_Window *predv =
		is_top ? TNULL : (struct rfb_Window *) v->rfbw_Node.tln_Pred;
	TBOOL winbackbuffer = (v->rfbw_Flags & RFBWFL_BACKBUFFER);

	/* dest rectangle */
	TINT dr[4];

	dr[0] = x;
	dr[1] = y;
	dr[2] = x + sr[2] - sr[0];
	dr[3] = y + sr[3] - sr[1];

	/* remove the window from window stack */
	TRemove(&v->rfbw_Node);

	TBOOL res = TFALSE;

	if (!winbackbuffer)
	{
		TINT s[4];

		s[0] = 0;
		s[1] = 0;
		s[2] = mod->rfb_Width - 1;
		s[3] = mod->rfb_Height - 1;

		if (region_intersect(dr, s))
		{
			s[0] += dx;
			s[1] += dy;
			s[2] += dx;
			s[3] += dy;
			if (region_intersect(dr, s))
				res = fbp_copyarea_int(mod, succv, dx, dy, dr);
		}
	}

	/* update dest/clip rectangle */
	sr[0] += dx;
	sr[1] += dy;
	sr[2] += dx;
	sr[3] += dy;
	if (!winbackbuffer)
	{
		v->rfbw_UserClipRect.r[0] += dx;
		v->rfbw_UserClipRect.r[1] += dy;
		v->rfbw_UserClipRect.r[2] += dx;
		v->rfbw_UserClipRect.r[3] += dy;
	}

	rfb_setwinrect(mod, v);
	rfb_setrealcliprect(mod, v);

	if (winbackbuffer)
		rfb_markdirty(mod, v, v->rfbw_WinRect.r);

	/* reinsert in window stack */
	if (predv)
	{
		TInsert(&mod->rfb_VisualList, &v->rfbw_Node, &predv->rfbw_Node);
		if (res)
			rbp_move_expose(mod, v, predv, dx, dy);
	}
	else
		TAddHead(&mod->rfb_VisualList, &v->rfbw_Node);

	struct RectPool *pool = &mod->rfb_RectPool;
	struct Region R;

	if (region_init(pool, &R, old.r))
	{
		region_subrect(pool, &R, v->rfbw_ScreenRect.r);
		struct TNode *next, *node = R.rg_Rects.rl_List.tlh_Head.tln_Succ;

		for (; (next = node->tln_Succ); node = next)
		{
			struct RectNode *r = (struct RectNode *) node;

			rfb_damage(mod, r->rn_Rect, v);
		}
		region_free(pool, &R);
	}

	if (res)
	{
		/* also expose regions coming from outside the screen */
		struct Region R2;

		if (region_init(pool, &R2, old.r))
		{
			struct Rect scr;

			REGION_RECT_SET(&scr, 0, 0, mod->rfb_Width - 1,
				mod->rfb_Height - 1);
			region_subrect(pool, &R2, scr.r);
			region_shift(&R2, dx, dy);
			region_andrect(pool, &R2, scr.r, 0, 0);
			struct TNode *next, *node = R2.rg_Rects.rl_List.tlh_Head.tln_Succ;

			for (; (next = node->tln_Succ); node = next)
			{
				struct RectNode *r = (struct RectNode *) node;

				rfb_damage(mod, r->rn_Rect, predv);
			}
			region_free(pool, &R2);
		}
	}

	rfb_flush_clients(mod, TTRUE);
}

/*****************************************************************************/

static void rfb_resizewindow(struct rfb_Display *mod, struct rfb_Window *v,
	TINT w, TINT h)
{
	TAPTR TExecBase = TGetExecBase(mod);
	TINT *sr = v->rfbw_ScreenRect.r;
	TINT x = sr[0];
	TINT y = sr[1];

	if (w < 1)
		w = 1;
	if (h < 1)
		h = 1;

	TINT oldw = sr[2] - x + 1;
	TINT oldh = sr[3] - y + 1;

	if (w != oldw || h != oldh)
	{
		TDBPRINTF(TDB_INFO, ("new window size: %d,%d\n", w, h));

		if (v->rfbw_Flags & RFBWFL_BACKBUFFER)
			rfb_resizewinbuffer(mod, v, oldw, oldh, w, h);

		struct Rect old = v->rfbw_ScreenRect;

		sr[2] = x + w - 1;
		sr[3] = y + h - 1;
		rfb_setwinrect(mod, v);

		if (!(v->rfbw_Flags & RFBWFL_USERCLIP))
		{
			TINT *c = v->rfbw_UserClipRect.r;

			c[2] = v->rfbw_WinRect.r[2];
			c[3] = v->rfbw_WinRect.r[3];
		}

		rfb_setrealcliprect(mod, v);

		TIMSG *imsg;

		if (rfb_getimsg(mod, v, &imsg, TITYPE_NEWSIZE))
		{
			imsg->timsg_X = x;
			imsg->timsg_Y = y;
			imsg->timsg_Width = w;
			imsg->timsg_Height = h;
			TPutMsg(v->rfbw_IMsgPort, TNULL, imsg);
			TDBPRINTF(TDB_TRACE, ("send newsize %d %d %d->%d %d->%d\n",
					x, y, oldw, w, oldh, h));
		}

		if (w - oldw < 0 || h - oldh < 0)
		{
			struct RectPool *pool = &mod->rfb_RectPool;
			struct Region R;

			if (region_init(pool, &R, old.r))
			{
				region_subrect(pool, &R, v->rfbw_ScreenRect.r);
				struct TNode *next, *node = 
					R.rg_Rects.rl_List.tlh_Head.tln_Succ;

				for (; (next = node->tln_Succ); node = next)
				{
					struct RectNode *r = (struct RectNode *) node;

					rfb_damage(mod, r->rn_Rect, v);
				}
				region_free(pool, &R);
			}
		}
	}
}

/*****************************************************************************/

static THOOKENTRY TTAG rfb_getattrfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct rfb_attrdata *data = hook->thk_Data;
	TTAGITEM *item = obj;
	struct rfb_Window *v = data->v;
	struct rfb_Display *mod = data->mod;

	switch (item->tti_Tag)
	{
		default:
			return TTRUE;
		case TVisual_Width:
			*((TINT *) item->tti_Value) =
				REGION_RECT_WIDTH(&v->rfbw_ScreenRect);
			break;
		case TVisual_Height:
			*((TINT *) item->tti_Value) =
				REGION_RECT_HEIGHT(&v->rfbw_ScreenRect);
			break;
		case TVisual_ScreenWidth:
			*((TINT *) item->tti_Value) = mod->rfb_Width;
			break;
		case TVisual_ScreenHeight:
			*((TINT *) item->tti_Value) = mod->rfb_Height;
			break;
		case TVisual_WinLeft:
			*((TINT *) item->tti_Value) = v->rfbw_ScreenRect.r[0];
			break;
		case TVisual_WinTop:
			*((TINT *) item->tti_Value) = v->rfbw_ScreenRect.r[1];
			break;
		case TVisual_MinWidth:
			*((TINT *) item->tti_Value) = v->rfbw_MinWidth;
			break;
		case TVisual_MinHeight:
			*((TINT *) item->tti_Value) = v->rfbw_MinHeight;
			break;
		case TVisual_MaxWidth:
			*((TINT *) item->tti_Value) = v->rfbw_MaxWidth;
			break;
		case TVisual_MaxHeight:
			*((TINT *) item->tti_Value) = v->rfbw_MaxHeight;
			break;
		case TVisual_Device:
			*((TAPTR *) item->tti_Value) = data->mod;
			break;
		case TVisual_Window:
			*((TAPTR *) item->tti_Value) = v;
			break;
		case TVisual_HaveWindowManager:
			*((TBOOL *) item->tti_Value) = TFALSE;
			break;
	}
	data->num++;
	return TTRUE;
}

static void rfb_getattrs(struct rfb_Display *mod, struct TVRequest *req)
{
	struct rfb_attrdata data;
	struct THook hook;

	data.v = req->tvr_Op.GetAttrs.Window;
	data.num = 0;
	data.mod = mod;
	TInitHook(&hook, rfb_getattrfunc, &data);

	TForEachTag(req->tvr_Op.GetAttrs.Tags, &hook);
	req->tvr_Op.GetAttrs.Num = data.num;
}

/*****************************************************************************/

static THOOKENTRY TTAG rfb_setattrfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct rfb_attrdata *data = hook->thk_Data;
	TTAGITEM *item = obj;
	struct rfb_Window *v = data->v;

	switch (item->tti_Tag)
	{
		case TVisual_WinLeft:
			data->newx = (TINT) item->tti_Value;
			break;
		case TVisual_WinTop:
			data->newy = (TINT) item->tti_Value;
			break;
		case TVisual_Width:
			data->neww = (TINT) item->tti_Value;
			break;
		case TVisual_Height:
			data->newh = (TINT) item->tti_Value;
			break;
		case TVisual_MinWidth:
			v->rfbw_MinWidth = (TINT) item->tti_Value;
			break;
		case TVisual_MinHeight:
			v->rfbw_MinHeight = (TINT) item->tti_Value;
			break;
		case TVisual_MaxWidth:
		{
			TINT maxw = (TINT) item->tti_Value;

			v->rfbw_MaxWidth = maxw > 0 ? maxw : RFB_HUGE;
			break;
		}
		case TVisual_MaxHeight:
		{
			TINT maxh = (TINT) item->tti_Value;

			v->rfbw_MaxHeight = maxh > 0 ? maxh : RFB_HUGE;
			break;
		}
		case TVisual_WindowHints:
			data->hints = (TSTRPTR) item->tti_Value;
			break;
		default:
			return TTRUE;
	}
	data->num++;
	return TTRUE;
}

static void rfb_setattrs(struct rfb_Display *mod, struct TVRequest *req)
{
	TAPTR TExecBase = TGetExecBase(mod);
	struct rfb_attrdata data;
	struct THook hook;
	struct rfb_Window *v = req->tvr_Op.SetAttrs.Window;

	if ((v->rfbw_Flags & RFBWFL_IS_ROOT) && mod->rfb_RndDevice)
	{
		mod->rfb_RndRequest->tvr_Req.io_Command = TVCMD_SETATTRS;
		mod->rfb_RndRequest->tvr_Op.SetAttrs.Window = mod->rfb_RndInstance;
		mod->rfb_RndRequest->tvr_Op.SetAttrs.Tags = req->tvr_Op.SetAttrs.Tags;
		TDoIO(&mod->rfb_RndRequest->tvr_Req);
	}

	data.newx = v->rfbw_ScreenRect.r[0];
	data.newy = v->rfbw_ScreenRect.r[1];
	data.neww = REGION_RECT_WIDTH(&v->rfbw_ScreenRect);
	data.newh = REGION_RECT_HEIGHT(&v->rfbw_ScreenRect);

	data.v = v;
	data.num = 0;
	data.mod = mod;
	data.hints = TNULL;
	TInitHook(&hook, rfb_setattrfunc, &data);
	TForEachTag(req->tvr_Op.SetAttrs.Tags, &hook);
	req->tvr_Op.SetAttrs.Num = data.num;

	TSTRPTR s = data.hints;

	if (s)
	{
		TINT c;

		while ((c = *s++))
		{
			switch (c)
			{
				case 't':
				{
					if (!(v->rfbw_Flags & RFBWFL_IS_POPUP) &&
						TFIRSTNODE(&mod->rfb_VisualList) != &v->rfbw_Node)
					{
						TLock(mod->rfb_Lock);
						TRemove(&v->rfbw_Node);
						TAddHead(&mod->rfb_VisualList, &v->rfbw_Node);
						TUnlock(mod->rfb_Lock);
						rfb_damage(mod, v->rfbw_ScreenRect.r, TNULL);
					}
					break;
				}
			}
		}
	}

	TINT x = data.newx;
	TINT y = data.newy;
	TINT w = data.neww;
	TINT h = data.newh;

	w = TCLAMP(v->rfbw_MinWidth, w, v->rfbw_MaxWidth);
	h = TCLAMP(v->rfbw_MinHeight, h, v->rfbw_MaxHeight);

	rfb_movewindow(mod, v, x, y);
	rfb_resizewindow(mod, v, w, h);
}

/*****************************************************************************/

struct rfb_drawtagdata
{
	struct rfb_Window *v;
	struct rfb_Display *mod;
	TINT x0, x1, y0, y1;
};

static THOOKENTRY TTAG rfb_drawtagfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct rfb_drawtagdata *data = hook->thk_Data;
	TTAGITEM *item = obj;

	switch (item->tti_Tag)
	{
		case TVisualDraw_X0:
			data->x0 = item->tti_Value;
			break;
		case TVisualDraw_Y0:
			data->y0 = item->tti_Value;
			break;
		case TVisualDraw_X1:
			data->x1 = item->tti_Value;
			break;
		case TVisualDraw_Y1:
			data->y1 = item->tti_Value;
			break;
		case TVisualDraw_NewX:
			data->x0 = data->x1;
			data->x1 = item->tti_Value;
			break;
		case TVisualDraw_NewY:
			data->y0 = data->y1;
			data->y1 = item->tti_Value;
			break;
		case TVisualDraw_FgPen:
			rfb_setfgpen(data->mod, data->v, item->tti_Value);
			break;
		case TVisualDraw_BgPen:
			rfb_setbgpen(data->mod, data->v, item->tti_Value);
			break;
		case TVisualDraw_Command:
			switch (item->tti_Value)
			{
				case TVCMD_FRECT:
				{
					TINT r[] = { data->x0, data->y0, data->x1, data->y1 };
					struct rfb_Pen *pen = 
						(struct rfb_Pen *) data->v->rfbw_FGPen;

					fbp_drawfrect(data->mod, data->v, r, pen);
					break;
				}
				case TVCMD_RECT:
				{
					TINT r[] = { data->x0, data->y0, data->x1, data->y1 };
					struct rfb_Pen *pen = 
						(struct rfb_Pen *) data->v->rfbw_FGPen;

					fbp_drawrect(data->mod, data->v, r, pen);
					break;
				}
				case TVCMD_LINE:
				{
					TINT r[] = { data->x0, data->y0, data->x1, data->y1 };
					struct rfb_Pen *pen = 
						(struct rfb_Pen *) data->v->rfbw_FGPen;

					fbp_drawline(data->mod, data->v, r, pen);
					break;
				}
			}

			break;
	}
	return TTRUE;
}

static void rfb_drawtags(struct rfb_Display *mod, struct TVRequest *req)
{
	struct THook hook;
	struct rfb_drawtagdata data;

	data.v = req->tvr_Op.DrawTags.Window;
	data.mod = mod;

	TInitHook(&hook, rfb_drawtagfunc, &data);
	TForEachTag(req->tvr_Op.DrawTags.Tags, &hook);
}

/*****************************************************************************/

static void rfb_drawtext(struct rfb_Display *mod, struct TVRequest *req)
{
	struct rfb_Window *v = req->tvr_Op.Text.Window;

	rfb_hostdrawtext(mod, v, req->tvr_Op.Text.Text,
		req->tvr_Op.Text.Length,
		req->tvr_Op.Text.X + v->rfbw_WinRect.r[0],
		req->tvr_Op.Text.Y + v->rfbw_WinRect.r[1], req->tvr_Op.Text.FgPen);
}

/*****************************************************************************/

static void rfb_setfont(struct rfb_Display *mod, struct TVRequest *req)
{
	rfb_hostsetfont(mod, req->tvr_Op.SetFont.Window, req->tvr_Op.SetFont.Font);
}

/*****************************************************************************/

static void rfb_openfont(struct rfb_Display *mod, struct TVRequest *req)
{
	req->tvr_Op.OpenFont.Font =
		rfb_hostopenfont(mod, req->tvr_Op.OpenFont.Tags);
}

/*****************************************************************************/

static void rfb_textsize(struct rfb_Display *mod, struct TVRequest *req)
{
	req->tvr_Op.TextSize.Width =
		rfb_hosttextsize(mod, req->tvr_Op.TextSize.Font,
		req->tvr_Op.TextSize.Text, req->tvr_Op.TextSize.NumChars);
}

/*****************************************************************************/

static void rfb_getfontattrs(struct rfb_Display *mod, struct TVRequest *req)
{
	struct rfb_attrdata data;
	struct THook hook;

	data.mod = mod;
	data.font = req->tvr_Op.GetFontAttrs.Font;
	data.num = 0;
	TInitHook(&hook, rfb_hostgetfattrfunc, &data);

	TForEachTag(req->tvr_Op.GetFontAttrs.Tags, &hook);
	req->tvr_Op.GetFontAttrs.Num = data.num;
}

/*****************************************************************************/

static void rfb_closefont(struct rfb_Display *mod, struct TVRequest *req)
{
	rfb_hostclosefont(mod, req->tvr_Op.CloseFont.Font);
}

/*****************************************************************************/

static void rfb_queryfonts(struct rfb_Display *mod, struct TVRequest *req)
{
	req->tvr_Op.QueryFonts.Handle =
		rfb_hostqueryfonts(mod, req->tvr_Op.QueryFonts.Tags);
}

/*****************************************************************************/

static void rfb_getnextfont(struct rfb_Display *mod, struct TVRequest *req)
{
	req->tvr_Op.GetNextFont.Attrs =
		rfb_hostgetnextfont(mod, req->tvr_Op.GetNextFont.Handle);
}

/*****************************************************************************/

LOCAL void rfb_docmd(struct rfb_Display *mod, struct TVRequest *req)
{
	switch (req->tvr_Req.io_Command)
	{
		case TVCMD_OPENWINDOW:
			rfb_openvisual(mod, req);
			break;
		case TVCMD_CLOSEWINDOW:
			rfb_closevisual(mod, req);
			break;
		case TVCMD_OPENFONT:
			rfb_openfont(mod, req);
			break;
		case TVCMD_CLOSEFONT:
			rfb_closefont(mod, req);
			break;
		case TVCMD_GETFONTATTRS:
			rfb_getfontattrs(mod, req);
			break;
		case TVCMD_TEXTSIZE:
			rfb_textsize(mod, req);
			break;
		case TVCMD_QUERYFONTS:
			rfb_queryfonts(mod, req);
			break;
		case TVCMD_GETNEXTFONT:
			rfb_getnextfont(mod, req);
			break;
		case TVCMD_SETINPUT:
			rfb_setinput(mod, req);
			break;
		case TVCMD_GETATTRS:
			rfb_getattrs(mod, req);
			break;
		case TVCMD_SETATTRS:
			rfb_setattrs(mod, req);
			break;
		case TVCMD_ALLOCPEN:
			rfb_allocpen(mod, req);
			break;
		case TVCMD_FREEPEN:
			rfb_freepen(mod, req);
			break;
		case TVCMD_SETFONT:
			rfb_setfont(mod, req);
			break;
		case TVCMD_CLEAR:
			rfb_clear(mod, req);
			break;
		case TVCMD_RECT:
			rfb_rect(mod, req);
			break;
		case TVCMD_FRECT:
			rfb_frect(mod, req);
			break;
		case TVCMD_LINE:
			rfb_line(mod, req);
			break;
		case TVCMD_PLOT:
			rfb_plot(mod, req);
			break;
		case TVCMD_TEXT:
			rfb_drawtext(mod, req);
			break;
		case TVCMD_DRAWSTRIP:
			rfb_drawstrip(mod, req);
			break;
		case TVCMD_DRAWTAGS:
			rfb_drawtags(mod, req);
			break;
		case TVCMD_DRAWFAN:
			rfb_drawfan(mod, req);
			break;
		case TVCMD_COPYAREA:
			rfb_copyarea(mod, req);
			break;
		case TVCMD_SETCLIPRECT:
			rfb_setcliprect(mod, req);
			break;
		case TVCMD_UNSETCLIPRECT:
			rfb_unsetcliprect(mod, req);
			break;
		case TVCMD_DRAWBUFFER:
			rfb_drawbuffer(mod, req);
			break;
		case TVCMD_FLUSH:
			rfb_flush(mod, req);
			break;
		default:
			TDBPRINTF(TDB_ERROR, ("Unknown command code: %d\n",
					req->tvr_Req.io_Command));
	}
}
