#ifndef _TEK_STDCALL_EXEC_H
#define _TEK_STDCALL_EXEC_H

/*
**	teklib/tek/stdcall/exec.h - exec module interface
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

/* -- Functions for bootstrapping Exec, not needed outside init code -- */

#define TExecDoExec(exec,cmd,tags) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TUINT,TTAGITEM *))(exec))[-9]))(exec,cmd,tags)

#define TExecCreateSysTask(exec,func,tags) \
	(*(((TMODCALL struct TTask *(**)(TAPTR,TTASKFUNC,TTAGITEM *))(exec))[-10]))(exec,func,tags)

#define TExecGetHALBase(exec) \
	(*(((TMODCALL struct THALBase *(**)(TAPTR))(exec))[-11]))(exec)

#define TExecOpenModule(exec,name,version,tags) \
	(*(((TMODCALL struct TModule *(**)(TAPTR,TSTRPTR,TUINT16,TTAGITEM *))(exec))[-12]))(exec,name,version,tags)

#define TExecCloseModule(exec,mod) \
	(*(((TMODCALL void(**)(TAPTR,struct TModule *))(exec))[-13]))(exec,mod)

#define TExecCopyMem(exec,src,dst,len) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR,TAPTR,TSIZE))(exec))[-14]))(exec,src,dst,len)

#define TExecFillMem(exec,dst,len,val) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR,TSIZE,TUINT8))(exec))[-15]))(exec,dst,len,val)

#define TExecCreateMemManager(exec,object,type,tags) \
	(*(((TMODCALL struct TMemManager *(**)(TAPTR,TAPTR,TUINT,TTAGITEM *))(exec))[-16]))(exec,object,type,tags)

#define TExecAlloc(exec,mm,size) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TMemManager *,TSIZE))(exec))[-17]))(exec,mm,size)

#define TExecAlloc0(exec,mm,size) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TMemManager *,TSIZE))(exec))[-18]))(exec,mm,size)

#define TExecQueryInterface(exec,mod,name,version,tags) \
	(*(((TMODCALL struct TInterface *(**)(TAPTR,struct TModule *,TSTRPTR,TUINT16,TTAGITEM *))(exec))[-19]))(exec,mod,name,version,tags)

#define TExecFree(exec,mem) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR))(exec))[-20]))(exec,mem)

#define TExecRealloc(exec,mem,nsize) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TAPTR,TSIZE))(exec))[-21]))(exec,mem,nsize)

#define TExecGetMemManager(exec,mem) \
	(*(((TMODCALL struct TMemManager *(**)(TAPTR,TAPTR))(exec))[-22]))(exec,mem)

#define TExecGetSize(exec,mem) \
	(*(((TMODCALL TSIZE(**)(TAPTR,TAPTR))(exec))[-23]))(exec,mem)

#define TExecCreateLock(exec,tags) \
	(*(((TMODCALL struct TLock *(**)(TAPTR,TTAGITEM *))(exec))[-24]))(exec,tags)

#define TExecLock(exec,lock) \
	(*(((TMODCALL void(**)(TAPTR,struct TLock *))(exec))[-25]))(exec,lock)

#define TExecUnlock(exec,lock) \
	(*(((TMODCALL void(**)(TAPTR,struct TLock *))(exec))[-26]))(exec,lock)

#define TExecAllocSignal(exec,sig) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT))(exec))[-27]))(exec,sig)

#define TExecFreeSignal(exec,sig) \
	(*(((TMODCALL void(**)(TAPTR,TUINT))(exec))[-28]))(exec,sig)

#define TExecSignal(exec,task,sig) \
	(*(((TMODCALL void(**)(TAPTR,struct TTask *,TUINT))(exec))[-29]))(exec,task,sig)

#define TExecSetSignal(exec,newsig,sigmask) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT,TUINT))(exec))[-30]))(exec,newsig,sigmask)

#define TExecWait(exec,sig) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT))(exec))[-31]))(exec,sig)

#define TExecStrEqual(exec,s1,s2) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TSTRPTR))(exec))[-32]))(exec,s1,s2)

#define TExecCreatePort(exec,tags) \
	(*(((TMODCALL struct TMsgPort *(**)(TAPTR,TTAGITEM *))(exec))[-33]))(exec,tags)

#define TExecPutMsg(exec,port,replyport,msg) \
	(*(((TMODCALL void(**)(TAPTR,struct TMsgPort *,struct TMsgPort *,TAPTR))(exec))[-34]))(exec,port,replyport,msg)

#define TExecGetMsg(exec,port) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TMsgPort *))(exec))[-35]))(exec,port)

#define TExecAckMsg(exec,msg) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR))(exec))[-36]))(exec,msg)

#define TExecReplyMsg(exec,msg) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR))(exec))[-37]))(exec,msg)

#define TExecDropMsg(exec,msg) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR))(exec))[-38]))(exec,msg)

#define TExecSendMsg(exec,port,msg) \
	(*(((TMODCALL TUINT(**)(TAPTR,struct TMsgPort *,TAPTR))(exec))[-39]))(exec,port,msg)

#define TExecWaitPort(exec,port) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TMsgPort *))(exec))[-40]))(exec,port)

#define TExecGetPortSignal(exec,port) \
	(*(((TMODCALL TUINT(**)(TAPTR,struct TMsgPort *))(exec))[-41]))(exec,port)

#define TExecGetUserPort(exec,task) \
	(*(((TMODCALL struct TMsgPort *(**)(TAPTR,struct TTask *))(exec))[-42]))(exec,task)

#define TExecGetSyncPort(exec,task) \
	(*(((TMODCALL struct TMsgPort *(**)(TAPTR,struct TTask *))(exec))[-43]))(exec,task)

#define TExecCreateTask(exec,a,tags) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct THook *,TTAGITEM *))(exec))[-44]))(exec,a,tags)

#define TExecFindTask(exec,name) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TSTRPTR))(exec))[-45]))(exec,name)

#define TExecGetTaskData(exec,task) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TTask *))(exec))[-46]))(exec,task)

#define TExecSetTaskData(exec,task,data) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TTask *,TAPTR))(exec))[-47]))(exec,task,data)

#define TExecGetTaskMemManager(exec,task) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TTask *))(exec))[-48]))(exec,task)

#define TExecAllocMsg(exec,size) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TSIZE))(exec))[-49]))(exec,size)

#define TExecAllocMsg0(exec,size) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TSIZE))(exec))[-50]))(exec,size)

#define TExecLockAtom(exec,atom,mode) \
	(*(((TMODCALL struct TAtom *(**)(TAPTR,TAPTR,TUINT))(exec))[-51]))(exec,atom,mode)

#define TExecUnlockAtom(exec,atom,mode) \
	(*(((TMODCALL void(**)(TAPTR,struct TAtom *,TUINT))(exec))[-52]))(exec,atom,mode)

#define TExecGetAtomData(exec,atom) \
	(*(((TMODCALL TTAG(**)(TAPTR,struct TAtom *))(exec))[-53]))(exec,atom)

#define TExecSetAtomData(exec,atom,data) \
	(*(((TMODCALL TTAG(**)(TAPTR,struct TAtom *,TTAG))(exec))[-54]))(exec,atom,data)

#define TExecCreatePool(exec,tags) \
	(*(((TMODCALL struct TMemPool *(**)(TAPTR,TTAGITEM *))(exec))[-55]))(exec,tags)

#define TExecAllocPool(exec,pool,size) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TMemPool *,TSIZE))(exec))[-56]))(exec,pool,size)

#define TExecFreePool(exec,pool,mem,size) \
	(*(((TMODCALL void(**)(TAPTR,struct TMemPool *,TAPTR,TSIZE))(exec))[-57]))(exec,pool,mem,size)

#define TExecReallocPool(exec,pool,mem,oize,nsize) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TMemPool *,TAPTR,TSIZE,TSIZE))(exec))[-58]))(exec,pool,mem,oize,nsize)

#define TExecPutIO(exec,ioreq) \
	(*(((TMODCALL void(**)(TAPTR,struct TIORequest *))(exec))[-59]))(exec,ioreq)

#define TExecWaitIO(exec,ioreq) \
	(*(((TMODCALL TINT(**)(TAPTR,struct TIORequest *))(exec))[-60]))(exec,ioreq)

#define TExecDoIO(exec,ioreq) \
	(*(((TMODCALL TINT(**)(TAPTR,struct TIORequest *))(exec))[-61]))(exec,ioreq)

#define TExecCheckIO(exec,ioreq) \
	(*(((TMODCALL TINT(**)(TAPTR,struct TIORequest *))(exec))[-62]))(exec,ioreq)

#define TExecAbortIO(exec,ioreq) \
	(*(((TMODCALL TINT(**)(TAPTR,struct TIORequest *))(exec))[-63]))(exec,ioreq)

/* -- Semi-private; to manipulate msgs in ports, not normally needed -- */

#define TExecInsertMsg(exec,port,msg,prevmsg,status) \
	(*(((TMODCALL void(**)(TAPTR,struct TMsgPort *,TAPTR,TAPTR,TUINT))(exec))[-64]))(exec,port,msg,prevmsg,status)

#define TExecRemoveMsg(exec,port,msg) \
	(*(((TMODCALL void(**)(TAPTR,struct TMsgPort *,TAPTR))(exec))[-65]))(exec,port,msg)

#define TExecGetMsgStatus(exec,msg) \
	(*(((TMODCALL TUINT(**)(TAPTR,TAPTR))(exec))[-66]))(exec,msg)

#define TExecSetMsgReplyPort(exec,msg,rport) \
	(*(((TMODCALL TUINT(**)(TAPTR,TAPTR,struct TMsgPort *))(exec))[-67]))(exec,msg,rport)

#define TExecSetPortHook(exec,port,hook) \
	(*(((TMODCALL struct THook *(**)(TAPTR,struct TMsgPort *,struct THook *))(exec))[-68]))(exec,port,hook)

/* -- on with public functions -- */

#define TExecAddModules(exec,im,flags) \
	(*(((TMODCALL TBOOL(**)(TAPTR,struct TModInitNode *,TUINT))(exec))[-69]))(exec,im,flags)

#define TExecRemModules(exec,im,flags) \
	(*(((TMODCALL TBOOL(**)(TAPTR,struct TModInitNode *,TUINT))(exec))[-70]))(exec,im,flags)

#define TExecAllocTimeRequest(exec,tags) \
	(*(((TMODCALL struct TTimeRequest *(**)(TAPTR,TTAGITEM *))(exec))[-71]))(exec,tags)

#define TExecFreeTimeRequest(exec,req) \
	(*(((TMODCALL void(**)(TAPTR,struct TTimeRequest *))(exec))[-72]))(exec,req)

#define TExecGetSystemTime(exec,t) \
	(*(((TMODCALL void(**)(TAPTR,TTIME *))(exec))[-73]))(exec,t)

#define TExecGetUniversalDate(exec,dt) \
	(*(((TMODCALL TINT(**)(TAPTR,TDATE *))(exec))[-74]))(exec,dt)

#define TExecGetLocalDate(exec,dt) \
	(*(((TMODCALL TINT(**)(TAPTR,TDATE *))(exec))[-75]))(exec,dt)

#define TExecWaitTime(exec,t,sig) \
	(*(((TMODCALL TUINT(**)(TAPTR,TTIME *,TUINT))(exec))[-76]))(exec,t,sig)

#define TExecWaitDate(exec,dt,sig) \
	(*(((TMODCALL TUINT(**)(TAPTR,TDATE *,TUINT))(exec))[-77]))(exec,dt,sig)

#define TExecScanModules(exec,tags) \
	(*(((TMODCALL struct THandle *(**)(TAPTR,TTAGITEM *))(exec))[-78]))(exec,tags)

#define TExecGetInitData(exec,task) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TTask *))(exec))[-79]))(exec,task)

#define TExecGetMsgSender(exec,task) \
	(*(((TMODCALL struct TTask *(**)(TAPTR,TAPTR))(exec))[-80]))(exec,msg)

#define TExecFreeTask(exec,task) \
	(*(((TMODCALL void(**)(TAPTR,struct TTask *))(exec))[-81]))(exec,task)

#endif /* _TEK_STDCALL_EXEC_H */
