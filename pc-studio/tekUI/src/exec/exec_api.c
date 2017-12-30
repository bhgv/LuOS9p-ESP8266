
/*
**	$Id: exec_api.c,v 1.6 2006/11/11 14:19:09 tmueller Exp $
**	teklib/mods/exec/exec_api.c - Exec module API implementation
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include "exec_mod.h"
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/string.h>

/*****************************************************************************/
/*
**	task = exec_createsystask(exec, func, tags)
**	Create TEKlib-internal task
*/

static THOOKENTRY TTAG exec_systaskdestroy(struct THook *hook, TAPTR obj,
	TTAG msg)
{
	if (msg == TMSG_DESTROY)
	{
		struct TTask *task = obj;
		struct TExecBase *TExecBase = TGetExecBase(task);
		struct THALBase *THALBase = TExecBase->texb_HALBase;
		THALDestroyThread(THALBase, &task->tsk_Thread);
		THALLock(THALBase, &TExecBase->texb_Lock);
		TREMOVE((struct TNode *) task);
		THALUnlock(THALBase, &TExecBase->texb_Lock);
		TDESTROY(&task->tsk_SyncPort.tmp_Handle);
		TDESTROY(&task->tsk_UserPort.tmp_Handle);
		TDESTROY(&task->tsk_HeapMemManager.tmm_Handle);
		THALDestroyLock(THALBase, &task->tsk_TaskLock);
		TFree(task);
	}
	return 0;
}

static void exec_initsystask(struct TExecBase *TExecBase, struct TTask *task)
{
	TAPTR hal = TExecBase->texb_HALBase;

	/* special initializations of reserved system tasks */
	if (TStrEqual(task->tsk_Handle.thn_Name, TTASKNAME_EXEC))
	{
		/* insert to execbase */
		TExecBase->texb_ExecTask = task;
		/* the exectask userport will be used for task messages */
		TExecBase->texb_ExecPort = &task->tsk_UserPort;
		/* the exectask syncport will be 'abused' for async mod replies */
		TExecBase->texb_ModReply = &task->tsk_SyncPort;
	}
	else if (TStrEqual(task->tsk_Handle.thn_Name, TTASKNAME_RAMLIB))
	{
		/* insert to execbase */
		TExecBase->texb_IOTask = task;
	}

	/* link to task list */
	THALLock(hal, &TExecBase->texb_Lock);
	TAddTail(&TExecBase->texb_TaskList, (struct TNode *) task);
	THALUnlock(hal, &TExecBase->texb_Lock);

	TDBPRINTF(TDB_TRACE,("added task name: %s\n", task->tsk_Handle.thn_Name));
}

EXPORT struct TTask *exec_CreateSysTask(struct TExecBase *TExecBase,
	TTASKFUNC func, struct TTagItem *tags)
{
	TAPTR hal = TExecBase->texb_HALBase;
	struct TTask *newtask;
	TUINT tnlen = 0;
	TSTRPTR tname;

	tname = (TSTRPTR) TGetTag(tags, TTask_Name, TNULL);
	if (tname)
	{
		TSTRPTR t = tname;
		while (*t++);
		tnlen = t - tname;
	}

	/* tasks are messages */
	newtask = TAlloc(&TExecBase->texb_MsgMemManager, sizeof(struct TTask) + tnlen);
	if (newtask)
	{
		TFillMem(newtask, sizeof(struct TTask), 0);
		if (THALInitLock(hal, &newtask->tsk_TaskLock))
		{
			if (exec_initmm(TExecBase, &newtask->tsk_HeapMemManager, TNULL,
				TMMT_MemManager, TNULL))
			{
				if (exec_initport(TExecBase, &newtask->tsk_UserPort, newtask,
					TTASK_SIG_USER))
				{
					if (exec_initport(TExecBase, &newtask->tsk_SyncPort,
						newtask, TTASK_SIG_SINGLE))
					{
						if (tname)
						{
							TSTRPTR t = (TSTRPTR) (newtask + 1);
							newtask->tsk_Handle.thn_Name = t;
							while ((*t++ = *tname++));
						}

						newtask->tsk_Handle.thn_Hook.thk_Entry =
							exec_systaskdestroy;

						newtask->tsk_Handle.thn_Owner =
							(struct TModule *) TExecBase;
						newtask->tsk_UserData =
							(TAPTR) TGetTag(tags, TTask_UserData, TNULL);
						newtask->tsk_InitData =
							(TAPTR) TGetTag(tags, TTask_InitData, TNULL);
						newtask->tsk_SigFree = ~((TUINT) TTASK_SIG_RESERVED);
						newtask->tsk_SigUsed = TTASK_SIG_RESERVED;
						newtask->tsk_Status = TTASK_RUNNING;

						/* create thread */
						if (THALInitThread(hal, &newtask->tsk_Thread, func,
							newtask))
						{
							exec_initsystask(TExecBase, newtask);
							if (func)
							{
								THALSignal(hal, &newtask->tsk_Thread,
									TTASK_SIG_SINGLE);
							}
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
		TFree(newtask);
	}

	return TNULL;
}

/*****************************************************************************/
/*
**	iseq = exec_StrEqual(exec, s1, s2)
*/

EXPORT TBOOL exec_StrEqual(struct TExecBase *TExecBase, TSTRPTR s1, TSTRPTR s2)
{
	if (s1 && s2)
	{
		TINT a;
		while ((a = *s1++) == *s2++)
		{
			if (a == 0)
				return TTRUE;
		}
	}
	return TFALSE;
}

/*****************************************************************************/
/*
**	sigs = exec_AllocSignal(execbase, signals)
**	Alloc signal(s) from a task
*/

EXPORT TUINT exec_AllocSignal(struct TExecBase *TExecBase, TUINT signals)
{
	struct TTask *task = THALFindSelf(TExecBase->texb_HALBase);
	return exec_allocsignal(TExecBase, task, signals);
}

/*****************************************************************************/
/*
**	exec_FreeSignal(execbase, signals)
**	Return signal(s) to a task
*/

EXPORT void exec_FreeSignal(struct TExecBase *TExecBase, TUINT signals)
{
	struct TTask *task = THALFindSelf(TExecBase->texb_HALBase);
	exec_freesignal(TExecBase, task, signals);
}

/*****************************************************************************/
/*
**	oldsignals = exec_SetSignal(exec, newsignals, sigmask)
**	Set/get task's signal state
*/

EXPORT TUINT exec_SetSignal(struct TExecBase *TExecBase, TUINT newsignals,
	TUINT sigmask)
{
	return THALSetSignal(TExecBase->texb_HALBase, newsignals, sigmask);
}

/*****************************************************************************/
/*
**	exec_Signal(exec, task, signals)
**	Send signals to a task. An event is thrown when not all of the
**	affecting signals are already present in the task's signal mask.
*/

EXPORT void exec_Signal(struct TExecBase *TExecBase, struct TTask *task,
	TUINT signals)
{
	if (task)
		THALSignal(TExecBase->texb_HALBase, &task->tsk_Thread, signals);
}

/*****************************************************************************/
/*
**	signals = exec_Wait(exec, sigmask)
**	Suspend task to wait for a set of signals
*/

EXPORT TUINT exec_Wait(struct TExecBase *TExecBase, TUINT sig)
{
	return sig ? THALWait(TExecBase->texb_HALBase, sig) : 0;
}

/*****************************************************************************/
/*
**	mmu = exec_GetTaskMemManager(exec, task)
**	Get task's heap memory manager. Allocations made from this
**	memory manager are automatically freed on exit of this task
*/

EXPORT TAPTR exec_GetTaskMemManager(struct TExecBase *TExecBase,
	struct TTask *task)
{
	if (task == TNULL)
		task = THALFindSelf(TExecBase->texb_HALBase);
	return &task->tsk_HeapMemManager;
}

/*****************************************************************************/
/*
**	port = exec_GetUserPort(exec, task)
**	Get task's user port
*/

EXPORT TAPTR exec_GetUserPort(struct TExecBase *TExecBase, struct TTask *task)
{
	if (task == TNULL)
		task = THALFindSelf(TExecBase->texb_HALBase);
	return &task->tsk_UserPort;
}

/*****************************************************************************/
/*
**	port = exec_GetSyncPort(exec, task)
**	Get task's sync port - the one being restricted to synchronized
**	message passing. Functions like TDoIO() and TSendMsg() are using it.
*/

EXPORT TAPTR exec_GetSyncPort(struct TExecBase *TExecBase, struct TTask *task)
{
	if (task == TNULL)
		task = THALFindSelf(TExecBase->texb_HALBase);
	return &task->tsk_SyncPort;
}

/*****************************************************************************/
/*
**	userdata = exec_GetTaskData(exec, task)
**	Get a task's userdata pointer
*/

EXPORT TAPTR exec_GetTaskData(struct TExecBase *TExecBase, struct TTask *task)
{
	if (task == TNULL)
		task = THALFindSelf(TExecBase->texb_HALBase);
	return task->tsk_UserData;
}

/*****************************************************************************/
/*
**	initdata = exec_GetInitData(exec, task)
**	Get a task's initdata pointer (internal)
*/

EXPORT TAPTR exec_GetInitData(struct TExecBase *TExecBase, struct TTask *task)
{
	if (task == TNULL)
		task = THALFindSelf(TExecBase->texb_HALBase);
	return task->tsk_InitData;
}

/*****************************************************************************/
/*
**	olddata = exec_SetTaskData(exec, task, data)
**	Set task's userdata pointer
*/

EXPORT TAPTR exec_SetTaskData(struct TExecBase *TExecBase, struct TTask *task,
	TAPTR data)
{
	TAPTR hal = TExecBase->texb_HALBase;
	TAPTR olddata;
	if (task == TNULL)
		task = THALFindSelf(hal);
	THALLock(hal, &task->tsk_TaskLock);
	olddata = task->tsk_UserData;
	task->tsk_UserData = data;
	THALUnlock(hal, &task->tsk_TaskLock);
	return olddata;
}

/*****************************************************************************/
/*
**	halbase = exec_GetHALBase(exec)
**	Get HAL module base pointer. As the HAL module interface is
**	currently a moving target and requires profound understanding of
**	the inner working, this should be considered a private function.
*/

EXPORT TAPTR exec_GetHALBase(struct TExecBase *TExecBase)
{
	return TExecBase->texb_HALBase;
}

/*****************************************************************************/
/*
**	mem = exec_AllocMsg(exec, size)
**	Allocate message memory
*/

EXPORT TAPTR exec_AllocMsg(struct TExecBase *TExecBase, TSIZE size)
{
	return TAlloc(&TExecBase->texb_MsgMemManager, size);
}

/*****************************************************************************/
/*
**	mem = exec_AllocMsg0(exec, task, size)
**	Allocate message memory, blank
*/

EXPORT TAPTR exec_AllocMsg0(struct TExecBase *TExecBase, TSIZE size)
{
	TAPTR mem = TAlloc(&TExecBase->texb_MsgMemManager, size);
	if (mem)
		TFillMem(mem, size, 0);
	return mem;
}

/*****************************************************************************/
/*
**	sig = exec_GetPortSignal(exec, port)
**	Get a message port's underlying signal
*/

EXPORT TUINT exec_GetPortSignal(struct TExecBase *TExecBase, TAPTR port)
{
	if (port)
		return ((struct TMsgPort *) port)->tmp_Signal;
	return 0;
}

/*****************************************************************************/
/*
**	data = exec_GetAtomData(exec, atom)
**	Get an atom's data pointer
*/

EXPORT TTAG exec_GetAtomData(struct TExecBase *TExecBase, struct TAtom *atom)
{
	if (atom)
		return atom->tatm_Data;
	return TNULL;
}

/*****************************************************************************/
/*
**	olddata = exec_SetAtomData(exec, atom, data)
**	Set an atom's data pointer
*/

EXPORT TTAG exec_SetAtomData(struct TExecBase *TExecBase, struct TAtom *atom,
	TTAG data)
{
	TTAG olddata = atom->tatm_Data;
	atom->tatm_Data = data;
	return olddata;
}

/*****************************************************************************/
/*
**	exec_PutIO(exec, ioreq)
**	Put an I/O request to an I/O device or handler,
**	preferring asynchronous operation.
*/

EXPORT void exec_PutIO(struct TExecBase *TExecBase, struct TIORequest *ioreq)
{
	ioreq->io_Flags &= ~TIOF_QUICK;
	TGETMSGPTR(ioreq)->tmsg_Flags = 0;
	_TModBeginIO(ioreq->io_Device, ioreq);
}

/*****************************************************************************/
/*
**	err = exec_WaitIO(exec, ioreq)
**	Wait for an I/O request to complete. If the request was
**	processed synchronously, drop through immediately.
*/

EXPORT TINT exec_WaitIO(struct TExecBase *TExecBase, struct TIORequest *ioreq)
{
	TAPTR hal = TExecBase->texb_HALBase;
	struct TMessage *msg = TGETMSGPTR(ioreq);
	TUINT status = msg->tmsg_Flags;

	if (status & TMSGF_SENT)	/* i.e. not processed quickly */
	{
		struct TMsgPort *replyport = ioreq->io_ReplyPort;

		while (status != (TMSG_STATUS_REPLIED | TMSGF_QUEUED))
		{
			THALWait(hal, replyport->tmp_Signal);
			status = msg->tmsg_Flags;
		}

		THALLock(hal, &replyport->tmp_Lock);
		TREMOVE((struct TNode *) msg);
		THALUnlock(hal, &replyport->tmp_Lock);

		msg->tmsg_Flags = 0;
	}

	return (TINT) ioreq->io_Error;
}

/*****************************************************************************/
/*
**	err = exec_DoIO(exec, ioreq)
**	Send an I/O request and wait for completion. Sets the TIOF_QUICK flag
**	for synchronous operation, if supported by the device.
*/

EXPORT TINT exec_DoIO(struct TExecBase *TExecBase, struct TIORequest *ioreq)
{
	ioreq->io_Flags |= TIOF_QUICK;
	TGETMSGPTR(ioreq)->tmsg_Flags = 0;
	_TModBeginIO(ioreq->io_Device, ioreq);
	TWaitIO(ioreq);
	THALSetSignal(TExecBase->texb_HALBase, 0, TTASK_SIG_SINGLE);
	return (TINT) ioreq->io_Error;
}

/*****************************************************************************/
/*
**	complete = exec_CheckIO(exec, ioreq)
**	Test if an I/O request is ready
*/

EXPORT TBOOL exec_CheckIO(struct TExecBase *TExecBase,
	struct TIORequest *ioreq)
{
	struct TMessage *msg = TGETMSGPTR(ioreq);
	TUINT status = msg->tmsg_Flags;
	return (TBOOL) (!(status & TMSGF_SENT) ||
		(status == (TMSG_STATUS_REPLIED | TMSGF_QUEUED)));
}

/*****************************************************************************/
/*
**	error = exec_AbortIO(exec, ioreq)
**	Attempt to abort an I/O request
*/

EXPORT TINT exec_AbortIO(struct TExecBase *TExecBase, struct TIORequest *ioreq)
{
	return _TModAbortIO(ioreq->io_Device, ioreq);
}

/*****************************************************************************/
/*
**	lock = exec_CreateLock(exec, tags)
**	Create locking object.
*/

static THOOKENTRY TTAG exec_destroyfreelock(struct THook *hook, TAPTR obj,
	TTAG msg)
{
	if (msg == TMSG_DESTROY)
	{
		struct TLock *lock = obj;
		struct TExecBase *TExecBase = (struct TExecBase *) TGetExecBase(lock);
		THALDestroyLock(TExecBase->texb_HALBase, &lock->tlk_HLock);
		TFree(lock);
	}
	return 0;
}

EXPORT TAPTR exec_CreateLock(struct TExecBase *TExecBase, TTAGITEM *tags)
{
	struct TLock *lock;
	lock = TAlloc(TNULL, sizeof(struct TLock));
	if (lock)
	{
		if (exec_initlock(TExecBase, lock))
		{
			lock->tlk_Handle.thn_Hook.thk_Entry = exec_destroyfreelock;
			return (TAPTR) lock;
		}
		TFree(lock);
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	exec_Lock(exec, lock)
*/

EXPORT void exec_Lock(struct TExecBase *TExecBase, struct TLock *lock)
{
	TAPTR hal = TExecBase->texb_HALBase;
	struct TTask *waiter = THALFindSelf(hal);
	struct TLockWait request;
	request.tlr_Task = waiter;
	THALLock(hal, &lock->tlk_HLock);
	if (lock->tlk_Owner == TNULL)
	{
		lock->tlk_Owner = waiter;
		lock->tlk_NestCount++;
		THALUnlock(hal, &lock->tlk_HLock);
	}
	else if (lock->tlk_Owner == waiter)
	{
		lock->tlk_NestCount++;
		THALUnlock(hal, &lock->tlk_HLock);
	}
	else
	{
		TAddTail(&lock->tlk_Waiters, &request.tlr_Node);
		lock->tlk_WaitCount++;
		THALUnlock(hal, &lock->tlk_HLock);
		THALWait(hal, TTASK_SIG_SINGLE);
	}
}

/*****************************************************************************/
/*
**	exec_Unlock(exec, lock)
*/

EXPORT void exec_Unlock(struct TExecBase *TExecBase, struct TLock *lock)
{
	TAPTR hal = TExecBase->texb_HALBase;
	THALLock(hal, &lock->tlk_HLock);
	lock->tlk_NestCount--;
	if (lock->tlk_NestCount == 0)
	{
		if (lock->tlk_WaitCount > 0)
		{
			struct TLockWait *request =
				(struct TLockWait *) TRemHead(&lock->tlk_Waiters);
			lock->tlk_WaitCount--;
			lock->tlk_Owner = request->tlr_Task;
			lock->tlk_NestCount = 1;
			THALSignal(hal, &request->tlr_Task->tsk_Thread, TTASK_SIG_SINGLE);
		}
		else
			lock->tlk_Owner = TNULL;
	}
	THALUnlock(hal, &lock->tlk_HLock);
}

/*****************************************************************************/
/*
**	port = exec_CreatePort(execbase, tags)
**	Create a message port
*/

static THOOKENTRY TTAG exec_destroyuserport(struct THook *hook, TAPTR obj,
	TTAG msg)
{
	if (msg == TMSG_DESTROY)
	{
		struct TMsgPort *port = obj;
		struct TExecBase *TExecBase = (struct TExecBase *) TGetExecBase(port);
		exec_freesignal(TExecBase, port->tmp_SigTask, port->tmp_Signal);
		THALDestroyLock(TExecBase->texb_HALBase, &port->tmp_Lock);
		TFree(port);
	}
	return 0;
}

EXPORT struct TMsgPort *exec_CreatePort(struct TExecBase *TExecBase,
	struct TTagItem *tags)
{
	struct TTask *task = THALFindSelf(TExecBase->texb_HALBase);
	struct TMsgPort *port =
		TAlloc(&task->tsk_HeapMemManager, sizeof(struct TMsgPort));
	if (port)
	{
		if (exec_initport(TExecBase, port, task, 0))
		{
			port->tmp_Hook = 
				(struct THook *) TGetTag(tags, TMsgPort_Hook, TNULL);
			/* overwrite destructor */
			port->tmp_Handle.thn_Hook.thk_Entry = exec_destroyuserport;
			return port;
		}
		TFree(port);
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	msg = exec_WaitPort(exec, port)
**	Wait for a message port to be non-empty
*/

EXPORT TAPTR exec_WaitPort(struct TExecBase *TExecBase, struct TMsgPort *port)
{
	struct TNode *node = TNULL;
	if (port)
	{
		TAPTR hal = TExecBase->texb_HALBase;
		TDBASSERT(99, THALFindSelf(hal) == port->tmp_SigTask);
		for (;;)
		{
			THALLock(hal, &port->tmp_Lock);
			node = port->tmp_MsgList.tlh_Head.tln_Succ;
			if (node->tln_Succ == TNULL)
				node = TNULL;
			THALUnlock(hal, &port->tmp_Lock);
			if (node)
				break;
			THALWait(hal, port->tmp_Signal);
		}
	}
	else
		TDBPRINTF(TDB_WARN,("port=TNULL\n"));
	return (TAPTR) node;
}

/*****************************************************************************/
/*
**	msg = exec_GetMsg(exec, port)
**	Get next pending message from message port
*/

EXPORT TAPTR exec_GetMsg(struct TExecBase *TExecBase, struct TMsgPort *port)
{
	if (port)
	{
		struct TMessage *msg;
		TAPTR hal = TExecBase->texb_HALBase;
		THALLock(hal, &port->tmp_Lock);
		msg = (struct TMessage *) TRemHead(&port->tmp_MsgList);
		THALUnlock(hal, &port->tmp_Lock);
		if (msg)
		{
			if (!(msg->tmsg_Flags & TMSGF_QUEUED))
				TDBPRINTF(TDB_ERROR,("got msg with TMSGF_QUEUED not set\n"));
			msg->tmsg_Flags &= ~TMSGF_QUEUED;
			return (TAPTR)((TINT8 *) (msg + 1) + sizeof(union TMemManagerInfo));
		}
	}
	else
		TDBPRINTF(TDB_INFO,("port=TNULL\n"));
	return TNULL;
}

/*****************************************************************************/
/*
**	exec_PutMsg(exec, port, replyport, msg)
**	Put msg to a message port, with a reply expected at replyport
*/

EXPORT void exec_PutMsg(struct TExecBase *TExecBase, struct TMsgPort *port,
	struct TMsgPort *replyport, TAPTR mem)
{
	if (port && mem)
	{
		TAPTR hal = TExecBase->texb_HALBase;
		struct TMessage *msg = TGETMSGPTR(mem);

		msg->tmsg_RPort = replyport;
		msg->tmsg_Sender = THALFindSelf(hal);

		THALLock(hal, &port->tmp_Lock);
		TAddTail(&port->tmp_MsgList, (struct TNode *) msg);
		msg->tmsg_Flags = TMSGF_SENT | TMSGF_QUEUED;
		if (port->tmp_Hook)
			TCALLHOOKPKT(port->tmp_Hook, port, (TTAG) msg);
		THALUnlock(hal, &port->tmp_Lock);

		THALSignal(hal, &port->tmp_SigTask->tsk_Thread, port->tmp_Signal);
	}
	else
		TDBPRINTF(TDB_WARN,("port/msg=TNULL\n"));
}

/*****************************************************************************/
/*
**	exec_AckMsg(execbase, msg)
**	Acknowledge a message to its sender. Unlike TReplyMsg(),
**	this function indicates that the message body is unmodified.
*/

EXPORT void exec_AckMsg(struct TExecBase *TExecBase, TAPTR mem)
{
	if (mem)
		exec_returnmsg(TExecBase, mem, TMSG_STATUS_ACKD | TMSGF_QUEUED);
	else
		TDBPRINTF(TDB_WARN,("msg=TNULL\n"));
}

/*****************************************************************************/
/*
**	exec_ReplyMsg(exec, msg)
**	Reply a message to its sender. This includes the semantic that the
**	message body has or may have been modified.
*/

EXPORT void exec_ReplyMsg(struct TExecBase *TExecBase, TAPTR mem)
{
	if (mem)
		exec_returnmsg(TExecBase, mem, TMSG_STATUS_REPLIED | TMSGF_QUEUED);
	else
		TDBPRINTF(TDB_WARN,("msg=TNULL\n"));
}

/*****************************************************************************/
/*
**	exec_DropMsg(exec, msg)
**	Drop a message, i.e. make it fail in the sender's context. This is
**	an important semantic for proxied messageports. In local address space,
**	it's particularly useful for synchronized messages sent with TSendMsg().
*/

EXPORT void exec_DropMsg(struct TExecBase *TExecBase, TAPTR mem)
{
	if (mem)
		exec_returnmsg(TExecBase, mem, TMSG_STATUS_FAILED | TMSGF_QUEUED);
	else
		TDBPRINTF(TDB_WARN,("msg=TNULL\n"));
}

/*****************************************************************************/
/*
**	success = exec_SendMsg(exec, port, msg)
**	Send message, two-way, blocking
*/

EXPORT TUINT exec_SendMsg(struct TExecBase *TExecBase, struct TMsgPort *port,
	TAPTR msg)
{
	if (port && msg)
	{
		struct TTask *task = THALFindSelf(TExecBase->texb_HALBase);
		return exec_sendmsg(TExecBase, task, port, msg);
	}
	TDBPRINTF(TDB_WARN,("port/msg=TNULL\n"));
	return TMSG_STATUS_FAILED;
}

/*****************************************************************************/
/*
**	task = exec_FindTask(exec, name)
**	Find a task by name, or the caller's own task, if name is TNULL
*/

EXPORT struct TTask *exec_FindTask(struct TExecBase *TExecBase, TSTRPTR name)
{
	TAPTR hal = TExecBase->texb_HALBase;
	struct TTask *task;
	if (name == TNULL)
		task = THALFindSelf(hal);
	else
	{
		THALLock(hal, &TExecBase->texb_Lock);
		task = (struct TTask *) TFindHandle(&TExecBase->texb_TaskList, name);
		THALUnlock(hal, &TExecBase->texb_Lock);
	}
	return task;
}

/*****************************************************************************/
/*
**	mod = exec_OpenModule(exec, name, version, tags)
**	Open a module and/or an module instance
*/

EXPORT TAPTR exec_OpenModule(struct TExecBase *TExecBase, TSTRPTR modname,
	TUINT16 version, struct TTagItem *tags)
{
	if (modname)
	{
		TTAGITEM extratags[2];
		struct TTask *task = THALFindSelf(TExecBase->texb_HALBase);
		union TTaskRequest *taskreq;

		/* called in module init? this is not allowed */
		if (task == TExecBase->texb_IOTask)
			return TNULL;

		/* Shortcut: provide access to a 'timer.device', which is actually
		** the hal module. A ptr to the Execbase must be supplied as well */

		if (TStrEqual((TSTRPTR) TMODNAME_TIMER, modname))
		{
			modname = (TSTRPTR) TMODNAME_HAL;
			extratags[0].tti_Tag = THalBase_Exec;
			extratags[0].tti_Value = (TTAG) TExecBase;
			extratags[1].tti_Tag = TTAG_MORE;
			extratags[1].tti_Value = (TTAG) tags;
			tags = extratags;
		}

		/* insert module request */
		task->tsk_ReqCode = TTREQ_OPENMOD;
		taskreq = &task->tsk_Request;
		taskreq->trq_Mod.trm_InitMod.tmd_Handle.thn_Name = modname;
		taskreq->trq_Mod.trm_InitMod.tmd_Version = version;
		taskreq->trq_Mod.trm_Module = TNULL;

		/* note that the task itself is the message, not taskreq */
		if (exec_sendmsg(TExecBase, task, TExecBase->texb_ExecPort, task))
		{
			struct TModule *mod = taskreq->trq_Mod.trm_Module;
			struct TModule *inst;

			if (!(mod->tmd_Flags & TMODF_OPENCLOSE))
				return mod; /* no instance, succeed */

			inst = (struct TModule *) TCALLHOOKPKT(&mod->tmd_Handle.thn_Hook,
				tags, TMSG_OPENMODULE);

 			if (inst)
				return inst; /* instance open succeeded */

			/* Failed - send module back.
			** Note that taskreq->mod needs to be re-initialized, because
			** it could have been used and overwritten in the meantime */

			task->tsk_ReqCode = TTREQ_CLOSEMOD;
			taskreq->trq_Mod.trm_Module = mod;
			exec_sendmsg(TExecBase, task, TExecBase->texb_ExecPort, task);
		}
	}
	else
		TDBPRINTF(TDB_WARN,("modname=TNULL\n"));

	return TNULL;
}

/*****************************************************************************/
/*
**	exec_CloseModule(exec, module)
**	Close module
*/

EXPORT void exec_CloseModule(struct TExecBase *TExecBase, struct TModule *inst)
{
	if (inst)
	{
		struct TTask *task = THALFindSelf(TExecBase->texb_HALBase);
		union TTaskRequest *taskreq = &task->tsk_Request;
		struct TModule *mod = inst->tmd_ModSuper;

		if (inst->tmd_Flags & TMODF_OPENCLOSE)
			TCALLHOOKPKT(&inst->tmd_Handle.thn_Hook, inst, TMSG_CLOSEMODULE);

		task->tsk_ReqCode = TTREQ_CLOSEMOD;
		taskreq->trq_Mod.trm_Module = mod;
		exec_sendmsg(TExecBase, task, TExecBase->texb_ExecPort, task);
	}
}

/*****************************************************************************/
/*
**	interface = exec_QueryInterface(exec, mod, name, version, tags)
**	Query a module for an interface
*/

EXPORT TAPTR exec_QueryInterface(struct TExecBase *TExecBase,
	struct TModule *mod, TSTRPTR name, TUINT16 version, struct TTagItem *tags)
{
	if (mod && (mod->tmd_Flags & TMODF_QUERYIFACE))
	{
		struct TInterfaceQuery ifq;
		struct TInterface *iface;
		ifq.tifq_Name = name;
		ifq.tifq_Version = version;
		ifq.tifq_Tags = tags;
		iface = (struct TInterface *) TCALLHOOKPKT(&mod->tmd_Handle.thn_Hook,
			&ifq, TMSG_QUERYIFACE);
		if (iface)
		{
			iface->tif_Handle.thn_Owner = mod;
			return iface;
		}
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	task = exec_CreateTask(exec, parenttask, function, initfunction, tags)
**	Create task
*/

EXPORT struct TTask * exec_CreateTask(struct TExecBase *TExecBase,
	struct THook *hook, struct TTagItem *tags)
{
	struct TTask *newtask = TNULL;
	if (hook)
	{
		struct TTagItem tasktags[3];
		union TTaskRequest *taskreq;
		struct TTask *task = THALFindSelf(TExecBase->texb_HALBase);

		tasktags[0].tti_Tag = TTAG_GOSUB;
		tasktags[0].tti_Value = (TTAG) tags; /* prefer user tags */
		tasktags[1].tti_Tag = TTask_CurrentDir;	/* override, if not present */
		/* by default, inherit currentdir from parent */
		tasktags[1].tti_Value = (TTAG) task->tsk_CurrentDir;
		tasktags[2].tti_Tag = TTAG_DONE;

		task->tsk_ReqCode = TTREQ_CREATETASK;
		taskreq = &task->tsk_Request;

		taskreq->trq_Task.trt_TaskHook = *hook;
		taskreq->trq_Task.trt_Tags = tasktags;

		if (exec_sendmsg(TExecBase, task, TExecBase->texb_ExecPort, task))
			newtask = taskreq->trq_Task.trt_Task;
	}
	else
		TDBPRINTF(TDB_WARN,("func/initfunc missing\n"));

	return newtask;
}

/*****************************************************************************/
/*
**	atom = exec_LockAtom(exec, data, mode)
**	Lock named atom
*/

EXPORT struct TAtom *exec_LockAtom(struct TExecBase *TExecBase, TAPTR atom,
	TUINT mode)
{
	if (atom)
	{
		struct TTask *task = THALFindSelf(TExecBase->texb_HALBase);
		task->tsk_ReqCode = TTREQ_LOCKATOM;
		task->tsk_Request.trq_Atom.tra_Atom = atom;
		task->tsk_Request.trq_Atom.tra_Task = task;
		task->tsk_Request.trq_Atom.tra_Mode = mode;
		if (exec_sendmsg(TExecBase, task, TExecBase->texb_ExecPort, task))
			atom = task->tsk_Request.trq_Atom.tra_Atom;
		if (atom && (mode & TATOMF_DESTROY))
			TUnlockAtom(atom, TATOMF_DESTROY);
	}
	return atom;
}

/*****************************************************************************/
/*
**	exec_UnlockAtom(exec, atom, mode)
**	Unlock an atom
*/

EXPORT void exec_UnlockAtom(struct TExecBase *TExecBase, struct TAtom *atom,
	TUINT mode)
{
	if (atom)
	{
		struct TTask *task = THALFindSelf(TExecBase->texb_HALBase);
		task->tsk_ReqCode = TTREQ_UNLOCKATOM;
		task->tsk_Request.trq_Atom.tra_Atom = atom;
		task->tsk_Request.trq_Atom.tra_Task = task;
		task->tsk_Request.trq_Atom.tra_Mode = mode | TATOMF_UNLOCK;
		exec_sendmsg(TExecBase, task, TExecBase->texb_ExecPort, task);
	}
}

/*****************************************************************************/
/*
**	exec_InsertMsg(exec, port, msg, predmsg, status)
**	Insert msg after predmsg in a msgport's queue and
**	set its status
*/

EXPORT void exec_InsertMsg(struct TExecBase *TExecBase, struct TMsgPort *port,
	TAPTR mem, TAPTR predmem, TUINT status)
{
	struct TMessage *msg = TGETMSGPTR(mem);
	struct TMessage *predmsg = predmem ? TGETMSGPTR(predmem) : TNULL;

	THALLock(TExecBase->texb_HALBase, &port->tmp_Lock);
	if (predmsg)
		TInsert(&port->tmp_MsgList, &msg->tmsg_Node, &predmsg->tmsg_Node);
	else
		TAddTail(&port->tmp_MsgList, &msg->tmsg_Node);

	THALUnlock(TExecBase->texb_HALBase, &port->tmp_Lock);

	msg->tmsg_Flags = status | TMSGF_QUEUED;
}

/*****************************************************************************/
/*
**	exec_RemoveMsg(exec, port, msg)
**	Remove msg from message port
*/

EXPORT void exec_RemoveMsg(struct TExecBase *TExecBase, struct TMsgPort *port,
	TAPTR mem)
{
	struct TMessage *msg = TGETMSGPTR(mem);
	THALLock(TExecBase->texb_HALBase, &port->tmp_Lock);
	#ifdef TDEBUG
	{
		struct TNode *next, *node = port->tmp_MsgList.tlh_Head.tln_Succ;

		for (; (next = node->tln_Succ); node = next)
		{
			if ((struct TMessage *) node == msg)
				break;
		}

		if ((struct TMessage *) node != msg)
		{
			TDBPRINTF(TDB_FAIL,("Message was not found in port\n"));
			TDBFATAL();
		}
	}
	#endif
	TREMOVE(&msg->tmsg_Node);
	THALUnlock(TExecBase->texb_HALBase, &port->tmp_Lock);
}

/*****************************************************************************/
/*
**	status = exec_GetMsgStatus(exec, msg)
**	Get message status
*/

EXPORT TUINT exec_GetMsgStatus(struct TExecBase *TExecBase, TAPTR mem)
{
	return TGETMSGPTR(mem)->tmsg_Flags;
}

/*****************************************************************************/
/*
**	oldrport = exec_SetMsgReplyPort(exec, msg, rport)
**	Set a message's replyport
*/

EXPORT struct TMsgPort *exec_SetMsgReplyPort(struct TExecBase *TExecBase,
	TAPTR mem, struct TMsgPort *rport)
{
	struct TMessage *msg = TGETMSGPTR(mem);
	struct TMsgPort *oport = msg->tmsg_RPort;
	msg->tmsg_RPort = rport;
	return oport;
}

/*****************************************************************************/
/*
**	oldfunc = exec_SetPortHook(exec, port, func)
**	Set MsgPort hook
*/

EXPORT struct THook *exec_SetPortHook(struct TExecBase *TExecBase,
	struct TMsgPort *port, struct THook *hook)
{
	struct THook *oldhook;
	THALLock(TExecBase->texb_HALBase, &port->tmp_Lock);
	oldhook = port->tmp_Hook;
	port->tmp_Hook = hook;
	THALUnlock(TExecBase->texb_HALBase, &port->tmp_Lock);
	return oldhook;
}

/*****************************************************************************/
/*
**	success = exec_AddModules(exec, modinitnode, flags)
**	Add a module init node to the list of internal modules.
**
**	success = exec_RemModules(exec, modinitnode, flags)
**	Remove a module init node from the list of internal modules
*/

static TBOOL exec_addremmodule(struct TExecBase *TExecBase,
	struct TModInitNode *tmin, TUINT reqcode, TUINT flags)
{
	struct TTask *task;
	union TTaskRequest *taskreq;

	if (tmin == TNULL || tmin->tmin_Modules == TNULL)
		return TFALSE;

	task = THALFindSelf(TExecBase->texb_HALBase);
	task->tsk_ReqCode = reqcode;

	taskreq = &task->tsk_Request;
	taskreq->trq_AddRemMod.trm_ModInitNode = tmin;
	taskreq->trq_AddRemMod.trm_Flags = flags;

	exec_sendmsg(TExecBase, task, &TExecBase->texb_IOTask->tsk_UserPort, task);
	return TTRUE;
}

EXPORT TBOOL exec_AddModules(struct TExecBase *TExecBase,
	struct TModInitNode *tmin, TUINT flags)
{
	return exec_addremmodule(TExecBase, tmin, TTREQ_ADDMOD, flags);
}

EXPORT TBOOL exec_RemModules(struct TExecBase *TExecBase,
	struct TModInitNode *tmin, TUINT flags)
{
	return exec_addremmodule(TExecBase, tmin, TTREQ_REMMOD, flags);
}

/*****************************************************************************/
/*
**	handle = exec_ScanModules(exec, tags)
**	Scan available modules
*/

struct ScanModHandle
{
	struct THandle handle;
	struct TList list;
	struct TNode **nptr;
};

struct ScanModNode
{
	struct TNode node;
	TTAGITEM tags[2];
};

static THOOKENTRY TTAG scm_hookfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
	switch (msg)
	{
		case TMSG_DESTROY:
		{
			struct ScanModHandle *hnd = obj;
			struct TExecBase *TExecBase = hnd->handle.thn_Owner;
			struct TNode *node, *next;
			node = hnd->list.tlh_Head.tln_Succ;
			for (; (next = node->tln_Succ); node = next)
			{
				struct ScanModNode *smn = (struct ScanModNode *) node;
				TRemove(&smn->node);
				TFree(smn);
			}
			TFree(hnd);
			break;
		}
		case TMSG_GETNEXTENTRY:
		{
			struct ScanModHandle *hnd = obj;
			struct TNode *next = *hnd->nptr;
			if (next->tln_Succ == TNULL)
			{
				hnd->nptr = &hnd->list.tlh_Head.tln_Succ;
				return TNULL;
			}
			hnd->nptr = (struct TNode **) next;
			return (TTAG) ((struct ScanModNode *) next)->tags;
		}
	}
	return 0;
}

static THOOKENTRY TTAG scm_addext(struct THook *hook, TAPTR obj, TTAG m)
{
	struct THALScanModMsg *msg = (struct THALScanModMsg *) m;
	struct ScanModHandle *hnd = hook->thk_Data;
	struct TExecBase *TExecBase = hnd->handle.thn_Owner;
	TINT len = msg->tsmm_Length;
	struct ScanModNode *scn =
		TAlloc(TNULL, sizeof(struct ScanModNode) + len + 1);
	if (scn)
	{
		TCopyMem(msg->tsmm_Name, scn + 1, len);
		((TUINT8 *) (scn + 1))[len] = 0;
		scn->tags[0].tti_Tag = TExec_ModuleName;
		scn->tags[0].tti_Value = (TTAG) (scn + 1);
		scn->tags[1].tti_Tag = TTAG_DONE;
		TAddTail(&hnd->list, &scn->node);
		return TTRUE;
	}
	return TFALSE;
}

EXPORT TAPTR exec_ScanModules(struct TExecBase *TExecBase, TTAGITEM *tags)
{
	struct ScanModHandle *hnd = TAlloc0(TNULL, sizeof(struct ScanModHandle));
	if (hnd)
	{
		struct TAtom *iatom;
		TSTRPTR prefix = (TSTRPTR) TGetTag(tags, TExec_ModulePrefix, TNULL);
		TSIZE plen = TStrLen(prefix);
		hnd->handle.thn_Owner = TExecBase;
		TInitHook(&hnd->handle.thn_Hook, scm_hookfunc, hnd);
		TInitList(&hnd->list);
		hnd->nptr = &hnd->list.tlh_Head.tln_Succ;
		/* scan internal modules: */
		iatom = TLockAtom("sys.imods", TATOMF_NAME | TATOMF_SHARED);
		if (iatom)
		{
			struct TInitModule *imods = (struct TInitModule *) TGetAtomData(iatom);
			if (imods)
			{
				TSTRPTR p;
				while ((p = imods->tinm_Name))
				{
					if (!prefix || !TStrNCmp(prefix, p, plen))
					{
						struct ScanModNode *scn;
						TSIZE len = TStrLen(p) + 1;
						scn = TAlloc(TNULL, sizeof(struct ScanModNode) + len);
						if (!scn)
						{
							TDestroy(&hnd->handle);
							hnd = TNULL;
							break;
						}
						TCopyMem(p, scn + 1, len);
						scn->tags[0].tti_Tag = TExec_ModuleName;
						scn->tags[0].tti_Value = (TTAG) (scn + 1);
						scn->tags[1].tti_Tag = TTAG_DONE;
						TAddTail(&hnd->list, &scn->node);
					}
					imods++;
				}
			}
			TUnlockAtom(iatom, 0);
		}

		/* scan external modules: */
		if (hnd)
		{
			struct THook scanhook;
			scanhook.thk_Entry = scm_addext;
			scanhook.thk_Data = hnd;
			if (!THALScanModules(TExecBase->texb_HALBase, prefix, &scanhook))
			{
				TDestroy(&hnd->handle);
				hnd = TNULL;
			}
		}
	}
	return hnd;
}

EXPORT struct TTask *exec_GetMsgSender(TEXECBASE *exec, TAPTR m)
{
	struct TMessage *msg = TGETMSGPTR(m);
	return msg->tmsg_Sender;
}
