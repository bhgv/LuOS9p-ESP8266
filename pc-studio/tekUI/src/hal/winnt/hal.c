
/*
**	$Id: hal.c,v 1.1 2006/08/25 21:23:42 tmueller Exp $
**	teklib/mods/hal/win32/hal.c - Windows implementation of the HAL layer
**
**	Written by Timm S. Mueller <tmueller@schulze-mueller.de>
**	contributions by Tobias Schwinger <tschwinger@isonews2.com>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <math.h>
#include <tek/debug.h>
#include <tek/mod/exec.h>
#include <tek/proto/exec.h>
#include <tek/mod/winnt/hal.h>

#include "hal_mod.h"

#define TEKHOST_EXTSTR		".dll"
#define TEKHOST_EXTLEN		4

#define USE_PERFCOUNTER

#if defined(ENABLE_LAZY_SINGLETON)
#warning using globals and MMTimer (-DENABLE_LAZY_SINGLETON)
#define USE_MMTIMER
#endif

/* 
** Some versions of MinGW appear to have an incomplete Interlocked API
** so just use GCC-style intrinsics directly which are also supported
** by Clang.
**
** Note that __MINGW32__ is defined for both 32 and 64 Bit Windows.
** We do not change existing macros in case the runtime uses them to
** implement the MS API.
*/
#if defined HAL_USE_ATOMICS && defined __MINGW32__ && !defined InterlockedAnd
#define InterlockedAnd __sync_fetch_and_and
#define InterlockedOr __sync_fetch_and_or
#endif

static void hal_getsystime(struct THALBase *hal, TTIME *time);

/*****************************************************************************/
/*
**	Host Init
*/

LOCAL TBOOL
hal_init(struct THALBase *hal, TTAGITEM *tags)
{
	struct HALSpecific *specific;
	specific = malloc(sizeof(struct HALSpecific));
	if (specific)
	{
		memset(specific, 0, sizeof(struct HALSpecific));
		specific->hsp_TLSIndex = TlsAlloc();
		if (specific->hsp_TLSIndex != 0xffffffff)
		{
			InitializeCriticalSection(&specific->hsp_DevLock);

			specific->hsp_SysDir =
				(TSTRPTR) TGetTag(tags, TExecBase_SysDir, TNULL);
			specific->hsp_ModDir =
				(TSTRPTR) TGetTag(tags, TExecBase_ModDir, TNULL);
			specific->hsp_ProgDir =
				(TSTRPTR) TGetTag(tags, TExecBase_ProgDir, TNULL);

			specific->hsp_Tags[0].tti_Tag = TExecBase_SysDir;
			specific->hsp_Tags[0].tti_Value = (TTAG) specific->hsp_SysDir;
			specific->hsp_Tags[1].tti_Tag = TExecBase_ModDir;
			specific->hsp_Tags[1].tti_Value = (TTAG) specific->hsp_ModDir;
			specific->hsp_Tags[2].tti_Tag = TExecBase_ProgDir;
			specific->hsp_Tags[2].tti_Value = (TTAG) specific->hsp_ProgDir;
			specific->hsp_Tags[3].tti_Tag = TTAG_DONE;

			/* get performance counter frequency, if available */

			#ifdef USE_PERFCOUNTER
			specific->hsp_UsePerfCounter =
				QueryPerformanceFrequency((LARGE_INTEGER *)
					&specific->hsp_PerfCFreq);
			if (specific->hsp_UsePerfCounter)
			{
				union { LARGE_INTEGER li; FILETIME ft; } filet;
				LONGLONG t;

				/* get performance counter start */
				QueryPerformanceCounter((LARGE_INTEGER *)
					&specific->hsp_PerfCStart);

				/* calibrate perfcounter to 1.1.1970 */
				GetSystemTimeAsFileTime(&filet.ft);
				t = filet.li.QuadPart / 10;	/* microseconds */
				t -= 11644473600000000ULL;	/* 1.1.1601 -> 1.1.1970 */
				specific->hsp_PerfCStartDate = t;
			}
			#endif

			hal->hmb_Specific = specific;

			return TTRUE;
		}
		free(specific);
	}
	return TFALSE;
}

LOCAL void
hal_exit(struct THALBase *hal)
{
	struct HALSpecific *specific = hal->hmb_Specific;
	DeleteCriticalSection(&specific->hsp_DevLock);
	TlsFree(specific->hsp_TLSIndex);
	free(specific);
}

LOCAL TAPTR
hal_allocself(TAPTR boot, TUINT size)
{
	return malloc(size);
}

LOCAL void
hal_freeself(TAPTR boot, TAPTR mem, TUINT size)
{
	free(mem);
}

/*****************************************************************************/
/*
**	Memory
*/

EXPORT TAPTR
hal_alloc(struct THALBase *hal, TSIZE size)
{
	return malloc(size);
}

EXPORT void
hal_free(struct THALBase *hal, TAPTR mem, TSIZE size)
{
	free(mem);
}

EXPORT TAPTR
hal_realloc(struct THALBase *hal, TAPTR oldmem, TSIZE oldsize, TSIZE newsize)
{
	return realloc(oldmem, newsize);
}

EXPORT void
hal_copymem(struct THALBase *hal, TAPTR from, TAPTR to, TSIZE numbytes)
{
	CopyMemory(to, from, numbytes);
}

EXPORT void
hal_fillmem(struct THALBase *hal, TAPTR dest, TSIZE numbytes, TUINT8 fillval)
{
	FillMemory(dest, numbytes, fillval);
}

/*****************************************************************************/
/*
**	Locking
*/

EXPORT TBOOL
hal_initlock(struct THALBase *hal, struct THALObject *lock)
{
	CRITICAL_SECTION *cs = THALNewObject(hal, lock, CRITICAL_SECTION);
	if (cs)
	{
		InitializeCriticalSection(cs);
		THALSetObject(lock, CRITICAL_SECTION, cs);
		return TTRUE;
	}
	TDBPRINTF(20,("could not create lock\n"));
	return TFALSE;
}

EXPORT void
hal_destroylock(struct THALBase *hal, struct THALObject *lock)
{
	CRITICAL_SECTION *cs = THALGetObject(lock, CRITICAL_SECTION);
	DeleteCriticalSection(cs);
	THALDestroyObject(hal, cs, CRITICAL_SECTION);
}

EXPORT void
hal_lock(struct THALBase *hal, struct THALObject *lock)
{
	CRITICAL_SECTION *cs;
	cs = THALGetObject(lock, CRITICAL_SECTION);
	EnterCriticalSection(cs);
}

EXPORT void
hal_unlock(struct THALBase *hal, struct THALObject *lock)
{
	CRITICAL_SECTION *cs = THALGetObject(lock, CRITICAL_SECTION);
	LeaveCriticalSection(cs);
}

/*****************************************************************************/
/*
**	Threads
*/

static unsigned _stdcall
hal_win32thread_entry(void *t)
{
	struct HALThread *wth = (struct HALThread *) t;
	struct THALBase *hal = wth->hth_HALBase;
	struct HALSpecific *hws = hal->hmb_Specific;

	TlsSetValue(hws->hsp_TLSIndex, t);

	hal_wait(hal, TTASK_SIG_SINGLE);

	(*wth->hth_Function)(wth->hth_Data);

	_endthreadex(0);
	return 0;
}

EXPORT TBOOL
hal_initthread(struct THALBase *hal, struct THALObject *thread,
	TTASKENTRY void (*function)(struct TTask *task), TAPTR data)
{
	struct HALThread *wth = THALNewObject(hal, thread, struct HALThread);
	if (wth)
	{
		wth->hth_SigEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (wth->hth_SigEvent)
		{
#ifndef HAL_USE_ATOMICS
			InitializeCriticalSection(&wth->hth_SigLock);
#endif
			wth->hth_SigState = 0;
			wth->hth_HALBase = hal;
			wth->hth_Data = data;
			wth->hth_Function = function;
			THALSetObject(thread, struct HALThread, wth);
			if (function)
			{
				wth->hth_Thread = (TAPTR) _beginthreadex(NULL, 0,
					hal_win32thread_entry, wth, 0, (HANDLE) &wth->hth_ThreadID);
				if (wth->hth_Thread) return TTRUE;
			}
			else
			{
				struct HALSpecific *hws = hal->hmb_Specific;
				TlsSetValue(hws->hsp_TLSIndex, wth);
				return TTRUE;
			}
#ifndef HAL_USE_ATOMICS
			DeleteCriticalSection(&wth->hth_SigLock);
#endif
			CloseHandle(wth->hth_SigEvent);
		}
		THALDestroyObject(hal, wth, struct HALThread);
	}
	TDBPRINTF(20,("could not create thread\n"));
	return TFALSE;
}

EXPORT void
hal_destroythread(struct THALBase *hal, struct THALObject *thread)
{
	struct HALThread *wth = THALGetObject(thread, struct HALThread);
	if (wth->hth_Function)
	{
		WaitForSingleObject(wth->hth_Thread, INFINITE);
		CloseHandle(wth->hth_Thread);
		CloseHandle(wth->hth_SigEvent);
#ifndef HAL_USE_ATOMICS
		DeleteCriticalSection(&wth->hth_SigLock);
#endif
	}
	THALDestroyObject(hal, wth, struct HALThread);
}

EXPORT TAPTR
hal_findself(struct THALBase *hal)
{
	struct HALSpecific *hws = hal->hmb_Specific;
	struct HALThread *wth = TlsGetValue(hws->hsp_TLSIndex);
	return wth->hth_Data;
}

/*****************************************************************************/
/*
**	Signals
*/

EXPORT void
hal_signal(struct THALBase *hal, struct THALObject *thread, TUINT signals)
{
	struct HALThread *wth = THALGetObject(thread, struct HALThread);

#ifndef HAL_USE_ATOMICS
	EnterCriticalSection(&wth->hth_SigLock);
	if (signals & ~wth->hth_SigState)
	{
		wth->hth_SigState |= signals;
		SetEvent(wth->hth_SigEvent);
	}
	LeaveCriticalSection(&wth->hth_SigLock);
#else
	if (signals & ~(TUINT) InterlockedOr(&wth->hth_SigState, signals))
		SetEvent(wth->hth_SigEvent);
#endif

}

EXPORT TUINT
hal_setsignal(struct THALBase *hal, TUINT newsig, TUINT sigmask)
{
	struct HALSpecific *hws = hal->hmb_Specific;
	struct HALThread *wth = TlsGetValue(hws->hsp_TLSIndex);
#ifndef HAL_USE_ATOMICS 
	TUINT oldsig;
	EnterCriticalSection(&wth->hth_SigLock);
	oldsig = wth->hth_SigState;
	wth->hth_SigState &= ~sigmask;
	wth->hth_SigState |= newsig;
	LeaveCriticalSection(&wth->hth_SigLock);
	return oldsig;
#else
	TUINT cmask = ~sigmask | newsig;
	TUINT before_consume = InterlockedAnd(&wth->hth_SigState, cmask);
	if (! newsig)
		return before_consume;
	TUINT before_publish = InterlockedOr(&wth->hth_SigState, newsig);
	return (before_consume & ~cmask) | (before_publish & cmask);
#endif
}

EXPORT TUINT
hal_wait(struct THALBase *hal, TUINT sigmask)
{
	struct HALSpecific *hws = hal->hmb_Specific;
	struct HALThread *wth = TlsGetValue(hws->hsp_TLSIndex);
	TUINT sig;
	for (;;)
	{
#ifndef HAL_USE_ATOMICS
		EnterCriticalSection(&wth->hth_SigLock);
		sig = wth->hth_SigState & sigmask;
		wth->hth_SigState &= ~sigmask;
		LeaveCriticalSection(&wth->hth_SigLock);
#else
		sig = InterlockedAnd(&wth->hth_SigState, ~sigmask) & sigmask;
#endif
		if (sig) break;
		WaitForSingleObject(wth->hth_SigEvent, INFINITE);
	}
	return sig;
}

#ifndef USE_MMTIMER

static TUINT
hal_timedwaitevent(struct THALBase *hal, struct HALThread *t,
	TTIME *tektime, TUINT sigmask)
{
	struct HALSpecific *hws = hal->hmb_Specific;
	struct HALThread *wth = TlsGetValue(hws->hsp_TLSIndex);

	TTIME waitt, curt;
	TUINT millis;
	TUINT sig;

	for (;;)
	{
#ifndef HAL_USE_ATOMICS
		EnterCriticalSection(&wth->hth_SigLock);
		sig = wth->hth_SigState & sigmask;
		wth->hth_SigState &= ~sigmask;
		LeaveCriticalSection(&wth->hth_SigLock);
#else
		sig = InterlockedAnd(&wth->hth_SigState, ~sigmask) & sigmask;
#endif
		if (sig)
			break;

		waitt = *tektime;
		hal_getsystime(hal, &curt);
		TSubTime(&waitt, &curt);
		if (waitt.tdt_Int64 < 0)
			break;

		if (waitt.tdt_Int64 > 1000000000000LL)
			millis = 1000000000;
		else
			millis = waitt.tdt_Int64 / 1000;

		if (millis > 0)
			WaitForSingleObject(wth->hth_SigEvent, millis);
	}

	return sig;
}

#endif

/*****************************************************************************/
/*
**	Time and date
*/

static void
hal_getsystime(struct THALBase *hal, TTIME *time)
{
	struct HALSpecific *hws = hal->hmb_Specific;
	if (hws->hsp_UsePerfCounter)
	{
		LONGLONG perft;
		QueryPerformanceCounter((LARGE_INTEGER *) &perft);
		perft -= hws->hsp_PerfCStart;
		perft *= 1000000;
		perft /= hws->hsp_PerfCFreq;
		/* make absolute */
		perft += hws->hsp_PerfCStartDate;
		time->tdt_Int64 = perft;
	}
	else
	{
		union { LARGE_INTEGER li; FILETIME ft; } filet;
		LONGLONG t;

		/* 1.1.1970 is as useless as 1.1.1601, but it fits better into a TTIME
		** structure. TTIME is used for relative time measurement only - it
		** doesn't have to be consistent across all systems */

		GetSystemTimeAsFileTime(&filet.ft);
		t = filet.li.QuadPart / 10; /* microseconds */
		t -= 11644473600000000ULL; /* 1.1.1601 -> 1.1.1970 */
		time->tdt_Int64 = t;
	}
}

/*****************************************************************************/
/*
**	err = hal_getsysdate(hal, datep, tzsecp)
**
**	Insert datetime into *datep. If tzsecp is NULL, *datep will
**	be set to local time. Otherwise, *datep will be set to UT,
**	and *tzsecp will be set to seconds west of GMT.
**
**	err = 0 - ok
**	err = 1 - no date resource available
**	err = 2 - no timezone info available
*/

static TINT hal_getdstfromlocaldate(struct THALBase *hal, TDATE *datep)
{
	union { LARGE_INTEGER li; FILETIME ft; } ltime;
	union { LARGE_INTEGER li; FILETIME ft; } utime;
	ltime.li.QuadPart = datep->tdt_Int64 * 10;
	LocalFileTimeToFileTime(&ltime.ft, &utime.ft);
	return (TINT) ((utime.li.QuadPart - ltime.li.QuadPart) / 10000000);
}

static TINT
hal_getsysdate(struct THALBase *hal, TDATE *datep, TINT *dsbiasp)
{
	union { LARGE_INTEGER li; FILETIME ft; } utime;
	union { LARGE_INTEGER li; FILETIME ft; } ltime;
	TUINT64 syst;
	TINT dsbias;

	GetSystemTimeAsFileTime(&utime.ft);
	syst = utime.li.QuadPart / 10; /* to microseconds since 1.1.1601 */

	FileTimeToLocalFileTime(&utime.ft, &ltime.ft);
	dsbias = (TINT) ((utime.li.QuadPart - ltime.li.QuadPart) / 10000000);

	if (dsbiasp)
		*dsbiasp = dsbias;
	else
		syst -= (TINT64) dsbias * 1000000;

	if (datep)
		datep->tdt_Int64 = syst;

	return 0;
}

/*****************************************************************************/
/*
**	Modules
*/

static TSTRPTR
getmodpathname(TSTRPTR path, TSTRPTR extra, TSTRPTR modname)
{
	TSTRPTR modpath;
	TINT l = strlen(path) + strlen(modname) + 5;	/* + .dll + \0 */
	if (extra) l += strlen(extra);
	modpath = malloc(l);
	if (modpath)
	{
		strcpy(modpath, path);
		if (extra) strcat(modpath, extra);
		strcat(modpath, modname);
		strcat(modpath, ".dll");
	}
	return modpath;
}

static TSTRPTR getmodpath(TSTRPTR path, TSTRPTR name)
{
	TINT l = strlen(path);
	TSTRPTR p = malloc(l + strlen(name) + 1);
	if (p)
	{
		strcpy(p, path);
		strcpy(p + l, name);
	}
	return p;
}

static TSTRPTR getmodsymbol(TSTRPTR modname)
{
	TINT l = strlen(modname) + 11;
	TSTRPTR sym = malloc(l);
	if (sym)
	{
		strcpy(sym, "_tek_init_");
		strcpy(sym + 10, modname);
	}
	return sym;
}

static TBOOL
scanpathtolist(struct THook *hook, TSTRPTR path, TSTRPTR name)
{
	WIN32_FIND_DATA fd;
	TBOOL success = TTRUE;
	HANDLE hfd = FindFirstFile(path, &fd);
	if (hfd != INVALID_HANDLE_VALUE)
	{
		TINT l, pl = strlen(name);
		struct THALScanModMsg msg;
		do
		{
			l = strlen(fd.cFileName);
			if (l < pl + TEKHOST_EXTLEN) continue;
			if (strcmp(fd.cFileName + l - TEKHOST_EXTLEN, TEKHOST_EXTSTR))
				continue;
			if (strncmp(fd.cFileName, name, pl)) continue;

			msg.tsmm_Name = fd.cFileName;
			msg.tsmm_Length = l - TEKHOST_EXTLEN;
			msg.tsmm_Flags = 0;
			success = (TBOOL) TCALLHOOKPKT(hook, TNULL, (TTAG) &msg);

			#if 0
			if (l >= pl + 4)
			{
				if (!strncmp(fd.cFileName, name, pl))
				{
					success = (*callb)(userdata, fd.cFileName, l - 4);
				}
			}
			#endif

		} while (success && FindNextFile(hfd, &fd));
		FindClose(hfd);
	}
	return success;
}

EXPORT TAPTR
hal_loadmodule(struct THALBase *hal, TSTRPTR name, TUINT16 version,
	TUINT *psize, TUINT *nsize)
{
	struct HALModule *mod = hal_alloc(hal, sizeof(struct HALModule));
	if (mod)
	{
		TSTRPTR modpath;
		struct HALSpecific *hws = hal->hmb_Specific;

		mod->hmd_Lib = TNULL;
		mod->hmd_InitFunc = TNULL;

		modpath = getmodpathname(hws->hsp_ProgDir, "mod\\", name);
		if (modpath)
		{
			TDBPRINTF(2,("trying %s\n", modpath));
			mod->hmd_Lib = LoadLibraryEx(modpath, 0,
				LOAD_WITH_ALTERED_SEARCH_PATH);
			free(modpath);
		}

		if (!mod->hmd_Lib)
		{
			modpath = getmodpathname(hws->hsp_ModDir, TNULL, name);
			if (modpath)
			{
				TDBPRINTF(2,("trying %s\n", modpath));
				mod->hmd_Lib = LoadLibraryEx(modpath, 0,
					LOAD_WITH_ALTERED_SEARCH_PATH);
				free(modpath);
			}
		}

		if (mod->hmd_Lib)
		{
			TSTRPTR modsym;
			modsym = getmodsymbol(name);
			if (modsym)
			{
				TDBPRINTF(2,("resolving %s\n", modsym));
				mod->hmd_InitFunc =
					(TAPTR) GetProcAddress(mod->hmd_Lib, modsym + 1);
				if (!mod->hmd_InitFunc)
				{
					mod->hmd_InitFunc =
						(TAPTR) GetProcAddress(mod->hmd_Lib, modsym);
				}
				free(modsym);
			}

			if (mod->hmd_InitFunc)
			{
				*psize = (*mod->hmd_InitFunc)(TNULL, TNULL, version, TNULL);
				if (*psize)
				{
					*nsize = (*mod->hmd_InitFunc)(TNULL, TNULL, 0xffff, TNULL);
					mod->hmd_Version = version;
					return (TAPTR) mod;
				}
			} else TDBPRINTF(5,("could not resolve %s entrypoint\n", name));

			FreeLibrary(mod->hmd_Lib);
		}
		free(mod);
	}
	return TNULL;
}

EXPORT TBOOL
hal_callmodule(struct THALBase *hal, TAPTR halmod, struct TTask *task, TAPTR mod)
{
	struct HALModule *hwm = halmod;
	return (TBOOL) (*hwm->hmd_InitFunc)(task, mod, hwm->hmd_Version, TNULL);
}

EXPORT void
hal_unloadmodule(struct THALBase *hal, TAPTR halmod)
{
	struct HALModule *hwm = halmod;
	FreeLibrary(hwm->hmd_Lib);
	TDBPRINTF(2,("module unloaded\n"));
	free(halmod);
}

EXPORT TBOOL
hal_scanmodules(struct THALBase *hal, TSTRPTR path, struct THook *hook)
{
	TSTRPTR p;
	struct HALSpecific *hws = hal->hmb_Specific;
	TBOOL success = TFALSE;

	p = getmodpath(hws->hsp_ProgDir, "mod\\*.dll");
	if (p)
	{
		TDBPRINTF(TDB_TRACE,("scanning %s\n", p));
		success = scanpathtolist(hook, p, path);
		free(p);
	}

	if (success)
	{
		p = getmodpath(hws->hsp_ModDir, "*.dll");
		if (p)
		{
			TDBPRINTF(TDB_TRACE,("scanning %s\n", p));
			success = scanpathtolist(hook, p, path);
			free(p);
		}
	}
	return success;
}

/*****************************************************************************/

#ifdef USE_MMTIMER

/*
**	MMTimer implementation
*/

static struct HALSpecific *g_hws;
static struct THALBase *g_hal;
static struct TList g_ReplyList;

static TBOOL hal_replytimereq(struct TTimeRequest *tr)
{
	TBOOL success = TFALSE;
	struct TMessage *msg = TGETMSGPTR(tr);
	struct TMsgPort *mp = msg->tmsg_RPort;
	CRITICAL_SECTION *mplock = THALGetObject((TAPTR) &mp->tmp_Lock,
		CRITICAL_SECTION);
	if (TryEnterCriticalSection(mplock))
	{
		struct TTask *sigtask = mp->tmp_SigTask;
		struct HALThread *t =
			THALGetObject((TAPTR) &sigtask->tsk_Thread, struct HALThread);
#ifndef HAL_USE_ATOMICS
		if (TryEnterCriticalSection(&t->hth_SigLock))
#endif
		{
			tr->ttr_Req.io_Error = 0;
			msg->tmsg_Flags = TMSG_STATUS_REPLIED | TMSGF_QUEUED;
			TAddTail(&mp->tmp_MsgList, &msg->tmsg_Node);
#ifndef HAL_USE_ATOMICS
			if (mp->tmp_Signal & ~t->hth_SigState)
			{
				t->hth_SigState |= mp->tmp_Signal;
				SetEvent(t->hth_SigEvent);
			}
			LeaveCriticalSection(&t->hth_SigLock);
#else
			if (mp->tmp_Signal &
					~(TUINT) InterlockedOr(&t->hth_SigState, mp->tmp_Signal))
				SetEvent(t->hth_SigEvent);
#endif
			success = TTRUE;
		}
		LeaveCriticalSection(mplock);
	}
	return success;
}

static void CALLBACK
hal_timeproc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	struct TNode *node, *nnode;

	if (TryEnterCriticalSection(&g_hws->hsp_DevLock))
	{
		TTIME curtime;
		hal_getsystime(g_hal, &curtime);
		node = g_hws->hsp_ReqList.tlh_Head.tln_Succ;
		for (; (nnode = node->tln_Succ); node = nnode)
		{
			struct TTimeRequest *tr = (struct TTimeRequest *) node;
			if (TCmpTime(&curtime, &tr->ttr_Data.ttr_Time) >= 0)
			{
				TRemove(node);
				TAddTail(&g_ReplyList, node);
			}
		}

		node = g_ReplyList.tlh_Head.tln_Succ;
		for (; (nnode = node->tln_Succ); node = nnode)
		{
			struct TTimeRequest *tr = (struct TTimeRequest *) node;
			TRemove(node);
			if (!hal_replytimereq(tr))
				TAddHead(&g_ReplyList, node);
		}

		LeaveCriticalSection(&g_hws->hsp_DevLock);
	}
}

LOCAL TCALLBACK struct TTimeRequest *
hal_open(struct THALBase *hal, struct TTask *task, TTAGITEM *tags)
{
	struct HALSpecific *hws = hal->hmb_Specific;
	TAPTR exec = (TAPTR) TGetTag(tags, THalBase_Exec, TNULL);
	struct TTimeRequest *req;
	hws->hsp_ExecBase = exec;

	req = TExecAllocMsg0(exec, sizeof(struct TTimeRequest));
	if (req)
	{
		EnterCriticalSection(&hws->hsp_DevLock);

		if (!hws->hsp_Timer)
		{
			if (timeBeginPeriod(1) == TIMERR_NOERROR)
			{
				g_hws = hws;
				g_hal = hal;
				TInitList(&g_ReplyList);
				TInitList(&hws->hsp_ReqList);
				hws->hsp_Timer =
					timeSetEvent(1, 1, hal_timeproc, TNULL, TIME_PERIODIC);
			}
		}

		if (hws->hsp_Timer)
		{
			hws->hsp_RefCount++;
			req->ttr_Req.io_Device = (struct TModule *) hal;
		}
		else
		{
			TExecFree(exec, req);
			req = TNULL;
		}

		LeaveCriticalSection(&hws->hsp_DevLock);
	}

	return req;
}

LOCAL TCALLBACK void
hal_close(struct THALBase *hal, struct TTask *task)
{
	struct HALSpecific *hws = hal->hmb_Specific;

	EnterCriticalSection(&hws->hsp_DevLock);

	if (hws->hsp_Timer)
	{
		if (--hws->hsp_RefCount == 0)
		{
			timeKillEvent(hws->hsp_Timer);
			timeEndPeriod(1);
			hws->hsp_Timer = TNULL;
		}
	}

	LeaveCriticalSection(&hws->hsp_DevLock);
}

#else

/*****************************************************************************/
/*
**	Timertask implementation
*/

static void TTASKENTRY hal_devfunc(struct TTask *task)
{
	TAPTR exec = TGetExecBase(task);
	struct THALBase *hal = TExecGetHALBase(exec);
	struct HALSpecific *hps = hal->hmb_Specific;
	struct HALThread *thread = TlsGetValue(hps->hsp_TLSIndex);
	TAPTR port = TExecGetUserPort(exec, task);
	struct TTimeRequest *msg;
	TUINT sig = 0;
	TTIME waittime, curtime;
	struct TNode *nnode, *node;

 	waittime.tdt_Int64 = 2000000000000000LL;

	for (;;)
	{
		sig = hal_timedwaitevent(hal, thread, &waittime,
			TTASK_SIG_ABORT | TTASK_SIG_USER);

		if (sig & TTASK_SIG_ABORT)
			break;

		EnterCriticalSection(&hps->hsp_DevLock);
		hal_getsystime(hal, &curtime);

		while ((msg = TExecGetMsg(exec, port)))
		{
			TAddTime(&msg->ttr_Data.ttr_Time, &curtime);
			TAddTail(&hps->hsp_ReqList, (struct TNode *) msg);
		}

		waittime.tdt_Int64 = 2000000000000000LL;
		node = hps->hsp_ReqList.tlh_Head.tln_Succ;
		for (; (nnode = node->tln_Succ); node = nnode)
		{
			struct TTimeRequest *tr = (struct TTimeRequest *) node;
			TTIME *tm = &tr->ttr_Data.ttr_Time;
			if (TCmpTime(&curtime, tm) >= 0)
			{
				TRemove(node);
				tr->ttr_Req.io_Error = 0;
				TExecReplyMsg(exec, node);
				continue;
			}
			if (TCmpTime(tm, &waittime) < 0)
				waittime = *tm;
		}

		LeaveCriticalSection(&hps->hsp_DevLock);
	}

	TDBPRINTF(2,("goodbye from HAL device\n"));
}

LOCAL TCALLBACK struct TTimeRequest *
hal_open(struct THALBase *hal, struct TTask *task, TTAGITEM *tags)
{
	struct HALSpecific *hws = hal->hmb_Specific;
	TAPTR exec = (TAPTR) TGetTag(tags, THalBase_Exec, TNULL);
	struct TTimeRequest *req;
	hws->hsp_ExecBase = exec;

	req = TExecAllocMsg0(exec, sizeof(struct TTimeRequest));
	if (req)
	{
		EnterCriticalSection(&hws->hsp_DevLock);

		if (!hws->hsp_DevTask)
		{
			TTAGITEM tasktags[2];
			tasktags[0].tti_Tag = TTask_Name;			/* set task name */
			tasktags[0].tti_Value = (TTAG) TTASKNAME_HALDEV;
			tasktags[1].tti_Tag = TTAG_DONE;
			TInitList(&hws->hsp_ReqList);
			hws->hsp_DevTask = TExecCreateSysTask(exec, hal_devfunc, tasktags);
		}

		if (hws->hsp_DevTask)
		{
			hws->hsp_RefCount++;
			req->ttr_Req.io_Device = (struct TModule *) hal;
		}
		else
		{
			TExecFree(exec, req);
			req = TNULL;
		}

		LeaveCriticalSection(&hws->hsp_DevLock);
	}

	return req;
}

LOCAL TCALLBACK void
hal_close(struct THALBase *hal, struct TTask *task)
{
	struct HALSpecific *hws = hal->hmb_Specific;

	EnterCriticalSection(&hws->hsp_DevLock);

	if (hws->hsp_DevTask)
	{
		if (--hws->hsp_RefCount == 0)
		{
			TDBPRINTF(2,("hal.device refcount dropped to 0\n"));
			TExecSignal(hws->hsp_ExecBase, hws->hsp_DevTask, TTASK_SIG_ABORT);
			TDBPRINTF(2,("destroy hal.device task...\n"));
			TDestroy(hws->hsp_DevTask);
			hws->hsp_DevTask = TNULL;
		}
	}

	LeaveCriticalSection(&hws->hsp_DevLock);
}

#endif

/*****************************************************************************/

EXPORT void
hal_beginio(struct THALBase *hal, struct TTimeRequest *req)
{
	struct HALSpecific *hws = hal->hmb_Specific;
	TAPTR exec = hws->hsp_ExecBase;
	TTIME curtime;
	TINT64 x = 0;

	switch (req->ttr_Req.io_Command)
	{
		/* execute asynchronously */
		case TTREQ_WAITLOCALDATE:
			/* microseconds west of GMT: */
			x = hal_getdstfromlocaldate(hal, &req->ttr_Data.ttr_Date);
			x *= 1000000;
		case TTREQ_WAITUNIVERSALDATE:
			x += req->ttr_Data.ttr_Date.tdt_Int64;
			x -= 11644473600000000ULL;
			req->ttr_Data.ttr_Date.tdt_Int64 = x;
			#ifndef USE_MMTIMER
			hal_getsystime(hal, &curtime);
			TSubTime(&req->ttr_Data.ttr_Time, &curtime);
			#endif
			goto addtimereq;

		case TTREQ_WAITTIME:
			#ifdef USE_MMTIMER
			hal_getsystime(hal, &curtime);
			TAddTime(&req->ttr_Data.ttr_Time, &curtime);
			#endif

		addtimereq:
		{
			#ifdef USE_MMTIMER
			struct TMessage *msg = TGETMSGPTR(req);
			EnterCriticalSection(&hws->hsp_DevLock);
			msg->tmsg_Flags = TMSG_STATUS_SENT | TMSGF_QUEUED;
			msg->tmsg_RPort = req->ttr_Req.io_ReplyPort;
			TAddTail(&hws->hsp_ReqList, (struct TNode *) req);
			LeaveCriticalSection(&hws->hsp_DevLock);
			#else
			TExecPutMsg(exec, TExecGetUserPort(exec, hws->hsp_DevTask),
				req->ttr_Req.io_ReplyPort, req);
			#endif
			return;
		}

		/* execute synchronously */
		default:
		case TTREQ_GETUNIVERSALDATE:
		{
			TINT timezone_bias;
			hal_getsysdate(hal, &req->ttr_Data.ttr_Date, &timezone_bias);
			break;
		}

		case TTREQ_GETLOCALDATE:
			hal_getsysdate(hal, &req->ttr_Data.ttr_Date, TNULL);
			break;

		case TTREQ_GETSYSTEMTIME:
			hal_getsystime(hal, &req->ttr_Data.ttr_Time);
			break;

		#if 0
		case TTREQ_GETDSFROMDATE:
			req->ttr_Data.ttr_DSSec =
				hal_dsfromdate(hal, &req->ttr_Data.ttr_Date.ttr_Date);
			break;
		#endif
	}

	req->ttr_Req.io_Error = 0;

	if (!(req->ttr_Req.io_Flags & TIOF_QUICK))
	{
		/* async operation indicated; reply to self */
		TExecReplyMsg(exec, req);
	}
}

EXPORT TINT
hal_abortio(struct THALBase *hal, struct TTimeRequest *req)
{
	struct HALSpecific *hws = hal->hmb_Specific;
	TAPTR exec = hws->hsp_ExecBase;
	TUINT status;

	#ifdef USE_MMTIMER

	EnterCriticalSection(&hws->hsp_DevLock);
	status = TExecGetMsgStatus(exec, req);
	if (status == (TMSG_STATUS_SENT | TMSGF_QUEUED))
	{
		/* enforce immediate expiration */
		req->ttr_Data.ttr_Time.tdt_Int64 = 0;
		req->ttr_Req.io_Error = TIOERR_ABORTED;
		TRemove(&req->ttr_Req.io_Node);
		while (!hal_replytimereq(req));
	}
	LeaveCriticalSection(&hws->hsp_DevLock);

	#else

	EnterCriticalSection(&hws->hsp_DevLock);
	status = TExecGetMsgStatus(exec, req);
	if (!(status & TMSGF_RETURNED))
	{
		if (status & TMSGF_QUEUED)
		{
			/* remove from ioport */
			TAPTR ioport = TExecGetUserPort(exec, hws->hsp_DevTask);
			TExecRemoveMsg(exec, ioport, req);
		}
		else
		{
			/* remove from reqlist */
			TRemove((struct TNode *) req);
			TExecSignal(exec, hws->hsp_DevTask, TTASK_SIG_USER);
		}
	}
	else
	{
		/* already replied */
		req = TNULL;
	}
	LeaveCriticalSection(&hws->hsp_DevLock);

	if (req)
	{
		req->ttr_Req.io_Error = TIOERR_ABORTED;
		TExecReplyMsg(exec, req);
	}

	#endif
	return 0;
}
