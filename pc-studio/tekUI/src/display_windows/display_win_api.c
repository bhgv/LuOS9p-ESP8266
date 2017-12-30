
/*
**	display_win_api.c - Windows display driver
**	Written by Timm S. Mueller <tmueller@schulze-mueller.de>
**	contributions by Tobias Schwinger <tschwinger@isonews2.com>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "display_win_mod.h"

#define RGB2COLORREF(rgb) \
	((((rgb) << 16) & 0xff0000) | ((rgb) & 0xff00) | (((rgb) >> 16) & 0xff))

static void
fb_closeall(WINDISPLAY *mod, WINWINDOW *win, TBOOL unref_font);

/*****************************************************************************/

LOCAL void
fb_openwindow(WINDISPLAY *mod, struct TVRequest *req)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	TTAGITEM *tags = req->tvr_Op.OpenWindow.Tags;
	WINWINDOW *win;

	req->tvr_Op.OpenWindow.Window = TNULL;

	for (;;)
	{
		RECT wrect;
		BITMAPINFOHEADER *bmi;
		TUINT style;
		TUINT exstyle;
		TIMSG *imsg;
		const char *classname;
		TSTRPTR title;

		win = TAlloc0(mod->fbd_MemMgr, sizeof(WINWINDOW));
		if (win == TNULL)
			break;

		TInitList(&win->penlist);

		InitializeCriticalSection(&win->fbv_LockExtents);

		win->fbv_Width = (TUINT) TGetTag(tags, TVisual_Width, FB_DEF_WIDTH);
		win->fbv_Height = (TUINT) TGetTag(tags, TVisual_Height, FB_DEF_HEIGHT);
		win->fbv_Left = (TUINT) TGetTag(tags, TVisual_WinLeft, 0xffffffff);
		win->fbv_Top = (TUINT) TGetTag(tags, TVisual_WinTop, 0xffffffff);
		win->fbv_Title = (TSTRPTR) TGetTag(tags, TVisual_Title,
			(TTAG) "TEKlib Visual");
		win->fbv_Borderless = TGetTag(tags, TVisual_Borderless, TFALSE);
		win->fbv_UserData = TGetTag(tags, TVisual_UserData, TNULL);

		win->fbv_MinWidth = (TINT) TGetTag(tags, TVisual_MinWidth, (TTAG) -1);
		win->fbv_MinHeight = (TINT) TGetTag(tags, TVisual_MinHeight, (TTAG) -1);
		win->fbv_MaxWidth = (TINT) TGetTag(tags, TVisual_MaxWidth, (TTAG) -1);
		win->fbv_MaxHeight = (TINT) TGetTag(tags, TVisual_MaxHeight, (TTAG) -1);

		if (win->fbv_Borderless)
		{
			style = (WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS) & ~WS_BORDER;
			classname = FB_DISPLAY_CLASSNAME_POPUP;
			title = NULL;
			exstyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
		}
		else
		{
			style = WS_OVERLAPPEDWINDOW;
			classname = FB_DISPLAY_CLASSNAME;
			title = win->fbv_Title;
			exstyle = 0;
		}

		wrect.left = wrect.right = wrect.top = wrect.bottom = 0;
		AdjustWindowRectEx(&wrect, style, FALSE, exstyle);
		win->fbv_BorderWidth = wrect.right - wrect.left;
		win->fbv_BorderHeight = wrect.bottom - wrect.top;
		win->fbv_BorderLeft = -wrect.left;
		win->fbv_BorderTop = -wrect.top;

		if (!win->fbv_Borderless)
		{
			TINT m1, m2, m3, m4;
			win_getminmax(win, &m1, &m2, &m3, &m4, TTRUE);
			win->fbv_Width = TCLAMP(m1, (TINT) win->fbv_Width, m3);
			win->fbv_Height = TCLAMP(m2, (TINT) win->fbv_Height, m4);
		}

		if (win->fbv_Left != 0xffffffff && win->fbv_Top != 0xffffffff)
		{
			wrect.left = win->fbv_Left;
			wrect.top = win->fbv_Top;
			wrect.right = win->fbv_Left + win->fbv_Width;
			wrect.bottom = win->fbv_Top + win->fbv_Height;
			if (!AdjustWindowRectEx(&wrect, style, FALSE, exstyle))
				break;

			win->fbv_Left = wrect.left;
			win->fbv_Top = wrect.top;
			win->fbv_HWnd = CreateWindowEx(exstyle, classname,
				title, style, win->fbv_Left, win->fbv_Top,
				wrect.right - wrect.left - win->fbv_BorderWidth, 
				wrect.bottom - wrect.top - win->fbv_BorderHeight,
				(HWND) NULL, (HMENU) NULL, mod->fbd_HInst, (LPVOID) NULL);
		}
		else
		{
			win->fbv_HWnd = CreateWindowEx(exstyle, classname,
				title, style, CW_USEDEFAULT, CW_USEDEFAULT,
				win->fbv_Width, win->fbv_Height,
				(HWND) NULL, (HMENU) NULL, mod->fbd_HInst, (LPVOID) NULL);
		}

		if (win->fbv_HWnd == TNULL)
			break;

		GetWindowRect(win->fbv_HWnd, &wrect);
		win->fbv_Left = wrect.left + win->fbv_BorderLeft;
		win->fbv_Top = wrect.top + win->fbv_BorderHeight;
		win->fbv_Width = wrect.right - wrect.left - win->fbv_BorderWidth;
		win->fbv_Height = wrect.bottom - wrect.top - win->fbv_BorderHeight;

		SetWindowLongPtr(win->fbv_HWnd, GWLP_USERDATA, (LONG_PTR) win);

		win->fbv_HDC = GetDC(win->fbv_HWnd);
		win->fbv_Display = mod;
		win->fbv_InputMask = (TUINT) TGetTag(tags, TVisual_EventMask, 0);
		win->fbv_IMsgPort = req->tvr_Op.OpenWindow.IMsgPort;

		bmi = &win->fbv_DrawBitMap;
		bmi->biSize = sizeof(BITMAPINFOHEADER);
		bmi->biPlanes = 1;
		bmi->biBitCount = 32;
		bmi->biCompression = BI_RGB;
		bmi->biSizeImage = 0;
		bmi->biXPelsPerMeter = 1;
		bmi->biYPelsPerMeter = 1;
		bmi->biClrUsed = 0;
		bmi->biClrImportant = 0;

		TInitList(&win->fbv_IMsgQueue);

		req->tvr_Op.OpenWindow.Window = win;

		/* init default font */
		win->fbv_CurrentFont = mod->fbd_FontManager.deffont;
		mod->fbd_FontManager.defref++;

		if (win->fbv_InputMask & TITYPE_INTERVAL)
			++mod->fbd_NumInterval;

		/* register default font */
		/*TDBPRINTF(TDB_TRACE,("Add window: %p\n", win->window));*/

		/* add window on top of window stack: */
		TAddHead(&mod->fbd_VisualList, &win->fbv_Node);

		SetBkMode(win->fbv_HDC, TRANSPARENT);
		
		ShowWindow(win->fbv_HWnd, SW_SHOWNORMAL);
		UpdateWindow(win->fbv_HWnd);

		if ((win->fbv_InputMask & TITYPE_FOCUS) &&
			(fb_getimsg(mod, win, &imsg, TITYPE_FOCUS)))
		{
			imsg->timsg_Code = 1;
			TAddTail(&win->fbv_IMsgQueue, &imsg->timsg_Node);
		}
		if ((win->fbv_InputMask & TITYPE_REFRESH) &&
			(fb_getimsg(mod, win, &imsg, TITYPE_REFRESH)))
		{
			imsg->timsg_X = 0;
			imsg->timsg_Y = 0;
			imsg->timsg_Width = win->fbv_Width;
			imsg->timsg_Height = win->fbv_Height;
			TAddTail(&win->fbv_IMsgQueue, &imsg->timsg_Node);
		}
		
		return;
	}

	TDBPRINTF(TDB_ERROR,("Window open failed\n"));
	fb_closeall(mod, win, TFALSE);
}

/*****************************************************************************/

static void
fb_closeall(WINDISPLAY *mod, WINWINDOW *win, TBOOL unref_font)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	struct FBPen *pen;

	if (win->fbv_HDC)
		ReleaseDC(win->fbv_HWnd, win->fbv_HDC);

	if (mod->fbd_WindowFocussedApp == win->fbv_HWnd)
		mod->fbd_WindowFocussedApp = NULL;
	if (mod->fbd_WindowUnderCursor == win->fbv_HWnd)
		mod->fbd_WindowUnderCursor = NULL;
	if (mod->fbd_WindowActivePopup == win->fbv_HWnd)
		mod->fbd_WindowActivePopup = NULL;

	if (win->fbv_HWnd)
		DestroyWindow(win->fbv_HWnd);

	while ((pen = (struct FBPen *) TRemHead(&win->penlist)))
		TFree(pen);

	if (unref_font)
		mod->fbd_FontManager.defref--;
	if (win->fbv_InputMask & TITYPE_INTERVAL)
		--mod->fbd_NumInterval;

	TFree(win);
}

/*****************************************************************************/

LOCAL void
fb_closewindow(WINDISPLAY *mod, struct TVRequest *req)
{
	WINWINDOW *win = req->tvr_Op.CloseWindow.Window;
	if (win == TNULL)
		return;
	TRemove(&win->fbv_Node);
	fb_closeall(mod, win, TTRUE);
}

/*****************************************************************************/

LOCAL void
fb_setinput(WINDISPLAY *mod, struct TVRequest *req)
{
	WINWINDOW *v = req->tvr_Op.SetInput.Window;
	TUINT oldmask = v->fbv_InputMask;
	TUINT newmask = req->tvr_Op.SetInput.Mask;

	if (oldmask & TITYPE_INTERVAL)
		--mod->fbd_NumInterval;
	if (newmask & TITYPE_INTERVAL)
		++mod->fbd_NumInterval;

	req->tvr_Op.SetInput.OldMask = oldmask;
	v->fbv_InputMask = newmask;
	TDBPRINTF(TDB_TRACE,("Setinputmask: %08x\n", v->fbv_InputMask));
	/* spool out possible remaining messages: */
	fb_sendimessages(mod);
}

/*****************************************************************************/

LOCAL void
fb_allocpen(WINDISPLAY *mod, struct TVRequest *req)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	WINWINDOW *v = req->tvr_Op.AllocPen.Window;
	TUINT rgb = req->tvr_Op.AllocPen.RGB;
	struct FBPen *pen = TAlloc(mod->fbd_MemMgr, sizeof(struct FBPen));
	if (pen)
	{
		COLORREF col = RGB2COLORREF(rgb);
		pen->rgb = rgb;
		pen->col = col;
		pen->brush = CreateSolidBrush(col);
		pen->pen = CreatePen(PS_SOLID, 0, col);
		TAddTail(&v->penlist, &pen->node);
		req->tvr_Op.AllocPen.Pen = (TVPEN) pen;
		return;
	}
	req->tvr_Op.AllocPen.Pen = TVPEN_UNDEFINED;
}

/*****************************************************************************/

LOCAL void
fb_freepen(WINDISPLAY *mod, struct TVRequest *req)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	if (req->tvr_Op.FreePen.Pen != TVPEN_UNDEFINED)
	{
		struct FBPen *pen = (struct FBPen *) req->tvr_Op.FreePen.Pen;
		TRemove(&pen->node);
		DeleteObject(pen->pen);
		DeleteObject(pen->brush);
		TFree(pen);
	}
}

/*****************************************************************************/

LOCAL void
fb_frect(WINDISPLAY *mod, struct TVRequest *req)
{
	WINWINDOW *win = req->tvr_Op.FRect.Window;
	TVPEN pen = req->tvr_Op.FRect.Pen;
	if (pen != TVPEN_UNDEFINED)
	{
		RECT r;
		r.left = req->tvr_Op.FRect.Rect[0];
		r.top = req->tvr_Op.FRect.Rect[1];
		r.right = r.left + req->tvr_Op.FRect.Rect[2];
		r.bottom = r.top + req->tvr_Op.FRect.Rect[3];
		FillRect(win->fbv_HDC, &r, ((struct FBPen *) pen)->brush);
	}
}

/*****************************************************************************/

LOCAL void
fb_line(WINDISPLAY *mod, struct TVRequest *req)
{
	WINWINDOW *win = req->tvr_Op.Line.Window;
	TVPEN pen = req->tvr_Op.Line.Pen;
	MoveToEx(win->fbv_HDC, req->tvr_Op.Line.Rect[0],
		req->tvr_Op.Line.Rect[1], NULL);
	SelectObject(win->fbv_HDC, ((struct FBPen *) pen)->pen);
	LineTo(win->fbv_HDC, req->tvr_Op.Line.Rect[2],
		req->tvr_Op.Line.Rect[3]);
}

/*****************************************************************************/

LOCAL void
fb_rect(WINDISPLAY *mod, struct TVRequest *req)
{
	WINWINDOW *win = req->tvr_Op.Rect.Window;
	TVPEN pen = req->tvr_Op.Rect.Pen;
	if (pen != TVPEN_UNDEFINED)
	{
		TINT *r = req->tvr_Op.Rect.Rect;
		SelectObject(win->fbv_HDC, ((struct FBPen *) pen)->pen);
		SelectObject(win->fbv_HDC, GetStockObject(NULL_BRUSH));
		Rectangle(win->fbv_HDC, r[0], r[1], r[0] + r[2], r[1] + r[3]);
	}
}

/*****************************************************************************/

LOCAL void
fb_plot(WINDISPLAY *mod, struct TVRequest *req)
{
	WINWINDOW *win = req->tvr_Op.Plot.Window;
	TUINT x = req->tvr_Op.Plot.Rect[0];
	TUINT y = req->tvr_Op.Plot.Rect[1];
	struct FBPen *fgpen = (struct FBPen *) req->tvr_Op.Plot.Pen;
	SetPixel(win->fbv_HDC, x, y, fgpen->col);
}

/*****************************************************************************/

LOCAL void
fb_drawstrip(WINDISPLAY *mod, struct TVRequest *req)
{
	TINT i;
	WINWINDOW *win = req->tvr_Op.Strip.Window;
	TINT *array = req->tvr_Op.Strip.Array;
	TINT num = req->tvr_Op.Strip.Num;
	TTAGITEM *tags = req->tvr_Op.Strip.Tags;
	TVPEN *penarray = (TVPEN *) TGetTag(tags, TVisual_PenArray, TNULL);
	POINT points[3];

	if (num < 3) return;

	if (penarray)
	{
		SelectObject(win->fbv_HDC, ((struct FBPen *) penarray[2])->pen);
		SelectObject(win->fbv_HDC, ((struct FBPen *) penarray[2])->brush);
	}
	else
	{
		TVPEN pen = (TVPEN) TGetTag(tags, TVisual_Pen, TVPEN_UNDEFINED);
		SelectObject(win->fbv_HDC, ((struct FBPen *) pen)->pen);
		SelectObject(win->fbv_HDC, ((struct FBPen *) pen)->brush);
	}

	Polygon(win->fbv_HDC, (LPPOINT) array, 3);

	if (num > 3)
	{
		points[0].x = array[0];
		points[0].y = array[1];
		points[1].x = array[2];
		points[1].y = array[3];
		points[2].x = array[4];
		points[2].y = array[5];
		for (i = 3; i < num; ++i)
		{
			if (penarray)
			{
				SelectObject(win->fbv_HDC, ((struct FBPen *) penarray[i])->pen);
				SelectObject(win->fbv_HDC, ((struct FBPen *) penarray[i])->brush);
			}
			points[0].x = points[1].x;
			points[0].y = points[1].y;
			points[1].x = points[2].x;
			points[1].y = points[2].y;
			points[2].x = array[i * 2];
			points[2].y = array[i * 2 + 1];
			Polygon(win->fbv_HDC, points, 3);
		}
	}
}

/*****************************************************************************/

LOCAL void
fb_drawfan(WINDISPLAY *mod, struct TVRequest *req)
{
	TINT i;
	WINWINDOW *win = req->tvr_Op.Fan.Window;
	TINT *array = req->tvr_Op.Fan.Array;
	TINT num = req->tvr_Op.Fan.Num;
	TTAGITEM *tags = req->tvr_Op.Fan.Tags;
	TVPEN *penarray = (TVPEN *) TGetTag(tags, TVisual_PenArray, TNULL);
	POINT points[3];

	if (num < 3) return;

	if (penarray)
	{
		SelectObject(win->fbv_HDC, ((struct FBPen *) penarray[2])->pen);
		SelectObject(win->fbv_HDC, ((struct FBPen *) penarray[2])->brush);
	}
	else
	{
		TVPEN pen = (TVPEN) TGetTag(tags, TVisual_Pen, TVPEN_UNDEFINED);
		SelectObject(win->fbv_HDC, ((struct FBPen *) pen)->pen);
		SelectObject(win->fbv_HDC, ((struct FBPen *) pen)->brush);
	}

	Polygon(win->fbv_HDC, (LPPOINT) array, 3);

	if (num > 3)
	{
		points[0].x = array[0];
		points[0].y = array[1];
		points[1].x = array[2];
		points[1].y = array[3];
		points[2].x = array[4];
		points[2].y = array[5];
		for (i = 3; i < num; ++i)
		{
			if (penarray)
			{
				SelectObject(win->fbv_HDC, ((struct FBPen *) penarray[i])->pen);
				SelectObject(win->fbv_HDC, ((struct FBPen *) penarray[i])->brush);
			}
			points[1].x = points[2].x;
			points[1].y = points[2].y;
			points[2].x = array[i * 2];
			points[2].y = array[i * 2 + 1];
			Polygon(win->fbv_HDC, points, 3);
		}
	}
}

/*****************************************************************************/

LOCAL void
fb_copyarea(WINDISPLAY *mod, struct TVRequest *req)
{
	WINWINDOW *win = req->tvr_Op.CopyArea.Window;
	struct THook *exposehook = (struct THook *)
		TGetTag(req->tvr_Op.CopyArea.Tags, TVisual_ExposeHook, TNULL);
	TINT *sr = req->tvr_Op.CopyArea.Rect;
	TINT dx = req->tvr_Op.CopyArea.DestX - sr[0];
	TINT dy = req->tvr_Op.CopyArea.DestY - sr[1];
	RECT r;

	r.left = sr[4];
	r.top = sr[5];
	r.right = sr[4] + sr[2];
	r.bottom = sr[5] + sr[3];

	if (exposehook)
	{
		RGNDATAHEADER *rdh = (RGNDATAHEADER *) win->fbv_RegionData;
		RECT *rd = (RECT *) (rdh + 1);
		HRGN updateregion = CreateRectRgn(0, 0, 0, 0);
		ScrollDC(win->fbv_HDC, dx, dy, &r, &r, updateregion, NULL);
		if (GetRegionData(updateregion, 1024, (LPRGNDATA) rdh))
		{
			TUINT i;
			for (i = 0; i < rdh->nCount; ++i)
				TCallHookPkt(exposehook, win, (TTAG) (rd + i));
		}
		else
		{
			TDBPRINTF(TDB_WARN,("Regiondata buffer too small\n"));
			InvalidateRgn(win->fbv_HWnd, updateregion, FALSE);
		}
		DeleteObject(updateregion);
	}
	else
		ScrollDC(win->fbv_HDC, dx, dy, &r, &r, NULL, NULL);
}

/*****************************************************************************/

LOCAL void
fb_setcliprect(WINDISPLAY *mod, struct TVRequest *req)
{
	WINWINDOW *win = req->tvr_Op.ClipRect.Window;
	RECT *cr = &win->fbv_ClipRect;
	HRGN rgn;
	cr->left = req->tvr_Op.ClipRect.Rect[0];
	cr->top = req->tvr_Op.ClipRect.Rect[1];
	cr->right = cr->left + req->tvr_Op.ClipRect.Rect[2];
	cr->bottom = cr->top + req->tvr_Op.ClipRect.Rect[3];
	rgn = CreateRectRgnIndirect(cr);
	SelectClipRgn(win->fbv_HDC, rgn);
	DeleteObject(rgn);
}

/*****************************************************************************/

LOCAL void
fb_unsetcliprect(WINDISPLAY *mod, struct TVRequest *req)
{
	WINWINDOW *win = req->tvr_Op.ClipRect.Window;
	RECT *cr = &win->fbv_ClipRect;
	SelectClipRgn(win->fbv_HDC, NULL);
	cr->left = 0;
	cr->top = 0;
	EnterCriticalSection(&win->fbv_LockExtents);
	cr->right = win->fbv_Width;
	cr->bottom = win->fbv_Height;
	LeaveCriticalSection(&win->fbv_LockExtents);
}

/*****************************************************************************/

LOCAL void
fb_clear(WINDISPLAY *mod, struct TVRequest *req)
{
	WINWINDOW *win = req->tvr_Op.Clear.Window;
	TVPEN pen = req->tvr_Op.Clear.Pen;
	if (pen != TVPEN_UNDEFINED)
	{
		RECT r;
		r.left = 0;
		r.top = 0;
		EnterCriticalSection(&win->fbv_LockExtents);
		r.right = win->fbv_Width;
		r.bottom = win->fbv_Height;
		LeaveCriticalSection(&win->fbv_LockExtents);
		FillRect(win->fbv_HDC, &r, ((struct FBPen *) pen)->brush);
	}
}

/*****************************************************************************/

LOCAL void
fb_drawbuffer(WINDISPLAY *mod, struct TVRequest *req)
{
	WINWINDOW *win = req->tvr_Op.DrawBuffer.Window;
	TINT *rrect = req->tvr_Op.DrawBuffer.RRect;
	BITMAPINFOHEADER *bmi = &win->fbv_DrawBitMap;
	bmi->biWidth = req->tvr_Op.DrawBuffer.TotWidth;
	bmi->biHeight = -rrect[3];
	TAPTR buf = req->tvr_Op.DrawBuffer.Buf;
	if (!buf)
		return;
	SetDIBitsToDevice(win->fbv_HDC,
		rrect[0], rrect[1],
		rrect[2], rrect[3],
		0, 0,
		0, rrect[3],
		buf,
		(const void *) bmi,
		DIB_RGB_COLORS);
}

/*****************************************************************************/

LOCAL void
fb_getselection(WINDISPLAY *mod, struct TVRequest *req)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	LPSTR utf8 = NULL;
	int utf8Bytes = 0;
	HANDLE clipData;
	LPCWSTR utf16; 

	req->tvr_Op.GetSelection.Data = TNULL;
	req->tvr_Op.GetSelection.Length = 0;

	if (!IsClipboardFormatAvailable(CF_UNICODETEXT))
	{
		TDBPRINTF(TDB_WARN,(
				  "no 'CF_UNICODETEXT' Windows clipboard data available\n"));
		return;
	}
	if (!OpenClipboard(mod->fbd_DeviceHWnd))
	{
		TDBPRINTF(TDB_WARN, ("cannot open Windows clipboard\n"));
		return;
	}
	clipData = GetClipboardData(CF_UNICODETEXT);
	if (clipData != NULL)
	{
		utf16 = GlobalLock(clipData);
		utf8Bytes = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, 
										NULL, 0, NULL, 0);
		if (utf8Bytes != 0)
		{
			utf8 = (LPSTR) TAlloc(TNULL, utf8Bytes);
			utf8Bytes = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, 
											utf8, utf8Bytes, NULL, 0);
		}
		GlobalUnlock(clipData);
	}
	else
		TDBPRINTF(TDB_WARN, ("no Windows clipboard data\n"));
	CloseClipboard();

	if (!utf8Bytes)
	{
		TFree(utf8);
		TDBPRINTF(TDB_WARN, (
				  "failed encoding UTF-8 from Windows clipboard\n"));
		return;
	}
	--utf8Bytes;
	TDBPRINTF(TDB_TRACE,("get selection: N = %d S = '%s'\n", utf8Bytes, utf8));
	req->tvr_Op.GetSelection.Data = utf8;
	req->tvr_Op.GetSelection.Length = utf8Bytes;
}

LOCAL void
fb_setselection(WINDISPLAY *mod, struct TVRequest *req)
{
	TBOOL ok = TFALSE;
	LPSTR utf8 = (LPSTR) req->tvr_Op.SetSelection.Data;
	int utf8Bytes = (int) req->tvr_Op.SetSelection.Length;
	HANDLE clipData = NULL;
	LPWSTR utf16 = NULL; 
	int utf16Units = 0;

	if (req->tvr_Op.SetSelection.Type != 2 || utf8Bytes <= 0) return;

	TDBPRINTF(TDB_TRACE,("set selection: N = %d S = '%s'\n", utf8Bytes, utf8));

	utf16Units = MultiByteToWideChar(CP_UTF8, 0, utf8, utf8Bytes, NULL, 0);
	if (utf16Units != 0)
	{
		clipData = GlobalAlloc(GMEM_MOVEABLE, (utf16Units + 1) * sizeof(WCHAR));

		utf16 = (LPWSTR) GlobalLock(clipData);
		utf16Units = MultiByteToWideChar(CP_UTF8, 0, utf8, utf8Bytes,
									 utf16, utf16Units);
		utf16[utf16Units] = L'\0';
		GlobalUnlock(clipData);
	}
	if (! utf16Units)
		TDBPRINTF(TDB_WARN, (
				  "failed decoding UTF-8 for Windows clipboard\n"));
	else if (OpenClipboard(mod->fbd_DeviceHWnd))
	{
		ok = SetClipboardData(CF_UNICODETEXT, clipData) != NULL;
		CloseClipboard();

		if (! ok) TDBPRINTF(TDB_WARN, ("failed setting Windows clipboard\n"));
	}
	else TDBPRINTF(TDB_WARN, ("cannot open Windows clipboard\n"));

	if (! ok)
		GlobalFree(clipData);
}

/*****************************************************************************/

static THOOKENTRY TTAG
getattrfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct attrdata *data = hook->thk_Data;
	TTAGITEM *item = obj;
	WINWINDOW *v = data->v;
	WINDISPLAY *mod = data->mod;

	switch (item->tti_Tag)
	{
		default:
			return TTRUE;
		case TVisual_UserData:
			*((TTAG *) item->tti_Value) = v->fbv_UserData;
			break;
		case TVisual_Width:
			*((TINT *) item->tti_Value) = v->fbv_Width;
			break;
		case TVisual_Height:
			*((TINT *) item->tti_Value) = v->fbv_Height;
			break;
		case TVisual_ScreenWidth:
			*((TINT *) item->tti_Value) = mod->fbd_ScreenWidth;
			break;
		case TVisual_ScreenHeight:
			*((TINT *) item->tti_Value) = mod->fbd_ScreenHeight;
			break;
		case TVisual_WinLeft:
			*((TINT *) item->tti_Value) = v->fbv_Left;
			break;
		case TVisual_WinTop:
			*((TINT *) item->tti_Value) = v->fbv_Top;
			break;
		case TVisual_MinWidth:
			*((TINT *) item->tti_Value) = v->fbv_MinWidth;
			break;
		case TVisual_MinHeight:
			*((TINT *) item->tti_Value) = v->fbv_MinHeight;
			break;
		case TVisual_MaxWidth:
			*((TINT *) item->tti_Value) = v->fbv_MaxWidth;
			break;
		case TVisual_MaxHeight:
			*((TINT *) item->tti_Value) = v->fbv_MaxHeight;
			break;
		case TVisual_Device:
			*((TAPTR *) item->tti_Value) = data->mod;
			break;
		case TVisual_Window:
			*((TAPTR *) item->tti_Value) = v;
			break;
		case TVisual_HaveWindowManager:
			*((TBOOL *) item->tti_Value) = TTRUE;
			break;
		case TVisual_HaveClipboard:
		case TVisual_HaveSelection:
			*((TBOOL *) item->tti_Value) = TFALSE;
			break;
	}
	data->num++;
	return TTRUE;
}

LOCAL void
fb_getattrs(WINDISPLAY *mod, struct TVRequest *req)
{
	struct attrdata data;
	struct THook hook;
	WINWINDOW *win = req->tvr_Op.GetAttrs.Window;

	data.v = win;
	data.num = 0;
	data.mod = mod;
	TInitHook(&hook, getattrfunc, &data);

	EnterCriticalSection(&win->fbv_LockExtents);
	TForEachTag(req->tvr_Op.GetAttrs.Tags, &hook);
	LeaveCriticalSection(&win->fbv_LockExtents);

	req->tvr_Op.GetAttrs.Num = data.num;
}

/*****************************************************************************/

static THOOKENTRY TTAG
setattrfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct attrdata *data = hook->thk_Data;
	TTAGITEM *item = obj;
	WINWINDOW *v = data->v;
	switch (item->tti_Tag)
	{
		default:
			return TTRUE;
		case TVisual_Width:
			data->neww = (TINT) item->tti_Value;
			break;
		case TVisual_Height:
			data->newh = (TINT) item->tti_Value;
			break;
		case TVisual_MinWidth:
			v->fbv_MinWidth = (TINT) item->tti_Value;
			break;
		case TVisual_MinHeight:
			v->fbv_MinHeight = (TINT) item->tti_Value;
			break;
		case TVisual_MaxWidth:
			v->fbv_MaxWidth = (TINT) item->tti_Value;
			break;
		case TVisual_MaxHeight:
			v->fbv_MaxHeight = (TINT) item->tti_Value;
			break;
	}
	data->num++;
	return TTRUE;
}

LOCAL void
fb_setattrs(WINDISPLAY *mod, struct TVRequest *req)
{
	struct attrdata data;
	struct THook hook;
	WINWINDOW *win = req->tvr_Op.SetAttrs.Window;
	TINT noww, nowh, neww, newh, minw, minh;
	data.v = win;
	data.num = 0;
	data.mod = mod;
	data.neww = -1;
	data.newh = -1;
	TInitHook(&hook, setattrfunc, &data);

	EnterCriticalSection(&win->fbv_LockExtents);

	TForEachTag(req->tvr_Op.SetAttrs.Tags, &hook);

	win_getminmax(win, &win->fbv_MinWidth, &win->fbv_MinHeight,
		&win->fbv_MaxWidth, &win->fbv_MaxHeight, TFALSE);

	noww = (TINT) win->fbv_Width;
	nowh = (TINT) win->fbv_Height;
	minw = (TINT) win->fbv_MinWidth;
	minh = (TINT) win->fbv_MinHeight;

	LeaveCriticalSection(&win->fbv_LockExtents);

	neww = data.neww < 0 ? noww : data.neww;
	newh = data.newh < 0 ? nowh : data.newh;

	if (neww < minw || newh < minh)
	{
		neww = TMAX(neww, minw);
		newh = TMAX(newh, minh);
		neww += win->fbv_BorderWidth;
		newh += win->fbv_BorderHeight;
		SetWindowPos(win->fbv_HWnd, NULL, 0, 0, neww, newh, SWP_NOMOVE);
	}

	req->tvr_Op.SetAttrs.Num = data.num;
}

/*****************************************************************************/

struct drawdata
{
	WINWINDOW *v;
	WINDISPLAY *mod;
	TINT x0, x1, y0, y1;
	struct FBPen *bgpen;
	struct FBPen *fgpen;
};

static THOOKENTRY TTAG
drawtagfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct drawdata *data = hook->thk_Data;
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
			data->fgpen = (struct FBPen *) item->tti_Value;
			SelectObject(data->v->fbv_HDC, data->fgpen->pen);
			break;
		case TVisualDraw_BgPen:
			data->bgpen = (struct FBPen *) item->tti_Value;
			break;
		case TVisualDraw_Command:
			switch (item->tti_Value)
			{
				case TVCMD_FRECT:
					if (data->fgpen)
					{
						RECT r;
						r.left = data->x0;
						r.top = data->y0;
						r.right = data->x0 + data->x1;
						r.bottom = data->y0 + data->y1;
						FillRect(data->v->fbv_HDC, &r, data->fgpen->brush);
					}
					break;

				case TVCMD_RECT:
					if (data->fgpen)
						Rectangle(data->v->fbv_HDC, data->x0, data->y0, 
								  data->x0 + data->x1, data->y0 + data->y1);
					break;

				case TVCMD_LINE:
					if (data->fgpen)
					{
						MoveToEx(data->v->fbv_HDC, data->x0, data->y0, NULL);
						LineTo(data->v->fbv_HDC, data->x1, data->y1);
					}
					break;
			}

			break;
	}
	return TTRUE;
}

LOCAL void
fb_drawtags(WINDISPLAY *mod, struct TVRequest *req)
{
	WINWINDOW *win = req->tvr_Op.DrawTags.Window;
	struct THook hook;
	struct drawdata data;
	data.v = win;
	data.mod = mod;
	data.fgpen = TNULL;
	data.bgpen = TNULL;
	TInitHook(&hook, drawtagfunc, &data);
	TForEachTag(req->tvr_Op.DrawTags.Tags, &hook);
}

/*****************************************************************************/

LOCAL void
fb_drawtext(WINDISPLAY *mod, struct TVRequest *req)
{
	WINWINDOW *win = req->tvr_Op.Text.Window;
	const char *text = req->tvr_Op.Text.Text;
	TINT len = req->tvr_Op.Text.Length;
	TUINT x = req->tvr_Op.Text.X;
	TUINT y = req->tvr_Op.Text.Y;
	struct FBPen *fgpen = (struct FBPen *) req->tvr_Op.Text.FgPen;
	int clen = utf8getlen((const unsigned char *) text, len);
	TUINT16 wstr[2048];
	MultiByteToWideChar(CP_UTF8, 0, text, len, (LPWSTR) &wstr, 2048);
	SetTextColor(win->fbv_HDC, fgpen->col);
	TextOutW(win->fbv_HDC, x, y, wstr, clen);
}

/*****************************************************************************/

LOCAL void
fb_setfont(WINDISPLAY *mod, struct TVRequest *req)
{
	fb_hostsetfont(mod, req->tvr_Op.SetFont.Window,
		req->tvr_Op.SetFont.Font);
}

/*****************************************************************************/

LOCAL void
fb_openfont(WINDISPLAY *mod, struct TVRequest *req)
{
	req->tvr_Op.OpenFont.Font =
		fb_hostopenfont(mod, req->tvr_Op.OpenFont.Tags);
	TDBPRINTF(TDB_INFO,("font opened: %p\n", req->tvr_Op.OpenFont.Font));
}

/*****************************************************************************/

LOCAL void
fb_textsize(WINDISPLAY *mod, struct TVRequest *req)
{
	req->tvr_Op.TextSize.Width = fb_hosttextsize(mod,
		req->tvr_Op.TextSize.Font, req->tvr_Op.TextSize.Text, 
		req->tvr_Op.TextSize.NumChars);
}

/*****************************************************************************/

LOCAL void fb_getfontattrs(WINDISPLAY *mod, struct TVRequest *req)
{
	struct attrdata data;
	data.num = 0;
	struct THook hook;
	SelectObject(mod->fbd_DeviceHDC, req->tvr_Op.GetFontAttrs.Font);
	if (GetTextMetrics(mod->fbd_DeviceHDC, &data.textmetric))
	{
		data.mod = mod;
		TInitHook(&hook, fb_hostgetfattrfunc, &data);
		TForEachTag(req->tvr_Op.GetFontAttrs.Tags, &hook);
	}
	req->tvr_Op.GetFontAttrs.Num = data.num;
}

/*****************************************************************************/

LOCAL void
fb_closefont(WINDISPLAY *mod, struct TVRequest *req)
{
	fb_hostclosefont(mod, req->tvr_Op.CloseFont.Font);
}

/*****************************************************************************/

LOCAL void
fb_queryfonts(WINDISPLAY *mod, struct TVRequest *req)
{
	req->tvr_Op.QueryFonts.Handle =
		fb_hostqueryfonts(mod, req->tvr_Op.QueryFonts.Tags);
}

/*****************************************************************************/

LOCAL void
fb_getnextfont(WINDISPLAY *mod, struct TVRequest *req)
{
	req->tvr_Op.GetNextFont.Attrs =
		fb_hostgetnextfont(mod, req->tvr_Op.GetNextFont.Handle);
}
