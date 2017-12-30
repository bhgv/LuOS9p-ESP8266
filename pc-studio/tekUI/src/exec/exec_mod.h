#ifndef _TEK_MOD_EXEC_MOD_H
#define _TEK_MOD_EXEC_MOD_H

/*
**	$Id: exec_mod.h,v 1.4 2006/09/10 14:47:57 tmueller Exp $
**	Exec module internal definitions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/mod/exec.h>
#include <tek/mod/hal.h>
#include <tek/mod/time.h>
#include <tek/mod/ioext.h>
#include <tek/proto/hal.h>
#include <tek/inline/exec.h>

/*****************************************************************************/

#define EXEC_VERSION	12
#define EXEC_REVISION	0
#define EXEC_NUMVECTORS	81

/*****************************************************************************/

#ifndef LOCAL
#define LOCAL	TMODINTERN
#endif

#ifndef EXPORT
#define EXPORT	TMODAPI
#endif

/*****************************************************************************/
/*
**	Reserved module calls
*/

#define _TModBeginIO(dev,msg)	\
	(*(((TMODCALL void(**)(TAPTR,TAPTR))(dev))[TMODV_BEGINIO]))(dev,msg)
#define _TModAbortIO(dev,msg)	\
	(*(((TMODCALL TINT(**)(TAPTR,TAPTR))(dev))[TMODV_ABORTIO]))(dev,msg)

/*****************************************************************************/
/*
**	Internal
*/

LOCAL TBOOL exec_initmm(TEXECBASE *exec, struct TMemManager *mmu,
	TAPTR allocator, TUINT mmutype, struct TTagItem *tags);
LOCAL TBOOL exec_initmemhead(union TMemHead *mh, TAPTR mem, TSIZE size,
	TUINT flags, TUINT bytealign);
LOCAL TBOOL exec_initport(TEXECBASE *exec, struct TMsgPort *port,
	struct TTask *task, TUINT prefsignal);
LOCAL TUINT exec_allocsignal(TEXECBASE *exec, struct TTask *task,
	TUINT signals);
LOCAL void exec_freesignal(TEXECBASE *exec, struct TTask *task,
	TUINT signals);
LOCAL TBOOL exec_initlock(TEXECBASE *exec, struct TLock *lock);
LOCAL void exec_returnmsg(TEXECBASE *exec, TAPTR mem, TUINT status);
LOCAL TUINT exec_sendmsg(TEXECBASE *exec, struct TTask *task,
	struct TMsgPort *port, TAPTR mem);

/*****************************************************************************/
/*
**	External
*/

EXPORT TBOOL exec_StrEqual(TEXECBASE *exec, TSTRPTR s1, TSTRPTR s2);
EXPORT TBOOL exec_DoExec(TEXECBASE *exec, TUINT cmd, struct TTagItem *tags);
EXPORT struct TTask *exec_CreateSysTask(TEXECBASE *exec, TTASKFUNC func,
	struct TTagItem *tags);
EXPORT TUINT exec_AllocSignal(TEXECBASE *exec, TUINT signals);
EXPORT void exec_FreeSignal(TEXECBASE *exec, TUINT signal);
EXPORT struct TMsgPort *exec_CreatePort(TEXECBASE *exec,
	struct TTagItem *tags);
EXPORT void exec_CloseModule(TEXECBASE *exec, struct TModule *module);
EXPORT struct TTask *exec_CreateTask(TEXECBASE *exec, struct THook *hook,
	struct TTagItem *tags);
EXPORT struct TAtom *exec_LockAtom(TEXECBASE *exec, TAPTR atom, TUINT mode);
EXPORT struct TMemManager *exec_CreateMemManager(TEXECBASE *exec,
	TAPTR allocator, TUINT mmutype, struct TTagItem *tags);
EXPORT TUINT exec_SendMsg(TEXECBASE *exec, struct TMsgPort *port, TAPTR msg);
EXPORT TUINT exec_SetSignal(TEXECBASE *exec, TUINT newsignals, TUINT sigmask);
EXPORT void exec_Signal(TEXECBASE *exec, struct TTask *task, TUINT signals);
EXPORT TUINT exec_Wait(TEXECBASE *exec, TUINT sigmask);
EXPORT void exec_UnlockAtom(TEXECBASE *exec, struct TAtom *atom, TUINT mode);
EXPORT void exec_Lock(TEXECBASE *exec, struct TLock *lock);
EXPORT void exec_Unlock(TEXECBASE *exec, struct TLock *lock);
EXPORT void exec_AckMsg(TEXECBASE *exec, TAPTR msg);
EXPORT void exec_DropMsg(TEXECBASE *exec, TAPTR msg);
EXPORT void exec_Free(TEXECBASE *exec, TAPTR mem);
EXPORT struct TTask *exec_FindTask(TEXECBASE *exec, TSTRPTR name);
EXPORT TAPTR exec_GetMsg(TEXECBASE *exec, struct TMsgPort *port);
EXPORT TAPTR exec_Alloc(TEXECBASE *exec, struct TMemManager *mmu, TSIZE size);
EXPORT TAPTR exec_Alloc0(TEXECBASE *exec, struct TMemManager *mmu, TSIZE size);
EXPORT TAPTR exec_OpenModule(TEXECBASE *exec, TSTRPTR modname, TUINT16 version,
	struct TTagItem *tags);
EXPORT TAPTR exec_QueryInterface(TEXECBASE *exec, struct TModule *mod,
	TSTRPTR name, TUINT16 version, struct TTagItem *tags);
EXPORT void exec_DropInterface(struct TExecBase *TExecBase,
	struct TInterface *iface);
EXPORT void exec_PutMsg(TEXECBASE *exec, struct TMsgPort *msgport,
	struct TMsgPort *replyport, TAPTR msg);
EXPORT TAPTR exec_Realloc(TEXECBASE *exec, TAPTR mem, TSIZE newsize);
EXPORT void exec_ReplyMsg(TEXECBASE *exec, TAPTR msg);
EXPORT TAPTR exec_WaitPort(TEXECBASE *exec, struct TMsgPort *port);
EXPORT TAPTR exec_GetTaskMemManager(TEXECBASE *exec, struct TTask *task);
EXPORT TAPTR exec_GetUserPort(TEXECBASE *exec, struct TTask *task);
EXPORT TAPTR exec_GetSyncPort(TEXECBASE *exec, struct TTask *task);
EXPORT TAPTR exec_GetTaskData(TEXECBASE *exec, struct TTask *task);
EXPORT TAPTR exec_SetTaskData(TEXECBASE *exec, struct TTask *task, TAPTR data);
EXPORT TAPTR exec_GetHALBase(TEXECBASE *exec);
EXPORT TAPTR exec_AllocMsg(TEXECBASE *exec, TSIZE size);
EXPORT TAPTR exec_AllocMsg0(TEXECBASE *exec, TSIZE size);
EXPORT TAPTR exec_CreateLock(TEXECBASE *exec, struct TTagItem *tags);
EXPORT TUINT exec_GetPortSignal(TEXECBASE *exec, TAPTR port);
EXPORT TTAG exec_GetAtomData(TEXECBASE *exec, struct TAtom *atom);
EXPORT TTAG exec_SetAtomData(TEXECBASE *exec, struct TAtom *atom, TTAG data);
EXPORT TSIZE exec_GetSize(TEXECBASE *exec, TAPTR mem);
EXPORT TAPTR exec_GetMemManager(TEXECBASE *exec, TAPTR mem);
EXPORT TAPTR exec_CreatePool(TEXECBASE *exec, struct TTagItem *tags);
EXPORT TAPTR exec_AllocPool(TEXECBASE *exec, struct TMemPool *pool,
	TSIZE size);
EXPORT void exec_FreePool(TEXECBASE *exec, struct TMemPool *pool, TINT8 *mem,
	TSIZE size);
EXPORT TAPTR exec_ReallocPool(TEXECBASE *exec, struct TMemPool *pool,
	TINT8 *oldmem, TSIZE oldsize, TSIZE newsize);
EXPORT void exec_PutIO(TEXECBASE *exec, struct TIORequest *ioreq);
EXPORT TINT exec_WaitIO(TEXECBASE *exec, struct TIORequest *ioreq);
EXPORT TINT exec_DoIO(TEXECBASE *exec, struct TIORequest *ioreq);
EXPORT TBOOL exec_CheckIO(TEXECBASE *exec, struct TIORequest *ioreq);
EXPORT TINT exec_AbortIO(TEXECBASE *exec, struct TIORequest *ioreq);
EXPORT TBOOL exec_AddModules(TEXECBASE *exec, struct TModInitNode *tmin,
	TUINT flags);
EXPORT TBOOL exec_RemModules(TEXECBASE *exec, struct TModInitNode *tmin,
	TUINT flags);
EXPORT struct TTimeRequest *exec_AllocTimeRequest(TEXECBASE *tmod,
	TTAGITEM *tags);
EXPORT void exec_FreeTimeRequest(TEXECBASE *tmod, struct TTimeRequest *req);
EXPORT void exec_GetSystemTime(TEXECBASE *tmod, TTIME *timep);
EXPORT TINT exec_GetLocalDate(TEXECBASE *tmod, TDATE *dtp);
EXPORT TINT exec_GetUniversalDate(TEXECBASE *tmod, TDATE *dtp);
EXPORT TUINT exec_WaitTime(TEXECBASE *tmod, TTIME *timep, TUINT sig);
EXPORT TUINT exec_WaitDate(TEXECBASE *tmod, TDATE *date, TUINT sig);

EXPORT TAPTR exec_ScanModules(struct TExecBase *TExecBase, TTAGITEM *tags);
EXPORT TAPTR exec_GetInitData(TEXECBASE *exec, struct TTask *task);
EXPORT struct TTask *exec_GetMsgSender(TEXECBASE *exec, TAPTR msg);
EXPORT void exec_FreeTask(TEXECBASE *TExecBase, struct TTask *task);

/*****************************************************************************/
/*
**	External, private functions
*/

EXPORT void exec_InsertMsg(TEXECBASE *exec, struct TMsgPort *port, TAPTR msg,
	TAPTR predmsg, TUINT status);
EXPORT void exec_RemoveMsg(TEXECBASE *exec, struct TMsgPort *port, TAPTR msg);
EXPORT TUINT exec_GetMsgStatus(TEXECBASE *exec, TAPTR mem);
EXPORT struct TMsgPort *exec_SetMsgReplyPort(TEXECBASE *exec, TAPTR mem,
	struct TMsgPort *rport);
EXPORT struct THook *exec_SetPortHook(TEXECBASE *exec, struct TMsgPort *port,
	struct THook *hook);

#endif
