
#include <tek/string.h>
#include "visual_mod.h"

/*****************************************************************************/

static struct TVRequest *visi_getreq(struct TVisualBase *inst, TUINT cmd,
    struct TDisplayBase *display, TTAGITEM *tags)
{
    struct TVisualBase *mod =
        (struct TVisualBase *) inst->vis_Module.tmd_ModSuper;
    struct TExecBase *TExecBase = TGetExecBase(mod);
    struct TVRequest *req = TNULL;

    if (display == TNULL)
        display = vis_opendisplay(mod, tags);

    if (display)
    {
        if (mod == inst)
        {
            req = TDisplayAllocReq(display);
            if (req)
            {
                req->tvr_Req.io_Command = cmd;
                req->tvr_Req.io_ReplyPort = TGetSyncPort(TNULL);
            }
        }
        else
        {
            /* try to unlink from free requests pool: */
            req = (struct TVRequest *) TRemHead(&inst->vis_ReqPool);

            if (req == TNULL)
            {
                /* peek into list of waiting requests: */
                req = (struct TVRequest *) TFIRSTNODE(&inst->vis_WaitList);
                if (req)
                {
                    if (inst->vis_NumRequests >= VISUAL_MAXREQPERINSTANCE)
                    {
                        /* wait and unlink from waitlist: */
                        TWaitIO(&req->tvr_Req);
                        TRemove(&req->tvr_Req.io_Node);
                    }
                    else
                        /* allocate new one: */
                        req = TNULL;
                }
            }

            if (req == TNULL)
            {
                req = TDisplayAllocReq(display);
                if (req)
                    inst->vis_NumRequests++;
            }

            if (req)
            {
                req->tvr_Req.io_Command = cmd;
                req->tvr_Req.io_ReplyPort = inst->vis_CmdRPort;
            }
        }
    }

    return req;
}

static void
visi_ungetreq(struct TVisualBase *mod, struct TVRequest *req)
{
    if (mod->vis_Module.tmd_ModSuper == (struct TModule *) mod)
    {
        TDisplayFreeReq((struct TDisplayBase *)
            req->tvr_Req.io_Device, req);
    }
    else
        TAddTail(&mod->vis_ReqPool, &req->tvr_Req.io_Node);
}

static void
visi_dosync(struct TVisualBase *mod, struct TVRequest *req)
{
    struct TExecBase *TExecBase = TGetExecBase(mod);
    TDoIO(&req->tvr_Req);
    visi_ungetreq(mod, req);
}

static void
visi_doasync(struct TVisualBase *mod, struct TVRequest *req)
{
    struct TExecBase *TExecBase = TGetExecBase(mod);
    TPutIO(&req->tvr_Req);
    TAddTail(&mod->vis_WaitList, &req->tvr_Req.io_Node);
}

/*****************************************************************************/

EXPORT struct TVisualBase *vis_openvisual(struct TVisualBase *mod,
    TTAGITEM *tags)
{
    struct TExecBase *TExecBase = TGetExecBase(mod);
    struct TVisualBase *inst;
    TTAGITEM otags[3];

    otags[0].tti_Tag = TVisual_NewInstance;
    otags[0].tti_Value = TTRUE;
    otags[1].tti_Tag = TTAG_MORE;
    otags[1].tti_Value = (TTAG) tags;

    inst = (struct TVisualBase *) TOpenModule("visual", 0, otags);
    if (inst)
    {
        struct TVRequest *req =
            visi_getreq(inst, TVCMD_OPENWINDOW, TNULL, tags);
        if (req)
        {
            otags[0].tti_Tag = TTAG_GOSUB;
            otags[0].tti_Value = (TTAG) tags;
            otags[1].tti_Tag = TVisual_UserData;
            otags[1].tti_Value = (TTAG) inst;
            otags[2].tti_Tag = TTAG_DONE;
            req->tvr_Op.OpenWindow.Window = TNULL;
            req->tvr_Op.OpenWindow.IMsgPort = inst->vis_IMsgPort;
            req->tvr_Op.OpenWindow.Tags = otags;
            TDoIO(&req->tvr_Req);
            inst->vis_Window = req->tvr_Op.OpenWindow.Window;
            if (inst->vis_Window)
            {
                inst->vis_InitRequest = req;
                inst->vis_Display = req->tvr_Req.io_Device;
                inst->vis_InputMask = (TUINT) TGetTag(tags,
                    TVisual_EventMask, 0);
                return inst;
            }
        }
        TCloseModule((struct TModule *) inst);
    }

    TDBPRINTF(TDB_ERROR,("open failed\n"));
    return TNULL;
}

/*****************************************************************************/

EXPORT void vis_closevisual(struct TVisualBase *mod, struct TVisualBase *inst)
{
    struct TExecBase *TExecBase = TGetExecBase(mod);
    struct TVRequest *req = visi_getreq(mod, TVCMD_CLOSEWINDOW,
        inst->vis_Display, TNULL);
    req->tvr_Req.io_Command = TVCMD_CLOSEWINDOW;
    req->tvr_Op.CloseWindow.Window = inst->vis_Window;
    visi_dosync(inst, req);
    TDisplayFreeReq(inst->vis_Display, inst->vis_InitRequest);
    TCloseModule((struct TModule *) inst);
}

/*****************************************************************************/

EXPORT struct TVisualBase *vis_attach(struct TVisualBase *mod, TTAGITEM *tags)
{
    struct TExecBase *TExecBase = TGetExecBase(mod);
    TTAGITEM otags[2];
    otags[0].tti_Tag = TVisual_Attach;
    otags[0].tti_Value = (TTAG) mod;
    otags[1].tti_Tag = TTAG_MORE;
    otags[1].tti_Value = (TTAG) tags;
    return (struct TVisualBase *) TOpenModule("visual", 0, otags);
}

/*****************************************************************************/

EXPORT struct TVRequest *vis_openfont(struct TVisualBase *mod, TTAGITEM *tags)
{
    struct TVRequest *fontreq = visi_getreq(mod, TVCMD_OPENFONT, 
        mod->vis_Display, tags);
    if (fontreq)
    {
        struct TExecBase *TExecBase = TGetExecBase(mod);
        fontreq->tvr_Op.OpenFont.Tags = tags;
        TDoIO(&fontreq->tvr_Req);
        if (fontreq->tvr_Op.OpenFont.Font)
            return fontreq;
        visi_ungetreq(mod, fontreq);
    }
    return TNULL;
}

/*****************************************************************************/

EXPORT void vis_closefont(struct TVisualBase *mod, struct TVRequest *fontreq)
{
    if (fontreq)
    {
        fontreq->tvr_Req.io_Command = TVCMD_CLOSEFONT;
        visi_dosync(mod, fontreq);
    }
}

/*****************************************************************************/

EXPORT TUINT vis_getfattrs(struct TVisualBase *mod, struct TVRequest *fontreq,
    TTAGITEM *tags)
{
    TUINT n = 0;
    if (fontreq)
    {
        struct TExecBase *TExecBase = TGetExecBase(mod);
        fontreq->tvr_Req.io_Command = TVCMD_GETFONTATTRS;
        fontreq->tvr_Op.GetFontAttrs.Tags = tags;
        TDoIO(&fontreq->tvr_Req);
        n = fontreq->tvr_Op.GetFontAttrs.Num;
    }
    return n;
}

/*****************************************************************************/

EXPORT TINT vis_textsize(struct TVisualBase *mod, struct TVRequest *fontreq,
    TSTRPTR t, TINT numchars)
{
    TINT size = -1;
    if (fontreq)
    {
        struct TExecBase *TExecBase = TGetExecBase(mod);
        fontreq->tvr_Req.io_Command = TVCMD_TEXTSIZE;
        fontreq->tvr_Op.TextSize.Text = t;
        fontreq->tvr_Op.TextSize.NumChars = numchars;
        TDoIO(&fontreq->tvr_Req);
        size = fontreq->tvr_Op.TextSize.Width;
    }
    return size;
}

/*****************************************************************************/

EXPORT TAPTR vis_queryfonts(struct TVisualBase *mod, TTAGITEM *tags)
{
    TAPTR handle = TNULL;
    struct TExecBase *TExecBase = TGetExecBase(mod);
    struct TVRequest *req = visi_getreq(mod, TVCMD_QUERYFONTS,
        mod->vis_Display, tags);
    if (req)
    {
        req->tvr_Op.QueryFonts.Tags = tags;
        TDoIO(&req->tvr_Req);
        handle = req->tvr_Op.QueryFonts.Handle;
        visi_ungetreq(mod, req);
    }
    return handle;
}

/*****************************************************************************/

EXPORT TTAGITEM *vis_getnextfont(struct TVisualBase *mod, TAPTR fqhandle)
{
    TTAGITEM *attrs;
    struct TExecBase *TExecBase = TGetExecBase(mod);
    struct TVRequest *req = visi_getreq(mod, TVCMD_GETNEXTFONT,
        TGetOwner(fqhandle), TNULL);
    req->tvr_Op.GetNextFont.Handle = fqhandle;
    TDoIO(&req->tvr_Req);
    attrs = req->tvr_Op.GetNextFont.Attrs;
    visi_ungetreq(mod, req);
    return attrs;
}

/*****************************************************************************/

EXPORT TAPTR vis_getport(struct TVisualBase *inst)
{
    return inst->vis_IMsgPort;
}

/*****************************************************************************/

EXPORT TUINT vis_setinput(struct TVisualBase *inst, TUINT cmask, TUINT smask)
{
    TUINT oldmask = inst->vis_InputMask;
    TUINT newmask = (oldmask & ~cmask) | smask;
    if (newmask != oldmask)
    {
        struct TVRequest *req = visi_getreq(inst, TVCMD_SETINPUT,
            inst->vis_Display, TNULL);
        req->tvr_Op.SetInput.Window = inst->vis_Window;
        req->tvr_Op.SetInput.Mask = newmask;
        visi_dosync(inst, req);
        inst->vis_InputMask = newmask;
    }
    return newmask;
}

/*****************************************************************************/

EXPORT TUINT vis_getattrs(struct TVisualBase *inst, TTAGITEM *tags)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_GETATTRS,
        inst->vis_Display, TNULL);
    req->tvr_Op.GetAttrs.Window = inst->vis_Window;
    req->tvr_Op.GetAttrs.Tags = tags;
    visi_dosync(inst, req);
    return req->tvr_Op.GetAttrs.Num;
}

/*****************************************************************************/

EXPORT TUINT vis_setattrs(struct TVisualBase *inst, TTAGITEM *tags)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_SETATTRS,
        inst->vis_Display, TNULL);
    req->tvr_Op.SetAttrs.Window = inst->vis_Window;
    req->tvr_Op.SetAttrs.Tags = tags;
    visi_dosync(inst, req);
    return req->tvr_Op.SetAttrs.Num;
}

/*****************************************************************************/

EXPORT TVPEN vis_allocpen(struct TVisualBase *inst, TUINT rgb)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_ALLOCPEN,
        inst->vis_Display, TNULL);
    req->tvr_Op.AllocPen.Window = inst->vis_Window;
    req->tvr_Op.AllocPen.RGB = rgb;
    visi_dosync(inst, req);
    return req->tvr_Op.AllocPen.Pen;
}

/*****************************************************************************/

EXPORT void vis_freepen(struct TVisualBase *inst, TVPEN pen)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_FREEPEN,
        inst->vis_Display, TNULL);
    req->tvr_Op.FreePen.Window = inst->vis_Window;
    req->tvr_Op.FreePen.Pen = pen;
    visi_doasync(inst, req);
}

/*****************************************************************************/
/* TODO: inst->display <-> font->display must match */

EXPORT void vis_setfont(struct TVisualBase *inst, struct TVRequest *fontreq)
{
    if (fontreq)
    {
        struct TExecBase *TExecBase = TGetExecBase(inst);
        fontreq->tvr_Req.io_Command = TVCMD_SETFONT;
        fontreq->tvr_Op.SetFont.Window = inst->vis_Window;
        TDoIO(&fontreq->tvr_Req);
    }
}

/*****************************************************************************/

EXPORT void vis_clear(struct TVisualBase *inst, TVPEN pen)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_CLEAR,
        inst->vis_Display, TNULL);
    req->tvr_Op.Clear.Window = inst->vis_Window;
    req->tvr_Op.Clear.Pen = pen;
    visi_doasync(inst, req);
}

/*****************************************************************************/

EXPORT void vis_rect(struct TVisualBase *inst, TINT x, TINT y, TINT w, TINT h,
    TVPEN pen)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_RECT,
        inst->vis_Display, TNULL);
    req->tvr_Op.Rect.Window = inst->vis_Window;
    req->tvr_Op.Rect.Rect[0] = x;
    req->tvr_Op.Rect.Rect[1] = y;
    req->tvr_Op.Rect.Rect[2] = w;
    req->tvr_Op.Rect.Rect[3] = h;
    req->tvr_Op.Rect.Pen = pen;
    visi_doasync(inst, req);
}

/*****************************************************************************/

EXPORT void vis_frect(struct TVisualBase *inst, TINT x, TINT y, TINT w, TINT h,
    TVPEN pen)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_FRECT,
        inst->vis_Display, TNULL);
    req->tvr_Op.FRect.Window = inst->vis_Window;
    req->tvr_Op.FRect.Rect[0] = x;
    req->tvr_Op.FRect.Rect[1] = y;
    req->tvr_Op.FRect.Rect[2] = w;
    req->tvr_Op.FRect.Rect[3] = h;
    req->tvr_Op.FRect.Pen = pen;
    visi_doasync(inst, req);
}

/*****************************************************************************/

EXPORT void vis_line(struct TVisualBase *inst, TINT x0, TINT y0, TINT x1,
    TINT y1, TVPEN pen)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_LINE,
        inst->vis_Display, TNULL);
    req->tvr_Op.Line.Window = inst->vis_Window;
    req->tvr_Op.Line.Rect[0] = x0;
    req->tvr_Op.Line.Rect[1] = y0;
    req->tvr_Op.Line.Rect[2] = x1;
    req->tvr_Op.Line.Rect[3] = y1;
    req->tvr_Op.Line.Pen = pen;
    visi_doasync(inst, req);
}

/*****************************************************************************/

EXPORT void vis_plot(struct TVisualBase *inst, TINT x, TINT y, TVPEN pen)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_PLOT,
        inst->vis_Display, TNULL);
    req->tvr_Op.Plot.Window = inst->vis_Window;
    req->tvr_Op.Plot.Rect[0] = x;
    req->tvr_Op.Plot.Rect[1] = y;
    req->tvr_Op.Plot.Pen = pen;
    visi_doasync(inst, req);
}

/*****************************************************************************/

EXPORT void vis_drawtags(struct TVisualBase *inst, TTAGITEM *tags)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_DRAWTAGS,
        inst->vis_Display, TNULL);
    req->tvr_Op.DrawTags.Window = inst->vis_Window;
    req->tvr_Op.DrawTags.Tags = tags;
    visi_dosync(inst, req);
}

/*****************************************************************************/

EXPORT void vis_text(struct TVisualBase *inst, TINT x, TINT y, TSTRPTR t,
    TUINT l, TVPEN fg)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_TEXT,
        inst->vis_Display, TNULL);
    req->tvr_Op.Text.Window = inst->vis_Window;
    req->tvr_Op.Text.X = x;
    req->tvr_Op.Text.Y = y;
    req->tvr_Op.Text.FgPen = fg;
    req->tvr_Op.Text.Text = t;
    req->tvr_Op.Text.Length = l;
    visi_dosync(inst, req);
}

/*****************************************************************************/

EXPORT void vis_drawstrip(struct TVisualBase *inst, TINT *array, TINT num,
    TINT x, TINT y, TINT w, TINT h, TINT iscursor,
    TTAGITEM *tags)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_DRAWSTRIP,
        inst->vis_Display, TNULL);
    req->tvr_Op.Strip.Window = inst->vis_Window;
    req->tvr_Op.Strip.Array = array;
    req->tvr_Op.Strip.Num = num;
    req->tvr_cursor.x = x;
    req->tvr_cursor.y = y;
    req->tvr_cursor.w = w;
    req->tvr_cursor.h = h;
    req->tvr_cursor.iscursor = iscursor;
    req->tvr_Op.Strip.Tags = tags;
    visi_dosync(inst, req);
}

/*****************************************************************************/

EXPORT void vis_drawfan(struct TVisualBase *inst, TINT *array, TINT num,
    TINT x, TINT y, TINT w, TINT h, TINT iscursor,
    TTAGITEM *tags)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_DRAWFAN,
        inst->vis_Display, TNULL);
    req->tvr_Op.Fan.Window = inst->vis_Window;
    req->tvr_Op.Fan.Array = array;
    req->tvr_Op.Fan.Num = num;
    req->tvr_cursor.x = x;
    req->tvr_cursor.y = y;
    req->tvr_cursor.w = w;
    req->tvr_cursor.h = h;
    req->tvr_cursor.iscursor = iscursor;
    req->tvr_Op.Fan.Tags = tags;
    visi_dosync(inst, req);
}

/*****************************************************************************/

EXPORT void vis_copyarea(struct TVisualBase *inst, TINT x, TINT y, TINT w,
    TINT h, TINT dx, TINT dy, TTAGITEM *tags)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_COPYAREA,
        inst->vis_Display, TNULL);
    req->tvr_Op.CopyArea.Window = inst->vis_Window;
    req->tvr_Op.CopyArea.Rect[0] = x;
    req->tvr_Op.CopyArea.Rect[1] = y;
    req->tvr_Op.CopyArea.Rect[2] = w;
    req->tvr_Op.CopyArea.Rect[3] = h;
    req->tvr_Op.CopyArea.DestX = dx;
    req->tvr_Op.CopyArea.DestY = dy;
    req->tvr_Op.CopyArea.Tags = tags;
    visi_dosync(inst, req);
}

/*****************************************************************************/

EXPORT void vis_setcliprect(struct TVisualBase *inst, TINT x, TINT y, TINT w,
    TINT h, TTAGITEM *tags)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_SETCLIPRECT,
        inst->vis_Display, TNULL);
    req->tvr_Op.ClipRect.Window = inst->vis_Window;
    req->tvr_Op.ClipRect.Rect[0] = x;
    req->tvr_Op.ClipRect.Rect[1] = y;
    req->tvr_Op.ClipRect.Rect[2] = w;
    req->tvr_Op.ClipRect.Rect[3] = h;
    req->tvr_Op.ClipRect.Tags = tags;
    visi_dosync(inst, req);
}

/*****************************************************************************/

EXPORT void vis_unsetcliprect(struct TVisualBase *inst)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_UNSETCLIPRECT,
        inst->vis_Display, TNULL);
    req->tvr_Op.ClipRect.Window = inst->vis_Window;
    visi_dosync(inst, req);
}

/*****************************************************************************/

static THOOKENTRY TTAG destroy_displayhandle(struct THook *hook, TAPTR obj, 
    TTAG msg)
{
    if (msg != TMSG_DESTROY)
        return 0;
    struct vis_DisplayHandle *hnd = obj;
    struct TExecBase *TExecBase = TGetExecBase(&hnd->handle);
    TCloseModule(hnd->display);
    TFree(hnd);
    return 0;
}

EXPORT TAPTR vis_opendisplay(struct TVisualBase *mod, TTAGITEM *tags)
{
    struct TExecBase *TExecBase = TGetExecBase(mod);
    /* displaybase in taglist? */
    TAPTR display =
        (struct TVRequest *) TGetTag(tags, TVisual_Display, TNULL);
    if (display == TNULL)
    {
        TSTRPTR name;
        struct vis_DisplayHandle *hnd;

        /* displayname in taglist? */
        name = (TSTRPTR) TGetTag(tags, TVisual_DisplayName, TNULL);

        /* use default name: */
        if (name == TNULL)
            name = DEF_DISPLAYNAME;

        /* lookup display by name: */
        TLock(mod->vis_Lock);
        hnd = (struct vis_DisplayHandle *) TFindHandle(&mod->vis_Displays, name);
        if (hnd)
            display = hnd->display;
        TUnlock(mod->vis_Lock);

        if (!display)
        {
            /* try to open named display: */
            TDBPRINTF(TDB_INFO,("loading module %s\n", name));
            display = TOpenModule(name, 0, tags);
            if (display)
            {
                hnd = TAlloc(TNULL, sizeof *hnd + TStrLen(name) + 1);
                if (hnd)
                {
                    TStrCpy((TSTRPTR) (hnd + 1), name);
                    hnd->handle.thn_Name = (TSTRPTR) (hnd + 1);
                    hnd->handle.thn_Owner = TExecBase;
                    hnd->display = display;
                    TInitHook(&hnd->handle.thn_Hook, destroy_displayhandle, hnd);
                    TLock(mod->vis_Lock);
                    TAddHead(&mod->vis_Displays, &hnd->handle.thn_Node);
                    TUnlock(mod->vis_Lock);
                }
                else
                {
                    TCloseModule(display);
                    display = TNULL;
                }
            }
        }
    }
    return display;
}

/*****************************************************************************/

EXPORT void vis_closedisplay(struct TVisualBase *mod, TAPTR display)
{
}

/*****************************************************************************/

EXPORT TAPTR vis_querydisplays(struct TVisualBase *mod, TTAGITEM *tags)
{
    return TNULL;
}

/*****************************************************************************/

EXPORT TTAGITEM *vis_getnextdisplay(struct TVisualBase *mod, TAPTR dqhandle)
{
    return TNULL;
}

/*****************************************************************************/

EXPORT void vis_drawbuffer(struct TVisualBase *inst,
    TINT x, TINT y, TAPTR buf, TINT w, TINT h, TINT totw, TTAGITEM *tags)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_DRAWBUFFER,
        inst->vis_Display, TNULL);
    req->tvr_Op.DrawBuffer.Window = inst->vis_Window;
    req->tvr_Op.DrawBuffer.Buf = buf;
    req->tvr_Op.DrawBuffer.RRect[0] = x;
    req->tvr_Op.DrawBuffer.RRect[1] = y;
    req->tvr_Op.DrawBuffer.RRect[2] = w;
    req->tvr_Op.DrawBuffer.RRect[3] = h;
    req->tvr_Op.DrawBuffer.TotWidth = totw;
    req->tvr_Op.DrawBuffer.Tags = tags;
    visi_dosync(inst, req);
}

/*****************************************************************************/

EXPORT TAPTR vis_getselection(struct TVisualBase *inst, TTAGITEM *tags)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_GETSELECTION,
        inst->vis_Display, TNULL);
    TSIZE *plen = (TSIZE *) TGetTag(tags, TVisual_SelectionLength, TNULL);
    req->tvr_Op.GetSelection.Type = TGetTag(tags, TVisual_SelectionType, 1);
    req->tvr_Op.GetSelection.Length = 0;
    req->tvr_Op.GetSelection.Data = TNULL;
    visi_dosync(inst, req);
    if (plen) *plen = req->tvr_Op.GetSelection.Length;
    return req->tvr_Op.GetSelection.Data;
}

/*****************************************************************************/

EXPORT TINT vis_setselection(struct TVisualBase *inst, TSTRPTR selection,
    TSIZE len, TTAGITEM *tags)
{
    struct TVRequest *req = visi_getreq(inst, TVCMD_SETSELECTION,
        inst->vis_Display, TNULL);
    req->tvr_Op.SetSelection.Type = TGetTag(tags, TVisual_SelectionType, 1);
    req->tvr_Op.SetSelection.Length = len;
    req->tvr_Op.SetSelection.Data = selection;
    visi_dosync(inst, req);
    return 0;
}
