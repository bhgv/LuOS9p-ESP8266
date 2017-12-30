
#include <assert.h>
#include <string.h>
#include "display_x11_mod.h"
#include <tek/lib/imgcache.h>
#include <tek/inline/exec.h>

#include <stdio.h>


#if defined(ENABLE_XSHM)
#warning using globals (-DENABLE_XSHM)

static TBOOL x11_shm_available = TTRUE;

static int shm_errhandler(Display *d, XErrorEvent *evt)
{
    TDBPRINTF(TDB_ERROR, ("Remote display - fallback to normal XPutImage\n"));
    x11_shm_available = TFALSE;
    return 0;
}

static void x11_releasesharedmemory(struct X11Display *mod,
    struct X11Window *v)
{
    if (v->shmsize > 0)
    {
        XShmDetach(mod->x11_Display, &v->shminfo);
        shmdt(v->shminfo.shmaddr);
        shmctl(v->shminfo.shmid, IPC_RMID, 0);
    }
}

static TAPTR x11_getsharedmemory(struct X11Display *mod, struct X11Window *v,
    size_t size)
{
    if (!(mod->x11_Flags & X11FL_SHMAVAIL))
        return TNULL;
    if (v->shmsize > 0 && size <= v->shmsize)
        return v->shminfo.shmaddr;
    x11_releasesharedmemory(mod, v);
    v->shminfo.shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0777);
    if (v->shminfo.shmid != -1)
    {
        XErrorHandler oldhnd;

        v->shminfo.readOnly = False;
        XSync(mod->x11_Display, 0);
        oldhnd = XSetErrorHandler(shm_errhandler);
        x11_shm_available = TTRUE;
        XShmAttach(mod->x11_Display, &v->shminfo);
        TDBPRINTF(TDB_TRACE, ("shmattach size=%d\n", (int) size));
        XSync(mod->x11_Display, 0);
        XSetErrorHandler(oldhnd);
        if (x11_shm_available)
        {
            v->shminfo.shmaddr = shmat(v->shminfo.shmid, 0, 0);
            v->shmsize = size;
        }
        else
        {
            shmdt(v->shminfo.shmaddr);
            shmctl(v->shminfo.shmid, IPC_RMID, 0);
            /* ah, just forget it altogether: */
            mod->x11_Flags &= ~X11FL_SHMAVAIL;
            v->shmsize = 0;
            return TNULL;
        }
    }
    return v->shminfo.shmaddr;
}

#endif

static void x11i_freepen(struct X11Display *mod, struct X11Window *v,
    struct X11Pen *pen)
{
    TAPTR TExecBase = TGetExecBase(mod);

    TRemove(&pen->node);
    XFreeColors(mod->x11_Display, v->colormap, &pen->color.pixel, 1, 0);
#if defined(ENABLE_XFT)
    if (mod->x11_Flags & X11FL_USE_XFT)
        XftColorFree(mod->x11_Display, mod->x11_Visual,
            v->colormap, &pen->xftcolor);
#endif
    TFree(pen);
}

static void x11_freepen(struct X11Display *mod, struct TVRequest *req)
{
    struct X11Window *v = req->tvr_Op.FreePen.Window;
    struct X11Pen *pen = (struct X11Pen *) req->tvr_Op.FreePen.Pen;

    x11i_freepen(mod, v, pen);
}

static void x11_freeimage(struct X11Display *mod, struct X11Window *v)
{
    if (v->image)
    {
        v->image->data = NULL;
        XDestroyImage(v->image);
        v->image = TNULL;
    }
}

static void x11_closevisual(struct X11Display *mod, struct TVRequest *req)
{
    TAPTR TExecBase = TGetExecBase(mod);
    struct X11Window *v = req->tvr_Op.OpenWindow.Window;
    struct X11Pen *pen;

    if (v == TNULL)
        return;

    TRemove(&v->node);
    if (TISLISTEMPTY(&mod->x11_vlist))
    {
        /* last window closed - clear global fullscreen state */
        mod->x11_Flags &= ~X11FL_FULLSCREEN;
    }

    if (v->eventmask & TITYPE_INTERVAL)
        mod->x11_NumInterval--;

    x11_freeimage(mod, v);
    TFree(v->tempbuf);
#if defined(ENABLE_XSHM)
    x11_releasesharedmemory(mod, v);
#endif
#if defined(ENABLE_XFT)
    if ((mod->x11_Flags & X11FL_USE_XFT) && v->draw)
        XftDrawDestroy(v->draw);
#endif
#if defined(ENABLE_XVID)
    if (v->flags & X11WFL_CHANGE_VIDMODE)
    {
        XUngrabKeyboard(mod->x11_Display, CurrentTime);
        XUngrabPointer(mod->x11_Display, CurrentTime);
    }
#endif

    if (v->window)
        XUnmapWindow(mod->x11_Display, v->window);
    if (v->gc)
        XFreeGC(mod->x11_Display, v->gc);
    if (v->window)
        XDestroyWindow(mod->x11_Display, v->window);

#if defined(ENABLE_XVID)
    if (v->flags & X11WFL_CHANGE_VIDMODE)
    {
        XF86VidModeSwitchToMode(mod->x11_Display, mod->x11_Screen,
            &mod->x11_OldMode);
        XFlush(mod->x11_Display);
        mod->x11_FullScreenWidth = 0;
        mod->x11_FullScreenHeight = 0;
    }
#endif

    while ((pen = (struct X11Pen *) TRemHead(&v->penlist)))
        x11i_freepen(mod, v, pen);

    if (v->colormap)
        XFreeColormap(mod->x11_Display, v->colormap);
    if (v->sizehints)
        XFree(v->sizehints);

    mod->x11_fm.defref--;

    mod->x11_NumWindows--;

    TFree(v);
}

static int x11_seteventmask(struct X11Display *mod, struct X11Window *v,
    TUINT eventmask)
{
    int x11_mask = v->base_mask;
    TUINT oldmask = v->eventmask;

    if (oldmask & TITYPE_INTERVAL)
        mod->x11_NumInterval--;
    if (eventmask & TITYPE_INTERVAL)
        mod->x11_NumInterval++;

    if (eventmask & TITYPE_REFRESH)
        x11_mask |= StructureNotifyMask | ExposureMask;
    if (eventmask & TITYPE_MOUSEOVER)
        x11_mask |= LeaveWindowMask | EnterWindowMask;
    if (eventmask & TITYPE_NEWSIZE)
        x11_mask |= StructureNotifyMask;
    if (eventmask & TITYPE_KEYDOWN)
        x11_mask |= KeyPressMask | KeyReleaseMask;
    if (eventmask & TITYPE_KEYUP)
        x11_mask |= KeyPressMask | KeyReleaseMask;
    if ((v->flags & X11WFL_IS_ROOTWINDOW) || (eventmask & TITYPE_MOUSEMOVE))
        x11_mask |= PointerMotionMask | OwnerGrabButtonMask |
            ButtonMotionMask | ButtonPressMask | ButtonReleaseMask;
    if (eventmask & TITYPE_MOUSEBUTTON)
        x11_mask |= ButtonPressMask | ButtonReleaseMask | OwnerGrabButtonMask;
    v->eventmask = eventmask;
    return x11_mask;
}

static void x11_openvisual(struct X11Display *mod, struct TVRequest *req)
{
    TAPTR TExecBase = TGetExecBase(mod);
    TTAGITEM *tags = req->tvr_Op.OpenWindow.Tags;
    struct X11Window *v = TAlloc0(mod->x11_MemMgr, sizeof(struct X11Window));
    XTextProperty title_prop;
    TBOOL save_under = TFALSE;

    req->tvr_Op.OpenWindow.Window = v;
    if (v == TNULL)
        return;

    v->userdata = TGetTag(tags, TVisual_UserData, TNULL);

    for (;;)
    {
        XSetWindowAttributes swa;
        TUINT swa_mask;
        XGCValues gcv;
        TUINT gcv_mask;
        struct FontNode *fn;
        TBOOL setfocus = TFALSE;
        TBOOL borderless = TGetTag(tags, TVisual_Borderless, TFALSE);
        TBOOL popupwindow = TGetTag(tags, TVisual_PopupWindow, TFALSE);
        TINT minw = (TINT) TGetTag(tags, TVisual_MinWidth, -1);
        TINT minh = (TINT) TGetTag(tags, TVisual_MinHeight, -1);
        TINT maxw = (TINT) TGetTag(tags, TVisual_MaxWidth, 1000000);
        TINT maxh = (TINT) TGetTag(tags, TVisual_MaxHeight, 1000000);
        TINT sw = mod->x11_ScreenWidth;
        TINT sh = mod->x11_ScreenHeight;

        if (mod->x11_FullScreenWidth != 0)
        {
            sw = mod->x11_FullScreenWidth;
            sh = mod->x11_FullScreenHeight;
        }

        swa_mask = CWColormap | CWEventMask;

        fn = mod->x11_fm.deffont;
        v->curfont = fn;
        mod->x11_fm.defref++;

        TInitList(&v->penlist);

        TInitList(&v->imsgqueue);
        v->imsgport = req->tvr_Op.OpenWindow.IMsgPort;

        v->sizehints = XAllocSizeHints();
        if (v->sizehints == TNULL)
            break;

        v->sizehints->flags = 0;

        v->title = (TSTRPTR)
            TGetTag(tags, TVisual_Title, (TTAG) "TEKlib visual");

        /* size/position calculation: */

        v->winwidth = (TINT) TGetTag(tags, TVisual_Width, X11_DEF_WINWIDTH);
        v->winheight = (TINT) TGetTag(tags, TVisual_Height, X11_DEF_WINHEIGHT);

        v->winwidth = TCLAMP(minw, v->winwidth, maxw);
        v->winheight = TCLAMP(minh, v->winheight, maxh);
        v->winwidth = TMIN(v->winwidth, sw);
        v->winheight = TMIN(v->winheight, sh);

        v->flags &= ~X11WFL_CHANGE_VIDMODE;

        if (TGetTag(tags, TVisual_FullScreen, TFALSE))
        {
#if defined(ENABLE_XVID)
            if (mod->x11_FullScreenWidth == 0)
            {
                XF86VidModeModeInfo **modes;
                int modeNum;
                int i;

                XF86VidModeGetAllModeLines(mod->x11_Display,
                    mod->x11_Screen, &modeNum, &modes);
                for (i = 0; i < modeNum; i++)
                {
                    if ((modes[i]->hdisplay == v->winwidth) &&
                        (modes[i]->vdisplay == v->winheight))
                    {
                        mod->x11_OldMode = *modes[0];
                        mod->x11_VidMode = *modes[i];
                        v->flags |= X11WFL_CHANGE_VIDMODE;
                        break;
                    }
                }
                XFree(modes);
            }
#endif

            if (!(v->flags & X11WFL_CHANGE_VIDMODE))
            {
                v->winwidth = sw;
                v->winheight = sh;
            }
            v->winleft = 0;
            v->wintop = 0;
            borderless = TTRUE;
            setfocus = TTRUE;
            mod->x11_Flags |= X11FL_FULLSCREEN;
        }
        else
        {
            if (TGetTag(tags, TVisual_Center, TFALSE))
            {
                v->winleft = (sw - v->winwidth) / 2;
                v->wintop = (sh - v->winheight) / 2;
            }
            else
            {
                v->winleft = (int) TGetTag(tags, TVisual_WinLeft, (TTAG) - 1);
                v->wintop = (int) TGetTag(tags, TVisual_WinTop, (TTAG) - 1);
            }

            if (mod->x11_Flags & X11FL_FULLSCREEN)
            {
                borderless = TTRUE;
                if (!TGetTag(tags, TVisual_Borderless, TFALSE))
                    setfocus = TTRUE;
                if (v->winleft == -1)
                    v->winleft = (sw - v->winwidth) / 2;
                if (v->wintop == -1)
                    v->wintop = (sh - v->winheight) / 2;
            }
        }

        if (v->winleft >= 0 || v->wintop >= 0)
            v->sizehints->flags |= USPosition | USSize;

        if (!borderless)
        {
            v->sizehints->min_width = (TINT)
                TGetTag(tags, TVisual_MinWidth, (TTAG) - 1);
            v->sizehints->min_height = (TINT)
                TGetTag(tags, TVisual_MinHeight, (TTAG) - 1);
            v->sizehints->max_width = (TINT)
                TGetTag(tags, TVisual_MaxWidth, (TTAG) - 1);
            v->sizehints->max_height = (TINT)
                TGetTag(tags, TVisual_MaxHeight, (TTAG) - 1);

            if (v->sizehints->max_width > 0)
                v->winwidth = TMIN(v->winwidth, v->sizehints->max_width);
            if (v->sizehints->max_height > 0)
                v->winheight = TMIN(v->winheight, v->sizehints->max_height);
            if (v->sizehints->min_width > 0)
                v->winwidth = TMAX(v->winwidth, v->sizehints->min_width);
            if (v->sizehints->min_height > 0)
                v->winheight = TMAX(v->winheight, v->sizehints->min_height);

            v->sizehints->min_width =
                v->sizehints->min_width <= 0 ? 1 : v->sizehints->min_width;
            v->sizehints->min_height =
                v->sizehints->min_height <= 0 ? 1 : v->sizehints->min_height;
            v->sizehints->max_width = v->sizehints->max_width <= 0 ?
                1000000 : v->sizehints->max_width;
            v->sizehints->max_height = v->sizehints->max_height <= 0 ?
                1000000 : v->sizehints->max_height;

            v->sizehints->flags |= PMinSize | PMaxSize;
        }

        v->winleft = TMAX(v->winleft, 0);
        v->wintop = TMAX(v->wintop, 0);

        if ((borderless || (mod->x11_Flags & X11FL_FULLSCREEN))
            && mod->x11_NumWindows == 0)
            v->flags |= X11WFL_IS_ROOTWINDOW;

        if (popupwindow || borderless)
        {
            swa_mask |= CWOverrideRedirect;
            swa.override_redirect = True;
            if (!(v->flags & X11WFL_IS_ROOTWINDOW))
                save_under = v->winwidth * v->winheight < sw * sh / 2;
        }

        if (save_under)
        {
            swa_mask |= CWSaveUnder;
            swa.save_under = True;
        }
        /*else
        {
            swa_mask |= CWBackingStore;
            swa.backing_store = True;
        }*/

        v->colormap = DefaultColormap(mod->x11_Display, mod->x11_Screen);
        if (v->colormap == TNULL)
            break;

        swa.colormap = v->colormap;

        v->base_mask = StructureNotifyMask | ExposureMask | FocusChangeMask;
        swa.event_mask = x11_seteventmask(mod, v,
            (TUINT) TGetTag(tags, TVisual_EventMask, 0));

        if (TGetTag(tags, TVisual_BlankCursor, TFALSE))
        {
            swa.cursor = mod->x11_NullCursor;
            swa_mask |= CWCursor;
        }
#if defined(ENABLE_DEFAULTCURSOR)
        else
        {
            swa.cursor = mod->x11_DefaultCursor;
            swa_mask |= CWCursor;
        }
#endif

#if defined(ENABLE_XVID)
        if (v->flags & X11WFL_CHANGE_VIDMODE)
        {
            XF86VidModeSwitchToMode(mod->x11_Display, mod->x11_Screen,
                &mod->x11_VidMode);
            XF86VidModeSetViewPort(mod->x11_Display, mod->x11_Screen, 0, 0);
            /* damned, argh: */
            XSync(mod->x11_Display, False);
            /*TTIME waitt = { 900000 };
               TWaitTime(&waitt, 0); */
            /* Is my computer broken? Or is it just KDE? */
        }
#endif

        v->window = XCreateWindow(mod->x11_Display,
            RootWindow(mod->x11_Display, mod->x11_Screen),
            v->winleft, v->wintop, v->winwidth, v->winheight,
            0, CopyFromParent, CopyFromParent, CopyFromParent, swa_mask, &swa);

        if (v->window == TNULL)
            break;

        /*Xutf8SetWMProperties(mod->x11_Display, v->window, v->title, v->title, 
           NULL, 0, v->sizehints, NULL, NULL); */

        if (v->sizehints->flags)
            XSetWMNormalHints(mod->x11_Display, v->window, v->sizehints);

#if defined(__hpux) || defined(__sparc)
        XStringListToTextProperty((char **) &v->title, 1, &title_prop);
#else
        if (Xutf8TextListToTextProperty(mod->x11_Display,
                (char **) &v->title, 1, XUTF8StringStyle, &title_prop) ==
            Success)
#endif
        {
            XSetWMProperties(mod->x11_Display, v->window, &title_prop,
                NULL, NULL, 0, NULL, NULL, NULL);
            XFree(title_prop.value);
        }

        v->atom_wm_delete_win = XInternAtom(mod->x11_Display,
            "WM_DELETE_WINDOW", True);
        XSetWMProtocols(mod->x11_Display, v->window,
            &v->atom_wm_delete_win, 1);

        if (mod->x11_Flags & X11FL_FULLSCREEN)
            XMapRaised(mod->x11_Display, v->window);
        else
            XMapWindow(mod->x11_Display, v->window);

        for (;;)
        {
            XEvent e;

            XNextEvent(mod->x11_Display, &e);
            if (e.type == MapNotify)
                break;
        }

#if defined(ENABLE_XVID)
        if (v->flags & X11WFL_CHANGE_VIDMODE)
        {
            XMoveWindow(mod->x11_Display, v->window, 0, 0);
            XResizeWindow(mod->x11_Display, v->window, v->winwidth,
                v->winheight);

            XGrabKeyboard(mod->x11_Display, v->window, True,
                GrabModeAsync, GrabModeAsync, CurrentTime);
            XGrabPointer(mod->x11_Display, v->window, True,
                ButtonPressMask, GrabModeAsync, GrabModeAsync,
                v->window, None, CurrentTime);
            XWarpPointer(mod->x11_Display, None, v->window, 0, 0, 0, 0, 0, 0);
            XWarpPointer(mod->x11_Display, None, v->window, 0, 0, 0, 0,
                v->winwidth / 2, v->winheight / 2);

            mod->x11_FullScreenWidth = v->winwidth;
            mod->x11_FullScreenHeight = v->winheight;
            mod->x11_ScreenWidth = v->winwidth;
            mod->x11_ScreenHeight = v->winheight;
        }
#endif

        if (setfocus)
            XSetInputFocus(mod->x11_Display, v->window, RevertToParent,
                CurrentTime);

        gcv.function = GXcopy; //GXxor; //GXcopy;
        gcv.fill_style = FillSolid;
        gcv.graphics_exposures = True;
        gcv_mask = GCFunction | GCFillStyle | GCGraphicsExposures;

        v->gc = XCreateGC(mod->x11_Display, v->window, gcv_mask, &gcv);
        XCopyGC(mod->x11_Display,
            DefaultGC(mod->x11_Display, mod->x11_Screen),
            GCForeground | GCBackground, v->gc);

#if defined(ENABLE_XFT)
        if (mod->x11_Flags & X11FL_USE_XFT)
        {
            v->draw = XftDrawCreate(mod->x11_Display,
                v->window, mod->x11_Visual, v->colormap);
            if (!v->draw)
                break;
        }
#endif

        v->bgpen = TVPEN_UNDEFINED;
        v->fgpen = TVPEN_UNDEFINED;

        TDBPRINTF(TDB_TRACE, ("Created new window: %p\n", v->window));
        TAddTail(&mod->x11_vlist, &v->node);

        /* success: */
        mod->x11_NumWindows++;

        /*mod->x11_RequestInProgress = req; */
        return;
    }

    /* failure: */
    x11_closevisual(mod, req);
    req->tvr_Op.OpenWindow.Window = TNULL;
}

static void x11_setinput(struct X11Display *mod, struct TVRequest *req)
{
    struct X11Window *v = req->tvr_Op.SetInput.Window;
    TUINT eventmask = req->tvr_Op.SetInput.Mask;

    XSelectInput(mod->x11_Display, v->window,
        x11_seteventmask(mod, v, eventmask));
    /* spool out possible remaining messages: */
    x11_sendimessages(mod);

}

static void x11_allocpen(struct X11Display *mod, struct TVRequest *req)
{
    TAPTR TExecBase = TGetExecBase(mod);
    struct X11Window *v = req->tvr_Op.AllocPen.Window;
    TUINT rgb = req->tvr_Op.AllocPen.RGB;
    struct X11Pen *pen = TAlloc(mod->x11_MemMgr, sizeof(struct X11Pen));

    if (pen)
    {
        TUINT r = (rgb >> 16) & 0xff;
        TUINT g = (rgb >> 8) & 0xff;
        TUINT b = rgb & 0xff;

        r = (r << 8) | r;
        g = (g << 8) | g;
        b = (b << 8) | b;

        pen->color.red = r;
        pen->color.green = g;
        pen->color.blue = b;
        pen->color.flags = DoRed | DoGreen | DoBlue;
        if (XAllocColor(mod->x11_Display, v->colormap, &pen->color))
        {
            TBOOL success = TTRUE;

#if defined(ENABLE_XFT)
            if (mod->x11_Flags & X11FL_USE_XFT)
            {
                XRenderColor xrcolor;

                xrcolor.red = r;
                xrcolor.green = g;
                xrcolor.blue = b;
                xrcolor.alpha = 0xffff;
                success = XftColorAllocValue(mod->x11_Display, mod->x11_Visual,
                    v->colormap, &xrcolor, &pen->xftcolor);
            }
#endif
            if (success)
            {
                TAddTail(&v->penlist, &pen->node);
                req->tvr_Op.AllocPen.Pen = (TVPEN) pen;
                return;
            }
            XFreeColors(mod->x11_Display, v->colormap, &pen->color.pixel,
                1, 0);
        }
        TFree(pen);
    }
    req->tvr_Op.AllocPen.Pen = TVPEN_UNDEFINED;
}

/*****************************************************************************/

static void setbgpen(struct X11Display *mod, struct X11Window *v, TVPEN pen)
{
    if (pen != v->bgpen && pen != TVPEN_UNDEFINED)
    {
        XGCValues gcv;

        gcv.background = ((struct X11Pen *) pen)->color.pixel;
        XChangeGC(mod->x11_Display, v->gc, GCBackground, &gcv);
        v->bgpen = pen;
    }
}

static TVPEN setfgpen(struct X11Display *mod, struct X11Window *v, TVPEN pen)
{
    TVPEN oldpen = v->fgpen;

    if (pen != oldpen && pen != TVPEN_UNDEFINED)
    {
        XGCValues gcv;

        gcv.foreground = ((struct X11Pen *) pen)->color.pixel;
        XChangeGC(mod->x11_Display, v->gc, GCForeground, &gcv);
        v->fgpen = pen;
        if (oldpen == (TVPEN) 0xffffffff)
            oldpen = pen;
    }
    return oldpen;
}

/*****************************************************************************/

static void x11_frect(struct X11Display *mod, struct TVRequest *req)
{
    struct X11Window *v = req->tvr_Op.FRect.Window;
    TINT x0 = req->tvr_Op.FRect.Rect[0];
    TINT y0 = req->tvr_Op.FRect.Rect[1];
    TINT x1 = x0 + req->tvr_Op.FRect.Rect[2] - 1;
    TINT y1 = y0 + req->tvr_Op.FRect.Rect[3] - 1;

    if (!REGION_OVERLAP(x0, y0, x1, y1, 0, 0, v->winwidth - 1, 
            v->winheight - 1))
        return;

    x0 = TMAX(x0, 0);
    y0 = TMAX(y0, 0);
    x1 = TMIN(x1, v->winwidth - 1);
    y1 = TMIN(y1, v->winheight - 1);

    setfgpen(mod, v, req->tvr_Op.FRect.Pen);
    XFillRectangle(mod->x11_Display, v->window, v->gc,
        x0, y0, x1 - x0 + 1, y1 - y0 + 1);
}

/*****************************************************************************/

static void x11_line(struct X11Display *mod, struct TVRequest *req)
{
    struct X11Window *v = req->tvr_Op.Line.Window;
    TINT x0 = req->tvr_Op.Line.Rect[0];
    TINT y0 = req->tvr_Op.Line.Rect[1];
    TINT x1 = req->tvr_Op.Line.Rect[2];
    TINT y1 = req->tvr_Op.Line.Rect[3];

    if (!REGION_OVERLAP(x0, y0, x1, y1, 0, 0, v->winwidth - 1,
            v->winheight - 1))
        return;
    setfgpen(mod, v, req->tvr_Op.Line.Pen);
    XDrawLine(mod->x11_Display, v->window, v->gc, x0, y0, x1, y1);
}

/*****************************************************************************/

static void x11_rect(struct X11Display *mod, struct TVRequest *req)
{
    struct X11Window *v = req->tvr_Op.Rect.Window;
    TINT x0 = req->tvr_Op.FRect.Rect[0];
    TINT y0 = req->tvr_Op.FRect.Rect[1];
    TINT x1 = x0 + req->tvr_Op.FRect.Rect[2] - 1;
    TINT y1 = y0 + req->tvr_Op.FRect.Rect[3] - 1;

    if (!REGION_OVERLAP(x0, y0, x1, y1, 0, 0, v->winwidth - 1, 
            v->winheight - 1))
        return;
    setfgpen(mod, v, req->tvr_Op.Rect.Pen);
    XDrawRectangle(mod->x11_Display, v->window, v->gc,
        x0, y0, x1 - x0, y1 - y0);
}

/*****************************************************************************/

static void x11_plot(struct X11Display *mod, struct TVRequest *req)
{
    struct X11Window *v = req->tvr_Op.Plot.Window;
    TUINT x0 = req->tvr_Op.Plot.Rect[0];
    TUINT y0 = req->tvr_Op.Plot.Rect[1];

    setfgpen(mod, v, req->tvr_Op.Plot.Pen);
    XDrawPoint(mod->x11_Display, v->window, v->gc, x0, y0);
}

/*****************************************************************************/

static void x11_drawstrip(struct X11Display *mod, struct TVRequest *req)
{
    TINT i;
    XPoint tri[3];
    struct X11Window *v = req->tvr_Op.Strip.Window;
    TINT *array = req->tvr_Op.Strip.Array;
    TINT num = req->tvr_Op.Strip.Num;
    TTAGITEM *tags = req->tvr_Op.Strip.Tags;
    TVPEN *penarray = (TVPEN *) TGetTag(tags, TVisual_PenArray, TNULL);
    
    int iscursor = req->tvr_cursor.iscursor;
    int cursor_x = req->tvr_cursor.x;
    int cursor_y = req->tvr_cursor.y;
    int cursor_w = req->tvr_cursor.w;
    int cursor_h = req->tvr_cursor.h;
    
    //printf("x11_drawstrip - %d, %x, (%d, %d, %d, %d)\n", iscursor, (unsigned)v->imgCursorBG,
    //    cursor_x, cursor_y, cursor_w, cursor_h
    //);
    if(iscursor && (
        !v->imgCursorBG ||
        !(
            v->old_crsr_x == cursor_x &&
            v->old_crsr_y == cursor_y &&
            v->old_crsr_w == cursor_w &&
            v->old_crsr_h == cursor_h
        )
      )
    ){
        if(v->imgCursorBG){
            XPutImage(
                mod->x11_Display, v->window, v->gc,
                v->imgCursorBG, 0, 0,
                v->old_crsr_x, v->old_crsr_y, 
                v->old_crsr_w, v->old_crsr_h
            );
            XDestroyImage(v->imgCursorBG); 
        }
        if(
            cursor_x >= 0 && cursor_x < v->winwidth &&
            cursor_y >= 0 && cursor_y < v->winheight
        ){
            int tdw = (cursor_x + cursor_w) - v->winwidth;
            int tdh = (cursor_y + cursor_h) - v->winheight;
            int tw = tdw < 0 ? cursor_w : cursor_w - tdw;
            int th = tdh < 0 ? cursor_h : cursor_h - tdh;

            v->imgCursorBG = XGetImage(
                mod->x11_Display, v->window, //v->gc,
                cursor_x, cursor_y, tw, th,
                AllPlanes ,ZPixmap
            );
            v->old_crsr_x = cursor_x; 
            v->old_crsr_y = cursor_y; 
            v->old_crsr_w = tw;
            v->old_crsr_h = th;
        }else{
            v->imgCursorBG = 0;
        }
    }

    if (num < 3)
        return;

    if (penarray)
    {
        setfgpen(mod, v, penarray[2]);
    }
    else
    {
        TVPEN pen = (TVPEN) TGetTag(tags, TVisual_Pen, TVPEN_UNDEFINED);

        setfgpen(mod, v, pen);
    }

    tri[0].x = (TINT16) array[0];
    tri[0].y = (TINT16) array[1];
    tri[1].x = (TINT16) array[2];
    tri[1].y = (TINT16) array[3];
    tri[2].x = (TINT16) array[4];
    tri[2].y = (TINT16) array[5];

    XFillPolygon(mod->x11_Display, v->window, v->gc, tri, 3,
        Convex, CoordModeOrigin);

    for (i = 3; i < num; i++)
    {
        tri[0].x = tri[1].x;
        tri[0].y = tri[1].y;
        tri[1].x = tri[2].x;
        tri[1].y = tri[2].y;
        tri[2].x = (TINT16) array[i * 2];
        tri[2].y = (TINT16) array[i * 2 + 1];

        if (penarray)
            setfgpen(mod, v, penarray[i]);

        XFillPolygon(mod->x11_Display, v->window, v->gc, tri, 3,
            Convex, CoordModeOrigin);
    }
}

/*****************************************************************************/

static void x11_drawfan(struct X11Display *mod, struct TVRequest *req)
{
    TINT i;
    XPoint tri[3];
    struct X11Window *v = req->tvr_Op.Fan.Window;
    TINT *array = req->tvr_Op.Fan.Array;
    TINT num = req->tvr_Op.Fan.Num;
    TTAGITEM *tags = req->tvr_Op.Fan.Tags;
    TVPEN pen = (TVPEN) TGetTag(tags, TVisual_Pen, TVPEN_UNDEFINED);
    TVPEN *penarray = (TVPEN *) TGetTag(tags, TVisual_PenArray, TNULL);
    
    int iscursor = req->tvr_cursor.iscursor;
    int cursor_x = req->tvr_cursor.x;
    int cursor_y = req->tvr_cursor.y;
    int cursor_w = req->tvr_cursor.w;
    int cursor_h = req->tvr_cursor.h;
    
    //printf("x11_drawfan - %d, %x, (%d, %d, %d, %d)\n", iscursor, (unsigned)v->imgCursorBG,
    //    cursor_x, cursor_y, cursor_w, cursor_h
    //);
    if(iscursor && (
        !v->imgCursorBG ||
        !(
            v->old_crsr_x == cursor_x &&
            v->old_crsr_y == cursor_y &&
            v->old_crsr_w == cursor_w &&
            v->old_crsr_h == cursor_h
        )
      )
    ){
        if(v->imgCursorBG){
            XPutImage(
                mod->x11_Display, v->window, v->gc,
                v->imgCursorBG, 0, 0,
                v->old_crsr_x, v->old_crsr_y, 
                v->old_crsr_w, v->old_crsr_h
            );
            XDestroyImage(v->imgCursorBG); 
        }
        if(
            cursor_x >= 0 && cursor_x < v->winwidth &&
            cursor_y >= 0 && cursor_y < v->winheight
        ){
            int tdw = (cursor_x + cursor_w) - v->winwidth;
            int tdh = (cursor_y + cursor_h) - v->winheight;
            int tw = tdw < 0 ? cursor_w : cursor_w - tdw;
            int th = tdh < 0 ? cursor_h : cursor_h - tdh;

            v->imgCursorBG = XGetImage(
                mod->x11_Display, v->window, //v->gc,
                cursor_x, cursor_y, tw, th,
                AllPlanes ,ZPixmap
            );
            v->old_crsr_x = cursor_x; 
            v->old_crsr_y = cursor_y; 
            v->old_crsr_w = tw;
            v->old_crsr_h = th;
        }else{
            v->imgCursorBG = 0;
        }
    }

    if (num < 3)
        return;

    if (penarray)
        setfgpen(mod, v, penarray[2]);
    else
        setfgpen(mod, v, pen);

    tri[0].x = (TINT16) array[0];
    tri[0].y = (TINT16) array[1];
    tri[1].x = (TINT16) array[2];
    tri[1].y = (TINT16) array[3];
    tri[2].x = (TINT16) array[4];
    tri[2].y = (TINT16) array[5];

    XFillPolygon(mod->x11_Display, v->window, v->gc, tri, 3,
        Convex, CoordModeOrigin);

    for (i = 3; i < num; i++)
    {
        tri[1].x = tri[2].x;
        tri[1].y = tri[2].y;
        tri[2].x = (TINT16) array[i * 2];
        tri[2].y = (TINT16) array[i * 2 + 1];

        if (penarray)
            setfgpen(mod, v, penarray[i]);

        XFillPolygon(mod->x11_Display, v->window, v->gc, tri, 3,
            Convex, CoordModeOrigin);
    }
}

/*****************************************************************************/

static void x11_copyarea(struct X11Display *mod, struct TVRequest *req)
{
    struct X11Window *v = req->tvr_Op.CopyArea.Window;
    TINT x = req->tvr_Op.CopyArea.Rect[0];
    TINT y = req->tvr_Op.CopyArea.Rect[1];
    TINT w = req->tvr_Op.CopyArea.Rect[2];
    TINT h = req->tvr_Op.CopyArea.Rect[3];
    TINT dx = req->tvr_Op.CopyArea.DestX;
    TINT dy = req->tvr_Op.CopyArea.DestY;

    XCopyArea(mod->x11_Display, v->window, v->window, v->gc,
        x, y, w, h, dx, dy);

    mod->x11_CopyExposeHook = (struct THook *)
        TGetTag(req->tvr_Op.CopyArea.Tags, TVisual_ExposeHook, TNULL);
    if (mod->x11_CopyExposeHook)
    {
        /* register request in progress: */
        mod->x11_RequestInProgress = req;
    }
}

/*****************************************************************************/

static void x11_setcliprect(struct X11Display *mod, struct TVRequest *req)
{
    struct X11Window *v = req->tvr_Op.ClipRect.Window;
    TINT x = req->tvr_Op.ClipRect.Rect[0];
    TINT y = req->tvr_Op.ClipRect.Rect[1];
    TINT w = req->tvr_Op.ClipRect.Rect[2];
    TINT h = req->tvr_Op.ClipRect.Rect[3];
    Region region;
    XRectangle rectangle;

    region = XCreateRegion();

    rectangle.x = (short) x;
    rectangle.y = (short) y;
    rectangle.width = (unsigned short) w;
    rectangle.height = (unsigned short) h;

    /* union rect into region */
    XUnionRectWithRegion(&rectangle, region, region);
    /* set clip region */
    XSetRegion(mod->x11_Display, v->gc, region);

#if defined(ENABLE_XFT)
    if (mod->x11_Flags & X11FL_USE_XFT)
        XftDrawSetClip(v->draw, region);
#endif

    XDestroyRegion(region);
}

/*****************************************************************************/

static void x11_unsetcliprect(struct X11Display *mod, struct TVRequest *req)
{
    struct X11Window *v = req->tvr_Op.ClipRect.Window;

    /*XSetClipMask(mod->x11_Display, v->gc, None); */
    XSetRegion(mod->x11_Display, v->gc, mod->x11_HugeRegion);
#if defined(ENABLE_XFT)
    if (mod->x11_Flags & X11FL_USE_XFT)
        XftDrawSetClip(v->draw, mod->x11_HugeRegion);
#endif
}

/*****************************************************************************/

static void x11_clear(struct X11Display *mod, struct TVRequest *req)
{
    struct X11Window *v = req->tvr_Op.Clear.Window;

    setfgpen(mod, v, req->tvr_Op.Clear.Pen);
    XFillRectangle(mod->x11_Display, v->window, v->gc,
        0, 0, v->winwidth, v->winheight);
        
    if(v->imgCursorBG){
        XDestroyImage(v->imgCursorBG);
        v->imgCursorBG = 0;
    }
}

/*****************************************************************************/

static THOOKENTRY TTAG getattrfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
    struct attrdata *data = hook->thk_Data;
    TTAGITEM *item = obj;
    struct X11Window *v = data->v;
    struct X11Display *mod = data->mod;

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
            *((TINT *) item->tti_Value) = mod->x11_ScreenWidth;
            break;
        case TVisual_ScreenHeight:
            *((TINT *) item->tti_Value) = mod->x11_ScreenHeight;
            break;
        case TVisual_WinLeft:
            *((TINT *) item->tti_Value) = v->winleft;
            break;
        case TVisual_WinTop:
            *((TINT *) item->tti_Value) = v->wintop;
            break;
        case TVisual_MinWidth:
            *((TINT *) item->tti_Value) = v->sizehints->min_width;
            break;
        case TVisual_MinHeight:
            *((TINT *) item->tti_Value) = v->sizehints->min_height;
            break;
        case TVisual_MaxWidth:
            *((TINT *) item->tti_Value) = v->sizehints->max_width;
            break;
        case TVisual_MaxHeight:
            *((TINT *) item->tti_Value) = v->sizehints->max_height;
            break;
        case TVisual_HaveClipboard:
        {
            struct X11Display *mod = data->mod;

            *((TINT *) item->tti_Value) = XGetSelectionOwner(mod->x11_Display,
                mod->x11_XA_CLIPBOARD) == v->window;
            break;
        }
        case TVisual_HaveSelection:
        {
            struct X11Display *mod = data->mod;

            *((TINT *) item->tti_Value) = XGetSelectionOwner(mod->x11_Display,
                mod->x11_XA_PRIMARY) == v->window;
            break;
        }
        case TVisual_Device:
            *((TAPTR *) item->tti_Value) = data->mod;
            break;
        case TVisual_Window:
            *((TAPTR *) item->tti_Value) = v;
            break;
        case TVisual_HaveWindowManager:
            /* depends, but we assume it is started: */
            *((TBOOL *) item->tti_Value) = TTRUE;
            break;
    }
    data->num++;
    return TTRUE;
}

static THOOKENTRY TTAG setattrfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
    struct attrdata *data = hook->thk_Data;
    TTAGITEM *item = obj;
    struct X11Window *v = data->v;

    switch (item->tti_Tag)
    {
        default:
            return TTRUE;
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
            v->sizehints->min_width = (TINT) item->tti_Value;
            break;
        case TVisual_MinHeight:
            v->sizehints->min_height = (TINT) item->tti_Value;
            break;
        case TVisual_MaxWidth:
            v->sizehints->max_width = (TINT) item->tti_Value;
            break;
        case TVisual_MaxHeight:
            v->sizehints->max_height = (TINT) item->tti_Value;
            break;
        case TVisual_HaveSelection:
        {
            struct X11Display *mod = data->mod;

            XSetSelectionOwner(mod->x11_Display, mod->x11_XA_PRIMARY,
                item->tti_Value ? v->window : None, CurrentTime);
            break;
        }
        case TVisual_HaveClipboard:
        {
            struct X11Display *mod = data->mod;

            XSetSelectionOwner(mod->x11_Display, mod->x11_XA_CLIPBOARD,
                item->tti_Value ? v->window : None, CurrentTime);
            break;
        }
    }
    data->num++;
    return TTRUE;
}

/*****************************************************************************/

static void x11_getattrs(struct X11Display *mod, struct TVRequest *req)
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

static void x11_setattrs(struct X11Display *mod, struct TVRequest *req)
{
    struct attrdata data;
    struct THook hook;
    struct X11Window *v = req->tvr_Op.SetAttrs.Window;

    data.v = v;
    data.num = 0;
    data.mod = mod;
    data.neww = v->winwidth;
    data.newh = v->winheight;
    data.newx = v->winleft;
    data.newy = v->wintop;
    TInitHook(&hook, setattrfunc, &data);

    TForEachTag(req->tvr_Op.SetAttrs.Tags, &hook);
    req->tvr_Op.SetAttrs.Num = data.num;

    if (v->sizehints->max_width < 0)
        v->sizehints->max_width = 1000000;
    if (v->sizehints->max_height < 0)
        v->sizehints->max_height = 1000000;

    v->sizehints->min_width = TMAX(v->sizehints->min_width, 0);
    v->sizehints->max_width = TMAX(v->sizehints->max_width,
        v->sizehints->min_width);
    v->sizehints->min_height = TMAX(v->sizehints->min_height, 0);
    v->sizehints->max_height = TMAX(v->sizehints->max_height,
        v->sizehints->min_height);

    TINT w = data.neww;
    TINT h = data.newh;
    TINT x = data.newx;
    TINT y = data.newy;

    TBOOL moveresize = TFALSE;

    if (w < v->sizehints->min_width || h < v->sizehints->min_height)
    {
        w = TMAX(w, v->sizehints->min_width);
        h = TMAX(h, v->sizehints->min_height);
        moveresize = TTRUE;
    }

    if (x != v->winleft || y != v->wintop ||
        w != v->winwidth || h != v->winheight)
    {
        v->winleft = x;
        v->wintop = y;
        moveresize = TTRUE;
    }

    if (moveresize)
    {
        XMoveResizeWindow(mod->x11_Display, v->window, x, y, w, h);
        mod->x11_RequestInProgress = req;
        v->flags |= X11WFL_WAIT_RESIZE;
    }

    XSetWMNormalHints(mod->x11_Display, v->window, v->sizehints);
}

/*****************************************************************************/

static void x11_drawtext(struct X11Display *mod, struct TVRequest *req)
{
    struct X11Window *v = req->tvr_Op.Text.Window;
    TSTRPTR text = req->tvr_Op.Text.Text;
    TINT len = req->tvr_Op.Text.Length;
    TUINT x = req->tvr_Op.Text.X;
    TUINT y = req->tvr_Op.Text.Y;
    struct X11Pen *fgpen = (struct X11Pen *) req->tvr_Op.Text.FgPen;

    setfgpen(mod, v, (TVPEN) fgpen);

#if defined(ENABLE_XFT)
    if (mod->x11_Flags & X11FL_USE_XFT)
    {
        XftFont *f = ((struct X11FontHandle *) v->curfont)->xftfont;

        XftDrawStringUtf8(v->draw, &fgpen->xftcolor,
            f, x, y + f->ascent, (FcChar8 *) text, len);
    }
    else
#endif
    {
        TSTRPTR latin = x11_utf8tolatin(mod, text, len, &len);

        if (latin)
        {
            XFontStruct *f = ((struct X11FontHandle *) v->curfont)->font;

            XDrawString(mod->x11_Display, v->window, v->gc,
                x, y + f->ascent, (char *) latin, len);
        }
    }
}

/*****************************************************************************/

static void x11_openfont(struct X11Display *mod, struct TVRequest *req)
{
    req->tvr_Op.OpenFont.Font =
        x11_hostopenfont(mod, req->tvr_Op.OpenFont.Tags);
}

/*****************************************************************************/

static void x11_textsize(struct X11Display *mod, struct TVRequest *req)
{
    req->tvr_Op.TextSize.Width =
        x11_hosttextsize(mod, req->tvr_Op.TextSize.Font,
        req->tvr_Op.TextSize.Text, req->tvr_Op.TextSize.NumChars);
}

/*****************************************************************************/

static void x11_getfontattrs(struct X11Display *mod, struct TVRequest *req)
{
    struct attrdata data;
    struct THook hook;

    data.mod = mod;
    data.font = req->tvr_Op.GetFontAttrs.Font;
    data.num = 0;
    TInitHook(&hook, x11_hostgetfattrfunc, &data);

    TForEachTag(req->tvr_Op.GetFontAttrs.Tags, &hook);
    req->tvr_Op.GetFontAttrs.Num = data.num;
}

/*****************************************************************************/

static void x11_setfont(struct X11Display *mod, struct TVRequest *req)
{
    x11_hostsetfont(mod, req->tvr_Op.SetFont.Window, req->tvr_Op.SetFont.Font);
}

/*****************************************************************************/

static void x11_closefont(struct X11Display *mod, struct TVRequest *req)
{
    x11_hostclosefont(mod, req->tvr_Op.CloseFont.Font);
}

/*****************************************************************************/

static void x11_queryfonts(struct X11Display *mod, struct TVRequest *req)
{
    req->tvr_Op.QueryFonts.Handle =
        x11_hostqueryfonts(mod, req->tvr_Op.QueryFonts.Tags);
}

/*****************************************************************************/

static void x11_getnextfont(struct X11Display *mod, struct TVRequest *req)
{
    req->tvr_Op.GetNextFont.Attrs =
        x11_hostgetnextfont(mod, req->tvr_Op.GetNextFont.Handle);
}

/*****************************************************************************/

struct drawdata
{
    struct X11Window *v;
    struct X11Display *mod;
    Display *display;
    Window window;
    GC gc;
    TINT x0, x1, y0, y1;
};

static THOOKENTRY TTAG drawtagfunc(struct THook *hook, TAPTR obj, TTAG msg)
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
                    XFillRectangle(data->display, data->window, data->gc,
                        data->x0, data->y0, data->x1, data->y1);
                    break;
                case TVCMD_RECT:
                    XDrawRectangle(data->display, data->window, data->gc,
                        data->x0, data->y0, data->x1 - 1, data->y1 - 1);
                    break;
                case TVCMD_LINE:
                    //data->gc.function = GXxor;
                    XDrawLine(data->display, data->window, data->gc,
                        data->x0, data->y0, data->x1, data->y1);
                    break;
            }
            break;
    }
    return TTRUE;
}

static void x11_drawtags(struct X11Display *mod, struct TVRequest *req)
{
    struct THook hook;
    struct drawdata data;

    data.v = req->tvr_Op.DrawTags.Window;
    data.mod = mod;
    data.display = mod->x11_Display;
    data.window = data.v->window;
    data.gc = data.v->gc;

    TInitHook(&hook, drawtagfunc, &data);
    TForEachTag(req->tvr_Op.DrawTags.Tags, &hook);
}

/*****************************************************************************/

static TUINT x11_getpixfmtfromimage(struct X11Display *mod,
    struct X11Window *v)
{
    XImage *img = v->image;
    TUINT rm = img->red_mask;
    TUINT gm = img->green_mask;
    TUINT bm = img->blue_mask;
    TUINT d = img->bits_per_pixel;
    TUINT pixfmt = TVPIXFMT_UNDEFINED;

    switch (d)
    {
        case 16:
            v->bpp = 2;
            if (rm == 0x00007c00 && gm == 0x000003e0 && bm == 0x0000001f)
                pixfmt = TVPIXFMT_0R5G5B5;
            else if (rm == 0x0000f800 && gm == 0x000007e0 && bm == 0x0000001f)
                pixfmt = TVPIXFMT_R5G6B5;
            break;
        case 24:
            v->bpp = 4;
            if (rm == 0x00ff0000 && gm == 0x0000ff00 && bm == 0x000000ff)
                pixfmt = TVPIXFMT_08R8G8B8;
            else if (rm == 0x000000ff && gm == 0x0000ff00 && bm == 0x00ff0000)
                pixfmt = TVPIXFMT_08B8G8R8;
            break;
        case 32:
            v->bpp = 4;
            if (rm == 0x00ff0000 && gm == 0x0000ff00 && bm == 0x000000ff)
                pixfmt = TVPIXFMT_08R8G8B8;
            else if (rm == 0x0000ff00 && gm == 0x00ff0000 && bm == 0xff000000)
                pixfmt = TVPIXFMT_B8G8R808;
            else if (rm == 0xff000000 && gm == 0x00ff0000 && bm == 0x0000ff00)
                pixfmt = TVPIXFMT_R8G8B808;
            break;
    }
    assert(pixfmt != TVPIXFMT_UNDEFINED);
    v->pixfmt = pixfmt;
    TDBPRINTF(TDB_INFO, ("pixfmt: %08x - bpp: %d\n", pixfmt, v->bpp));
    return pixfmt;
}

static XImage *x11_getdrawimage(struct X11Display *mod, struct X11Window *v,
    TINT w, TINT h, TUINT8 ** bufptr, TINT *bytes_per_line)
{
    if (w <= 0 || h <= 0)
        return TNULL;

    while (!v->image || w > v->imw || h > v->imh)
    {
        x11_freeimage(mod, v);

#if defined(ENABLE_XSHM)
        if (mod->x11_Flags & X11FL_SHMAVAIL)
        {
            /* TODO: buffer more images, not just 1 */
            v->image = XShmCreateImage(mod->x11_Display, mod->x11_Visual,
                mod->x11_DefaultDepth, ZPixmap, TNULL, &v->shminfo, w, h);
            if (v->image)
            {
                v->image->data = x11_getsharedmemory(mod, v,
                    v->image->bytes_per_line * v->image->height);
                if (v->image->data)
                {
                    v->imw = w;
                    v->imh = h;
                    v->flags |= X11WFL_IMG_SHM;
                    break;
                }
                x11_freeimage(mod, v);
            }
        }
#endif

        if (!v->image)
        {
            TAPTR TExecBase = TGetExecBase(mod);
            TUINT bpp = v->bpp;

            if (bpp == 0)
                bpp = mod->x11_DefaultBPP;
            if (v->tempbuf)
                TFree(v->tempbuf);
            v->tempbuf = TAlloc(TNULL, w * h * bpp);
            if (v->tempbuf)
            {
                v->image = XCreateImage(mod->x11_Display, mod->x11_Visual,
                    mod->x11_DefaultDepth, ZPixmap, 0, NULL, w, h, bpp * 8,
                    bpp * w);
                if (v->image)
                {
                    v->image->data = v->tempbuf;
                    v->imw = w;
                    v->imh = h;
                    break;
                }
                TFree(v->tempbuf);
                v->tempbuf = TNULL;
            }
        }

        return TNULL;
    }

    if (v->pixfmt == TVPIXFMT_UNDEFINED)
        x11_getpixfmtfromimage(mod, v);

    if (v->tempbuf)
    {
        *bufptr = (TUINT8 *) v->tempbuf;
        *bytes_per_line = v->imw * v->bpp;
    }
    else
    {
        *bufptr = (TUINT8 *) v->image->data;
        *bytes_per_line = v->image->bytes_per_line;
    }

    return v->image;
}

static void x11_putimage(struct X11Display *mod, struct X11Window *v,
    struct TVRequest *req, TINT x0, TINT y0, TINT w, TINT h)
{
#if defined(ENABLE_XSHM)
    if (v->flags & X11WFL_IMG_SHM)
    {
        XShmPutImage(mod->x11_Display, v->window, v->gc, v->image, 0, 0,
            x0, y0, w, h, 1);
        mod->x11_RequestInProgress = req;
    }
    else
#endif
        XPutImage(mod->x11_Display, v->window, v->gc, v->image, 0, 0,
            x0, y0, w, h);
}

/*****************************************************************************/

static void x11_drawbuffer(struct X11Display *mod, struct TVRequest *req)
{
    struct TVPixBuf src, dst;
    TTAGITEM *tags = req->tvr_Op.DrawBuffer.Tags;
    struct X11Window *v = req->tvr_Op.DrawBuffer.Window;
    TINT x = req->tvr_Op.DrawBuffer.RRect[0];
    TINT y = req->tvr_Op.DrawBuffer.RRect[1];
    TINT w = req->tvr_Op.DrawBuffer.RRect[2];
    TINT h = req->tvr_Op.DrawBuffer.RRect[3];

    src.tpb_Data = req->tvr_Op.DrawBuffer.Buf;
    src.tpb_Format = TGetTag(tags, TVisual_PixelFormat, TVPIXFMT_A8R8G8B8);
    src.tpb_BytesPerLine = req->tvr_Op.DrawBuffer.TotWidth *
        TVPIXFMT_BYTES_PER_PIXEL(src.tpb_Format);

#if defined(X11_PIXMAP_CACHE)
    struct TVImageCacheRequest *creq = (struct TVImageCacheRequest *)
        TGetTag(tags, TVisual_CacheRequest, TNULL);
    if (creq && v->pixfmt != TVPIXFMT_UNDEFINED)
    {
        struct ImageCacheState cstate;

        cstate.src = src;
        cstate.dst.tpb_Format = v->pixfmt;
        cstate.convert = pixconv_convert;
        int res = imgcache_lookup(&cstate, creq, x, y, w, h);

        if (res != TVIMGCACHE_FOUND && src.tpb_Data != TNULL)
            res = imgcache_store(&cstate, creq);
        if (res == TVIMGCACHE_FOUND || res == TVIMGCACHE_STORED)
            src = cstate.dst;
    }
#endif

    if (!src.tpb_Data || !x11_getdrawimage(mod, v, w, h, &dst.tpb_Data,
            &dst.tpb_BytesPerLine))
        return;

    dst.tpb_Format = v->pixfmt;
    pixconv_convert(&src, &dst, 0, 0, w - 1, h - 1, 0, 0, 0, 
        mod->x11_Flags & X11FL_SWAPBYTEORDER);
//    x11_putimage(mod, v, req, x, y, w, h);

    if(v->imgCursorBG){
//        if(
//            x <= v->old_crsr_x && 
//            y <= v->old_crsr_y && 
//            (x+w) >= (v->old_crsr_x + v->old_crsr_w) &&
//            (y+h) >= (v->old_crsr_y + v->old_crsr_h)
//        ){
            XPutImage(mod->x11_Display, v->window, v->gc, v->imgCursorBG, 0, 0,
                v->old_crsr_x, v->old_crsr_y, v->old_crsr_w, v->old_crsr_h);
            XDestroyImage(v->imgCursorBG);
            v->imgCursorBG = 0;
//        }else{
//            XPutImage(mod->x11_Display, v->window, v->gc, v->imgCursorBG, 0, 0,
//                v->old_crsr_x, v->old_crsr_y, v->old_crsr_w, v->old_crsr_h);
//            XDestroyImage(v->imgCursorBG);
//            v->imgCursorBG = 0;
//        }
    }
    
    x11_putimage(mod, v, req, x, y, w, h);
    
//    if(v->imgCursorBG){
//        v->imgCursorBG = XGetImage(
//                mod->x11_Display, v->window, //v->gc,
//                v->old_crsr_x, v->old_crsr_y, v->old_crsr_w, v->old_crsr_h,
//                AllPlanes ,ZPixmap
//        );
//    }
}

/*****************************************************************************/
/*
**  getselection:
*/

static void x11_getselection(struct X11Display *mod, struct TVRequest *req)
{
    struct X11Window *v = req->tvr_Op.Rect.Window;
    TAPTR TExecBase = TGetExecBase(mod);
    Display *display = mod->x11_Display;
    Window window = v->window;
    TUINT8 *clip = NULL;

    Atom selatom = req->tvr_Op.GetSelection.Type == 2 ?
        mod->x11_XA_PRIMARY : mod->x11_XA_CLIPBOARD;

    Window selectionowner = XGetSelectionOwner(display, selatom);

    req->tvr_Op.GetSelection.Data = TNULL;
    req->tvr_Op.GetSelection.Length = 0;

    if (selectionowner != None && selectionowner != window)
    {
        unsigned char *data = NULL;
        Atom type;
        int format;
        unsigned long len, bytesleft;

        XConvertSelection(display, selatom, mod->x11_XA_UTF8_STRING,
            selatom, window, CurrentTime);
        XFlush(display);
        for (;;)
        {
            XEvent evt;

            if (XCheckTypedEvent(display, SelectionNotify, &evt))
            {
                if (evt.xselection.requestor == window)
                    break;
            }
        }

        XGetWindowProperty(display, window, selatom, 0, 0, False,
            AnyPropertyType, &type, &format, &len, &bytesleft, &data);

        if (data)
        {
            XFree(data);
            data = NULL;
        }

        if (bytesleft)
        {
            if (XGetWindowProperty(display, window, selatom, 0,
                    bytesleft, False, AnyPropertyType,
                    &type, &format, &len, &bytesleft, &data) == Success)
            {
                clip = TAlloc(TNULL, len);
                if (clip)
                {
                    TCopyMem(data, clip, len);
                    req->tvr_Op.GetSelection.Data = clip;
                    req->tvr_Op.GetSelection.Length = len;
                }
                XFree(data);
            }
        }

        XDeleteProperty(display, window, selatom);
    }
}

LOCAL void x11_docmd(struct X11Display *inst, struct TVRequest *req)
{
    switch (req->tvr_Req.io_Command)
    {
        case TVCMD_OPENWINDOW:
            x11_openvisual(inst, req);
            break;
        case TVCMD_CLOSEWINDOW:
            x11_closevisual(inst, req);
            break;
        case TVCMD_OPENFONT:
            x11_openfont(inst, req);
            break;
        case TVCMD_CLOSEFONT:
            x11_closefont(inst, req);
            break;
        case TVCMD_GETFONTATTRS:
            x11_getfontattrs(inst, req);
            break;
        case TVCMD_TEXTSIZE:
            x11_textsize(inst, req);
            break;
        case TVCMD_QUERYFONTS:
            x11_queryfonts(inst, req);
            break;
        case TVCMD_GETNEXTFONT:
            x11_getnextfont(inst, req);
            break;
        case TVCMD_SETINPUT:
            x11_setinput(inst, req);
            break;
        case TVCMD_GETATTRS:
            x11_getattrs(inst, req);
            break;
        case TVCMD_SETATTRS:
            x11_setattrs(inst, req);
            break;
        case TVCMD_ALLOCPEN:
            x11_allocpen(inst, req);
            break;
        case TVCMD_FREEPEN:
            x11_freepen(inst, req);
            break;
        case TVCMD_SETFONT:
            x11_setfont(inst, req);
            break;
        case TVCMD_CLEAR:
            x11_clear(inst, req);
            break;
        case TVCMD_RECT:
            x11_rect(inst, req);
            break;
        case TVCMD_FRECT:
            x11_frect(inst, req);
            break;
        case TVCMD_LINE:
            x11_line(inst, req);
            break;
        case TVCMD_PLOT:
            x11_plot(inst, req);
            break;
        case TVCMD_TEXT:
            x11_drawtext(inst, req);
            break;
        case TVCMD_DRAWSTRIP:
            x11_drawstrip(inst, req);
            break;
        case TVCMD_DRAWTAGS:
            x11_drawtags(inst, req);
            break;
        case TVCMD_DRAWFAN:
            x11_drawfan(inst, req);
            break;
        case TVCMD_COPYAREA:
            x11_copyarea(inst, req);
            break;
        case TVCMD_SETCLIPRECT:
            x11_setcliprect(inst, req);
            break;
        case TVCMD_UNSETCLIPRECT:
            x11_unsetcliprect(inst, req);
            break;
        case TVCMD_DRAWBUFFER:
            x11_drawbuffer(inst, req);
            break;
        case TVCMD_GETSELECTION:
            x11_getselection(inst, req);
            break;
        case TVCMD_FLUSH:
            XFlush(inst->x11_Display);
            break;
        case TVCMD_SETSELECTION:
            /* not implemented on X11 */
            break;
        default:
            TDBPRINTF(TDB_ERROR, ("Unknown command code: %d\n",
                    req->tvr_Req.io_Command));
    }
}
