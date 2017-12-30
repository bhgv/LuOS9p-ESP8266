
/*
**	$Id: hal.c,v 1.5 2006/09/10 20:28:47 tmueller Exp $
**	teklib/src/hal/posix/hal.c - POSIX implementation of the HAL layer
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/mod/posix/hal.h>
#include <tek/mod/hal.h>

#include "hal_mod.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <dlfcn.h>

/* undefine to use waiting locks */
#define HAL_POSIX_SPINLOCK_USE
/* if defined, number of spins before falling back to waits */
#define HAL_POSIX_SPINLOCK_MAXCOUNT	1000

void tzset(void);
static void TTASKENTRY hal_devfunc(struct TTask *task);

/*****************************************************************************/

#ifdef TDEBUG
#define TRACKMEM
static int bytecount;
static int allocount;
static int maxbytecount;
static pthread_mutex_t alloclock;
#endif

/*****************************************************************************/
/*
**	Host init
*/

LOCAL TBOOL
hal_init(struct THALBase *hal, TTAGITEM *tags)
{
	struct HALSpecific *specific = malloc(sizeof(struct HALSpecific));
	if (specific)
	{
		memset(specific, 0, sizeof(struct HALSpecific));

		pthread_mutex_init(&specific->hsp_DevLock, TNULL);
		pthread_mutex_init(&specific->hsp_TimeLock, TNULL);

		if (pthread_key_create(&specific->hsp_TSDKey, NULL) == 0)
		{
			time_t curt;

			specific->hsp_SysDir = (TSTRPTR) TGetTag(tags,
				TExecBase_SysDir, (TTAG) TEKHOST_SYSDIR);
			specific->hsp_ModDir = (TSTRPTR) TGetTag(tags,
				TExecBase_ModDir, (TTAG) TEKHOST_MODDIR);
			specific->hsp_ProgDir = (TSTRPTR) TGetTag(tags,
				TExecBase_ProgDir, (TTAG) TEKHOST_PROGDIR);

			specific->hsp_Tags[0].tti_Tag = TExecBase_SysDir;
			specific->hsp_Tags[0].tti_Value = (TTAG) specific->hsp_SysDir;
			specific->hsp_Tags[1].tti_Tag = TExecBase_ModDir;
			specific->hsp_Tags[1].tti_Value = (TTAG) specific->hsp_ModDir;
			specific->hsp_Tags[2].tti_Tag = TExecBase_ProgDir;

			specific->hsp_Tags[2].tti_Value = (TTAG) specific->hsp_ProgDir;
			specific->hsp_Tags[3].tti_Tag = TTAG_DONE;

			hal->hmb_Specific = specific;

			tzset();	/* set global timezone variable */

			time(&curt);
			specific->hsp_TZSec = mktime(gmtime(&curt)) -
				mktime(localtime(&curt));

			#ifdef TRACKMEM
			pthread_mutex_init(&alloclock, TNULL);
			allocount = 0;
			bytecount = 0;
			maxbytecount = 0;
			#endif

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
	pthread_key_delete(specific->hsp_TSDKey);
	pthread_mutex_destroy(&specific->hsp_TimeLock);
	pthread_mutex_destroy(&specific->hsp_DevLock);
	free(specific);

	#ifdef TRACKMEM
	pthread_mutex_destroy(&alloclock);
	if (allocount || bytecount)
	{
		TDBPRINTF(10,("*** Global memory leak: %d allocs, %d bytes pending\n",
			allocount, bytecount));
	}
	TDBPRINTF(4,("*** Peak memory allocated: %d bytes\n", maxbytecount));
	#endif
}

LOCAL TAPTR
hal_allocself(TAPTR handle, TUINT size)
{
	return malloc(size);
}

LOCAL void
hal_freeself(TAPTR handle, TAPTR mem, TUINT size)
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
	TAPTR mem = malloc(size);
	#ifdef TRACKMEM
	if (mem)
	{
		pthread_mutex_lock(&alloclock);
		allocount++;
		bytecount += size;
		if (bytecount > maxbytecount) maxbytecount = bytecount;
		pthread_mutex_unlock(&alloclock);
	}
	#endif
	return mem;
}

EXPORT TAPTR
hal_realloc(struct THALBase *hal, TAPTR mem, TSIZE oldsize, TSIZE newsize)
{
	TAPTR newmem;

	#ifdef TRACKMEM
	pthread_mutex_lock(&alloclock);
	if (mem)
	{
		allocount--;
		bytecount -= oldsize;
	}
	#endif

	newmem = realloc(mem, newsize);

	#ifdef TRACKMEM
	if (newmem)
	{
		allocount++;
		bytecount += newsize;
	}
	pthread_mutex_unlock(&alloclock);
	#endif

	return newmem;
}

EXPORT void
hal_free(struct THALBase *hal, TAPTR mem, TSIZE size)
{
	#ifdef TRACKMEM
	if (mem)
	{
		pthread_mutex_lock(&alloclock);
		allocount--;
		bytecount -= size;
		pthread_mutex_unlock(&alloclock);
	}
	#endif
	free(mem);
}

EXPORT void
hal_copymem(struct THALBase *hal, TAPTR from, TAPTR to, TSIZE numbytes)
{
	memcpy(to, from, numbytes);
}

EXPORT void
hal_fillmem(struct THALBase *hal, TAPTR dest, TSIZE numbytes, TUINT8 fillval)
{
	memset(dest, (int) fillval, numbytes);
}

/*****************************************************************************/
/*
**	Locks
*/

EXPORT TBOOL
hal_initlock(struct THALBase *hal, struct THALObject *lock)
{
	pthread_mutex_t *mut = THALNewObject(hal, lock, pthread_mutex_t);
	if (mut)
	{
		pthread_mutex_init(mut, TNULL);
		THALSetObject(lock, pthread_mutex_t, mut);
		return TTRUE;
	}
	TDBPRINTF(20,("could not create lock\n"));
	return TFALSE;
}

EXPORT void
hal_destroylock(struct THALBase *hal, struct THALObject *lock)
{
	pthread_mutex_t *mut = THALGetObject(lock, pthread_mutex_t);
	if (pthread_mutex_destroy(mut)) TDBPRINTF(20,("mutex_destroy\n"));
	THALDestroyObject(hal, mut, pthread_mutex_t);
}

EXPORT void
hal_lock(struct THALBase *hal, struct THALObject *lock)
{
	pthread_mutex_t *mut = THALGetObject(lock, pthread_mutex_t);
	#if defined(HAL_POSIX_SPINLOCK_USE)
	#if defined(HAL_POSIX_SPINLOCK_MAXCOUNT)
	int maxc = HAL_POSIX_SPINLOCK_MAXCOUNT;
	#endif
	while (pthread_mutex_trylock(mut))
	{
		#if defined(HAL_POSIX_SPINLOCK_MAXCOUNT)
		if (--maxc == 0)
		{
			/* wait for lock */
			pthread_mutex_lock(mut);
			break;
		}
		#endif
	}
	#else
	pthread_mutex_lock(mut);
	#endif
}

EXPORT void
hal_unlock(struct THALBase *hal, struct THALObject *lock)
{
	pthread_mutex_t *mut = THALGetObject(lock, pthread_mutex_t);
	pthread_mutex_unlock(mut);
}

/*****************************************************************************/
/*
**	Threads
*/

static void *
hal_posixthread_entry(struct HALThread *thread)
{
	struct THALBase *hal = thread->hth_HALBase;
	struct HALSpecific *hps = hal->hmb_Specific;

	if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL))
		TDBPRINTF(20,("pthread_setcancelstate\n"));

	if (pthread_setspecific(hps->hsp_TSDKey, (void *) thread))
		TDBPRINTF(20,("failed to set TSD key\n"));

	/* wait for initial signal to run */
	hal_wait(hal, TTASK_SIG_SINGLE);

	/* call function */
	(*thread->hth_Function)(thread->hth_Data);

	return NULL;
}

EXPORT TBOOL
hal_initthread(struct THALBase *hal, struct THALObject *thread,
	TTASKENTRY void (*function)(struct TTask *task), TAPTR data)
{
	struct HALThread *t = THALNewObject(hal, thread, struct HALThread);
	if (t)
	{
		if (pthread_cond_init(&t->hth_SigCond, NULL) == 0)
		{
			pthread_mutex_init(&t->hth_SigMutex, NULL);
			t->hth_SigState = 0;
			t->hth_Function = function;
			t->hth_Data = data;
			t->hth_HALBase = hal;
			THALSetObject(thread, struct HALThread, t);
			if (function)
			{
				if (pthread_create(&t->hth_PThread, NULL,
					(void *(*)(void *)) hal_posixthread_entry, t) == 0)
					return TTRUE;
			}
			else
			{
				struct HALSpecific *hps = hal->hmb_Specific;
				if (pthread_setspecific(hps->hsp_TSDKey, (void *) t) == 0)
					return TTRUE;
			}
			pthread_cond_destroy(&t->hth_SigCond);
		}
		THALDestroyObject(hal, t, struct HALThread);
	}
	TDBPRINTF(20,("could not create thread\n"));
	return TFALSE;
}

EXPORT void
hal_destroythread(struct THALBase *hal, struct THALObject *thread)
{
	struct HALThread *t = THALGetObject(thread, struct HALThread);
	if (t->hth_Function)
	{
		if (pthread_join(t->hth_PThread, NULL)) TDBPRINTF(20,("pthread_join\n"));
	}
	if (pthread_mutex_destroy(&t->hth_SigMutex))
		TDBPRINTF(20,("mutex_destroy\n"));
	if (pthread_cond_destroy(&t->hth_SigCond))
		TDBPRINTF(20,("cond_destroy\n"));
	THALDestroyObject(hal, t, struct HALThread);
}

EXPORT TAPTR
hal_findself(struct THALBase *hal)
{
	struct HALSpecific *hps = hal->hmb_Specific;
	struct HALThread *t = pthread_getspecific(hps->hsp_TSDKey);
	return t->hth_Data;
}

/*****************************************************************************/
/*
**	Signals
*/

EXPORT void
hal_signal(struct THALBase *hal, struct THALObject *thread, TUINT signals)
{
	struct HALThread *t = THALGetObject(thread, struct HALThread);
	pthread_mutex_lock(&t->hth_SigMutex);
	if (signals & ~t->hth_SigState)
	{
		t->hth_SigState |= signals;
		if (pthread_cond_signal(&t->hth_SigCond))
			TDBPRINTF(20,("cond_signal\n"));
	}
	pthread_mutex_unlock(&t->hth_SigMutex);
}

EXPORT TUINT
hal_setsignal(struct THALBase *hal, TUINT newsig, TUINT sigmask)
{
	TUINT oldsig;
	struct HALSpecific *hps = hal->hmb_Specific;
	struct HALThread *t = pthread_getspecific(hps->hsp_TSDKey);
	pthread_mutex_lock(&t->hth_SigMutex);
	newsig &= sigmask;
	oldsig = t->hth_SigState;
	t->hth_SigState &= ~sigmask;
	t->hth_SigState |= newsig;
	if (newsig & ~oldsig)
	{
		if (pthread_cond_signal(&t->hth_SigCond))
			TDBPRINTF(20,("cond_signal\n"));
	}

	pthread_mutex_unlock(&t->hth_SigMutex);
	return oldsig;
}

EXPORT TUINT
hal_wait(struct THALBase *hal, TUINT sigmask)
{
	TUINT sig;
	struct HALSpecific *hps = hal->hmb_Specific;
	struct HALThread *t = pthread_getspecific(hps->hsp_TSDKey);

	pthread_mutex_lock(&t->hth_SigMutex);
	for (;;)
	{
		sig = t->hth_SigState & sigmask;
		t->hth_SigState &= ~sigmask;
		if (sig) break;
		pthread_cond_wait(&t->hth_SigCond, &t->hth_SigMutex);
	}
	pthread_mutex_unlock(&t->hth_SigMutex);
	return sig;
}

static TUINT
hal_timedwaitevent(TAPTR hal, struct HALThread *t, TTIME *wt,
	TUINT sigmask)
{
	TUINT sig;

	struct timespec tv;
	tv.tv_sec = wt->tdt_Int64 / 1000000;
	tv.tv_nsec = (wt->tdt_Int64 % 1000000) * 1000;

	pthread_mutex_lock(&t->hth_SigMutex);

	for (;;)
	{
		sig = t->hth_SigState & sigmask;
		t->hth_SigState &= ~sigmask;
		if (sig) break;
		if (pthread_cond_timedwait(&t->hth_SigCond,
			&t->hth_SigMutex, &tv) == ETIMEDOUT) break;
	}

	pthread_mutex_unlock(&t->hth_SigMutex);
	return sig;
}

/*****************************************************************************/
/*
**	Time and date
*/

static void
hal_getsystime(struct THALBase *hal, TTIME *time)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time->tdt_Int64 = (TINT64) tv.tv_sec * 1000000 + tv.tv_usec;
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

static time_t
hal_timefromdate(struct THALBase *hal, TDATE *dt)
{
	/*
	**	1.1.1601 ... 1.1.1970:
	**		134774 d
	**		11644473600 s
	**		11644473600000 ms
	**		11644473600000000 us
	*/

	TUINT64 t = dt->tdt_Int64;
	t -= 11644473600000000ULL;
	t /= 1000000;
	return (time_t) t;
}

static TINT
hal_tzbias(struct THALBase *hal)
{
	struct HALSpecific *hps = hal->hmb_Specific;
	return hps->hsp_TZSec;
}

static TINT
hal_dsbias(struct THALBase *hal, time_t t)
{
	struct HALSpecific *hps = hal->hmb_Specific;
	struct tm *tm;
	TINT dstsec;
	while (pthread_mutex_trylock(&hps->hsp_TimeLock));
	tm = localtime(&t);
	dstsec = tm->tm_isdst ? -3600 : 0;
	pthread_mutex_unlock(&hps->hsp_TimeLock);
	return dstsec;
}

static TINT
hal_datebias(struct THALBase *hal, TDATE *dt)
{
	return hal_tzbias(hal) + hal_dsbias(hal, hal_timefromdate(hal, dt));
}

static TINT
hal_getsysdate(struct THALBase *hal, TDATE *datep, TINT *tzsecp)
{
	struct timeval tv;
	TINT tzsec;
	TUINT64 syst;

	gettimeofday(&tv, NULL);
	tzsec = hal_tzbias(hal) + hal_dsbias(hal, tv.tv_sec);

	syst = tv.tv_sec;

	if (tzsecp)
		*tzsecp = tzsec;
	else
		syst -= tzsec;

	if (datep)
	{
		syst *= 1000000;
		syst += tv.tv_usec;
		syst += 11644473600000000ULL;
		datep->tdt_Int64 = syst;
	}

	return 0;
}

/*****************************************************************************/
/*
**	Module open and entry
*/

static TAPTR
hal_getmodule(TSTRPTR modpath)
{
	return dlopen(modpath, RTLD_NOW);
}

static TMODINITFUNC
hal_getsymaddr(TAPTR mod, TSTRPTR name)
{
	union
	{
		TMODINITFUNC func;
		TAPTR object;
	} entry;

	entry.object = dlsym(mod, name);
	return entry.func;
}

static void
hal_closemodule(TAPTR mod)
{
	dlclose(mod);
}

/*****************************************************************************/
/*
**	Module loading and calling
*/

static TSTRPTR
hal_getmodpathname(TSTRPTR path, TSTRPTR extra, TSTRPTR modname)
{
	TSTRPTR modpath;
	TINT l;
	if (!path || !modname) return TNULL;
	l = strlen(path) + strlen(modname) + TEKHOST_EXTLEN + 1;
	if (extra) l += strlen(extra);
	modpath = malloc(l);
	if (modpath)
	{
		strcpy(modpath, path);
		if (extra) strcat(modpath, extra);
		strcat(modpath, modname);
		strcat(modpath, TEKHOST_EXTSTR);
	}
	return modpath;
}

static TSTRPTR
hal_getmodsymbol(TSTRPTR modname)
{
	TINT l = strlen(modname) + 11;
	TSTRPTR sym = malloc(l);
	if (sym)
	{
		#ifdef TSYS_DARWIN
			strcpy(sym, "_tek_init_");
		#else
			strcpy(sym, "tek_init_");
		#endif
		strcat(sym, modname);
	}
	return sym;
}

EXPORT TAPTR
hal_loadmodule(struct THALBase *hal, TSTRPTR name, TUINT16 version, TUINT *psize,
	TUINT *nsize)
{
	struct HALModule *halmod = hal_alloc(hal, sizeof(struct HALModule));
	if (halmod)
	{
		struct HALSpecific *hps = hal->hmb_Specific;
		TSTRPTR modpath;

		halmod->hmd_Lib = TNULL;

		modpath = hal_getmodpathname(hps->hsp_ProgDir, "mod/", name);
		if (modpath)
		{
			TDBPRINTF(3,("trying %s\n", modpath));
			halmod->hmd_Lib = hal_getmodule(modpath);
			free(modpath);
		}

		if (!halmod->hmd_Lib)
		{
			modpath = hal_getmodpathname(hps->hsp_ModDir, TNULL, name);
			if (modpath)
			{
				TDBPRINTF(3,("trying %s\n", modpath));
				halmod->hmd_Lib = hal_getmodule(modpath);
				free(modpath);
			}
		}

		if (halmod->hmd_Lib)
		{
			TSTRPTR modsym = hal_getmodsymbol(name);
			if (modsym)
			{
				TDBPRINTF(3,("resolving %s\n", modsym));
				halmod->hmd_InitFunc = hal_getsymaddr(halmod->hmd_Lib, modsym);
				free(modsym);

				if (halmod->hmd_InitFunc)
				{
					*psize = (*halmod->hmd_InitFunc)
						(TNULL, TNULL, version, TNULL);

					if (*psize)
					{
						*nsize = (*halmod->hmd_InitFunc)
							(TNULL, TNULL, 0xffff, TNULL);
						halmod->hmd_Version = version;
						TDBPRINTF(3,("have module\n"));
						return (TAPTR) halmod;

					} else TDBPRINTF(5,("module %s returned 0\n", name));

				} else
					TDBPRINTF(10,("could not resolve %s entrypoint\n", name));
			}

			hal_closemodule(halmod->hmd_Lib);

		} else TDBPRINTF(5,("could not open module %s\n", name));

		hal_free(hal, halmod, sizeof(struct HALModule));
	}
	return TNULL;
}

EXPORT TBOOL
hal_callmodule(struct THALBase *hal, TAPTR mod, struct TTask *task, TAPTR data)
{
	struct HALModule *hum = mod;
	return (TBOOL) (*hum->hmd_InitFunc)(task, data, hum->hmd_Version, TNULL);
}

EXPORT void
hal_unloadmodule(struct THALBase *hal, TAPTR halmod)
{
	struct HALModule *hum = halmod;
	hal_closemodule(hum->hmd_Lib);
 	hal_free(hal, halmod, sizeof(struct HALModule));
	TDBPRINTF(2,("module unloaded\n"));
}

/*****************************************************************************/
/*
**	Module scanning
*/

static TSTRPTR
hal_getmodpath(TSTRPTR path, TSTRPTR name)
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

static TBOOL
hal_scanpathtolist(struct THook *hook, TSTRPTR path, TSTRPTR name)
{
	TBOOL success = TTRUE;
	DIR *dfd = opendir(path);
	if (dfd)
	{
		TINT l;
		struct THALScanModMsg msg;
		struct dirent *de;
		TINT pl = strlen(name);
		while (success && (de = readdir(dfd)))
		{
			l = strlen(de->d_name);
			if (l < pl + TEKHOST_EXTLEN) continue;
			if (strcmp(de->d_name + l - TEKHOST_EXTLEN, TEKHOST_EXTSTR))
				continue;
			if (strncmp(de->d_name, name, pl)) continue;

			msg.tsmm_Name = de->d_name;
			msg.tsmm_Length = l - TEKHOST_EXTLEN;
			msg.tsmm_Flags = 0;

			success = (TBOOL) TCALLHOOKPKT(hook, TNULL, (TTAG) &msg);
		}
		closedir(dfd);
	}
	return success;
}

EXPORT TBOOL
hal_scanmodules(struct THALBase *hal, TSTRPTR path, struct THook *hook)
{
	TSTRPTR p;
	struct HALSpecific *hps = hal->hmb_Specific;
	TBOOL success = TFALSE;

	p = hal_getmodpath(hps->hsp_ProgDir, "mod/");
	if (p)
	{
		success = hal_scanpathtolist(hook, p, path);
		free(p);
	}

	if (success)
	{
		p = hal_getmodpath(hps->hsp_ModDir, "");
		if (p)
		{
			success = hal_scanpathtolist(hook, p, path);
			free(p);
		}
	}

	return success;
}

/*****************************************************************************/
/*
**	HAL device - instance open/close
*/

LOCAL struct TTimeRequest *
hal_open(struct THALBase *hal, struct TTask *task, TTAGITEM *tags)
{
	struct HALSpecific *hps = hal->hmb_Specific;
	TAPTR exec = (TAPTR) TGetTag(tags, THalBase_Exec, TNULL);
	struct TTimeRequest *req;
	hps->hsp_ExecBase = exec;

	req = TExecAllocMsg0(exec, sizeof(struct TTimeRequest));
	if (req)
	{
		pthread_mutex_lock(&hps->hsp_DevLock);

		if (!hps->hsp_DevTask)
		{
			TTAGITEM tasktags[2];
			tasktags[0].tti_Tag = TTask_Name;			/* set task name */
			tasktags[0].tti_Value = (TTAG) TTASKNAME_HALDEV;
			tasktags[1].tti_Tag = TTAG_DONE;
			TINITLIST(&hps->hsp_ReqList);
			hps->hsp_DevTask = TExecCreateSysTask(exec, hal_devfunc, tasktags);
		}

		if (hps->hsp_DevTask)
		{
			hps->hsp_RefCount++;
			req->ttr_Req.io_Device = (struct TModule *) hal;
		}
		else
		{
			TExecFree(exec, req);
			req = TNULL;
		}

		pthread_mutex_unlock(&hps->hsp_DevLock);
	}

	return req;
}

/*****************************************************************************/

LOCAL void
hal_close(struct THALBase *hal, struct TTask *task)
{
	struct HALSpecific *hps = hal->hmb_Specific;
	pthread_mutex_lock(&hps->hsp_DevLock);
	if (hps->hsp_DevTask)
	{
		if (--hps->hsp_RefCount == 0)
		{
			TDBPRINTF(2,("hal.device refcount dropped to 0\n"));
			TExecSignal(hps->hsp_ExecBase, hps->hsp_DevTask, TTASK_SIG_ABORT);
			TDBPRINTF(2,("destroy hal.device task...\n"));
			TDESTROY(hps->hsp_DevTask);
			hps->hsp_DevTask = TNULL;
		}
	}
	pthread_mutex_unlock(&hps->hsp_DevLock);
}

/*****************************************************************************/

static void TTASKENTRY hal_devfunc(struct TTask *task)
{
	TAPTR exec = TGetExecBase(task);
	struct THALBase *hal = (struct THALBase *) TExecGetHALBase(exec);
	struct HALSpecific *hps = hal->hmb_Specific;
	struct HALThread *thread = pthread_getspecific(hps->hsp_TSDKey);
	TAPTR port = TExecGetUserPort(exec, task);
	struct TTimeRequest *msg;
	TUINT sig = 0;
	TTIME waittime, curtime;
	struct TNode *nnode, *node;

 	waittime.tdt_Int64 = 0x7fffffffffffffffULL;

	for (;;)
	{
		sig = hal_timedwaitevent(hal, thread, &waittime,
			TTASK_SIG_ABORT | TTASK_SIG_USER);

		if (sig & TTASK_SIG_ABORT)
			break;

		pthread_mutex_lock(&hps->hsp_DevLock);
		hal_getsystime(hal, &curtime);

		while ((msg = TExecGetMsg(exec, port)))
		{
			TAddTime(&msg->ttr_Data.ttr_Time, &curtime);
			TAddTail(&hps->hsp_ReqList, (struct TNode *) msg);
		}

		waittime.tdt_Int64 = 0x7fffffffffffffffULL;

		node = hps->hsp_ReqList.tlh_Head.tln_Succ;
		for (; (nnode = node->tln_Succ); node = nnode)
		{
			struct TTimeRequest *tr = (struct TTimeRequest *) node;
			TTIME *tm = &tr->ttr_Data.ttr_Time;
			if (TCmpTime(&curtime, tm) >= 0)
			{
				TREMOVE(node);
				TExecReplyMsg(exec, node);
				continue;
			}
			if (TCmpTime(tm, &waittime) < 0)
				waittime = *tm;
		}

		pthread_mutex_unlock(&hps->hsp_DevLock);
	}

	TDBPRINTF(2,("goodbye from HAL device\n"));
}

EXPORT void
hal_beginio(struct THALBase *hal, struct TTimeRequest *req)
{
	struct HALSpecific *hps = hal->hmb_Specific;
	TAPTR exec = hps->hsp_ExecBase;
	TTIME nowtime;
	TUINT64 x = 0;

	switch (req->ttr_Req.io_Command)
	{
		/* execute asynchronously */
		case TTREQ_WAITLOCALDATE:
			x = hal_datebias(hal, &req->ttr_Data.ttr_Date);
			x *= 1000000;
		case TTREQ_WAITUNIVERSALDATE:
			x += req->ttr_Data.ttr_Date.tdt_Int64;
			x -= 11644473600000000ULL;
			req->ttr_Data.ttr_Time.tdt_Int64 = x;
			hal_getsystime(hal, &nowtime);
			TSubTime(&req->ttr_Data.ttr_Time, &nowtime);
			/* relative time */
		case TTREQ_WAITTIME:
			TExecPutMsg(exec, TExecGetUserPort(exec, hps->hsp_DevTask),
				req->ttr_Req.io_ReplyPort, req);
			return;

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
			req->ttr_Data.ttr_DSBias = hal_dsbias(hal,
				hal_timefromdate(hal, &req->ttr_Data.ttr_Date));
			break;
		#endif
	}

	if (!(req->ttr_Req.io_Flags & TIOF_QUICK))
	{
		/* async operation indicated; reply to self */
		TExecReplyMsg(exec, req);
	}
}

EXPORT TINT
hal_abortio(struct THALBase *hal, struct TTimeRequest *req)
{
	struct HALSpecific *hps = hal->hmb_Specific;
	TAPTR exec = hps->hsp_ExecBase;
	TUINT status;

	pthread_mutex_lock(&hps->hsp_DevLock);
	status = TExecGetMsgStatus(exec, req);
	if (!(status & TMSGF_RETURNED))
	{
		if (status & TMSGF_QUEUED)
		{
			/* remove from ioport */
			TAPTR ioport = TExecGetUserPort(exec, hps->hsp_DevTask);
			TExecRemoveMsg(exec, ioport, req);
		}
		else
		{
			/* remove from reqlist */
			TREMOVE((struct TNode *) req);
			TExecSignal(exec, hps->hsp_DevTask, TTASK_SIG_USER);
		}
	}
	else
	{
		/* already replied */
		req = TNULL;
	}
	pthread_mutex_unlock(&hps->hsp_DevLock);

	if (req)
	{
		req->ttr_Req.io_Error = TIOERR_ABORTED;
		TExecReplyMsg(exec, req);
	}

	return 0;
}
