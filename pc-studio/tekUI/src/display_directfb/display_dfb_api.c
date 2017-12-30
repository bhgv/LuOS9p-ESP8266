
/*
**	teklib/src/display_dfb/display_dfb_api.c - DirectFB Display driver
**	Written by Franciska Schulze <fschulze at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "display_dfb_mod.h"

TBOOL getimsg(DFBDISPLAY *mod, DFBWINDOW *v, TIMSG **msgptr, TUINT type);
TVOID genimsg(DFBDISPLAY *mod, DFBWINDOW *vold, DFBWINDOW *vnew, TUINT type);

/*****************************************************************************/

LOCAL void
dfb_openvisual(DFBDISPLAY *mod, struct TVRequest *req)
{
	TTAGITEM *tags = req->tvr_Op.OpenWindow.Tags;
	TAPTR exec = TGetExecBase(mod);
	DFBWINDOW *v;
	DFBWindowDescription wdsc;

	struct FontNode *fn;

	v = TExecAlloc0(exec, mod->dfb_MemMgr, sizeof(DFBWINDOW));
	req->tvr_Op.OpenWindow.Window = v;
	if (v == TNULL) return;

	v->userdata = TGetTag(tags, TVisual_UserData, TNULL);
	v->eventmask = (TUINT) TGetTag(tags, TVisual_EventMask, 0);

	TInitList(&v->penlist);
	v->bgpen = TVPEN_UNDEFINED;
	v->fgpen = TVPEN_UNDEFINED;

	TInitList(&v->imsgqueue);
	v->imsgport = req->tvr_Op.OpenWindow.IMsgPort;

	v->title = (TSTRPTR)
		TGetTag(tags, TVisual_Title, (TTAG) "TEKlib visual");

	/* size/position calculation: */

	v->winwidth = (TINT) TGetTag(tags,
		TVisual_Width,
		(TTAG) TMIN(mod->dfb_ScrWidth, DEF_WINWIDTH));
	v->winheight = (TINT) TGetTag(tags,
		TVisual_Height,
		(TTAG) TMIN(mod->dfb_ScrHeight, DEF_WINHEIGHT));

	if (TGetTag(tags, TVisual_Center, TFALSE))
	{
		v->winleft = (mod->dfb_ScrWidth - v->winwidth) / 2;
		v->wintop = (mod->dfb_ScrHeight - v->winheight) / 2;
	}
	else if (TGetTag(tags, TVisual_FullScreen, TFALSE))
	{
		v->winwidth = mod->dfb_ScrWidth;
		v->winheight = mod->dfb_ScrHeight;
		v->winleft = 0;
		v->wintop = 0;
	}
	else
	{
		v->winleft = (TINT) TGetTag(tags, TVisual_WinLeft, (TTAG) -1);
		v->wintop = (TINT) TGetTag(tags, TVisual_WinTop, (TTAG) -1);
	}

	if (!TGetTag(tags, TVisual_Borderless, TFALSE))
	{
		v->wminwidth = (TINT) TGetTag(tags, TVisual_MinWidth, (TTAG) -1);
		v->wminheight = (TINT) TGetTag(tags, TVisual_MinHeight, (TTAG) -1);
		v->wmaxwidth = (TINT) TGetTag(tags, TVisual_MaxWidth, (TTAG) -1);
		v->wmaxheight = (TINT) TGetTag(tags, TVisual_MaxHeight, (TTAG) -1);

		if (v->wmaxwidth > 0)
			v->winwidth = TMIN(v->winwidth, v->wmaxwidth);
		if (v->wmaxheight > 0)
			v->winheight = TMIN(v->winheight, v->wmaxheight);
		if (v->wminwidth > 0)
			v->winwidth = TMAX(v->winwidth, v->wminwidth);
		if (v->wminheight > 0)
			v->winheight = TMAX(v->winheight, v->wminheight);

		v->wminwidth = v->wminwidth <= 0 ? 1 : v->wminwidth;
		v->wminheight = v->wminheight <= 0 ? 1 : v->wminheight;
		v->wmaxwidth = v->wmaxwidth <= 0 ? 1000000 : v->wmaxwidth;
		v->wmaxheight = v->wmaxheight <= 0 ? 1000000 : v->wmaxheight;
	}

	v->winleft = TMAX(v->winleft, 0);
	v->wintop = TMAX(v->wintop, 0);

	if (TGetTag(tags, TVisual_Borderless, TFALSE))
		v->borderless = TTRUE;

	wdsc.flags = (DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT);
	wdsc.posx   = v->winleft;
	wdsc.posy   = v->wintop;
	wdsc.width  = v->winwidth;
	wdsc.height = v->winheight;

	if (mod->dfb_Layer->CreateWindow(mod->dfb_Layer, &wdsc, &v->window) == DFB_OK)
	{
		v->window->GetSurface(v->window, &v->winsurface);
		v->window->SetOpacity(v->window, 0xFF);
		v->window->GetID(v->window, &v->winid);
		v->window->AttachEventBuffer(v->window, mod->dfb_Events);

		v->window->RequestFocus(v->window);
		v->window->RaiseToTop(v->window);

		if (!v->borderless)
		{
			/* generate focus events */
			genimsg(mod, (DFBWINDOW *)mod->dfb_Focused, v, TITYPE_FOCUS);
			mod->dfb_Focused = (TAPTR) v;
		}

		v->winsurface->SetColor(v->winsurface, 0x00, 0x00, 0xff, 0xff);
		v->winsurface->DrawRectangle(v->winsurface, 0, 0, wdsc.width, wdsc.height);
		v->winsurface->SetColor(v->winsurface, 0x80, 0x80, 0x80, 0xff);
		v->winsurface->FillRectangle(v->winsurface, 1, 1, wdsc.width-2, wdsc.height-2);
		v->winsurface->Flip(v->winsurface, NULL, 0);

		/* init default font */
		fn = mod->dfb_fm.deffont;
		v->curfont = fn;
		mod->dfb_fm.defref++;

		if (v->winsurface->SetFont(v->winsurface, fn->font) == DFB_OK)
		{
			if (v->winsurface->SetFont(v->winsurface, fn->font) == DFB_OK)
			{
				TDBPRINTF(TDB_TRACE,("Add window: %p\n", v->window));
				TAddHead(&mod->dfb_vlist, &v->node);

				/* success: */
				return;
			}
		}
     }

	TExecFree(exec, v);

	/* failure: */
	TDBPRINTF(TDB_ERROR,("Open failed\n"));
	req->tvr_Op.OpenWindow.Window = TNULL;
}

/*****************************************************************************/

LOCAL void
dfb_closevisual(DFBDISPLAY *mod, struct TVRequest *req)
{
	struct DFBPen *pen;
	TAPTR exec = TGetExecBase(mod);
	DFBWINDOW *v = req->tvr_Op.CloseWindow.Window;
	if (v == TNULL) return;

	TDBPRINTF(TDB_INFO,("Visual close\n"));

	TRemove(&v->node);

	if (mod->dfb_Focused == (TAPTR) v)
	{
		/* pass focus on to the next window */
		DFBWINDOW *vt = (DFBWINDOW *)TFIRSTNODE(&mod->dfb_vlist);
		if (vt) genimsg(mod, TNULL, vt, TITYPE_FOCUS);
		mod->dfb_Focused = (TAPTR) vt;
	}

	if (mod->dfb_Active == (TAPTR) v)
		mod->dfb_Active = TNULL;

	while ((pen = (struct DFBPen *) TRemHead(&v->penlist)))
	{
		/* free pens */
		TRemove(&pen->node);
		TExecFree(mod->dfb_ExecBase, pen);
	}

	v->winsurface->Release(v->winsurface);
	v->window->Release(v->window);
	mod->dfb_fm.defref--;
	TExecFree(exec, v);
}

/*****************************************************************************/

LOCAL void
dfb_setinput(DFBDISPLAY *mod, struct TVRequest *req)
{
	DFBWINDOW *v = req->tvr_Op.SetInput.Window;
	req->tvr_Op.SetInput.OldMask = v->eventmask;
	v->eventmask = req->tvr_Op.SetInput.Mask;
	/* spool out possible remaining messages: */
	dfb_sendimessages(mod, TFALSE);
}

/*****************************************************************************/

static void
setbgpen(DFBDISPLAY *mod, DFBWINDOW *v, TVPEN pen)
{
	if (pen != v->bgpen && pen != TVPEN_UNDEFINED)
		v->bgpen = pen;
}

static TVPEN
setfgpen(DFBDISPLAY *mod, DFBWINDOW *v, TVPEN pen)
{
	TVPEN oldpen = v->fgpen;
	if (pen != oldpen && pen != TVPEN_UNDEFINED)
	{
		struct DFBPen *dfbpen = (struct DFBPen *) pen;
		v->winsurface->SetColor(v->winsurface, dfbpen->r, dfbpen->g,
			dfbpen->b, dfbpen->a);
		v->fgpen = pen;
		if (oldpen == (TVPEN) 0xffffffff) oldpen = pen;
	}
	return oldpen;
}

/*****************************************************************************/

LOCAL void
dfb_allocpen(DFBDISPLAY *mod, struct TVRequest *req)
{
	DFBWINDOW *v = req->tvr_Op.AllocPen.Window;
	TUINT rgb = req->tvr_Op.AllocPen.RGB;
	struct DFBPen *pen = TExecAlloc(mod->dfb_ExecBase, mod->dfb_MemMgr,
		sizeof(struct DFBPen));
	if (pen)
	{
		pen->a = rgb >> 24;
		pen->r = ((rgb >> 16) & 0xff);
		pen->g = ((rgb >> 8) & 0xff);
		pen->b = rgb & 0xff;
		TAddTail(&v->penlist, &pen->node);
		req->tvr_Op.AllocPen.Pen = (TVPEN) pen;
		return;
	}
	req->tvr_Op.AllocPen.Pen = TVPEN_UNDEFINED;
}

/*****************************************************************************/

LOCAL void
dfb_freepen(DFBDISPLAY *mod, struct TVRequest *req)
{
	struct DFBPen *pen = (struct DFBPen *) req->tvr_Op.FreePen.Pen;
	TRemove(&pen->node);
	TExecFree(mod->dfb_ExecBase, pen);
}

/*****************************************************************************/

LOCAL void
dfb_frect(DFBDISPLAY *mod, struct TVRequest *req)
{
	DFBWINDOW *v = req->tvr_Op.FRect.Window;
	TUINT x = req->tvr_Op.FRect.Rect[0];
	TUINT y = req->tvr_Op.FRect.Rect[1];
	TUINT w = req->tvr_Op.FRect.Rect[2];
	TUINT h = req->tvr_Op.FRect.Rect[3];

	setfgpen(mod, v, req->tvr_Op.FRect.Pen);
	v->winsurface->FillRectangle(v->winsurface, x, y, w, h);
}

/*****************************************************************************/

LOCAL void
dfb_line(DFBDISPLAY *mod, struct TVRequest *req)
{
	DFBWINDOW *v = req->tvr_Op.Line.Window;
	TUINT x0 = req->tvr_Op.Line.Rect[0];
	TUINT y0 = req->tvr_Op.Line.Rect[1];
	TUINT x1 = req->tvr_Op.Line.Rect[2];
	TUINT y1 = req->tvr_Op.Line.Rect[3];

	setfgpen(mod, v, req->tvr_Op.Line.Pen);
	v->winsurface->DrawLine(v->winsurface, x0, y0, x1, y1);
}

/*****************************************************************************/

LOCAL void
dfb_rect(DFBDISPLAY *mod, struct TVRequest *req)
{
	DFBWINDOW *v = req->tvr_Op.Rect.Window;
	TUINT x = req->tvr_Op.Rect.Rect[0];
	TUINT y = req->tvr_Op.Rect.Rect[1];
	TUINT w = req->tvr_Op.Rect.Rect[2];
	TUINT h = req->tvr_Op.Rect.Rect[3];

	setfgpen(mod, v, req->tvr_Op.Rect.Pen);
	v->winsurface->DrawRectangle(v->winsurface, x, y, w, h);
}

/*****************************************************************************/

LOCAL void
dfb_plot(DFBDISPLAY *mod, struct TVRequest *req)
{
	DFBWINDOW *v = req->tvr_Op.Plot.Window;
	TUINT x = req->tvr_Op.Plot.Rect[0];
	TUINT y = req->tvr_Op.Plot.Rect[1];

	setfgpen(mod, v, req->tvr_Op.Plot.Pen);
	v->winsurface->FillRectangle(v->winsurface, x, y, 1, 1);
}

/*****************************************************************************/

LOCAL void
dfb_drawstrip(DFBDISPLAY *mod, struct TVRequest *req)
{
	TINT i, x0, y0, x1, y1, x2, y2;
	DFBWINDOW *v = req->tvr_Op.Strip.Window;
	TINT *array = req->tvr_Op.Strip.Array;
	TINT num = req->tvr_Op.Strip.Num;
	TTAGITEM *tags = req->tvr_Op.Strip.Tags;
	TVPEN pen = (TVPEN) TGetTag(tags, TVisual_Pen, TVPEN_UNDEFINED);
	TVPEN *penarray = (TVPEN *) TGetTag(tags, TVisual_PenArray, TNULL);

	if (num < 3) return;

	if (penarray)
		setfgpen(mod, v, penarray[2]);
	else
		setfgpen(mod, v, pen);

	x0 = array[0];
	y0 = array[1];
	x1 = array[2];
	y1 = array[3];
	x2 = array[4];
	y2 = array[5];

	v->winsurface->FillTriangle(v->winsurface, x0, y0, x1, y1, x2, y2);

	for (i = 3; i < num; i++)
	{
		x0 = x1;
		y0 = y1;
		x1 = x2;
		y1 = y2;
		x2 = array[i*2];
		y2 = array[i*2+1];

		if (penarray)
			setfgpen(mod, v, penarray[i]);

		v->winsurface->FillTriangle(v->winsurface, x0, y0, x1, y1, x2, y2);
	}
}

/*****************************************************************************/

LOCAL void
dfb_drawfan(DFBDISPLAY *mod, struct TVRequest *req)
{
	TINT i, x0, y0, x1, y1, x2, y2;
	DFBWINDOW *v = req->tvr_Op.Fan.Window;
	TINT *array = req->tvr_Op.Fan.Array;
	TINT num = req->tvr_Op.Fan.Num;
	TTAGITEM *tags = req->tvr_Op.Fan.Tags;
	TVPEN pen = (TVPEN) TGetTag(tags, TVisual_Pen, TVPEN_UNDEFINED);
	TVPEN *penarray = (TVPEN *) TGetTag(tags, TVisual_PenArray, TNULL);

	if (num < 3) return;

	if (penarray)
		setfgpen(mod, v, penarray[2]);
	else
		setfgpen(mod, v, pen);

	x0 = array[0];
	y0 = array[1];
	x1 = array[2];
	y1 = array[3];
	x2 = array[4];
	y2 = array[5];

	v->winsurface->FillTriangle(v->winsurface, x0, y0, x1, y1, x2, y2);

	for (i = 3; i < num; i++)
	{
		x1 = x2;
		y1 = y2;
		x2 = array[i*2];
		y2 = array[i*2+1];

		if (penarray)
			setfgpen(mod, v, penarray[i]);

		v->winsurface->FillTriangle(v->winsurface, x0, y0, x1, y1, x2, y2);
	}
}

/*****************************************************************************/

LOCAL void
dfb_drawarc(DFBDISPLAY *mod, struct TVRequest *req)
{
	TDBPRINTF(TDB_ERROR,("dfb_drawarc: not yet implemented!\n"));
}

/*****************************************************************************/

LOCAL void
dfb_drawfarc(DFBDISPLAY *mod, struct TVRequest *req)
{
	TDBPRINTF(TDB_ERROR,("dfb_drawfarc: not yet implemented!\n"));
}

/*****************************************************************************/

LOCAL void
dfb_copyarea(DFBDISPLAY *mod, struct TVRequest *req)
{
	DFBWINDOW *v = req->tvr_Op.CopyArea.Window;
	DFBRectangle *rect = (DFBRectangle *)req->tvr_Op.CopyArea.Rect;
	TINT x = req->tvr_Op.CopyArea.DestX;
	TINT y = req->tvr_Op.CopyArea.DestY;

	v->winsurface->Blit(v->winsurface, v->winsurface, rect, x, y);
}

/*****************************************************************************/

LOCAL void
dfb_setcliprect(DFBDISPLAY *mod, struct TVRequest *req)
{
	DFBWINDOW *v = req->tvr_Op.ClipRect.Window;
	DFBRegion clip;

	clip.x1 = req->tvr_Op.ClipRect.Rect[0];
	clip.y1 = req->tvr_Op.ClipRect.Rect[1];
	clip.x2 = clip.x1 + req->tvr_Op.ClipRect.Rect[2] - 1;
	clip.y2 = clip.y1 + req->tvr_Op.ClipRect.Rect[3] - 1;
	v->winsurface->SetClip(v->winsurface, &clip);
}

/*****************************************************************************/

LOCAL void
dfb_unsetcliprect(DFBDISPLAY *mod, struct TVRequest *req)
{
	DFBWINDOW *v = req->tvr_Op.ClipRect.Window;
	DFBRegion clip;

	clip.x1 = 0;
	clip.y1 = 0;
	clip.x2 = v->winwidth;
	clip.y2 = v->winheight;
	v->winsurface->SetClip(v->winsurface, &clip);
}

/*****************************************************************************/

LOCAL void
dfb_clear(DFBDISPLAY *mod, struct TVRequest *req)
{
	DFBWINDOW *v = req->tvr_Op.Clear.Window;
	struct DFBPen *pen = (struct DFBPen *) req->tvr_Op.Clear.Pen;
	setfgpen(mod, v, req->tvr_Op.Clear.Pen);
	v->winsurface->Clear(v->winsurface, pen->r, pen->g, pen->b, pen->a);
}

/*****************************************************************************/

LOCAL void
dfb_drawbuffer(DFBDISPLAY *mod, struct TVRequest *req)
{
	DFBWINDOW *v = req->tvr_Op.DrawBuffer.Window;
	TINT w = req->tvr_Op.DrawBuffer.RRect[2];
	TINT h = req->tvr_Op.DrawBuffer.RRect[3];
	TINT xd = req->tvr_Op.DrawBuffer.RRect[0];
	TINT yd = req->tvr_Op.DrawBuffer.RRect[1];
	TAPTR buf = req->tvr_Op.DrawBuffer.Buf;
	if (!buf)
		return;
	TINT totw = req->tvr_Op.DrawBuffer.TotWidth;
	IDirectFBSurface *surface = NULL;
	DFBSurfaceDescription dsc;
	DFBRectangle rect = { 0, 0, w, h };

	dsc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT
			  | DSDESC_PREALLOCATED;
	dsc.width = w;
	dsc.height = h;
	dsc.pixelformat = DSPF_RGB32;
	dsc.preallocated[0].data = buf;
	dsc.preallocated[0].pitch = totw * DFB_BYTES_PER_PIXEL(DSPF_RGB32);

	if (mod->dfb_DFB->CreateSurface(mod->dfb_DFB, &dsc, &surface) == DFB_OK)
	{
		v->winsurface->Blit(v->winsurface, surface, &rect, xd, yd);
		surface->Release(surface);
	}
}

/*****************************************************************************/

static THOOKENTRY TTAG
getattrfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct attrdata *data = hook->thk_Data;
	TTAGITEM *item = obj;
	DFBWINDOW *v = data->v;
	DFBDISPLAY *mod = data->mod;

	switch (item->tti_Tag)
	{
		default:
			return TTRUE;
		case TVisual_UserData:
			*((TTAG *) item->tti_Value) = v->userdata;
			break;
		case TVisual_Width:
			*((TINT *) item->tti_Value) = v->winwidth;
			break;
		case TVisual_Height:
			*((TINT *) item->tti_Value) = v->winheight;
			break;
		case TVisual_ScreenWidth:
			*((TINT *) item->tti_Value) = mod->dfb_ScrWidth;
			break;
		case TVisual_ScreenHeight:
			*((TINT *) item->tti_Value) = mod->dfb_ScrHeight;
			break;
		case TVisual_WinLeft:
			*((TINT *) item->tti_Value) = v->winleft;
			break;
		case TVisual_WinTop:
			*((TINT *) item->tti_Value) = v->wintop;
			break;
		case TVisual_MinWidth:
			*((TINT *) item->tti_Value) = v->wminwidth;
			break;
		case TVisual_MinHeight:
			*((TINT *) item->tti_Value) = v->wminheight;
			break;
		case TVisual_MaxWidth:
			*((TINT *) item->tti_Value) = v->wmaxwidth;
			break;
		case TVisual_MaxHeight:
			*((TINT *) item->tti_Value) = v->wmaxheight;
			break;
		case TVisual_HaveWindowManager:
			*((TBOOL *) item->tti_Value) = TFALSE;
			break;
	}
	data->num++;
	return TTRUE;
}

LOCAL void
dfb_getattrs(DFBDISPLAY *mod, struct TVRequest *req)
{
	struct attrdata data;
	struct THook hook;

	data.v = req->tvr_Op.GetAttrs.Window;
	data.num = 0;
	data.mod = mod;
	TInitHook(&hook, getattrfunc, &data);

	TForEachTag(req->tvr_Op.GetAttrs.Tags, &hook);
	req->tvr_Op.GetAttrs.Num = data.num;
}

/*****************************************************************************/

static THOOKENTRY TTAG
setattrfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct attrdata *data = hook->thk_Data;
	TTAGITEM *item = obj;
	DFBWINDOW *v = data->v;

	switch (item->tti_Tag)
	{
		default:
			return TTRUE;
		case TVisual_MinWidth:
			v->wminwidth = (TINT) item->tti_Value;
			data->sizechanged = TTRUE;
			break;
		case TVisual_MinHeight:
			v->wminheight = (TINT) item->tti_Value;
			data->sizechanged = TTRUE;
			break;
		case TVisual_MaxWidth:
			v->wmaxwidth = (TINT) item->tti_Value;
			data->sizechanged = TTRUE;
			break;
		case TVisual_MaxHeight:
			v->wmaxheight = (TINT) item->tti_Value;
			data->sizechanged = TTRUE;
			break;
	}
	data->num++;
	return TTRUE;
}

LOCAL void
dfb_setattrs(DFBDISPLAY *mod, struct TVRequest *req)
{
	struct attrdata data;
	struct THook hook;
	DFBWINDOW *v = req->tvr_Op.SetAttrs.Window;

	data.v = v;
	data.num = 0;
	data.mod = mod;
	data.sizechanged = TFALSE;
	TInitHook(&hook, setattrfunc, &data);

	TForEachTag(req->tvr_Op.SetAttrs.Tags, &hook);
	req->tvr_Op.SetAttrs.Num = data.num;

	if (data.sizechanged)
	{
		TIMSG *imsg;

		v->wminwidth = v->wminwidth <= 0 ? 1 : v->wminwidth;
		v->wminheight = v->wminheight <= 0 ? 1 : v->wminheight;
		v->wmaxwidth = v->wmaxwidth <= 0 ? 1000000 : v->wmaxwidth;
		v->wmaxheight = v->wmaxheight <= 0 ? 1000000 : v->wmaxheight;

		if (v->wmaxwidth > 0)
			v->winwidth = TMIN(v->winwidth, v->wmaxwidth);
		if (v->wmaxheight > 0)
			v->winheight = TMIN(v->winheight, v->wmaxheight);
		if (v->wminwidth > 0)
			v->winwidth = TMAX(v->winwidth, v->wminwidth);
		if (v->wminheight > 0)
			v->winheight = TMAX(v->winheight, v->wminheight);

		v->window->Resize(v->window, v->winwidth, v->winheight);

		if ((v->eventmask & TITYPE_NEWSIZE) &&
			getimsg(mod, v, &imsg, TITYPE_NEWSIZE))
		{
			imsg->timsg_Width = v->winwidth;
			imsg->timsg_Height = v->winheight;
			TAddTail(&v->imsgqueue, &imsg->timsg_Node);
		}

		if ((v->eventmask & TITYPE_REFRESH) &&
			getimsg(mod, v, &imsg, TITYPE_REFRESH))
		{
			imsg->timsg_X = v->winleft;
			imsg->timsg_Y = v->wintop;
			imsg->timsg_Width = v->winwidth;
			imsg->timsg_Height = v->winheight;
			TAddTail(&v->imsgqueue, &imsg->timsg_Node);
		}
	}
}

/*****************************************************************************/

struct drawdata
{
	DFBWINDOW *v;
	DFBDISPLAY *mod;
	TINT x0, x1, y0, y1;
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
			setfgpen(data->mod, data->v, item->tti_Value);
			break;
		case TVisualDraw_BgPen:
			setbgpen(data->mod, data->v, item->tti_Value);
			break;
		case TVisualDraw_Command:
			switch (item->tti_Value)
			{
				case TVCMD_FRECT:
					data->v->winsurface->FillRectangle(data->v->winsurface, data->x0,
						data->y0, data->x1, data->y1);
					break;
				case TVCMD_RECT:
					data->v->winsurface->DrawRectangle(data->v->winsurface, data->x0,
						data->y0, data->x1, data->y1);
					break;
				case TVCMD_LINE:
					data->v->winsurface->DrawLine(data->v->winsurface, data->x0,
						data->y0, data->x1, data->y1);
					break;
			}

			break;
	}
	return TTRUE;
}

LOCAL void
dfb_drawtags(DFBDISPLAY *mod, struct TVRequest *req)
{
	struct THook hook;
	struct drawdata data;
	data.v = req->tvr_Op.DrawTags.Window;
	data.mod = mod;

	TInitHook(&hook, drawtagfunc, &data);
	TForEachTag(req->tvr_Op.DrawTags.Tags, &hook);
}

/*****************************************************************************/

LOCAL void
dfb_drawtext(DFBDISPLAY *mod, struct TVRequest *req)
{
	DFBWINDOW *v = req->tvr_Op.Text.Window;
	TSTRPTR text = req->tvr_Op.Text.Text;
	TINT len = req->tvr_Op.Text.Length;
	TUINT x = req->tvr_Op.Text.X;
	TUINT y = req->tvr_Op.Text.Y;
	TINT w;
	/*TINT h = ((struct FontNode *) v->curfont)->height;*/
	struct DFBPen *fgpen = (struct DFBPen *) req->tvr_Op.Text.FgPen;
	TINT ascent = ((struct FontNode *) v->curfont)->ascent;
	IDirectFBFont *f = ((struct FontNode *) v->curfont)->font;
	f->GetStringWidth(f, text, len, &w);
	setfgpen(mod, v, (TVPEN) fgpen);
	v->winsurface->DrawString(v->winsurface, text, len, x, y + ascent, 
		DSTF_LEFT);
}

/*****************************************************************************/

LOCAL void
dfb_setfont(DFBDISPLAY *mod, struct TVRequest *req)
{
	dfb_hostsetfont(mod, req->tvr_Op.SetFont.Window,
		req->tvr_Op.SetFont.Font);
}

/*****************************************************************************/

LOCAL void
dfb_openfont(DFBDISPLAY *mod, struct TVRequest *req)
{
	req->tvr_Op.OpenFont.Font =
		dfb_hostopenfont(mod, req->tvr_Op.OpenFont.Tags);
}

/*****************************************************************************/

LOCAL void
dfb_textsize(DFBDISPLAY *mod, struct TVRequest *req)
{
	req->tvr_Op.TextSize.Width = dfb_hosttextsize(mod, 
		req->tvr_Op.TextSize.Font, req->tvr_Op.TextSize.Text, 
		req->tvr_Op.TextSize.NumChars);
}

/*****************************************************************************/

LOCAL void
dfb_getfontattrs(DFBDISPLAY *mod, struct TVRequest *req)
{
	struct attrdata data;
	struct THook hook;

	data.mod = mod;
	data.font = req->tvr_Op.GetFontAttrs.Font;
	data.num = 0;
	TInitHook(&hook, dfb_hostgetfattrfunc, &data);

	TForEachTag(req->tvr_Op.GetFontAttrs.Tags, &hook);
	req->tvr_Op.GetFontAttrs.Num = data.num;
}

/*****************************************************************************/

LOCAL void
dfb_closefont(DFBDISPLAY *mod, struct TVRequest *req)
{
	dfb_hostclosefont(mod, req->tvr_Op.CloseFont.Font);
}

/*****************************************************************************/

LOCAL void
dfb_queryfonts(DFBDISPLAY *mod, struct TVRequest *req)
{
	req->tvr_Op.QueryFonts.Handle =
		dfb_hostqueryfonts(mod, req->tvr_Op.QueryFonts.Tags);
}

/*****************************************************************************/

LOCAL void
dfb_getnextfont(DFBDISPLAY *mod, struct TVRequest *req)
{
	req->tvr_Op.GetNextFont.Attrs =
		dfb_hostgetnextfont(mod, req->tvr_Op.GetNextFont.Handle);
}

/*****************************************************************************/

