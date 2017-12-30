#ifndef _TEK_HAL_HAL_MOD_H
#define	_TEK_HAL_HAL_MOD_H

/*
**	$Id: hal_mod.h,v 1.3 2006/09/10 01:10:04 tmueller Exp $
**	teklib/mods/hal/hal_mod.h - HAL module internal definitions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>
#include <tek/teklib.h>
#include <tek/mod/hal.h>
#include <tek/mod/time.h>

#ifndef LOCAL
#define LOCAL
#endif

#ifndef EXPORT
#define EXPORT TMODAPI
#endif

/*****************************************************************************/
/*
**	HAL module structure
*/

struct THALBase
{
	/* Module header */
	struct TModule hmb_Module;
	/* Ptr to platform-specific data */
	TAPTR hmb_Specific;
	/* Ptr to boot handle */
	TAPTR hmb_BootHnd;
};

/*****************************************************************************/
/*
**	internal prototypes
*/

LOCAL TBOOL hal_init(struct THALBase *hal, TTAGITEM *tags);
LOCAL void hal_exit(struct THALBase *hal);

LOCAL TAPTR hal_allocself(TAPTR boot, TUINT size);
LOCAL void hal_freeself(TAPTR boot, TAPTR mem, TUINT size);

LOCAL struct TTimeRequest *hal_open(struct THALBase *hal, struct TTask *task, TTAGITEM *tags);
LOCAL void hal_close(struct THALBase *hal, struct TTask *task);

/*****************************************************************************/
/*
**	API prototypes
*/

EXPORT void hal_beginio(struct THALBase *hal, struct TTimeRequest *req);
EXPORT TINT hal_abortio(struct THALBase *hal, struct TTimeRequest *req);
EXPORT TAPTR hal_alloc(struct THALBase *hal, TSIZE size);
EXPORT void hal_free(struct THALBase *hal, TAPTR mem, TSIZE size);
EXPORT TAPTR hal_realloc(struct THALBase *hal, TAPTR mem, TSIZE oldsize,
	TSIZE newsize);
EXPORT void hal_copymem(struct THALBase *hal, TAPTR from, TAPTR to, TSIZE len);
EXPORT void hal_fillmem(struct THALBase *hal, TAPTR dest, TSIZE numbytes,
	TUINT8 fillval);
EXPORT TBOOL hal_initlock(struct THALBase *hal, struct THALObject *lock);
EXPORT void hal_destroylock(struct THALBase *hal, struct THALObject *lock);
EXPORT void hal_lock(struct THALBase *hal, struct THALObject *lock);
EXPORT void hal_unlock(struct THALBase *hal, struct THALObject *lock);
EXPORT TBOOL hal_initthread(struct THALBase *hal, struct THALObject *thread,
	TTASKENTRY void (*function)(struct TTask *task), TAPTR data);
EXPORT void hal_destroythread(struct THALBase *hal, struct THALObject *thread);
EXPORT TAPTR hal_findself(struct THALBase *hal);
EXPORT TAPTR hal_loadmodule(struct THALBase *hal, TSTRPTR name, TUINT16 version,
	TUINT *psize, TUINT *nsize);
EXPORT TBOOL hal_callmodule(struct THALBase *hal, TAPTR halmod, struct TTask *task,
	TAPTR mod);
EXPORT void hal_unloadmodule(struct THALBase *hal, TAPTR halmod);
EXPORT TBOOL hal_scanmodules(struct THALBase *hal, TSTRPTR path, struct THook *hook);
EXPORT TTAG hal_getattr(struct THALBase *hal, TUINT tag, TTAG defval);
EXPORT TUINT hal_wait(struct THALBase *hal, TUINT signals);
EXPORT void hal_signal(struct THALBase *hal, struct THALObject *thread, TUINT signals);
EXPORT TUINT hal_setsignal(struct THALBase *hal, TUINT newsig, TUINT sigmask);

#endif
