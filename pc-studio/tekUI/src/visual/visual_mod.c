
/*
**	teklib/src/visual/visual_mod.c - Visual module
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "visual_mod.h"

static struct TVisualBase *vis_modopen(struct TVisualBase *mod,
	TTAGITEM *tags);
static void vis_modclose(struct TVisualBase *mod);
static TBOOL vis_init(struct TVisualBase *mod);
static void vis_exit(struct TVisualBase *mod);

static const TMFPTR
vis_vectors[VISUAL_NUMVECTORS] =
{
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,

	(TMFPTR) vis_openvisual,
	(TMFPTR) vis_closevisual,
	(TMFPTR) vis_attach,
	(TMFPTR) vis_openfont,
	(TMFPTR) vis_closefont,
	(TMFPTR) vis_getfattrs,
	(TMFPTR) vis_textsize,
	(TMFPTR) vis_queryfonts,
	(TMFPTR) vis_getnextfont,

	(TMFPTR) vis_getport,
	(TMFPTR) vis_setinput,
	(TMFPTR) vis_getattrs,
	(TMFPTR) vis_setattrs,
	(TMFPTR) vis_allocpen,
	(TMFPTR) vis_freepen,
	(TMFPTR) vis_setfont,
	(TMFPTR) vis_clear,
	(TMFPTR) vis_rect,
	(TMFPTR) vis_frect,
	(TMFPTR) vis_line,
	(TMFPTR) vis_plot,
	(TMFPTR) vis_text,

	(TMFPTR) vis_drawstrip,
	(TMFPTR) vis_drawtags,
	(TMFPTR) vis_drawfan,
	(TMFPTR) vis_copyarea,
	(TMFPTR) vis_drawbuffer,

	(TMFPTR) vis_setcliprect,
	(TMFPTR) vis_unsetcliprect,

	(TMFPTR) vis_opendisplay,
	(TMFPTR) vis_closedisplay,
	(TMFPTR) vis_querydisplays,
	(TMFPTR) vis_getnextdisplay,
	
	(TMFPTR) vis_getselection,
	(TMFPTR) vis_setselection,
};

static void
vis_destroy(struct TVisualBase *mod)
{
	TDestroy(mod->vis_Lock);
}

static THOOKENTRY TTAG
vis_dispatch(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct TVisualBase *mod = (struct TVisualBase *) hook->thk_Data;
	switch (msg)
	{
		case TMSG_DESTROY:
			vis_destroy(mod);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) vis_modopen(mod, obj);
		case TMSG_CLOSEMODULE:
			vis_modclose(obj);
	}
	return 0;
}

TMODENTRY TUINT
tek_init_visual(struct TTask *task, struct TModule *vis, TUINT16 version,
	TTAGITEM *tags)
{
	struct TVisualBase *mod = (struct TVisualBase *) vis;
	if (mod == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * VISUAL_NUMVECTORS;

		if (version <= VISUAL_VERSION)
			return sizeof(struct TVisualBase);

		return 0;
	}

	for (;;)
	{
		struct TExecBase *TExecBase = TGetExecBase(mod);
		mod->vis_Lock = TCreateLock(TNULL);
		if (mod->vis_Lock == TNULL) break;

		mod->vis_Module.tmd_Version = VISUAL_VERSION;
		mod->vis_Module.tmd_Revision = VISUAL_REVISION;
		mod->vis_Module.tmd_Handle.thn_Hook.thk_Entry = vis_dispatch;
		mod->vis_Module.tmd_Flags = TMODF_VECTORTABLE | TMODF_OPENCLOSE;
		TInitVectors(&mod->vis_Module, vis_vectors, VISUAL_NUMVECTORS);
		return TTRUE;
	}

	vis_destroy(mod);
	return TFALSE;
}

/*****************************************************************************/
/*
**	Module open/close
*/

static void vis_destroyports(struct TVisualBase *mod)
{
	if (mod->vis_Flags & TVISFL_CMDRPORT_OWNER)
		TDestroy(mod->vis_CmdRPort);
	if (mod->vis_Flags & TVISFL_IMSGPORT_OWNER)
		TDestroy(mod->vis_IMsgPort);
}

static struct TVisualBase *
vis_modopen(struct TVisualBase *mod, TTAGITEM *tags)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	struct TVisualBase *inst = TNULL;
	TBOOL success = TTRUE;

	TLock(mod->vis_Lock);

	if (mod->vis_RefCount == 0)
		success = vis_init(mod);

	if (success)
		mod->vis_RefCount++;

	TUnlock(mod->vis_Lock);

	if (success)
	{
		struct TVisualBase *base =
			(struct TVisualBase *) TGetTag(tags, TVisual_Attach, TNULL);
		if (TGetTag(tags, TVisual_NewInstance, TFALSE)) base = mod;

		if (base)
		{
			inst = (struct TVisualBase *) TNewInstance((struct TModule *) base,
				base->vis_Module.tmd_PosSize, base->vis_Module.tmd_NegSize);
			if (inst)
			{
				TInitList(&inst->vis_ReqPool);
				TInitList(&inst->vis_WaitList);

				inst->vis_IMsgPort = (struct TMsgPort *)
					TGetTag(tags, TVisual_IMsgPort, TNULL);
				if (inst->vis_IMsgPort == TNULL)
				{
					inst->vis_IMsgPort = TCreatePort(TNULL);
					inst->vis_Flags |= TVISFL_IMSGPORT_OWNER;
				}

				inst->vis_CmdRPort = (struct TMsgPort *)
					TGetTag(tags, TVisual_CmdRPort, TNULL);
				if (inst->vis_CmdRPort == TNULL)
				{
					inst->vis_CmdRPort = TCreatePort(TNULL);
					inst->vis_Flags |= TVISFL_CMDRPORT_OWNER;
				}

				if (inst->vis_IMsgPort == TNULL || inst->vis_CmdRPort == TNULL)
				{
					vis_destroyports(inst);
					TFreeInstance(&inst->vis_Module);
					inst = TNULL;
				}
			}
			if (inst == TNULL)
				vis_modclose(mod);
		}
		else
			inst = mod;
	}

	return inst;
}

static void
vis_modclose(struct TVisualBase *inst)
{
	struct TVisualBase *mod =
		(struct TVisualBase *) inst->vis_Module.tmd_ModSuper;
	struct TExecBase *TExecBase = TGetExecBase(mod);
	if (inst != mod)
	{
		struct TNode *node;
		TAPTR imsg;

		if (inst->vis_Flags & TVISFL_IMSGPORT_OWNER)
		{
			TINT n = 0;
			while ((imsg = TGetMsg(inst->vis_IMsgPort)))
			{
				TAckMsg(imsg);
				n++;
			}
			if (n > 0)
				TDBPRINTF(TDB_WARN,("freed %d pending input message\n", n));
		}

		while ((node = TRemHead(&inst->vis_ReqPool)))
			TFree(node);

		while ((node = TRemHead(&inst->vis_WaitList)))
		{
			TWaitIO((struct TIORequest *) node);
			TFree(node);
		}

		vis_destroyports(inst);
		TFreeInstance(&inst->vis_Module);
	}
	TLock(mod->vis_Lock);
	if (--mod->vis_RefCount == 0)
		vis_exit(mod);
	TUnlock(mod->vis_Lock);
}

/*****************************************************************************/
/*
**	Module init/exit
*/

static TBOOL
vis_init(struct TVisualBase *mod)
{
	TInitList(&mod->vis_Displays);
	return TTRUE;
}

static void
vis_exit(struct TVisualBase *mod)
{
	if (mod->vis_InitRequest)
		TDisplayFreeReq((struct TDisplayBase *)
			mod->vis_InitRequest->tvr_Req.io_Device,
			mod->vis_InitRequest);
	TDestroyList(&mod->vis_Displays);
}
