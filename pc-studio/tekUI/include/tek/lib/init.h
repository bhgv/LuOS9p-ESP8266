#ifndef _TEK_INIT_H
#define _TEK_INIT_H

/*
**	$Id: init.h,v 1.5 2006/09/10 20:31:27 tmueller Exp $
**	teklib/boot/init.h - Startup library definitions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/mod/hal.h>

/*****************************************************************************/
/*
**	This initdata packet will be attached to exectask's userdata
**	during the lifetime of apptask.
*/

struct TEKlibInit
{
	/* inital boot handle */
	TAPTR tli_BootHnd;
	/* host-specific ptr to hal module */
	TAPTR tli_HALMod;
	/* host-specific ptr to exec module */
	TAPTR tli_ExecMod;
	/* hal entry function */
	TMODINITFUNC tli_HALEntry;
	/* exec entry function */
	TMODINITFUNC tli_ExecEntry;
	/* hal module base */
	TAPTR tli_HALBase;
	/* exec module base */
	struct TExecBase *tli_ExecBase;
	/* PROGDIR: */
	TSTRPTR tli_ProgDir;
	/* SYS: */
	TSTRPTR tli_SysDir;
	/* system-wide module directory */
	TSTRPTR tli_ModDir;
	/* host-specific execbase thread */
	struct THALObject tli_ExecThread;
	/* to signal apptask that exec is running */
	struct THALObject tli_InitEvent;
	/* execbase task handle */
	struct TTask *tli_ExecTask;
	/* application task */
	struct TTask *tli_AppTask;
	/* io task handle */
	struct TTask *tli_IOTask;
	/* original destructor for app task */
	struct THook tli_OrgAppTaskHook;
	/* attributes for HAL init */
	TTAGITEM tli_HALTags[7];
	/* attributes for Exec init */
	TTAGITEM tli_ExecTags[3];
	/* named atom holding the argv vector */
	TAPTR tli_AtomArgV;
	/* named atom holding a ptr to retvalue */
	TAPTR tli_AtomRetValP;
	/* named atom holding a ptr to unique ID */
	TAPTR tli_AtomUnique;
	/* named atom holding initmodules */
	TAPTR tli_AtomInitMods;
	/* named atom holding argument string */
	TAPTR tli_AtomArgs;
	/* named atom holding progname */
	TAPTR tli_AtomProgName;
	/* unique ID */
	TUINT tli_UniqueID;
};

/*****************************************************************************/
/*
**	User entrypoint
*/

extern TTASKENTRY void TEKMain(struct TTask *task);

/*****************************************************************************/
/*
**	TEKlib init prototypes
*/

TLIBAPI TAPTR TEKlib_Init(TTAGITEM *tags);
TLIBAPI void TEKlib_Exit(TAPTR handle);
TLIBAPI TAPTR TEKlib_Alloc(TAPTR handle, TUINT size);
TLIBAPI void TEKlib_Free(TAPTR handle, TAPTR mem, TUINT size);
TLIBAPI void TEKlib_FreeVec(TAPTR handle, TAPTR mem);
TLIBAPI TSTRPTR TEKlib_GetSysDir(TAPTR handle, TTAGITEM *tags);
TLIBAPI TSTRPTR TEKlib_GetModDir(TAPTR handle, TTAGITEM *tags);
TLIBAPI TSTRPTR TEKlib_GetProgDir(TAPTR handle, TTAGITEM *tags);
TLIBAPI TAPTR TEKlib_LoadModule(TAPTR handle, TSTRPTR progdir, TSTRPTR moddir,
	TSTRPTR modname, TTAGITEM *tags);
TLIBAPI void TEKlib_CloseModule(TAPTR handle, TAPTR halmod);
TLIBAPI TMODINITFUNC TEKlib_GetEntry(TAPTR handle, TAPTR halmod, TSTRPTR name);
TLIBAPI TUINT TEKlib_CallModule(TAPTR handle, TAPTR ModBase, TMODINITFUNC entry,
	struct TTask *task, TAPTR mod, TUINT16 version, TTAGITEM *tags);

#endif
