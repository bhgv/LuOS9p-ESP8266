
/*
**	$Id: host.c,v 1.2 2006/09/10 00:58:05 tmueller Exp $
**	teklib/src/teklib/posix/host.c - Host-specific startup implementation
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/lib/init.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <sys/param.h>
#include <dlfcn.h>
#if defined(ENABLE_LAZY_SINGLETON)
#include <pthread.h>
#endif

#ifdef PATH_MAX
#define MAX_PATH_LEN	PATH_MAX
#else
#define MAX_PATH_LEN	MAXPATHLEN
#endif

struct host_modhandle
{
	union
	{
		TMODINITFUNC func;
		TAPTR object;
	} entry;
	TUINT type;
};

#define TYPE_DLL	0
#define TYPE_LIB	1

/*****************************************************************************/
/*
**	lookup internal module
*/

static struct TInitModule *
host_lookupmodule(TSTRPTR modname, TTAGITEM *tags)
{
	struct TInitModule *imod =
		(struct TInitModule *) TGetTag(tags, TExecBase_ModInit, TNULL);
	if (imod)
	{
		while (imod->tinm_Name)
		{
			if (!strcmp(imod->tinm_Name, modname))
				return imod;
			imod++;
		}
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	host init/exit
*/

TLIBAPI TAPTR
TEKlib_Init(TTAGITEM *tags)
{
	return (TAPTR) 1;
}

TLIBAPI void
TEKlib_Exit(TAPTR boot)
{
}

/*****************************************************************************/
/*
**	ref/unref
*/

#if defined(ENABLE_LAZY_SINGLETON)
#warning using globals (-DENABLE_LAZY_SINGLETON)
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static TAPTR g_handle = TNULL;
#endif

TLIBAPI TAPTR 
TEKlib_DoRef(struct TTask *(*func)(struct TTagItem *), struct TTagItem *tags)
{
#if defined(ENABLE_LAZY_SINGLETON)
	TAPTR handle;
	pthread_mutex_lock(&g_lock);
	if (g_handle)
		handle = TExecFindTask(TGetExecBase(g_handle), TNULL);
	else
		handle = g_handle = (*func)(tags);
	pthread_mutex_unlock(&g_lock);
	return handle;
#else
	return (*func)(tags);
#endif
}

TLIBAPI void 
TEKlib_DoUnref(void (*func)(TAPTR), TAPTR handle)
{
#if defined(ENABLE_LAZY_SINGLETON)
	pthread_mutex_lock(&g_lock);
	if (handle == g_handle)
	{
#endif
		(*func)(handle);
#if defined(ENABLE_LAZY_SINGLETON)
		g_handle = TNULL;
	}
	pthread_mutex_unlock(&g_lock);
#endif
}

/*****************************************************************************/
/*
**	host alloc/free
*/

TLIBAPI TAPTR
TEKlib_Alloc(TAPTR boot, TUINT size)
{
	TAPTR mem = malloc(size);
	return mem;
}

TLIBAPI void
TEKlib_Free(TAPTR boot, TAPTR mem, TUINT size)
{
	if (mem) free(mem);
}

TLIBAPI void
TEKlib_FreeVec(TAPTR boot, TAPTR mem)
{
	if (mem) free(mem);
}

/*****************************************************************************/
/*
**	determine TEKlib global system directory
*/

TLIBAPI TSTRPTR
TEKlib_GetSysDir(TAPTR boot, TTAGITEM *tags)
{
	TSTRPTR sysdir;
	TSTRPTR s;
	TINT l;

	s = (TSTRPTR) TGetTag(tags, TExecBase_SysDir, (TTAG) TEKHOST_SYSDIR);
	l = strlen(s);

	sysdir = TEKlib_Alloc(boot, l + 1);
	if (sysdir)
		strcpy(sysdir, s);

	return sysdir;
}

/*****************************************************************************/
/*
**	determine TEKlib global module directory
*/

TLIBAPI TSTRPTR
TEKlib_GetModDir(TAPTR boot, TTAGITEM *tags)
{
	TSTRPTR sysdir;
	TSTRPTR s;
	TINT l;

	s = (TSTRPTR) TGetTag(tags, TExecBase_ModDir, (TTAG) TEKHOST_MODDIR);
	l = strlen(s);

	sysdir = TEKlib_Alloc(boot, l + 1);
	if (sysdir)
		strcpy(sysdir, s);

	return sysdir;
}

/*****************************************************************************/
/*
**	determine the path to the application, which will
**	later resolve to "PROGDIR:" in teklib semantics.
*/

TLIBAPI TSTRPTR
TEKlib_GetProgDir(TAPTR boot, TTAGITEM *tags)
{
	TSTRPTR progdir = TNULL;
	TSTRPTR *argv;
	TINT argc;

	progdir = (TSTRPTR) TGetTag(tags, TExecBase_ProgDir, TEKHOST_PROGDIR);
	if (progdir)
	{
		TINT l = strlen(progdir);
		TSTRPTR p = TEKlib_Alloc(boot, l + 1);
		if (p)
		{
			strcpy(p, progdir);
			return p;
		}
		return TNULL;
	}

	argc = (TINT) TGetTag(tags, TExecBase_ArgC, 0);
	argv = (TSTRPTR *) TGetTag(tags, TExecBase_ArgV, TNULL);

	if (argc >= 1 && argv)
	{
		TSTRPTR olddir = TEKlib_Alloc(boot, MAX_PATH_LEN);
		if (olddir)
		{
			if (getcwd(olddir, MAX_PATH_LEN))
			{
				progdir = TEKlib_Alloc(boot, MAX_PATH_LEN + 1);
				if (progdir)
				{
					TBOOL success = TFALSE;
					TINT l = 0;
					TSTRPTR s = argv[0];
					TINT c;

					while (*s)
					{
						s++;
						l++;
					}

					if (l > 0)
					{
						success = TTRUE;
						while ((c = *(--s)))
						{
							if (c == '/')
								break;
							l--;
						}
						if (l > 0)
						{
							TSTRPTR d, pathpart = TEKlib_Alloc(boot, l + 1);
							success = TFALSE;
							if (pathpart)
							{
								s = argv[0];
								d = pathpart;
								while (l--)
									*d++ = *s++;
								*d = 0;
								success = (chdir(pathpart) == 0);
								TEKlib_FreeVec(boot, pathpart);
							}
						}
					}

					if (success)
						success = (getcwd(progdir, MAX_PATH_LEN) != TNULL);

					if (!(chdir(olddir) == 0))
						success = TFALSE;

					if (success)
						strcat(progdir, "/");
					else
					{
						TEKlib_FreeVec(boot, progdir);
						progdir = TNULL;
					}
				}
			}
 			TEKlib_FreeVec(boot, olddir);
		}
	}
	return progdir;
}

/*****************************************************************************/
/*
**	load a module. first try progdir/modname, then moddir
*/

static TAPTR
host_getmodule(TSTRPTR modpath)
{
	return dlopen(modpath, RTLD_NOW);
}

static TMODINITFUNC
host_getsymaddr(TAPTR mod, TSTRPTR name)
{
	union
	{
		TMODINITFUNC func;
		TAPTR object;
	} entry;

	entry.object = dlsym(mod, name);
	return entry.func;
}

static void closemodule(TAPTR mod)
{
	dlclose(mod);
}

/*****************************************************************************/

TLIBAPI TAPTR
TEKlib_LoadModule(TAPTR boot, TSTRPTR progdir, TSTRPTR moddir, TSTRPTR modname,
	TTAGITEM *tags)
{
	TAPTR knmod = TNULL;
	TINT len1, len2, len3;
	TSTRPTR t;
	struct TInitModule *imod;
	struct host_modhandle *handle;

	handle = TEKlib_Alloc(boot, sizeof(*handle));
	if (!handle)
		return TNULL;

	imod = host_lookupmodule(modname, tags);
	if (imod)
	{
		handle->entry.func = imod->tinm_InitFunc;
		handle->type = TYPE_LIB;
		return handle;
	}

	len1 = progdir ? strlen(progdir) : 0;
	len2 = moddir ? strlen(moddir) : 0;
	len3 = strlen(modname);

	/* + mod/ + .so + \0 */
	t = TEKlib_Alloc(boot, TMAX(len1, len2) + len3 + 4 + TEKHOST_EXTLEN + 1);
	if (t)
	{
		if (progdir)
			strcpy(t, progdir);
		strcpy(t + len1, "mod/");
		strcpy(t + len1 + 4, modname);
		strcpy(t + len1 + 4 + len3, TEKHOST_EXTSTR);

		TDBPRINTF(3,("trying dlopen %s\n", t));
		knmod = host_getmodule(t);
		if (!knmod)
		{
			if (moddir) strcpy(t, moddir);
			strcpy(t + len2, modname);
			strcpy(t + len2 + len3, TEKHOST_EXTSTR);

			TDBPRINTF(3,("trying dlopen %s\n", t));
			knmod = host_getmodule(t);
		}

		if (!knmod)
			TDBPRINTF(20,("dlopen %s failed\n", modname));

		TEKlib_FreeVec(boot, t);
	}

	if (knmod)
	{
		handle->entry.object = knmod;
		handle->type = TYPE_DLL;
	}
	else
	{
		TEKlib_FreeVec(boot, handle);
		handle = TNULL;
	}

	return handle;
}

/*****************************************************************************/
/*
**	close module
*/

TLIBAPI void
TEKlib_CloseModule(TAPTR boot, TAPTR knmod)
{
	struct host_modhandle *handle = knmod;
	if (handle->type == TYPE_DLL)
		closemodule(handle->entry.object);
	TEKlib_FreeVec(boot, handle);
}

/*****************************************************************************/
/*
**	get module entry
*/

TLIBAPI TMODINITFUNC
TEKlib_GetEntry(TAPTR boot, TAPTR knmod, TSTRPTR name)
{
	struct host_modhandle *handle = knmod;
	if (handle->type == TYPE_DLL)
		return host_getsymaddr(handle->entry.object, name);
	return handle->entry.func;
}

/*****************************************************************************/
/*
**	call module
*/

TLIBAPI TUINT
TEKlib_CallModule(TAPTR boot, TAPTR ModBase, TMODINITFUNC entry,
	struct TTask *task, TAPTR mod, TUINT16 version, TTAGITEM *tags)
{
	return (*entry)(task, mod, version, tags);
}

