
/*
**	teklib/src/display_dfb/display_dfb_mod.c - DirectFB Display driver
**	Written by Franciska Schulze <fschulze at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <unistd.h>
#include <fcntl.h>

#include "display_dfb_mod.h"

static TAPTR dfb_modopen(DFBDISPLAY *mod, TTAGITEM *tags);
static void dfb_modclose(DFBDISPLAY *mod);
static TMODAPI void dfb_beginio(DFBDISPLAY *mod, struct TVRequest *req);
static TMODAPI TINT dfb_abortio(DFBDISPLAY *mod, struct TVRequest *req);
static TMODAPI struct TVRequest *dfb_allocreq(DFBDISPLAY *mod);
static TMODAPI void dfb_freereq(DFBDISPLAY *mod, struct TVRequest *req);
static TBOOL dfb_initinstance(struct TTask *task);
static void dfb_exitinstance(DFBDISPLAY *inst);
static void dfb_taskfunc(struct TTask *task);
LOCAL void dfb_wake(DFBDISPLAY *inst);
static void dfb_processevent(DFBDISPLAY *mod);
static TVOID dfb_processvisualevent(DFBDISPLAY *mod, DFBWINDOW *v, DFBEvent *evt);
static TBOOL processkey(DFBDISPLAY *mod, DFBWINDOW *v, DFBWindowEvent *ev, TBOOL keydown);
TBOOL getimsg(DFBDISPLAY *mod, DFBWINDOW *v, TIMSG **msgptr, TUINT type);
TVOID genimsg(DFBDISPLAY *mod, DFBWINDOW *vold, DFBWINDOW *vnew, TUINT type);

static const TMFPTR
dfb_vectors[DFBDISPLAY_NUMVECTORS] =
{
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) dfb_beginio,
	(TMFPTR) dfb_abortio,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,

	(TMFPTR) dfb_allocreq,
	(TMFPTR) dfb_freereq,
};

static void
dfb_destroy(DFBDISPLAY *mod)
{
	TDBPRINTF(TDB_TRACE,("Module destroy...\n"));
	TDestroy((struct THandle *) mod->dfb_Lock);
}

static THOOKENTRY TTAG
dfb_dispatch(struct THook *hook, TAPTR obj, TTAG msg)
{
	DFBDISPLAY *mod = (DFBDISPLAY *) hook->thk_Data;
	switch (msg)
	{
		case TMSG_DESTROY:
			dfb_destroy(mod);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) dfb_modopen(mod, obj);
		case TMSG_CLOSEMODULE:
			dfb_modclose(obj);
			break;
		case TMSG_INITTASK:
			return dfb_initinstance(obj);
		case TMSG_RUNTASK:
			dfb_taskfunc(obj);
			break;
	}
	return 0;
}

TMODENTRY TUINT
tek_init_display_directfb(struct TTask *task, struct TModule *vis, TUINT16 version,
	TTAGITEM *tags)
{
	DFBDISPLAY *mod = (DFBDISPLAY *) vis;

	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * DFBDISPLAY_NUMVECTORS;

		if (version <= DFBDISPLAY_VERSION)
			return sizeof(DFBDISPLAY);

		return 0;
	}

	TDBPRINTF(TDB_TRACE,("Module init...\n"));

	if (DirectFBInit(TNULL, TNULL) == DFB_OK)
	{
		for (;;)
		{
			mod->dfb_ExecBase = TGetExecBase(mod);
			mod->dfb_Lock = TExecCreateLock(mod->dfb_ExecBase, TNULL);
			if (mod->dfb_Lock == TNULL) break;

			mod->dfb_Module.tmd_Version = DFBDISPLAY_VERSION;
			mod->dfb_Module.tmd_Revision = DFBDISPLAY_REVISION;
			mod->dfb_Module.tmd_Handle.thn_Hook.thk_Entry = dfb_dispatch;
			mod->dfb_Module.tmd_Flags = TMODF_VECTORTABLE | TMODF_OPENCLOSE;
			TInitVectors((struct TModule *) mod, dfb_vectors,
				DFBDISPLAY_NUMVECTORS);
			return TTRUE;
		}
		dfb_destroy(mod);
	}

	return TFALSE;
}

/*****************************************************************************/
/*
**	Module open/close
*/

static TAPTR dfb_modopen(DFBDISPLAY *mod, TTAGITEM *tags)
{
	TBOOL success = TTRUE;
	TExecLock(mod->dfb_ExecBase, mod->dfb_Lock);
	if (mod->dfb_RefCount == 0)
		success = dfb_init(mod, tags);
	if (success)
		mod->dfb_RefCount++;
	TExecUnlock(mod->dfb_ExecBase, mod->dfb_Lock);
	if (success)
	{
		/* Attributes that can be queried during open: */
		TTAG p = TGetTag(tags, TVisual_HaveWindowManager, TNULL);
		if (p) *((TBOOL *) p) = TFALSE;
		return mod;
	}
	return TNULL;
}

static void
dfb_modclose(DFBDISPLAY *mod)
{
	TDBPRINTF(TDB_TRACE,("Device close\n"));
	TExecLock(mod->dfb_ExecBase, mod->dfb_Lock);
	if (--mod->dfb_RefCount == 0)
		dfb_exit(mod);
	TExecUnlock(mod->dfb_ExecBase, mod->dfb_Lock);
}

/*****************************************************************************/
/*
**	BeginIO/AbortIO
*/

static TMODAPI void
dfb_beginio(DFBDISPLAY *mod, struct TVRequest *req)
{
	TExecPutMsg(mod->dfb_ExecBase, mod->dfb_CmdPort,
		req->tvr_Req.io_ReplyPort, req);
	dfb_wake(mod);
}

static TMODAPI TINT
dfb_abortio(DFBDISPLAY *mod, struct TVRequest *req)
{
	return -1;
}

/*****************************************************************************/
/*
**	AllocReq/FreeReq
*/

static TMODAPI struct TVRequest *
dfb_allocreq(DFBDISPLAY *mod)
{
	struct TVRequest *req = TExecAllocMsg(mod->dfb_ExecBase,
		sizeof(struct TVRequest));
	if (req)
		req->tvr_Req.io_Device = (struct TModule *) mod;
	return req;
}

static TMODAPI void
dfb_freereq(DFBDISPLAY *mod, struct TVRequest *req)
{
	TExecFree(mod->dfb_ExecBase, req);
}

/*****************************************************************************/
/*
**	Module init/exit
*/

LOCAL TBOOL
dfb_init(DFBDISPLAY *mod, TTAGITEM *tags)
{
	for (;;)
	{
		TTAGITEM tags[2];
		tags[0].tti_Tag = TTask_UserData;
		tags[0].tti_Value = (TTAG) mod;
		tags[1].tti_Tag = TTAG_DONE;
		mod->dfb_Task = TExecCreateTask(mod->dfb_ExecBase,
			&mod->dfb_Module.tmd_Handle.thn_Hook, tags);
		if (mod->dfb_Task == TNULL) break;

		mod->dfb_CmdPort = TExecGetUserPort(mod->dfb_ExecBase, mod->dfb_Task);
		mod->dfb_CmdPortSignal = TExecGetPortSignal(mod->dfb_ExecBase,
			mod->dfb_CmdPort);

		return TTRUE;
	}

	dfb_exit(mod);
	return TFALSE;
}

LOCAL void
dfb_exit(DFBDISPLAY *mod)
{
	if (mod->dfb_Task)
	{
		TExecSignal(mod->dfb_ExecBase, mod->dfb_Task, TTASK_SIG_ABORT);
		dfb_wake(mod);
		TDestroy((struct THandle *) mod->dfb_Task);
	}
}

/*****************************************************************************/

static IDirectFBSurface *
loadimage (IDirectFB *dfb, const char *filename)
{
	IDirectFBImageProvider *provider;
	IDirectFBSurface *surface = NULL;
	DFBSurfaceDescription dsc;

	if (dfb->CreateImageProvider(dfb, filename, &provider) != DFB_OK)
		return TNULL;

	provider->GetSurfaceDescription(provider, &dsc);
	dsc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
	dsc.pixelformat = DSPF_ARGB;
	if (dfb->CreateSurface(dfb, &dsc, &surface) == DFB_OK)
		provider->RenderTo(provider, surface, NULL);

	provider->Release(provider);

	return surface;
}

/*****************************************************************************/

static TBOOL dfb_initinstance(struct TTask *task)
{
	DFBDISPLAY *inst = TExecGetTaskData(TGetExecBase(task), task);

	for (;;)
	{
		TTAGITEM ftags[3];
		int pipefd[2];
		IDirectFBSurface *layer_surface;
		DFBWindowDescription wdsc;

		/* list of free input messages: */
		TInitList(&inst->dfb_imsgpool);

		/* list of all open visuals: */
		TInitList(&inst->dfb_vlist);

		/* init fontmanager and default font */
		TInitList(&inst->dfb_fm.openfonts);

		inst->dfb_FDSigPipeRead = -1;
		inst->dfb_FDSigPipeWrite = -1;

		/* init directfb in administrative mode */
		if (DirectFBCreate(&inst->dfb_DFB) != DFB_OK)
			break;

		inst->dfb_DFB->GetDeviceDescription(inst->dfb_DFB, &inst->dfb_DevDsc);

		if (inst->dfb_DFB->GetDisplayLayer(inst->dfb_DFB, DLID_PRIMARY,
			&inst->dfb_Layer) != DFB_OK)
			break;

		inst->dfb_Layer->SetCooperativeLevel(inst->dfb_Layer, DLSCL_ADMINISTRATIVE);

		if (!((inst->dfb_DevDsc.blitting_flags & DSBLIT_BLEND_ALPHACHANNEL) &&
			(inst->dfb_DevDsc.blitting_flags & DSBLIT_BLEND_COLORALPHA)))
		{
			inst->dfb_LayerConfig.flags = DLCONF_BUFFERMODE;
			inst->dfb_LayerConfig.buffermode = DLBM_FRONTONLY;
			inst->dfb_Layer->SetConfiguration(inst->dfb_Layer, &inst->dfb_LayerConfig);
		}

		if (inst->dfb_Layer->GetSurface(inst->dfb_Layer, &layer_surface) != DFB_OK)
			break;

		layer_surface->GetSize(layer_surface, &inst->dfb_ScrWidth, &inst->dfb_ScrHeight);
		layer_surface->Release(layer_surface);
		TDBPRINTF(TDB_INFO,("screen dimension: %d x %d\n", inst->dfb_ScrWidth,
			inst->dfb_ScrHeight));

		inst->dfb_Layer->GetConfiguration(inst->dfb_Layer, &inst->dfb_LayerConfig);
		inst->dfb_Layer->EnableCursor(inst->dfb_Layer, 1);
		inst->dfb_Layer->SetBackgroundColor(inst->dfb_Layer, 0, 0, 0, 0);

		/* init custom cursor */
		inst->dfb_CursorSurface = loadimage(inst->dfb_DFB, DEF_CURSORFILE);
		inst->dfb_Layer->SetCursorShape(inst->dfb_Layer, inst->dfb_CursorSurface, 0, 0);

		if (inst->dfb_DFB->CreateInputEventBuffer(inst->dfb_DFB, DICAPS_ALL,
			DFB_FALSE, &inst->dfb_Events) != DFB_OK)
			break;
		if (inst->dfb_Events->CreateFileDescriptor(inst->dfb_Events,
			&inst->dfb_FDInput) != DFB_OK)
			break;

		fcntl(inst->dfb_FDInput, F_SETFL, O_NONBLOCK);

		if (pipe(pipefd) != 0)
			break;
		inst->dfb_FDSigPipeRead = pipefd[0];
		inst->dfb_FDSigPipeWrite = pipefd[1];
		inst->dfb_FDMax = TMAX(inst->dfb_FDSigPipeRead, inst->dfb_FDInput) + 1;
		fcntl(inst->dfb_FDSigPipeWrite, F_SETFL, O_NONBLOCK);
		fcntl(inst->dfb_FDSigPipeRead, F_SETFL, O_NONBLOCK);

		/* create root window */
		wdsc.flags = (DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT);
		wdsc.posx   = 0;
		wdsc.posy   = 0;
		wdsc.width  = inst->dfb_ScrWidth;
		wdsc.height = inst->dfb_ScrHeight;

		if (inst->dfb_Layer->CreateWindow(inst->dfb_Layer, &wdsc,
			&inst->dfb_RootWindow) != DFB_OK)
			break;

		if (inst->dfb_RootWindow->SetOptions(inst->dfb_RootWindow, DWOP_KEEP_SIZE |
			DWOP_KEEP_POSITION | DWOP_KEEP_STACKING | DWOP_INDESTRUCTIBLE) != DFB_OK)
			break;

		if (inst->dfb_RootWindow->SetOpacity(inst->dfb_RootWindow, 0xFF) != DFB_OK)
			break;

		if (inst->dfb_RootWindow->AttachEventBuffer(inst->dfb_RootWindow,
			inst->dfb_Events) != DFB_OK)
			break;

		ftags[0].tti_Tag = TVisual_FontName;
		ftags[0].tti_Value = (TTAG) FNT_DEFNAME;
		ftags[1].tti_Tag = TVisual_FontPxSize;
		ftags[1].tti_Value = (TTAG) FNT_DEFPXSIZE;
		ftags[2].tti_Tag = TTAG_DONE;
		inst->dfb_fm.deffont = dfb_hostopenfont(inst, ftags);
		/* if (inst->dfb_fm.deffont == TNULL) break; */

		TDBPRINTF(TDB_INFO,("instance init successful\n"));
		return TTRUE;
	}

	dfb_exitinstance(inst);

	return TFALSE;
}

static void
dfb_exitinstance(DFBDISPLAY *inst)
{
	struct TNode *imsg, *node, *next;

	/* free pooled input messages: */
	while ((imsg = TRemHead(&inst->dfb_imsgpool)))
		TExecFree(inst->dfb_ExecBase, imsg);

	/* free queued input messages in all open visuals: */
	node = inst->dfb_vlist.tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
	{
		DFBWINDOW *v = (DFBWINDOW *) node;

		/* unset active font in all open visuals */
		v->curfont = TNULL;

		while ((imsg = TRemHead(&v->imsgqueue)))
			TExecFree(inst->dfb_ExecBase, imsg);
	}

	/* force closing of default font */
	inst->dfb_fm.defref = 0;
	/* close all fonts */
	node = inst->dfb_fm.openfonts.tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
		dfb_hostclosefont(inst, (TAPTR) node);

	if (inst->dfb_FDSigPipeRead != -1)
	{
		close(inst->dfb_FDSigPipeRead);
		close(inst->dfb_FDSigPipeWrite);
	}

	if (inst->dfb_RootWindow)
		inst->dfb_RootWindow->Release(inst->dfb_RootWindow);

	if (inst->dfb_Events)
		inst->dfb_Events->Release(inst->dfb_Events);

	if (inst->dfb_CursorSurface)
		inst->dfb_CursorSurface->Release(inst->dfb_CursorSurface);

	if (inst->dfb_Layer)
		inst->dfb_Layer->Release(inst->dfb_Layer);

	if (inst->dfb_DFB)
		inst->dfb_DFB->Release(inst->dfb_DFB);
}

/*****************************************************************************/

static void
dfb_docmd(DFBDISPLAY *inst, struct TVRequest *req)
{
	switch (req->tvr_Req.io_Command)
	{
		case TVCMD_OPENWINDOW: dfb_openvisual(inst, req); break;
		case TVCMD_CLOSEWINDOW: dfb_closevisual(inst, req); break;
		case TVCMD_OPENFONT: dfb_openfont(inst, req); break;
		case TVCMD_CLOSEFONT: dfb_closefont(inst, req); break;
		case TVCMD_GETFONTATTRS: dfb_getfontattrs(inst, req); break;
		case TVCMD_TEXTSIZE: dfb_textsize(inst, req); break;
		case TVCMD_QUERYFONTS: dfb_queryfonts(inst, req); break;
		case TVCMD_GETNEXTFONT: dfb_getnextfont(inst, req); break;
		case TVCMD_SETINPUT: dfb_setinput(inst, req); break;
		case TVCMD_GETATTRS: dfb_getattrs(inst, req); break;
		case TVCMD_SETATTRS: dfb_setattrs(inst, req); break;
		case TVCMD_ALLOCPEN: dfb_allocpen(inst, req); break;
		case TVCMD_FREEPEN: dfb_freepen(inst, req); break;
		case TVCMD_SETFONT: dfb_setfont(inst, req); break;
		case TVCMD_CLEAR: dfb_clear(inst, req); break;
		case TVCMD_RECT: dfb_rect(inst, req); break;
		case TVCMD_FRECT: dfb_frect(inst, req); break;
		case TVCMD_LINE: dfb_line(inst, req); break;
		case TVCMD_PLOT: dfb_plot(inst, req); break;
		case TVCMD_TEXT: dfb_drawtext(inst, req); break;
		case TVCMD_DRAWSTRIP: dfb_drawstrip(inst, req); break;
		case TVCMD_DRAWTAGS: dfb_drawtags(inst, req); break;
		case TVCMD_DRAWFAN: dfb_drawfan(inst, req); break;
		case TVCMD_DRAWARC: dfb_drawarc(inst, req); break;
		case TVCMD_DRAWFARC: dfb_drawfarc(inst, req); break;
		case TVCMD_COPYAREA: dfb_copyarea(inst, req); break;
		case TVCMD_SETCLIPRECT: dfb_setcliprect(inst, req); break;
		case TVCMD_UNSETCLIPRECT: dfb_unsetcliprect(inst, req); break;
		case TVCMD_DRAWBUFFER: dfb_drawbuffer(inst, req); break;
		default:
			TDBPRINTF(TDB_ERROR,("Unknown command code: %d\n",
			req->tvr_Req.io_Command));
	}
}

static void dfb_taskfunc(struct TTask *task)
{
	static char pipebuf[256];
	DFBDISPLAY *inst = TExecGetTaskData(TGetExecBase(task), task);
	TUINT sig;
	fd_set rset;
	struct TVRequest *req;
	struct timeval tv;
	DFBWINDOW *v;
	struct TNode *node, *next;

	/* interval time: 1/50s: */
	TTIME intt = { 20000 };
	/* next absolute time to send interval message: */
	TTIME nextt;
	TTIME waitt, nowt;

	TExecGetSystemTime(inst->dfb_ExecBase, &nextt);
	TAddTime(&nextt, &intt);

	TDBPRINTF(TDB_ERROR,("Device instance running\n"));

	do
	{
		TBOOL do_interval = TFALSE;

		while ((req = TExecGetMsg(inst->dfb_ExecBase, inst->dfb_CmdPort)))
		{
			dfb_docmd(inst, req);
			TExecReplyMsg(inst->dfb_ExecBase, req);
		}

		node = inst->dfb_vlist.tlh_Head.tln_Succ;
		for (; (next = node->tln_Succ); node = next)
		{
			v = (DFBWINDOW *) node;
			v->winsurface->Flip(v->winsurface, NULL, 0);
		}

		FD_ZERO(&rset);
		FD_SET(inst->dfb_FDInput, &rset);
		FD_SET(inst->dfb_FDSigPipeRead, &rset);

		/* calculate new delta to wait: */
		TExecGetSystemTime(inst->dfb_ExecBase, &nowt);
		waitt = nextt;
		TSubTime(&waitt, &nowt);

		tv.tv_sec = waitt.tdt_Int64 / 1000000;
		tv.tv_usec = waitt.tdt_Int64 % 1000000;

		/* wait for display, signal fd and timeout: */
		if (select(inst->dfb_FDMax, &rset, NULL, NULL, &tv) > 0)
		{
			if (FD_ISSET(inst->dfb_FDSigPipeRead, &rset))
			{
				/* consume signals: */
				while (read(inst->dfb_FDSigPipeRead, pipebuf,
					sizeof(pipebuf)) == sizeof(pipebuf));
			}

			if (FD_ISSET(inst->dfb_FDInput, &rset))
			{
				/* process input messages: */
				dfb_processevent(inst);
			}
		}

		/* check if time interval has expired: */
		TExecGetSystemTime(inst->dfb_ExecBase, &nowt);
		if (TCmpTime(&nowt, &nextt) > 0)
		{
			/* expired; send interval: */
			do_interval = TTRUE;
			TAddTime(&nextt, &intt);
			if (TCmpTime(&nowt, &nextt) >= 0)
			{
				/* nexttime expired already; create new time from now: */
				nextt = nowt;
				TAddTime(&nextt, &intt);
			}
		}

		/* send out input messages to owners: */
		dfb_sendimessages(inst, do_interval);

		/* get signal state: */
		sig = TExecSetSignal(inst->dfb_ExecBase, 0, TTASK_SIG_ABORT);

	} while (!(sig & TTASK_SIG_ABORT));

	TDBPRINTF(TDB_ERROR,("Device instance closedown\n"));

	/* closedown: */
	dfb_exitinstance(inst);
}

LOCAL void dfb_wake(DFBDISPLAY *inst)
{
	char sig = 0;
	write(inst->dfb_FDSigPipeWrite, &sig, 1);
}

/*****************************************************************************/
/*
**	ProcessEvents
*/

static void
dfb_processevent(DFBDISPLAY *mod)
{
	while (TTRUE)
	{
		DFBEvent evt;
		DFBWINDOW *v = mod->dfb_Focused;
		int res = read(mod->dfb_FDInput, &evt, sizeof(DFBEvent));

		if (res == -1)
			break;

		if (mod->dfb_Focused)
		{
			DFBWindowEvent *ev = (DFBWindowEvent *)&evt;
			struct TNode *next, *node = mod->dfb_vlist.tlh_Head.tln_Succ;

			for (; (next = node->tln_Succ); node = next)
			{
				DFBWINDOW *vc = (DFBWINDOW *)node;

				if (ev->cx >= vc->winleft && ev->cx < vc->winleft+vc->winwidth
					&& ev->cy >= vc->wintop && ev->cy < vc->wintop+vc->winheight)
				{
					if ((DFBWINDOW *) mod->dfb_Active != vc)
					{
						/* generate mouseover event */
						genimsg(mod, (DFBWINDOW *)mod->dfb_Active, vc, TITYPE_MOUSEOVER);
						mod->dfb_Active = (TAPTR) vc;
					}

					/* popup active */
					if (vc->borderless == TTRUE)
						v = vc;

					break;
				}
			}

			dfb_processvisualevent(mod, v, &evt);
		}
	}
}

static TVOID
dfb_processvisualevent(DFBDISPLAY *mod, DFBWINDOW *v, DFBEvent *evt)
{
	TIMSG *imsg;
	DFBWindowEvent *ev = (DFBWindowEvent *)evt;

	switch (ev->type)
	{
		case DWET_BUTTONDOWN:
		case DWET_BUTTONUP:
		{
			mod->dfb_MouseX = ev->cx - v->winleft;
			mod->dfb_MouseY = ev->cy - v->wintop;

			if (v->eventmask & TITYPE_MOUSEBUTTON)
			{
				if (getimsg(mod, v, &imsg, TITYPE_MOUSEBUTTON))
				{
					if (ev->type == DWET_BUTTONDOWN)
					{
						switch (ev->button)
						{
							case DIBI_LEFT:
								imsg->timsg_Code = TMBCODE_LEFTDOWN;
								break;
							case DIBI_MIDDLE:
								imsg->timsg_Code = TMBCODE_MIDDLEDOWN;
								break;
							case DIBI_RIGHT:
								imsg->timsg_Code = TMBCODE_RIGHTDOWN;
								break;
							case DIBI_LAST:
								break;
						}
					}
					else /* DWET_BUTTONUP */
					{
						switch (ev->button)
						{
							case DIBI_LEFT:
								imsg->timsg_Code = TMBCODE_LEFTUP;
								break;
							case DIBI_MIDDLE:
								imsg->timsg_Code = TMBCODE_MIDDLEUP;
								break;
							case DIBI_RIGHT:
								imsg->timsg_Code = TMBCODE_RIGHTUP;
								break;
							case DIBI_LAST:
								break;
						}
					}
					imsg->timsg_MouseX = mod->dfb_MouseX;
					imsg->timsg_MouseY = mod->dfb_MouseY;

					TAddTail(&v->imsgqueue, &imsg->timsg_Node);
				}
			}
			break;
		}
		case DWET_MOTION:
		{
			mod->dfb_MouseX = ev->cx - v->winleft;
			mod->dfb_MouseY = ev->cy - v->wintop;

			if (v->eventmask & TITYPE_MOUSEMOVE)
			{
				if (getimsg(mod, v, &imsg, TITYPE_MOUSEMOVE))
				{
					imsg->timsg_MouseX = mod->dfb_MouseX;
					imsg->timsg_MouseY = mod->dfb_MouseY;
					TAddTail(&v->imsgqueue, &imsg->timsg_Node);
				}
			}

			break;
		}
		case DWET_KEYDOWN:
		case DWET_KEYUP:
		{
			if (v->borderless == TTRUE)
				processkey(mod, mod->dfb_Focused, ev, (ev->type == DWET_KEYDOWN));
			else
				processkey(mod, v, ev, (ev->type == DWET_KEYDOWN));

			break;
		}
		default:
			break;
	}
}

static TBOOL processkey(DFBDISPLAY *mod, DFBWINDOW *v, DFBWindowEvent *ev, TBOOL keydown)
{
	TBOOL newkey = TFALSE;
	TUINT evtype = 0;
	TUINT newqual;
	TUINT evmask = v->eventmask;
	TIMSG *imsg;

	mod->dfb_MouseX = ev->cx;
	mod->dfb_MouseY = ev->cy;

	switch (ev->key_id)
	{
		case DIKI_SHIFT_L:
			newqual = TKEYQ_LSHIFT;
			break;
		case DIKI_SHIFT_R:
			newqual = TKEYQ_RSHIFT;
			break;
		case DIKI_CONTROL_L:
			newqual = TKEYQ_LCTRL;
			break;
		case DIKI_CONTROL_R:
			newqual = TKEYQ_RCTRL;
			break;
		case DIKI_ALT_L:
			newqual = TKEYQ_LALT;
			break;
		case DIKI_ALT_R:
			newqual = TKEYQ_RALT;
			break;
		default:
			newqual = 0;
	}

	if (newqual != 0)
	{
		if (keydown)
			mod->dfb_KeyQual |= newqual;
		else
			mod->dfb_KeyQual &= ~newqual;
	}

	if (keydown && (evmask & TITYPE_KEYDOWN))
		evtype = TITYPE_KEYDOWN;
	else if (!keydown && (evmask & TITYPE_KEYUP))
		evtype = TITYPE_KEYUP;

	if (evtype && getimsg(mod, v, &imsg, evtype))
	{
		imsg->timsg_Qualifier = mod->dfb_KeyQual;

		if (ev->key_symbol >= DIKS_F1 && ev->key_symbol <= DIKS_F12)
		{
			imsg->timsg_Code = (ev->key_symbol - DIKS_F1) + TKEYC_F1;
			newkey = TTRUE;
		}
		else if (ev->key_symbol < 256)
		{
			imsg->timsg_Code = ev->key_symbol; /* cooked ASCII code */
			newkey = TTRUE;
		}
		else if (ev->key_id >= DIKI_KP_0 && ev->key_id <= DIKI_KP_9)
		{
			imsg->timsg_Code = (ev->key_id - DIKI_KP_0) + 48;
			imsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
			newkey = TTRUE;
		}
		else
		{
			newkey = TTRUE;
			switch (ev->key_symbol)
			{
				case DIKS_CURSOR_LEFT:
					imsg->timsg_Code = TKEYC_CRSRLEFT;
					break;
				case DIKS_CURSOR_RIGHT:
					imsg->timsg_Code = TKEYC_CRSRRIGHT;
					break;
				case DIKS_CURSOR_UP:
					imsg->timsg_Code = TKEYC_CRSRUP;
					break;
				case DIKS_CURSOR_DOWN:
					imsg->timsg_Code = TKEYC_CRSRDOWN;
					break;
				case DIKS_ESCAPE:
					imsg->timsg_Code = TKEYC_ESC;
					break;
				case DIKS_DELETE:
					imsg->timsg_Code = TKEYC_DEL;
					break;
				case DIKS_BACKSPACE:
					imsg->timsg_Code = TKEYC_BCKSPC;
					break;
				case DIKS_TAB:
					imsg->timsg_Code = TKEYC_TAB;
					break;
				case DIKS_RETURN:
					imsg->timsg_Code = TKEYC_RETURN;
					break;
				case DIKS_HELP:
					imsg->timsg_Code = TKEYC_HELP;
					break;
				case DIKS_INSERT:
					imsg->timsg_Code = TKEYC_INSERT;
					break;
				case DIKS_PAGE_UP:
					imsg->timsg_Code = TKEYC_PAGEUP;
					break;
				case DIKS_PAGE_DOWN:
					imsg->timsg_Code = TKEYC_PAGEDOWN;
					break;
				case DIKS_BEGIN:
					imsg->timsg_Code = TKEYC_POSONE;
					break;
				case DIKS_END:
					imsg->timsg_Code = TKEYC_POSEND;
					break;
				case DIKS_PRINT:
					imsg->timsg_Code = TKEYC_PRINT;
					break;
				case DIKS_SCROLL_LOCK:
					imsg->timsg_Code = TKEYC_SCROLL;
					break;
				case DIKS_PAUSE:
					imsg->timsg_Code = TKEYC_PAUSE;
					break;
				default:
				{
					switch (ev->key_id)
					{
						case DIKI_KP_ENTER:
							imsg->timsg_Code = TKEYC_RETURN;
							imsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
							break;
						case DIKI_KP_DECIMAL:
							imsg->timsg_Code = '.';
							imsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
							break;
						case DIKI_KP_PLUS:
							imsg->timsg_Code = '+';
							imsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
							break;
						case DIKI_KP_MINUS:
							imsg->timsg_Code = '-';
							imsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
							break;
						case DIKI_KP_MULT:
							imsg->timsg_Code = '*';
							imsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
							break;
						case DIKI_KP_DIV:
							imsg->timsg_Code = '/';
							imsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
							break;
						default:
							newkey = TFALSE;
					}

					break;
				}
			}
		}

		if (!newkey && newqual)
		{
			imsg->timsg_Code = TKEYC_NONE;
			newkey = TTRUE;
		}

		if (newkey)
		{
			TINTPTR len =
				(TINTPTR) utf8encode(imsg->timsg_KeyCode, imsg->timsg_Code) -
				(TINTPTR) imsg->timsg_KeyCode;
			imsg->timsg_KeyCode[len] = 0;

			imsg->timsg_MouseX = mod->dfb_MouseX;
			imsg->timsg_MouseY = mod->dfb_MouseY;
			TAddTail(&v->imsgqueue, &imsg->timsg_Node);
		}
		else
		{
			/* put back message: */
			TAddTail(&mod->dfb_imsgpool, &imsg->timsg_Node);
		}

	}

	return newkey;
}

/*****************************************************************************/

TVOID genimsg(DFBDISPLAY *mod, DFBWINDOW *vold, DFBWINDOW *vnew, TUINT type)
{
	TIMSG *imsg;

	if (vold && vold->eventmask & type)
	{
		if (getimsg(mod, vold, &imsg, type))
		{
			imsg->timsg_Code = TFALSE;
			TAddTail(&vold->imsgqueue, &imsg->timsg_Node);
		}
	}

	if (vnew->eventmask & type)
	{
		if (getimsg(mod, vnew, &imsg, type))
		{
			imsg->timsg_Code = TTRUE;
			TAddTail(&vnew->imsgqueue, &imsg->timsg_Node);
		}
	}
}

/*****************************************************************************/

TBOOL getimsg(DFBDISPLAY *mod, DFBWINDOW *v, TIMSG **msgptr, TUINT type)
{
	TIMSG *msg = (TIMSG *) TRemHead(&mod->dfb_imsgpool);
	if (msg == TNULL)
		msg = TExecAllocMsg0(mod->dfb_ExecBase, sizeof(TIMSG));
	if (msg)
	{
		msg->timsg_Instance = v;
		msg->timsg_UserData = v->userdata;
		msg->timsg_Type = type;
		msg->timsg_Qualifier = mod->dfb_KeyQual;
		msg->timsg_MouseX = mod->dfb_MouseX;
		msg->timsg_MouseY = mod->dfb_MouseY;
		TExecGetSystemTime(mod->dfb_ExecBase, &msg->timsg_TimeStamp);
		*msgptr = msg;
		return TTRUE;
	}
	*msgptr = TNULL;
	return TFALSE;
}

LOCAL void dfb_sendimessages(DFBDISPLAY *mod, TBOOL do_interval)
{
	struct TNode *next, *node = mod->dfb_vlist.tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
	{
		DFBWINDOW *v = (DFBWINDOW *) node;
		TIMSG *imsg;

		if (do_interval && (v->eventmask & TITYPE_INTERVAL) &&
			getimsg(mod, v, &imsg, TITYPE_INTERVAL))
			TExecPutMsg(mod->dfb_ExecBase, v->imsgport, TNULL, imsg);

		while ((imsg = (TIMSG *) TRemHead(&v->imsgqueue)))
			TExecPutMsg(mod->dfb_ExecBase, v->imsgport, TNULL, imsg);
	}
}
