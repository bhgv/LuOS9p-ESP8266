#ifndef _TEK_INLINE_EXEC_H
#define _TEK_INLINE_EXEC_H

/*
**	teklib/tek/inline/exec.h - exec inline calls
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/proto/exec.h>

#define TGetHALBase() \
	(*(((TMODCALL struct THALBase *(**)(TAPTR))(TExecBase))[-11]))(TExecBase)

#define TOpenModule(name,version,tags) \
	(*(((TMODCALL struct TModule *(**)(TAPTR,TSTRPTR,TUINT16,TTAGITEM *))(TExecBase))[-12]))(TExecBase,name,version,tags)

#define TCloseModule(mod) \
	(*(((TMODCALL void(**)(TAPTR,struct TModule *))(TExecBase))[-13]))(TExecBase,mod)

#define TCopyMem(src,dst,len) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR,TAPTR,TSIZE))(TExecBase))[-14]))(TExecBase,src,dst,len)

#define TFillMem(dst,len,val) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR,TSIZE,TUINT8))(TExecBase))[-15]))(TExecBase,dst,len,val)

#define TCreateMemManager(object,type,tags) \
	(*(((TMODCALL struct TMemManager *(**)(TAPTR,TAPTR,TUINT,TTAGITEM *))(TExecBase))[-16]))(TExecBase,object,type,tags)

#define TAlloc(mm,size) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TMemManager *,TSIZE))(TExecBase))[-17]))(TExecBase,mm,size)

#define TAlloc0(mm,size) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TMemManager *,TSIZE))(TExecBase))[-18]))(TExecBase,mm,size)

#define TQueryInterface(mod,name,version,tags) \
	(*(((TMODCALL struct TInterface *(**)(TAPTR,struct TModule *,TSTRPTR,TUINT16,TTAGITEM *))(TExecBase))[-19]))(TExecBase,mod,name,version,tags)

#define TFree(mem) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR))(TExecBase))[-20]))(TExecBase,mem)

#define TRealloc(mem,nsize) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TAPTR,TSIZE))(TExecBase))[-21]))(TExecBase,mem,nsize)

#define TGetMemManager(mem) \
	(*(((TMODCALL struct TMemManager *(**)(TAPTR,TAPTR))(TExecBase))[-22]))(TExecBase,mem)

#define TGetSize(mem) \
	(*(((TMODCALL TSIZE(**)(TAPTR,TAPTR))(TExecBase))[-23]))(TExecBase,mem)

#define TCreateLock(tags) \
	(*(((TMODCALL struct TLock *(**)(TAPTR,TTAGITEM *))(TExecBase))[-24]))(TExecBase,tags)

#define TLock(lock) \
	(*(((TMODCALL void(**)(TAPTR,struct TLock *))(TExecBase))[-25]))(TExecBase,lock)

#define TUnlock(lock) \
	(*(((TMODCALL void(**)(TAPTR,struct TLock *))(TExecBase))[-26]))(TExecBase,lock)

#define TAllocSignal(sig) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT))(TExecBase))[-27]))(TExecBase,sig)

#define TFreeSignal(sig) \
	(*(((TMODCALL void(**)(TAPTR,TUINT))(TExecBase))[-28]))(TExecBase,sig)

#define TSignal(task,sig) \
	(*(((TMODCALL void(**)(TAPTR,struct TTask *,TUINT))(TExecBase))[-29]))(TExecBase,task,sig)

#define TSetSignal(newsig,sigmask) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT,TUINT))(TExecBase))[-30]))(TExecBase,newsig,sigmask)

#define TWait(sig) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT))(TExecBase))[-31]))(TExecBase,sig)

#define TStrEqual(s1,s2) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TSTRPTR))(TExecBase))[-32]))(TExecBase,s1,s2)

#define TCreatePort(tags) \
	(*(((TMODCALL struct TMsgPort *(**)(TAPTR,TTAGITEM *))(TExecBase))[-33]))(TExecBase,tags)

#define TPutMsg(port,replyport,msg) \
	(*(((TMODCALL void(**)(TAPTR,struct TMsgPort *,struct TMsgPort *,TAPTR))(TExecBase))[-34]))(TExecBase,port,replyport,msg)

#define TGetMsg(port) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TMsgPort *))(TExecBase))[-35]))(TExecBase,port)

#define TAckMsg(msg) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR))(TExecBase))[-36]))(TExecBase,msg)

#define TReplyMsg(msg) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR))(TExecBase))[-37]))(TExecBase,msg)

#define TDropMsg(msg) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR))(TExecBase))[-38]))(TExecBase,msg)

#define TSendMsg(port,msg) \
	(*(((TMODCALL TUINT(**)(TAPTR,struct TMsgPort *,TAPTR))(TExecBase))[-39]))(TExecBase,port,msg)

#define TWaitPort(port) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TMsgPort *))(TExecBase))[-40]))(TExecBase,port)

#define TGetPortSignal(port) \
	(*(((TMODCALL TUINT(**)(TAPTR,struct TMsgPort *))(TExecBase))[-41]))(TExecBase,port)

#define TGetUserPort(task) \
	(*(((TMODCALL struct TMsgPort *(**)(TAPTR,struct TTask *))(TExecBase))[-42]))(TExecBase,task)

#define TGetSyncPort(task) \
	(*(((TMODCALL struct TMsgPort *(**)(TAPTR,struct TTask *))(TExecBase))[-43]))(TExecBase,task)

#define TCreateTask(a,tags) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct THook *,TTAGITEM *))(TExecBase))[-44]))(TExecBase,a,tags)

#define TFindTask(name) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TSTRPTR))(TExecBase))[-45]))(TExecBase,name)

#define TGetTaskData(task) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TTask *))(TExecBase))[-46]))(TExecBase,task)

#define TSetTaskData(task,data) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TTask *,TAPTR))(TExecBase))[-47]))(TExecBase,task,data)

#define TGetTaskMemManager(task) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TTask *))(TExecBase))[-48]))(TExecBase,task)

#define TAllocMsg(size) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TSIZE))(TExecBase))[-49]))(TExecBase,size)

#define TAllocMsg0(size) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TSIZE))(TExecBase))[-50]))(TExecBase,size)

#define TLockAtom(atom,mode) \
	(*(((TMODCALL struct TAtom *(**)(TAPTR,TAPTR,TUINT))(TExecBase))[-51]))(TExecBase,atom,mode)

#define TUnlockAtom(atom,mode) \
	(*(((TMODCALL void(**)(TAPTR,struct TAtom *,TUINT))(TExecBase))[-52]))(TExecBase,atom,mode)

#define TGetAtomData(atom) \
	(*(((TMODCALL TTAG(**)(TAPTR,struct TAtom *))(TExecBase))[-53]))(TExecBase,atom)

#define TSetAtomData(atom,data) \
	(*(((TMODCALL TTAG(**)(TAPTR,struct TAtom *,TTAG))(TExecBase))[-54]))(TExecBase,atom,data)

#define TCreatePool(tags) \
	(*(((TMODCALL struct TMemPool *(**)(TAPTR,TTAGITEM *))(TExecBase))[-55]))(TExecBase,tags)

#define TAllocPool(pool,size) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TMemPool *,TSIZE))(TExecBase))[-56]))(TExecBase,pool,size)

#define TFreePool(pool,mem,size) \
	(*(((TMODCALL void(**)(TAPTR,struct TMemPool *,TAPTR,TSIZE))(TExecBase))[-57]))(TExecBase,pool,mem,size)

#define TReallocPool(pool,mem,oize,nsize) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TMemPool *,TAPTR,TSIZE,TSIZE))(TExecBase))[-58]))(TExecBase,pool,mem,oize,nsize)

#define TPutIO(ioreq) \
	(*(((TMODCALL void(**)(TAPTR,struct TIORequest *))(TExecBase))[-59]))(TExecBase,ioreq)

#define TWaitIO(ioreq) \
	(*(((TMODCALL TINT(**)(TAPTR,struct TIORequest *))(TExecBase))[-60]))(TExecBase,ioreq)

#define TDoIO(ioreq) \
	(*(((TMODCALL TINT(**)(TAPTR,struct TIORequest *))(TExecBase))[-61]))(TExecBase,ioreq)

#define TCheckIO(ioreq) \
	(*(((TMODCALL TINT(**)(TAPTR,struct TIORequest *))(TExecBase))[-62]))(TExecBase,ioreq)

#define TAbortIO(ioreq) \
	(*(((TMODCALL TINT(**)(TAPTR,struct TIORequest *))(TExecBase))[-63]))(TExecBase,ioreq)

#define TInsertMsg(port,msg,prevmsg,status) \
	(*(((TMODCALL void(**)(TAPTR,struct TMsgPort *,TAPTR,TAPTR,TUINT))(TExecBase))[-64]))(TExecBase,port,msg,prevmsg,status)

#define TRemoveMsg(port,msg) \
	(*(((TMODCALL void(**)(TAPTR,struct TMsgPort *,TAPTR))(TExecBase))[-65]))(TExecBase,port,msg)

#define TGetMsgStatus(msg) \
	(*(((TMODCALL TUINT(**)(TAPTR,TAPTR))(TExecBase))[-66]))(TExecBase,msg)

#define TSetMsgReplyPort(msg,rport) \
	(*(((TMODCALL TUINT(**)(TAPTR,TAPTR,struct TMsgPort *))(TExecBase))[-67]))(TExecBase,msg,rport)

#define TSetPortHook(port,hook) \
	(*(((TMODCALL struct THook *(**)(TAPTR,struct TMsgPort *,struct THook *))(TExecBase))[-68]))(TExecBase,port,hook)

#define TAddModules(im,flags) \
	(*(((TMODCALL TBOOL(**)(TAPTR,struct TModInitNode *,TUINT))(TExecBase))[-69]))(TExecBase,im,flags)

#define TRemModules(im,flags) \
	(*(((TMODCALL TBOOL(**)(TAPTR,struct TModInitNode *,TUINT))(TExecBase))[-70]))(TExecBase,im,flags)

#define TAllocTimeRequest(tags) \
	(*(((TMODCALL struct TTimeRequest *(**)(TAPTR,TTAGITEM *))(TExecBase))[-71]))(TExecBase,tags)

#define TFreeTimeRequest(req) \
	(*(((TMODCALL void(**)(TAPTR,struct TTimeRequest *))(TExecBase))[-72]))(TExecBase,req)

#define TGetSystemTime(t) \
	(*(((TMODCALL void(**)(TAPTR,TTIME *))(TExecBase))[-73]))(TExecBase,t)

#define TGetUniversalDate(dt) \
	(*(((TMODCALL TINT(**)(TAPTR,TDATE *))(TExecBase))[-74]))(TExecBase,dt)

#define TGetLocalDate(dt) \
	(*(((TMODCALL TINT(**)(TAPTR,TDATE *))(TExecBase))[-75]))(TExecBase,dt)

#define TWaitTime(t,sig) \
	(*(((TMODCALL TUINT(**)(TAPTR,TTIME *,TUINT))(TExecBase))[-76]))(TExecBase,t,sig)

#define TWaitDate(dt,sig) \
	(*(((TMODCALL TUINT(**)(TAPTR,TDATE *,TUINT))(TExecBase))[-77]))(TExecBase,dt,sig)

#define TScanModules(tags) \
	(*(((TMODCALL struct THandle *(**)(TAPTR,TTAGITEM *))(TExecBase))[-78]))(TExecBase,tags)

#define TGetInitData(task) \
	(*(((TMODCALL TAPTR(**)(TAPTR,struct TTask *))(TExecBase))[-79]))(TExecBase,task)

#define TGetMsgSender(msg) \
	(*(((TMODCALL struct TTask *(**)(TAPTR,TAPTR))(TExecBase))[-80]))(TExecBase,msg)

#define TFreeTask(msg) \
	(*(((TMODCALL void(**)(TAPTR,struct TTask *))(TExecBase))[-81]))(TExecBase,task)

#endif /* _TEK_INLINE_EXEC_H */
