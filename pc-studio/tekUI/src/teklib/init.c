
/*
**	$Id: init.c,v 1.5 2006/09/10 20:30:07 tmueller Exp $
**	teklib/src/teklib/init.c - TEKlib initialization
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/lib/init.h>

/*****************************************************************************/
/*
**	Application-specific properties in named atoms
*/

static TAPTR
init_newatom(TAPTR exec, TSTRPTR name, TAPTR ptr)
{
	struct TAtom *atom = TExecLockAtom(exec, name, TATOMF_CREATE | TATOMF_NAME);
	if (atom)
	{
		TExecSetAtomData(exec, atom, (TTAG) ptr);
		TExecUnlockAtom(exec, atom, TATOMF_KEEP);
	}
	return atom;
}

static TINT
init_addatomfromtag(TAPTR exec, TAPTR *atomp, TSTRPTR name, TTAG init)
{
	if (init)
	{
		*atomp = init_newatom(exec, name, (TAPTR) init);
		if (*atomp == TNULL)
			return 1;
	}
	else
		*atomp = TNULL;

	return 0;
}

static TBOOL
init_appatoms(struct TEKlibInit *init, TTAGITEM *tags)
{
	TAPTR exec = init->tli_ExecBase;
	TINT error;

	error = init_addatomfromtag(exec, &init->tli_AtomInitMods,
		"sys.imods", TGetTag(tags, TExecBase_ModInit, TNULL));
	error += init_addatomfromtag(exec, &init->tli_AtomArgV,
		"sys.argv", TGetTag(tags, TExecBase_ArgV, TNULL));
	error += init_addatomfromtag(exec, &init->tli_AtomArgs,
		"sys.arguments", TGetTag(tags, TExecBase_Arguments, TNULL));
	error += init_addatomfromtag(exec, &init->tli_AtomProgName,
		"sys.progname", TGetTag(tags, TExecBase_ProgName, TNULL));
	error += init_addatomfromtag(exec, &init->tli_AtomRetValP,
		"sys.returnvalue", TGetTag(tags, TExecBase_RetValP, TNULL));
	error += ((init->tli_AtomUnique =
		init_newatom(exec, "sys.uniqueid", &init->tli_UniqueID)) == TNULL);

	return (TBOOL) (error == 0);
}

/*****************************************************************************/
/*
**	halbase = openhalmodule(init, tags)
**	closehalmodule(init)
*/

static TAPTR
init_openhalmodule(struct TEKlibInit *init, TTAGITEM *usertags)
{
	init->tli_HALBase = TNULL;
	init->tli_HALMod = TEKlib_LoadModule(init->tli_BootHnd,
		init->tli_ProgDir, init->tli_ModDir, TMODNAME_HAL, usertags);
	if (init->tli_HALMod)
	{
		/* get HAL module entrypoint */
		init->tli_HALEntry = TEKlib_GetEntry(init->tli_BootHnd,
			init->tli_HALMod, "tek_init_hal");
		if (init->tli_HALEntry)
		{
			/* prefer user tags */
			init->tli_HALTags[0].tti_Tag = TTAG_GOSUB;
			init->tli_HALTags[0].tti_Value = (TTAG) usertags;

			/* ptr to HAL module base */
			init->tli_HALTags[1].tti_Tag = TExecBase_HAL;
			init->tli_HALTags[1].tti_Value = (TTAG) &init->tli_HALBase;

			/* applicaton path */
			init->tli_HALTags[2].tti_Tag = TExecBase_ProgDir;
			init->tli_HALTags[2].tti_Value = (TTAG) init->tli_ProgDir;

			/* TEKlib system directory */
			init->tli_HALTags[3].tti_Tag = TExecBase_SysDir;
			init->tli_HALTags[3].tti_Value = (TTAG) init->tli_SysDir;

			/* TEKlib module directory */
			init->tli_HALTags[4].tti_Tag = TExecBase_ModDir;
			init->tli_HALTags[4].tti_Value = (TTAG) init->tli_ModDir;

			/* Boot handle */
			init->tli_HALTags[5].tti_Tag = TExecBase_BootHnd;
			init->tli_HALTags[5].tti_Value = (TTAG) init->tli_BootHnd;

			init->tli_HALTags[6].tti_Tag = TTAG_DONE;

			/* call module entry. this will place a pointer to HALbase
			into the variable being pointed to by TExecBase_HAL */

			TEKlib_CallModule(init->tli_BootHnd, init->tli_HALMod,
				init->tli_HALEntry, TNULL, (TAPTR) 1, 0, init->tli_HALTags);

			if (init->tli_HALBase)
				return init->tli_HALBase;
		}
		TEKlib_CloseModule(init->tli_BootHnd, init->tli_HALMod);
	}
	return TNULL;
}

static void
init_closehalmodule(struct TEKlibInit *init)
{
	TDESTROY(init->tli_HALBase);
	TEKlib_CloseModule(init->tli_BootHnd, init->tli_HALMod);
}

/*****************************************************************************/
/*
**	execbase = openexecmodule(init, tags)
**	closeexecmodule(init)
*/

static TAPTR
init_openexecmodule(struct TEKlibInit *init, TTAGITEM *usertags)
{
	/* load exec module */
	init->tli_ExecMod = TEKlib_LoadModule(init->tli_BootHnd,
		init->tli_ProgDir, init->tli_ModDir, "exec", usertags);
	if (init->tli_ExecMod)
	{
		/* get exec module entrypoint */
		init->tli_ExecEntry = TEKlib_GetEntry(init->tli_BootHnd,
			init->tli_ExecMod, "tek_init_exec");
		if (init->tli_ExecEntry)
		{
			TUINT psize, nsize;
			TUINT16 version;
			struct TExecBase *execbase;

			/* get version and size */
			version = (TUINT16) TGetTag(init->tli_HALTags,
				TExecBase_Version, 0);
			psize = TEKlib_CallModule(init->tli_BootHnd, init->tli_ExecMod,
				init->tli_ExecEntry, TNULL, TNULL, version, init->tli_HALTags);
			nsize = TEKlib_CallModule(init->tli_BootHnd, init->tli_ExecMod,
				init->tli_ExecEntry, TNULL, TNULL, 0xffff, init->tli_HALTags);

			/* get memory for the exec module */
			execbase = TEKlib_Alloc(init->tli_BootHnd, nsize + psize);
			if (execbase)
			{
				/* initialize execbase */
				execbase = (struct TExecBase *) (((TINT8 *) execbase) + nsize);
				if (TEKlib_CallModule(init->tli_BootHnd, init->tli_ExecMod,
					init->tli_ExecEntry, TNULL, execbase, 0,
					init->tli_HALTags))
				{
					init->tli_ExecBase = execbase;
					return execbase;
				}
				TEKlib_Free(init->tli_BootHnd, ((TINT8 *) execbase) - nsize,
					nsize + psize);
			}
		}
		TEKlib_CloseModule(init->tli_BootHnd, init->tli_ExecMod);
	}
	init->tli_ExecBase = TNULL;
	return TNULL;
}

static void
init_closeexecmodule(struct TEKlibInit *init)
{
	struct TModule *execbase = (struct TModule *) init->tli_ExecBase;
	TINT nsize = execbase->tmd_NegSize;
	TDESTROY(execbase);
	TEKlib_Free(init->tli_BootHnd, ((TINT8 *) execbase) - nsize,
		execbase->tmd_NegSize + nsize);
	TEKlib_CloseModule(init->tli_BootHnd, init->tli_ExecMod);
}

/*****************************************************************************/
/*
**	Run an Execbase context ("exec", "ramlib")
*/

static TTASKENTRY void
init_execfunc(struct TTask *task)
{
	TAPTR exec = TGetExecBase(task);
	TExecDoExec(exec, TEXEC_CMD_RUN, TNULL);
}

/*****************************************************************************/
/*
**	Apptask destructor
**	call previously saved task destructor, signal execbase to abort,
**	closedown execbase, shutdown exec and HAL modules, free all memory
*/

static void
init_destroyatom(TAPTR exec, TAPTR atom, TSTRPTR name)
{
	if (TExecLockAtom(exec, atom,
		TATOMF_TRY | TATOMF_DESTROY) != atom)
		TDBPRINTF(20, ("atom '%s' is still in use\n", name));
}

static void init_tek_destroy(TAPTR apptask)
{
	TAPTR exec, exectask, boot;
	struct TEKlibInit *init;

	exec = TGetExecBase(apptask);
	exectask = TExecFindTask(exec, TTASKNAME_EXEC);
	init = TExecGetInitData(exec, exectask);
	boot = init->tli_BootHnd;

	init_destroyatom(exec, init->tli_AtomUnique, "sys.unique");
	init_destroyatom(exec, init->tli_AtomRetValP, "sys.returnvalue");
	init_destroyatom(exec, init->tli_AtomProgName, "sys.progname");
	init_destroyatom(exec, init->tli_AtomArgs, "sys.arguments");
	init_destroyatom(exec, init->tli_AtomArgV, "sys.argv");
	init_destroyatom(exec, init->tli_AtomInitMods, "sys.imods");

	/* deinitializations in basetask: */
	TExecDoExec(init->tli_ExecBase, TEXEC_CMD_EXIT, TNULL);

	TExecSignal(init->tli_ExecBase, init->tli_ExecTask, TTASK_SIG_ABORT);
	TExecSignal(init->tli_ExecBase, init->tli_IOTask, TTASK_SIG_ABORT);
	TDESTROY(init->tli_ExecTask);
	TDESTROY(init->tli_IOTask);

	TCALLHOOKPKT(&init->tli_OrgAppTaskHook, apptask, TNULL);

	init_closeexecmodule(init);
	init_closehalmodule(init);

	TEKlib_FreeVec(boot, init->tli_ProgDir);
	TEKlib_FreeVec(boot, init->tli_SysDir);
	TEKlib_FreeVec(boot, init->tli_ModDir);
	TEKlib_Free(boot, init, sizeof(struct TEKlibInit));

	TEKlib_Exit(boot);
}

static THOOKENTRY TTAG
init_destroyapptask(struct THook *hook, TAPTR apptask, TTAG msg)
{
	TDBASSERT(20, msg == TMSG_DESTROY);
	TEKlib_DoUnref(init_tek_destroy, apptask);
	return 0;
}

/*****************************************************************************/
/*
**	Create application task in current context
*/

static struct TTask *init_tek_create(TTAGITEM *usertags)
{
	struct TEKlibInit *init;
	TAPTR boot = TEKlib_Init(usertags);
	if (boot == TNULL)
		return TNULL;
	init = TEKlib_Alloc(boot, sizeof(struct TEKlibInit));
	if (init)
	{
		init->tli_BootHnd = boot;
		init->tli_ProgDir = TEKlib_GetProgDir(boot, usertags);
		init->tli_SysDir = TEKlib_GetSysDir(boot, usertags);
		init->tli_ModDir = TEKlib_GetModDir(boot, usertags);

		/* load host abstraction module */
		if (init_openhalmodule(init, usertags))
		{
			/* load exec module */
			if (init_openexecmodule(init, usertags))
			{
				/* place application task into current context */
				init->tli_ExecTags[0].tti_Tag = TTask_Name;
				init->tli_ExecTags[0].tti_Value = (TTAG) TTASKNAME_ENTRY;
				init->tli_ExecTags[1].tti_Tag = TTask_InitData;
				init->tli_ExecTags[1].tti_Value = (TTAG) init;
				init->tli_ExecTags[2].tti_Tag = TTAG_MORE;
				init->tli_ExecTags[2].tti_Value = (TTAG) usertags;
				init->tli_AppTask = TExecCreateSysTask(init->tli_ExecBase,
					TNULL, init->tli_ExecTags);
				if (init->tli_AppTask)
				{
					/* fill in missing fields in execbase */
					((struct TModule *) init->tli_ExecBase)->tmd_HALMod =
						init->tli_ExecMod;
					((struct TModule *) init->tli_ExecBase)->tmd_InitTask =
						init->tli_AppTask;

					/* create ramlib task */
					init->tli_ExecTags[0].tti_Value = (TTAG) TTASKNAME_RAMLIB;
					init->tli_IOTask = TExecCreateSysTask(init->tli_ExecBase,
						init_execfunc, init->tli_ExecTags);
					if (init->tli_IOTask)
					{
						/* create execbase task */
						init->tli_ExecTags[0].tti_Value =
							(TTAG) TTASKNAME_EXEC;
						init->tli_ExecTask =
							TExecCreateSysTask(init->tli_ExecBase,
								init_execfunc, init->tli_ExecTags);
						if (init->tli_ExecTask)
						{
							/* this is the backdoor for the remaining
							** initializations in the entrytask context */
							if (TExecDoExec(init->tli_ExecBase, TEXEC_CMD_INIT, TNULL))
							{
								struct THandle *ath =
									(struct THandle *) init->tli_AppTask;
								/* overwrite apptask destructor */
								init->tli_OrgAppTaskHook = ath->thn_Hook;
								TInitHook(&ath->thn_Hook,
									init_destroyapptask, TNULL);
								init_appatoms(init, usertags);
								/* application is running */
								return init->tli_AppTask;
							}
							TExecSignal(init->tli_ExecBase, init->tli_ExecTask,
								TTASK_SIG_ABORT);
							TDESTROY(init->tli_ExecTask);
						}
						TExecSignal(init->tli_ExecBase, init->tli_IOTask,
							TTASK_SIG_ABORT);
						TDESTROY(init->tli_IOTask);
					}
					TDESTROY(init->tli_AppTask);
				}
				init_closeexecmodule(init);
			}
			else
				TDBPRINTF(20, ("could not open Exec module\n"));
			init_closehalmodule(init);
		}
		else
			TDBPRINTF(20, ("could not open HAL module\n"));

		TEKlib_FreeVec(boot, init->tli_ProgDir);
		TEKlib_FreeVec(boot, init->tli_SysDir);
		TEKlib_FreeVec(boot, init->tli_ModDir);
		TEKlib_Free(boot, init, sizeof(struct TEKlibInit));
	}

	TEKlib_Exit(boot);
	return TNULL;
}

TLIBAPI struct TTask *TEKCreate(TTAGITEM *usertags)
{
	return TEKlib_DoRef(init_tek_create, usertags);
}
