
/*
**	$Id: hal_mod.c,v 1.6 2006/11/11 14:19:09 tmueller Exp $
**	teklib/src/hal/hal_mod.c - HAL module
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "hal_mod.h"
#include <tek/teklib.h>
#include <tek/mod/exec.h>
#include <tek/debug.h>

#define HAL_VERSION		4
#define HAL_REVISION	0
#define HAL_NUMVECTORS	28

static THOOKENTRY TTAG hal_dispatch(struct THook *hook, TAPTR obj, TTAG msg);
static const TMFPTR hal_vectors[HAL_NUMVECTORS];

/*****************************************************************************/
/*
**	Module Init
*/

TMODENTRY TUINT
tek_init_hal(struct TTask *task, struct TModule *mod, TUINT16 version,
	TTAGITEM *tags)
{
	struct THALBase *hal = (struct THALBase *) mod;
	struct THALBase **halbaseptr;
	TAPTR boot;

	if (hal == TNULL)
	{
		if (version == 0xffff)
			return sizeof(TAPTR) * HAL_NUMVECTORS; /* negative size */

		if (version <= HAL_VERSION)
			return sizeof(struct THALBase); /* positive size */

		return 0;
	}

	boot = (TAPTR) TGetTag(tags, TExecBase_BootHnd, TNULL);

	halbaseptr = (struct THALBase **) TGetTag(tags, TExecBase_HAL, TNULL);
	*halbaseptr = TNULL;

	hal = hal_allocself(boot,
		sizeof(struct THALBase) + sizeof(TAPTR) * HAL_NUMVECTORS);

	if (!hal) return 0;
	hal = (struct THALBase *) (((TAPTR *) hal) + HAL_NUMVECTORS);

	hal_fillmem(hal, hal, sizeof(struct THALBase), 0);

	hal->hmb_BootHnd = boot;

	hal->hmb_Module.tmd_Version = HAL_VERSION;
	hal->hmb_Module.tmd_Revision = HAL_REVISION;

	hal->hmb_Module.tmd_Handle.thn_Name = TMODNAME_HAL;
	hal->hmb_Module.tmd_ModSuper = (struct TModule *) hal;
	hal->hmb_Module.tmd_NegSize = sizeof(TAPTR) * HAL_NUMVECTORS;
	hal->hmb_Module.tmd_PosSize = sizeof(struct THALBase);
	hal->hmb_Module.tmd_RefCount = 1;
	hal->hmb_Module.tmd_Flags =
		TMODF_INITIALIZED | TMODF_VECTORTABLE | TMODF_OPENCLOSE;
	hal->hmb_Module.tmd_Handle.thn_Hook.thk_Entry = hal_dispatch;
	hal->hmb_Module.tmd_Handle.thn_Hook.thk_Data = hal;

	TInitVectors(&hal->hmb_Module, hal_vectors, HAL_NUMVECTORS);

	if (hal_init(hal, tags))
	{
		*halbaseptr = hal;
		return TTRUE;
	}

	hal_freeself(boot, ((TAPTR *) hal) - HAL_NUMVECTORS,
		hal->hmb_Module.tmd_NegSize + hal->hmb_Module.tmd_PosSize);

	return TFALSE;
}

/*****************************************************************************/

static const TMFPTR
hal_vectors[HAL_NUMVECTORS] =
{
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) hal_beginio,
	(TMFPTR) hal_abortio,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,
	(TMFPTR) TNULL,

	(TMFPTR) hal_getattr,

	(TMFPTR) hal_alloc,
	(TMFPTR) hal_free,
	(TMFPTR) hal_realloc,
	(TMFPTR) hal_copymem,
	(TMFPTR) hal_fillmem,

	(TMFPTR) hal_initlock,
	(TMFPTR) hal_destroylock,
	(TMFPTR) hal_lock,
	(TMFPTR) hal_unlock,

	(TMFPTR) hal_initthread,
	(TMFPTR) hal_destroythread,
	(TMFPTR) hal_findself,

	(TMFPTR) hal_wait,
	(TMFPTR) hal_signal,
	(TMFPTR) hal_setsignal,

	(TMFPTR) hal_loadmodule,
	(TMFPTR) hal_callmodule,
	(TMFPTR) hal_unloadmodule,

	(TMFPTR) hal_scanmodules,
};

/*****************************************************************************/

static THOOKENTRY TTAG
hal_dispatch(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct THALBase *mod = (struct THALBase *) hook->thk_Data;
	switch (msg)
	{
		case TMSG_DESTROY:
			hal_exit(mod);
			hal_freeself(mod->hmb_BootHnd, ((TAPTR *) mod) - HAL_NUMVECTORS,
				mod->hmb_Module.tmd_NegSize + mod->hmb_Module.tmd_PosSize);
			break;
		case TMSG_OPENMODULE:
			return (TTAG) hal_open(mod, TNULL, obj);
		case TMSG_CLOSEMODULE:
			hal_close(obj, TNULL);
	}
	return 0;
}

/*****************************************************************************/
/*
**	Get a HAL attribute
*/

EXPORT TTAG
hal_getattr(struct THALBase *hal, TUINT tag, TTAG defval)
{
	return TGetTag((TTAGITEM *) hal->hmb_Specific, tag, defval);
}
