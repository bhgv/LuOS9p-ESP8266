#ifndef _TEK_STDCALL_HAL_H
#define _TEK_STDCALL_HAL_H

/*
**	$Id: hal.h $
**	teklib/tek/stdcall/hal.h - hal module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define THALGetAttr(hal,tag,defval) \
	(*(((TMODCALL TTAG(**)(TAPTR,TUINT,TTAG))(hal))[-9]))(hal,tag,defval)

#define THALAlloc(hal,size) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TUINT))(hal))[-10]))(hal,size)

#define THALFree(hal,mem,size) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR,TUINT))(hal))[-11]))(hal,mem,size)

#define THALRealloc(hal,mem,oldsize,newsize) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TAPTR,TUINT,TUINT))(hal))[-12]))(hal,mem,oldsize,newsize)

#define THALCopyMem(hal,src,dst,size) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR,TAPTR,TUINT))(hal))[-13]))(hal,src,dst,size)

#define THALFillMem(hal,dst,len,val) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR,TUINT,TUINT8))(hal))[-14]))(hal,dst,len,val)

#define THALInitLock(hal,lock) \
	(*(((TMODCALL TBOOL(**)(TAPTR,struct THALObject *))(hal))[-15]))(hal,lock)

#define THALDestroyLock(hal,lock) \
	(*(((TMODCALL void(**)(TAPTR,struct THALObject *))(hal))[-16]))(hal,lock)

#define THALLock(hal,lock) \
	(*(((TMODCALL void(**)(TAPTR,struct THALObject *))(hal))[-17]))(hal,lock)

#define THALUnlock(hal,lock) \
	(*(((TMODCALL void(**)(TAPTR,struct THALObject *))(hal))[-18]))(hal,lock)

#define THALInitThread(hal,thread,func,data) \
	(*(((TMODCALL TBOOL(**)(TAPTR,struct THALObject *,TTASKFUNC,TAPTR))(hal))[-19]))(hal,thread,func,data)

#define THALDestroyThread(hal,thread) \
	(*(((TMODCALL void(**)(TAPTR,struct THALObject *))(hal))[-20]))(hal,thread)

#define THALFindSelf(hal) \
	(*(((TMODCALL TAPTR(**)(TAPTR))(hal))[-21]))(hal)

#define THALWait(hal,signals) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT))(hal))[-22]))(hal,signals)

#define THALSignal(hal,thread,signals) \
	(*(((TMODCALL void(**)(TAPTR,struct THALObject *,TUINT))(hal))[-23]))(hal,thread,signals)

#define THALSetSignal(hal,newsigs,sigs) \
	(*(((TMODCALL TUINT(**)(TAPTR,TUINT,TUINT))(hal))[-24]))(hal,newsigs,sigs)

#define THALLoadModule(hal,name,version,possize,negsize) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TSTRPTR,TUINT16,TUINT *,TUINT *))(hal))[-25]))(hal,name,version,possize,negsize)

#define THALCallModule(hal,mod,task,data) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TAPTR,struct TTask *,TAPTR))(hal))[-26]))(hal,mod,task,data)

#define THALUnloadModule(hal,mod) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR))(hal))[-27]))(hal,mod)

#define THALScanModules(hal,prefix,hook) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,struct THook *))(hal))[-28]))(hal,prefix,hook)

#endif /* _TEK_STDCALL_HAL_H */
