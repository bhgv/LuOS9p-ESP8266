
/*
**	$Id: exec_doexec.c,v 1.6 2006/11/11 14:19:09 tmueller Exp $
**	teklib/src/exec/exec_doexec.c - Exec task contexts
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/io.h>
#include "exec_mod.h"

static void exec_requestmod(TEXECBASE *exec, struct TTask *taskmsg);
static void exec_closemod(TEXECBASE *exec, struct TTask *taskmsg);
static void exec_replymod(TEXECBASE *exec, struct TTask *taskmsg);
static void exec_main(TEXECBASE *exec, struct TTask *task,
	struct TTagItem *tags);
static void exec_ramlib(TEXECBASE *exec, struct TTask *task,
	struct TTagItem *tags);
static void exec_loadmod(TEXECBASE *exec, struct TTask *task,
	struct TTask *taskmsg);
static void exec_unloadmod(TEXECBASE *exec, struct TTask *task,
	struct TTask *taskmsg);
static void exec_newtask(TEXECBASE *exec, struct TTask *taskmsg);
static struct TTask *exec_createtask(TEXECBASE *exec,
	struct THook *hook, struct TTagItem *tags);
static TTASKENTRY void exec_taskentryfunc(struct TTask *task);
static void exec_childinit(TEXECBASE *exec);
static void exec_childexit(TEXECBASE *exec);
static void exec_lockatom(TEXECBASE *exec, struct TTask *msg);
static void exec_unlockatom(TEXECBASE *exec, struct TTask *msg);

/*****************************************************************************/
/*
**	success = TDoExec(exec, cmd, tags)
**	entrypoint for exec internal services and initializations
*/

EXPORT TBOOL exec_DoExec(TEXECBASE *TExecBase, TUINT cmd,
	struct TTagItem *tags)
{
	struct TTask *task = TFindTask(TNULL);
	switch (cmd)
	{
		case TEXEC_CMD_RUN:
			/* determine which context to run: */
			if (task == TFindTask(TTASKNAME_EXEC))
			{
				/* perform execbase context */
				exec_main(TExecBase, task, tags);
				return TTRUE;
			}
			else if (task == TFindTask(TTASKNAME_RAMLIB))
			{
				/* perform ramlib context */
				exec_ramlib(TExecBase, task, tags);
				return TTRUE;
			}
			break;

		case TEXEC_CMD_INIT:
			/* place for initializations in entry context */
			task->tsk_TimeReq = TAllocTimeRequest(TNULL);
			if (task->tsk_TimeReq)
				return TTRUE;
			break;

		case TEXEC_CMD_EXIT:
			/* place for de-initializations in entry context */
			TFreeTimeRequest(task->tsk_TimeReq);
			return TTRUE;
	}
	return TFALSE;
}

/*****************************************************************************/
/*
**	Ramlib task - Module loading
*/

static void exec_ramlib(TEXECBASE *TExecBase, struct TTask *task,
	struct TTagItem *tags)
{
	struct TMsgPort *port = &task->tsk_UserPort;
	TUINT waitsig = TTASK_SIG_ABORT | port->tmp_Signal;

	struct TTask *taskmsg;
	TINT n = 0;

	for (;;)
	{
		TUINT sig = THALWait(TExecBase->texb_HALBase, waitsig);
		if (sig & TTASK_SIG_ABORT)
			break;

		while ((taskmsg = TGetMsg(port)))
		{
			switch (taskmsg->tsk_ReqCode)
			{
				case TTREQ_LOADMOD:
					exec_loadmod(TExecBase, task, taskmsg);
					break;

				case TTREQ_UNLOADMOD:
					exec_unloadmod(TExecBase, task, taskmsg);
					break;

				case TTREQ_ADDMOD:
					TAddHead(&TExecBase->texb_IntModList, (struct TNode *)
						taskmsg->tsk_Request.trq_AddRemMod.trm_ModInitNode);
					taskmsg->tsk_Request.trq_AddRemMod.trm_Result = TTRUE;
					TReplyMsg(taskmsg);
					break;

				case TTREQ_REMMOD:
					/* should removal only succeed if flags == 0 or
					flags == TMODF_INITIALIZED and nestcount == 0 ? */
					TRemove((struct TNode *)
						taskmsg->tsk_Request.trq_AddRemMod.trm_ModInitNode);
					taskmsg->tsk_Request.trq_AddRemMod.trm_Result = TTRUE;
					TReplyMsg(taskmsg);
					break;
			}
		}
	}

	while ((taskmsg = TGetMsg(port)))
		n++;

	if (n > 0)
	{
		TDBPRINTF(TDB_FAIL,("Module requests pending: %d\n", n));
		TDBFATAL();
	}
}

/*****************************************************************************/

static void exec_loadmod(TEXECBASE *TExecBase, struct TTask *task,
	struct TTask *taskmsg)
{
	TAPTR halmod = TNULL;
	TAPTR hal = TExecBase->texb_HALBase;
	union TTaskRequest *req = &taskmsg->tsk_Request;
	TSTRPTR modname = req->trq_Mod.trm_InitMod.tmd_Handle.thn_Name;
	TUINT nsize = 0, psize = 0;
	const struct TInitModule *imod = TNULL;
	struct TNode *nnode, *node;

	/* try to open from list of internal modules: */

	node = TExecBase->texb_IntModList.tlh_Head.tln_Succ;
	for (; (nnode = node->tln_Succ); node = nnode)
	{
		const struct TInitModule *im = 
			((struct TModInitNode *) node)->tmin_Modules;
		TSTRPTR tempn;

		for (; (tempn = im->tinm_Name); im++)
		{
			if (TStrEqual(tempn, modname))
			{
				psize = (*im->tinm_InitFunc)(TNULL, TNULL,
					req->trq_Mod.trm_InitMod.tmd_Version, TNULL);
				if (psize)
					nsize = (*im->tinm_InitFunc)(TNULL, TNULL, 0xffff, TNULL);
				imod = im;
				break;
			}
		}
	}

	if (psize == 0)
	{
		/* try to load module via HAL interface: */

		halmod = THALLoadModule(hal, modname,
			req->trq_Mod.trm_InitMod.tmd_Version, &psize, &nsize);
	}

	if (psize)
	{
		TINT nlen;
		TSTRPTR s = modname;
		TINT8 *mmem;

		while (*s++);
		nlen = s - modname;

		mmem = TAlloc(TNULL, psize + nsize + nlen);
		if (mmem)
		{
			TBOOL success = TFALSE;
			struct TModule *mod = (struct TModule *) (mmem + nsize);
			TSTRPTR d = (TSTRPTR) mmem + nsize + psize;

			TFillMem(mmem, psize + nsize, 0);

			s = modname;
			/* insert and copy name: */
			mod->tmd_Handle.thn_Name = d;
			while ((*d++ = *s++));

			/* modsuper: */
			mod->tmd_Handle.thn_Hook.thk_Data = mod;
			mod->tmd_Handle.thn_Owner = (struct TModule *) TExecBase;
			mod->tmd_ModSuper = mod;
			mod->tmd_InitTask = task;
			mod->tmd_HALMod = halmod;
			mod->tmd_PosSize = psize;
			mod->tmd_NegSize = nsize;
			mod->tmd_RefCount = 1;

			if (halmod)
				success = THALCallModule(hal, halmod, task, mod);
			else
				success = (*imod->tinm_InitFunc)(task, mod,
					req->trq_Mod.trm_InitMod.tmd_Version, TNULL);

			if (success)
			{
				if ((mod->tmd_Flags & TMODF_VECTORTABLE) &&
					nsize < sizeof(TMFPTR) * TMODV_NUMRESERVED)
				{
					TDBPRINTF(TDB_FAIL,
						("Module %s hasn't reserved enough vectors\n",
						mod->tmd_Handle.thn_Name));
					TDBFATAL();
				}
				req->trq_Mod.trm_Module = mod;
				TReplyMsg(taskmsg);
				return;
			}

			TFree(mmem);
		}

		if (halmod)
			THALUnloadModule(hal, halmod);
	}

	/* failed */
	TDropMsg(taskmsg);
}

/*****************************************************************************/

static void exec_unloadmod(TEXECBASE *TExecBase, struct TTask *task,
	struct TTask *taskmsg)
{
	TAPTR hal = TExecBase->texb_HALBase;
	union TTaskRequest *req = &taskmsg->tsk_Request;
	struct TModule *mod = req->trq_Mod.trm_Module;

	TDBPRINTF(TDB_TRACE,("unload mod: %s\n", mod->tmd_Handle.thn_Name));

	/* Invoke module destructor: */
	TDESTROY(mod);

	/* Unload: */
	if (mod->tmd_HALMod)
		THALUnloadModule(hal, mod->tmd_HALMod);

	/* Free: */
	TFree((TINT8 *) mod - mod->tmd_NegSize);

	/* Restore original replyport: */
	TGETMSGPTR(taskmsg)->tmsg_RPort = req->trq_Mod.trm_RPort;

	/* Return to caller: */
	TAckMsg(taskmsg);
}

/*****************************************************************************/
/*
**	Exec task
*/

static void exec_checkmodules(TEXECBASE *TExecBase)
{
	TINT n = 0;
	struct TNode *nnode, *node = TExecBase->texb_ModList.tlh_Head.tln_Succ;
	for (; (nnode = node->tln_Succ); node = nnode)
	{
		struct TModule *mod = (struct TModule *) node;
		if (mod == (struct TModule *) TExecBase ||
			mod == (struct TModule *) TExecBase->texb_HALBase)
			continue;
		TDBPRINTF(TDB_FAIL,
			("Module not closed: %s\n", mod->tmd_Handle.thn_Name));
		n++;
	}

	if (n > 0)
	{
		TDBPRINTF(TDB_FAIL,("Applications must close all modules"));
		TDBFATAL();
	}
}

static void exec_main(TEXECBASE *TExecBase, struct TTask *exectask,
	struct TTagItem *tags)
{
	struct TMsgPort *execport = TExecBase->texb_ExecPort;
	struct TMsgPort *modreply = TExecBase->texb_ModReply;
	TUINT waitsig = TTASK_SIG_ABORT | execport->tmp_Signal |
		modreply->tmp_Signal | TTASK_SIG_CHILDINIT | TTASK_SIG_CHILDEXIT;
	struct TTask *taskmsg;

	for (;;)
	{
		TUINT sig = THALWait(TExecBase->texb_HALBase, waitsig);
		if (sig & TTASK_SIG_ABORT)
			break;

		if (sig & modreply->tmp_Signal)
			while ((taskmsg = TGetMsg(modreply)))
				exec_replymod(TExecBase, taskmsg);

		if (sig & execport->tmp_Signal)
		{
			while ((taskmsg = TGetMsg(execport)))
			{
				switch (taskmsg->tsk_ReqCode)
				{
					case TTREQ_OPENMOD:
						exec_requestmod(TExecBase, taskmsg);
						break;

					case TTREQ_CLOSEMOD:
						exec_closemod(TExecBase, taskmsg);
						break;

					case TTREQ_CREATETASK:
						exec_newtask(TExecBase, taskmsg);
						break;

					case TTREQ_DESTROYTASK:
					{
						/* insert backptr to self */
						taskmsg->tsk_Request.trq_Task.trt_Parent = taskmsg;
						/* add request to taskexitlist */
						TAddTail(&TExecBase->texb_TaskExitList,
							&taskmsg->tsk_Request.trq_Task.trt_Node);
						/* force list processing */
						sig |= TTASK_SIG_CHILDEXIT;
						break;
					}

					case TTREQ_LOCKATOM:
						exec_lockatom(TExecBase, taskmsg);
						break;

					case TTREQ_UNLOCKATOM:
						exec_unlockatom(TExecBase, taskmsg);
						break;
				}
			}
		}

		if (sig & TTASK_SIG_CHILDINIT)
			exec_childinit(TExecBase);

		if (sig & TTASK_SIG_CHILDEXIT)
			exec_childexit(TExecBase);
	}

	if (TExecBase->texb_NumTasks || TExecBase->texb_NumInitTasks)
	{
		TDBPRINTF(TDB_FAIL,("%d tasks still running, exiting forcibly\n",
			TExecBase->texb_NumTasks + TExecBase->texb_NumInitTasks));
		TDBFATAL();
	}

	exec_checkmodules(TExecBase);
}

/*****************************************************************************/

static void exec_requestmod(TEXECBASE *TExecBase, struct TTask *taskmsg)
{
	union TTaskRequest *req = &taskmsg->tsk_Request;
	struct TModule *mod =
		(struct TModule *) TFindHandle(&TExecBase->texb_ModList,
		req->trq_Mod.trm_InitMod.tmd_Handle.thn_Name);

	if (mod == TNULL)
	{
		/* Prepare load request: */
		taskmsg->tsk_ReqCode = TTREQ_LOADMOD;

		/* Backup msg original replyport: */
		req->trq_Mod.trm_RPort = TGETMSGREPLYPORT(taskmsg);

		/* Init list of waiters: */
		TINITLIST(&req->trq_Mod.trm_Waiters);

		/* Mark module as uninitialized: */
		req->trq_Mod.trm_InitMod.tmd_Flags = 0;

		/* Insert request as fake/uninitialized module to modlist: */
		TAddTail(&TExecBase->texb_ModList, (struct TNode *) req);

		/* Forward this request to I/O task: */
		TPutMsg(&TExecBase->texb_IOTask->tsk_UserPort,
			TExecBase->texb_ModReply, taskmsg);

		return;
	}

	if (mod->tmd_Version < req->trq_Mod.trm_InitMod.tmd_Version)
	{
		/* Mod version insufficient: */
		TDropMsg(taskmsg);
		return;
	}

	if (mod->tmd_Flags & TMODF_INITIALIZED)
	{
		/* Request succeeded, reply: */
		mod->tmd_RefCount++;
		req->trq_Mod.trm_Module = mod;
		TReplyMsg(taskmsg);
		return;
	}

	/* Mod is an initializing request, add to list of its waiters: */

	req->trq_Mod.trm_ReqTask = taskmsg;

	TAddTail(&((union TTaskRequest *) mod)->trq_Mod.trm_Waiters,
		(struct TNode *) req);
}

/*****************************************************************************/

static void exec_replymod(TEXECBASE *TExecBase, struct TTask *taskmsg)
{
	struct TModule *mod;
	union TTaskRequest *req;
	struct TNode *node, *tnode;

	req = &taskmsg->tsk_Request;
	node = req->trq_Mod.trm_Waiters.tlh_Head.tln_Succ;
	mod = req->trq_Mod.trm_Module;

	/* Unlink fake/initializing module request from modlist: */
	TREMOVE((struct TNode *) req);

	/* Restore original replyport: */
	TGETMSGPTR(taskmsg)->tmsg_RPort = req->trq_Mod.trm_RPort;

	if (TGETMSGSTATUS(taskmsg) == TMSG_STATUS_FAILED)
	{
		/* Fail-reply to all waiters: */
		while ((tnode = node->tln_Succ))
		{
			TREMOVE(node);
			/* Reply to opener: */
			TDropMsg(((union TTaskRequest *) node)->trq_Mod.trm_ReqTask);
			node = tnode;
		}

		/* Forward failure to opener: */
		TDropMsg(taskmsg);
		return;
	}

	/* Mark as ready: */
	mod->tmd_Flags |= TMODF_INITIALIZED;

	/* Link real module to modlist: */
	TAddTail(&TExecBase->texb_ModList, (struct TNode *) mod);

	/* Send replies to all waiters: */
	while ((tnode = node->tln_Succ))
	{
		TREMOVE(node);

		/* Insert module: */
		((union TTaskRequest *) node)->trq_Mod.trm_Module = mod;

		/* Reply to opener: */
		TReplyMsg(((union TTaskRequest *) node)->trq_Mod.trm_ReqTask);

		mod->tmd_RefCount++;
		node = tnode;
	}

	/* Forward success to opener: */
	TReplyMsg(taskmsg);
}

/*****************************************************************************/

static void exec_closemod(TEXECBASE *TExecBase, struct TTask *taskmsg)
{
	struct TModule *mod;
	union TTaskRequest *req;

	req = &taskmsg->tsk_Request;
	mod = req->trq_Mod.trm_Module;

	if (--mod->tmd_RefCount == 0)
	{
		/* Remove from modlist: */
		TREMOVE((struct TNode *) mod);

		/* Prepare unload request: */
		taskmsg->tsk_ReqCode = TTREQ_UNLOADMOD;

		/* Backup msg original replyport: */
		req->trq_Mod.trm_RPort = TGETMSGREPLYPORT(taskmsg);

		/* Forward this request to I/O task: */
		TPutMsg(&TExecBase->texb_IOTask->tsk_UserPort, TNULL, taskmsg);

		return;
	}

	/* Confirm: */
	TAckMsg(taskmsg);
}

/*****************************************************************************/

static void exec_newtask(TEXECBASE *TExecBase, struct TTask *taskmsg)
{
	struct TTask *newtask;
	union TTaskRequest *req = &taskmsg->tsk_Request;
	req->trq_Task.trt_Task = newtask = exec_createtask(TExecBase,
		&req->trq_Task.trt_TaskHook, req->trq_Task.trt_Tags);
	if (newtask)
	{
		/* Insert ptr to self, i.e. the requesting parent: */
		req->trq_Task.trt_Parent = taskmsg;
		/* Add request (not taskmsg) to list of initializing tasks: */
		TAddTail(&TExecBase->texb_TaskInitList, (struct TNode *) req);
		TExecBase->texb_NumInitTasks++;
	}
	else
		TDropMsg(taskmsg);
}

/*****************************************************************************/
/*
**	create task
*/

static THOOKENTRY TTAG exec_usertaskdestroy(struct THook *h, TAPTR obj,
	TTAG msg)
{
	if (msg == TMSG_DESTROY)
	{
		struct TTask *task = obj;
		TEXECBASE *TExecBase = (TEXECBASE *) TGetExecBase(task);
		struct TTask *self = THALFindSelf(TExecBase->texb_HALBase);
		if (self == task)
			TDBPRINTF(TDB_INFO,("task cannot destroy itself, ignored\n"));
		else
		{
			union TTaskRequest *req = &self->tsk_Request;
			self->tsk_ReqCode = TTREQ_DESTROYTASK;
			req->trq_Task.trt_Task = task;
			exec_sendmsg(TExecBase, self, TExecBase->texb_ExecPort, self);
			exec_FreeTask(TExecBase, task);
		}
	}
	return 0;
}

static struct TTask *exec_createtask(TEXECBASE *TExecBase, struct THook *hook,
	struct TTagItem *tags)
{
	TAPTR hal = TExecBase->texb_HALBase;
	TUINT tnlen = 0;
	struct TTask *newtask;
	TSTRPTR tname;

	tname = (TSTRPTR) TGetTag(tags, TTask_Name, TNULL);
	if (tname)
	{
		TSTRPTR t = tname;
		while (*t++);
		tnlen = t - tname;
	}

	/* note that tasks are message allocations */
	newtask = TAlloc(&TExecBase->texb_MsgMemManager, sizeof(struct TTask) + tnlen);
	if (newtask == TNULL)
		return TNULL;

	TFillMem(newtask, sizeof(struct TTask), 0);
	if (THALInitLock(hal, &newtask->tsk_TaskLock))
	{
		if (exec_initmm(TExecBase, &newtask->tsk_HeapMemManager, TNULL,
			TMMT_MemManager, TNULL))
		{
			if (exec_initport(TExecBase, &newtask->tsk_UserPort, newtask,
				TTASK_SIG_USER))
			{
				if (exec_initport(TExecBase, &newtask->tsk_SyncPort, newtask,
					TTASK_SIG_SINGLE))
				{
					if (tname)
					{
						TSTRPTR t = (TSTRPTR) (newtask + 1);
						newtask->tsk_Handle.thn_Name = t;
						while ((*t++ = *tname++));
					}

					newtask->tsk_Handle.thn_Hook.thk_Entry =
						exec_usertaskdestroy;
					newtask->tsk_Handle.thn_Owner =
						(struct TModule *) TExecBase;
					newtask->tsk_UserData =
						(TAPTR) TGetTag(tags, TTask_UserData, TNULL);
					newtask->tsk_SigFree = ~((TUINT) TTASK_SIG_RESERVED);
					newtask->tsk_SigUsed = TTASK_SIG_RESERVED;
					newtask->tsk_Status = TTASK_INITIALIZING;

					newtask->tsk_Request.trq_Task.trt_TaskHook = *hook;
					newtask->tsk_Request.trq_Task.trt_Tags = tags;

					if (THALInitThread(hal, &newtask->tsk_Thread,
						exec_taskentryfunc, newtask))
					{
						/* kick it to life */
						THALSignal(hal, &newtask->tsk_Thread,
							TTASK_SIG_SINGLE);
						return newtask;
					}

					TDESTROY(&newtask->tsk_SyncPort.tmp_Handle);
				}
				TDESTROY(&newtask->tsk_UserPort.tmp_Handle);
			}
			TDESTROY(&newtask->tsk_HeapMemManager.tmm_Handle);
		}
		THALDestroyLock(hal, &newtask->tsk_TaskLock);
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	Task entry
*/

static void exec_closetaskfh(struct TTask *task, TFILE *fh, TUINT flag)
{
	if (task->tsk_Flags & flag)
	{
		TREMOVE((struct TNode *) fh);
		TIOCloseFile(task->tsk_IOBase, fh);
	}
}

static TTASKENTRY void exec_taskentryfunc(struct TTask *task)
{
	TEXECBASE *TExecBase = (TEXECBASE *) TGetExecBase(task);
	union TTaskRequest *req = &task->tsk_Request;
	/* need a copy: */
	struct THook taskhook = req->trq_Task.trt_TaskHook;
	TTAGITEM *tags = req->trq_Task.trt_Tags;

	TAPTR currentdir = (TAPTR) TGetTag(tags, TTask_CurrentDir, TNULL);
	TUINT status = 0;

	if (currentdir)
	{
		task->tsk_IOBase = (struct TIOBase *) TOpenModule("io", 0, TNULL);
		if (task->tsk_IOBase)
		{
			TAPTR newcd = TIODupLock(task->tsk_IOBase, currentdir);
			if (newcd)
			{
				/* new lock on currentdir duplicated from parent */
				TIOChangeDir(task->tsk_IOBase, newcd);
			}
			else
				goto closedown;
		}
		else
			goto closedown;
	}

	task->tsk_TimeReq = TAllocTimeRequest(TNULL);
	if (task->tsk_TimeReq == TNULL)
		goto closedown;

	task->tsk_FHIn = (TAPTR) TGetTag(tags, TTask_InputFH, TNULL);
	task->tsk_FHOut = (TAPTR) TGetTag(tags, TTask_OutputFH, TNULL);
	task->tsk_FHErr = (TAPTR) TGetTag(tags, TTask_ErrorFH, TNULL);

	status = TCALLHOOKPKT(&taskhook, task, TMSG_INITTASK);
	if (status)
	{
		/* change from initializing to running state */
		task->tsk_Status = TTASK_RUNNING;
		THALSignal(TExecBase->texb_HALBase,
			&TExecBase->texb_ExecTask->tsk_Thread, TTASK_SIG_CHILDINIT);
		TCALLHOOKPKT(&taskhook, task, TMSG_RUNTASK);
	}

	if (task->tsk_Flags & (TTASKF_CLOSEOUTPUT | TTASKF_CLOSEINPUT |
		TTASKF_CLOSEERROR))
	{
		if (task->tsk_IOBase == TNULL)
		{
			/* Someone passed a filehandle to this task, but we don't
			even have the I/O module open. It should be investigated if
			this can fail: */
			task->tsk_IOBase = (struct TIOBase *) TOpenModule("io", 0, TNULL);
			if (task->tsk_IOBase == TNULL)
				TDBFATAL();
		}
		exec_closetaskfh(task, task->tsk_FHOut, TTASKF_CLOSEOUTPUT);
		exec_closetaskfh(task, task->tsk_FHIn, TTASKF_CLOSEINPUT);
		exec_closetaskfh(task, task->tsk_FHErr, TTASKF_CLOSEERROR);
	}

closedown:

	TFreeTimeRequest(task->tsk_TimeReq);
	task->tsk_TimeReq = TNULL;

	if (task->tsk_IOBase)
	{
		/* if we are responsible for a currentdir lock, close it */
		TAPTR cd = TIOChangeDir(task->tsk_IOBase, TNULL);
		if (cd)
			TIOUnlockFile(task->tsk_IOBase, cd);
		TCloseModule((struct TModule *) task->tsk_IOBase);
	}

	if (status)
	{
		/* succeeded */
		task->tsk_Status = TTASK_FINISHED;
		THALSignal(TExecBase->texb_HALBase,
			&TExecBase->texb_ExecTask->tsk_Thread, TTASK_SIG_CHILDEXIT);
	}
	else
	{
		/* system initialization failed */
		task->tsk_Status = TTASK_FAILED;
		THALSignal(TExecBase->texb_HALBase,
			&TExecBase->texb_ExecTask->tsk_Thread, TTASK_SIG_CHILDINIT);
	}
}

/*****************************************************************************/

EXPORT void exec_FreeTask(TEXECBASE *TExecBase, struct TTask *task)
{
	TAPTR hal = TExecBase->texb_HALBase;
	THALDestroyThread(hal, &task->tsk_Thread);
	TDESTROY(&task->tsk_SyncPort.tmp_Handle);
	TDESTROY(&task->tsk_UserPort.tmp_Handle);
	TDESTROY(&task->tsk_HeapMemManager.tmm_Handle);
	THALDestroyLock(hal, &task->tsk_TaskLock);
	TFree(task);
}

/*****************************************************************************/
/*
**	handle tasks that change from initializing to running state
*/

static void exec_childinit(TEXECBASE *TExecBase)
{
	TAPTR hal = TExecBase->texb_HALBase;
	struct TNode *nnode, *node = TExecBase->texb_TaskInitList.tlh_Head.tln_Succ;
	while ((nnode = node->tln_Succ))
	{
		union TTaskRequest *req = (union TTaskRequest *) node;
		struct TTask *task = req->trq_Task.trt_Task;
		struct TTask *taskmsg = req->trq_Task.trt_Parent;
		if (task->tsk_Status == TTASK_FAILED)
		{
			/* Remove request from taskinitlist: */
			TREMOVE((struct TNode *) req);
			TExecBase->texb_NumInitTasks--;
			/* Destroy task corpse: */
			exec_FreeTask(TExecBase, task);
			/* Fail-reply taskmsg to sender: */
			TDropMsg(taskmsg);
		}
		else if (task->tsk_Status != TTASK_INITIALIZING)
		{
			/* Remove taskmsg from taskinitlist: */
			TREMOVE((struct TNode *) req);
			TExecBase->texb_NumInitTasks--;
			/* Link task to list of running tasks: */
			THALLock(hal, &TExecBase->texb_Lock);
			/* note: using node as tempnode argument here */
			TAddTail(&TExecBase->texb_TaskList, (struct TNode *) task);
			THALUnlock(hal, &TExecBase->texb_Lock);
			TExecBase->texb_NumTasks++;
			/* Reply taskmsg: */
			TReplyMsg(taskmsg);
		}
		node = nnode;
	}
}

/*****************************************************************************/
/*
**	handle exiting tasks
*/

static void exec_childexit(TEXECBASE *TExecBase)
{
	TAPTR hal = TExecBase->texb_HALBase;
	struct TNode *nnode, *node = TExecBase->texb_TaskExitList.tlh_Head.tln_Succ;
	while ((nnode = node->tln_Succ))
	{
		union TTaskRequest *req = (union TTaskRequest *) node;
		struct TTask *task = req->trq_Task.trt_Task;
		struct TTask *taskmsg = req->trq_Task.trt_Parent;
		if (task->tsk_Status == TTASK_FINISHED)
		{
			/* Unlink task from list of running tasks: */
			THALLock(hal, &TExecBase->texb_Lock);
			TREMOVE((struct TNode *) task);
			THALUnlock(hal, &TExecBase->texb_Lock);
			/* Note that task is freed in sender context */
			/* Unlink taskmsg from list of exiting tasks: */
			TREMOVE((struct TNode *) req);
			TExecBase->texb_NumTasks--;
			/* Reply to caller: */
			TReplyMsg(taskmsg);
		}
		node = nnode;
	}
}

/*****************************************************************************/
/*
**	Atoms
*/

static void
exec_replyatom(TEXECBASE *TExecBase, struct TTask *msg, struct TAtom *atom)
{
	msg->tsk_Request.trq_Atom.tra_Atom = atom;
	TReplyMsg(msg);
}

static TAPTR
exec_lookupatom(TEXECBASE *TExecBase, TSTRPTR name)
{
	return TFindHandle(&TExecBase->texb_AtomList, name);
}

static struct TAtom *exec_newatom(TEXECBASE *TExecBase, TSTRPTR name)
{
	struct TAtom *atom;
	TSTRPTR s, d;

	s = d = name;
	while (*s++);

	atom = TAlloc0(TNULL, sizeof(struct TAtom) + (s - d));
	if (atom)
	{
		atom->tatm_Handle.thn_Owner = (struct TModule *) TExecBase;
		s = d;
		d = (TSTRPTR) (atom + 1);
		atom->tatm_Handle.thn_Name = d;
		while ((*d++ = *s++));
		TINITLIST(&atom->tatm_Waiters);
		atom->tatm_State = TATOMF_LOCKED;
		atom->tatm_Nest = 1;
		TAddHead(&TExecBase->texb_AtomList, (struct TNode *) atom);
		TDBPRINTF(TDB_TRACE,("atom %s created - nest: 1\n", name));
	}

	return atom;
}

/*****************************************************************************/

static void exec_lockatom(TEXECBASE *TExecBase, struct TTask *msg)
{
	TUINT mode = msg->tsk_Request.trq_Atom.tra_Mode;
	struct TAtom *atom = msg->tsk_Request.trq_Atom.tra_Atom;
	struct TTask *task = msg->tsk_Request.trq_Atom.tra_Task;

	switch (mode & (TATOMF_CREATE|TATOMF_SHARED|TATOMF_NAME|TATOMF_TRY))
	{
		case TATOMF_CREATE | TATOMF_SHARED | TATOMF_NAME:
		case TATOMF_CREATE | TATOMF_NAME:

			atom = exec_lookupatom(TExecBase, (TSTRPTR) atom);
			if (atom)
				goto obtain;
			goto create;

		case TATOMF_CREATE | TATOMF_SHARED | TATOMF_TRY | TATOMF_NAME:
		case TATOMF_CREATE | TATOMF_TRY | TATOMF_NAME:

			atom = exec_lookupatom(TExecBase, (TSTRPTR) atom);
			if (atom)
			{
				atom = TNULL; /* already exists - deny */
				goto reply;
			}

		create:
			atom = exec_newatom(TExecBase, msg->tsk_Request.trq_Atom.tra_Atom);

		reply:
			exec_replyatom(TExecBase, msg, atom);
			return;

		case TATOMF_NAME | TATOMF_SHARED | TATOMF_TRY:
		case TATOMF_NAME | TATOMF_SHARED:
		case TATOMF_NAME | TATOMF_TRY:
		case TATOMF_NAME:

			atom = exec_lookupatom(TExecBase, (TSTRPTR) atom);

		case TATOMF_SHARED | TATOMF_TRY:
		case TATOMF_SHARED:
		case TATOMF_TRY:
		case 0:

			if (atom)
				goto obtain;

		fail:
		default:

			atom = TNULL;
			goto reply;

		obtain:

			if (atom->tatm_State & TATOMF_LOCKED)
			{
				if (atom->tatm_State & TATOMF_SHARED)
				{
					if (mode & TATOMF_SHARED)
					{
		nest:			atom->tatm_Nest++;
						TDBPRINTF(TDB_TRACE,("nest: %d\n", atom->tatm_Nest));
						goto reply;
					}
				}
				else
					if (atom->tatm_Owner == task)
						goto nest;

				if (mode & TATOMF_TRY)
					goto fail;
			}
			else
			{
				if (atom->tatm_Nest)
					TDBPRINTF(TDB_FAIL,
						("atom->nestcount %d!\n", atom->tatm_Nest));

				atom->tatm_State = TATOMF_LOCKED;
				if (mode & TATOMF_SHARED)
				{
					atom->tatm_State |= TATOMF_SHARED;
					atom->tatm_Owner = TNULL;
				}
				else
					atom->tatm_Owner = task;
				atom->tatm_Nest = 1;
				TDBPRINTF(TDB_TRACE,
					("atom taken. nest: %d\n", atom->tatm_Nest));
				goto reply;
			}

			/* put this request into atom's list of waiters */
			msg->tsk_Request.trq_Atom.tra_Mode = mode & TATOMF_SHARED;
			TAddTail(&atom->tatm_Waiters, &msg->tsk_Request.trq_Atom.tra_Node);
			TDBPRINTF(TDB_TRACE,("must wait\n"));
	}
}

/*****************************************************************************/

static void exec_unlockatom(TEXECBASE *TExecBase, struct TTask *msg)
{
	TUINT mode = msg->tsk_Request.trq_Atom.tra_Mode;
	struct TAtom *atom = msg->tsk_Request.trq_Atom.tra_Atom;

	atom->tatm_Nest--;
	TDBPRINTF(TDB_TRACE,("unlock. nest: %d\n", atom->tatm_Nest));

	if (atom->tatm_Nest == 0)
	{
		union TTaskRequest *waiter;

		atom->tatm_State = 0;
		atom->tatm_Owner = TNULL;

		if (mode & TATOMF_DESTROY)
		{
			struct TNode *nextnode, *node = atom->tatm_Waiters.tlh_Head.tln_Succ;
			while ((nextnode = node->tln_Succ))
			{
				waiter = (union TTaskRequest *) node;
				TREMOVE(node);
				exec_replyatom(TExecBase, waiter->trq_Atom.tra_Task, TNULL);
				node = nextnode;
			}

			TDBPRINTF(TDB_TRACE,
				("atom free: %s\n", atom->tatm_Handle.thn_Name));
			TREMOVE((struct TNode *) atom);
			TFree(atom);
		}
		else
		{
			waiter = (union TTaskRequest *) TRemHead(&atom->tatm_Waiters);
			if (waiter)
			{
				TUINT waitmode = waiter->trq_Atom.tra_Mode;

				atom->tatm_Nest++;
				atom->tatm_State = TATOMF_LOCKED;
				TDBPRINTF(TDB_TRACE,
					("restarted waiter. nest: %d\n", atom->tatm_Nest));
				exec_replyatom(TExecBase, waiter->trq_Atom.tra_Task, atom);

				/* just restarted a shared waiter?
					then restart ALL shared waiters */

				if (waitmode & TATOMF_SHARED)
				{
					struct TNode *nextnode, *node =
						atom->tatm_Waiters.tlh_Head.tln_Succ;
					atom->tatm_State |= TATOMF_SHARED;
					while ((nextnode = node->tln_Succ))
					{
						waiter = (union TTaskRequest *) node;
						if (waiter->trq_Atom.tra_Mode & TATOMF_SHARED)
						{
							TREMOVE(node);
							atom->tatm_Nest++;
							TDBPRINTF(TDB_TRACE,
								("restarted waiter. nest: %d\n",
								atom->tatm_Nest));
							exec_replyatom(TExecBase,
								waiter->trq_Atom.tra_Task, atom);
						}
						node = nextnode;
					}
				}
				else
					atom->tatm_Owner = waiter->trq_Atom.tra_Task;
			}
		}
	}
	TAckMsg(msg);
}
