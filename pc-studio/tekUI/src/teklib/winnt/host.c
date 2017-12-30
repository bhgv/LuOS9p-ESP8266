
/*
**	$Id: host.c,v 1.1 2006/08/25 21:23:42 tmueller Exp $
**	boot/win32/host.c - Win32 startup implementation
*/

#include <tek/lib/init.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winreg.h>
#include <process.h>

struct ModHandle
{
	TAPTR entry;
	TUINT type;
};

#define TYPE_DLL	0
#define TYPE_LIB	1

/*****************************************************************************/
/*
**	lookup internal module
*/

static struct TInitModule *
lookupmodule(TSTRPTR modname, TTAGITEM *tags)
{
	struct TInitModule *imod =
		(struct TInitModule *) TGetTag(tags, TExecBase_ModInit, TNULL);
	if (imod)
	{
		while (imod->tinm_Name)
		{
			if (!strcmp(imod->tinm_Name, modname))
			{
				return imod;
			}
			imod++;
		}
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	Get common directory
*/

static TSTRPTR
getcommondir(TSTRPTR extra)
{
	HKEY key;
	TSTRPTR cfd = TNULL;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"Software\\Microsoft\\Windows\\CurrentVersion\\",
			0, KEY_READ, &key) == ERROR_SUCCESS)
	{
		DWORD len = 0;
		if (RegQueryValueEx(key, "CommonFilesDir", NULL, NULL, NULL, &len) ==
			ERROR_SUCCESS)
		{
			if (len > 0)
			{
				cfd = malloc(len + (extra? strlen(extra) : 0) + 1);
				if (cfd)
				{
					if (RegQueryValueEx(key, "CommonFilesDir", NULL, NULL,
						(LPBYTE) cfd, &len) == ERROR_SUCCESS)
					{
						if (cfd[len - 2] != '\\') strcat(cfd, "\\");
						if (extra) strcat(cfd, extra);
					}
					else
					{
						free(cfd);
						cfd = TNULL;
					}
				}
			}
		}
		RegCloseKey(key);
	}
	return cfd;
}

/*****************************************************************************/

TAPTR
TEKlib_Init(TTAGITEM *tags)
{
	return (TAPTR) 1;
}

void
TEKlib_Exit(TAPTR handle)
{
}

/*****************************************************************************/
/*
**	ref/unref
*/

#if defined(ENABLE_LAZY_SINGLETON)
#ifndef _MSC_VER
#warning using globals (-DENABLE_LAZY_SINGLETON)
#endif
static LONG g_lock = 0;
static void *g_handle = TNULL;
#endif

TLIBAPI TAPTR 
TEKlib_DoRef(struct TTask *(*func)(struct TTagItem *), struct TTagItem *tags)
{
#if defined(ENABLE_LAZY_SINGLETON)
	TAPTR handle;
	while (InterlockedIncrement(&g_lock) > 1)
	{
		Sleep(1);
		InterlockedDecrement(&g_lock);
	}
	if (g_handle)
		handle = TExecFindTask(TGetExecBase(g_handle), TNULL);
	else
		handle = g_handle = (*func)(tags);
	InterlockedDecrement(&g_lock);
	return handle;
#else
	return (*func)(tags);
#endif
}

TLIBAPI void 
TEKlib_DoUnref(void (*func)(TAPTR), TAPTR handle)
{
#if defined(ENABLE_LAZY_SINGLETON)
	while (InterlockedIncrement(&g_lock) > 1)
	{
		Sleep(1);
		InterlockedDecrement(&g_lock);
	}
	if (handle == g_handle)
	{
#endif
		(*func)(handle);
#if defined(ENABLE_LAZY_SINGLETON)
		g_handle = TNULL;
	}
	InterlockedDecrement(&g_lock);
#endif
}

/*****************************************************************************/
/*
**	host alloc/free
*/

TAPTR
TEKlib_Alloc(TAPTR boot, TUINT size)
{
	TAPTR mem = malloc(size);
	return mem;
}

void
TEKlib_Free(TAPTR boot, TAPTR mem, TUINT size)
{
	if (mem) free(mem);
}

void
TEKlib_FreeVec(TAPTR boot, TAPTR mem)
{
	if (mem) free(mem);
}

/*****************************************************************************/
/*
**	determine TEKlib global system directory
*/

TSTRPTR
TEKlib_GetSysDir(TAPTR boot, TTAGITEM *tags)
{
	TSTRPTR sysdir;
	TSTRPTR s;

	s = (TSTRPTR) TGetTag(tags, TExecBase_SysDir, TNULL);
	if (s)
	{
		sysdir = TEKlib_Alloc(boot, strlen(s) + 1);
		if (sysdir)
		{
			strcpy(sysdir, s);
		}
	}
	else
	{
		sysdir = getcommondir("tek\\");
	}

	TDBPRINTF(TDB_TRACE,("got sysdir: %s\n", sysdir));
	return sysdir;
}

/*****************************************************************************/
/*
**	determine TEKlib global module directory
*/

TSTRPTR
TEKlib_GetModDir(TAPTR boot, TTAGITEM *tags)
{
	TSTRPTR moddir;
	TSTRPTR s;

	s = (TSTRPTR) TGetTag(tags, TExecBase_ModDir, TNULL);
	if (s)
	{
		moddir = TEKlib_Alloc(boot, strlen(s) + 1);
		if (moddir)
		{
			strcpy(moddir, s);
		}
	}
	else
	{
		moddir = getcommondir("tek\\mod\\");
	}

	TDBPRINTF(TDB_TRACE,("got moddir: %s\n", moddir));
	return moddir;
}

/*****************************************************************************/
/*
**	determine the path to the application, which will
**	later resolve to "PROGDIR:"
*/

TSTRPTR
TEKlib_GetProgDir(TAPTR boot, TTAGITEM *tags)
{
	TSTRPTR progdir;
	TSTRPTR s;
	s = (TSTRPTR) TGetTag(tags, TExecBase_ProgDir, TNULL);
	if (s)
	{
		progdir = TEKlib_Alloc(boot, strlen(s) + 1);
		if (progdir)
		{
			strcpy(progdir, s);
		}
	}
	else
	{
		progdir = malloc(MAX_PATH + 1);
		if (progdir)
		{
			if (GetModuleFileName(NULL, progdir, MAX_PATH + 1))
			{
				TSTRPTR p = progdir;
				while (*p++);
				while (*(--p) != '\\');
				p[1] = 0;
			}
		}
	}

	TDBPRINTF(TDB_TRACE,("got progdir: %s\n", progdir));
	return progdir;
}

/*****************************************************************************/
/*
**	load a module. first try progdir/modname, then moddir
*/

TAPTR
TEKlib_LoadModule(TAPTR boot, TSTRPTR progdir, TSTRPTR moddir, TSTRPTR modname,
	TTAGITEM *tags)
{
	TAPTR knmod = TNULL;
	TINT len1, len2, len3;
	TSTRPTR t;
	struct TInitModule *imod;
	struct ModHandle *handle;

	handle = TEKlib_Alloc(boot, sizeof(struct ModHandle));
	if (!handle) return TNULL;

	imod = lookupmodule(modname, tags);
	if (imod)
	{
		handle->entry = imod->tinm_InitFunc;
		handle->type = TYPE_LIB;
		return handle;
	}

	len1 = progdir? strlen(progdir) : 0;
	len2 = moddir? strlen(moddir) : 0;
	len3 = strlen(modname);

	/* + mod\ + .dll + \0 */
	t = TEKlib_Alloc(boot, TMAX(len1, len2) + len3 + 4 + 4 + 1);
	if (t)
	{
		if (progdir) strcpy(t, progdir);
		strcpy(t + len1, "mod\\");
		strcpy(t + len1 + 4, modname);
		strcpy(t + len1 + 4 + len3, ".dll");

		TDBPRINTF(TDB_INFO,("trying LoadLibrary %s\n", t));
		knmod = LoadLibraryEx(t, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
		if (!knmod)
		{
			if (moddir) strcpy(t, moddir);
			strcpy(t + len2, modname);
			strcpy(t + len2 + len3, ".dll");

			TDBPRINTF(TDB_INFO,("trying LoadLibrary %s\n", t));
			knmod = LoadLibraryEx(t, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
		}

		if (!knmod)
		{
			TDBPRINTF(TDB_WARN,("LoadLibrary %s failed\n", modname));
		}

		TEKlib_Free(boot, t, 0);	/* dummy size */
	}

	if (knmod)
	{
		handle->entry = knmod;
		handle->type = TYPE_DLL;
	}
	else
	{
		TEKlib_Free(boot, handle, 0);	/* dummy size */
		handle = TNULL;
	}

	return handle;
}

/*****************************************************************************/
/*
**	close module
*/

void
TEKlib_CloseModule(TAPTR boot, TAPTR knmod)
{
	struct ModHandle *handle = knmod;
	if (handle->type == TYPE_DLL) FreeLibrary(handle->entry);
	TEKlib_Free(boot, handle, 0);	/* dummy size */
}

/*****************************************************************************/
/*
**	get module entry
*/

TMODINITFUNC
TEKlib_GetEntry(TAPTR boot, TAPTR knmod, TSTRPTR name)
{
	TAPTR initfunc;
	struct ModHandle *handle = knmod;
	if (handle->type == TYPE_LIB) return handle->entry;
	TDBPRINTF(TDB_TRACE,("Looking up symbol: %s\n", name));
	initfunc = GetProcAddress(handle->entry, name);
	if (initfunc == TNULL)
		TDBPRINTF(TDB_ERROR,("Error looking up symbol: %s\n", name));
	return (TMODINITFUNC) initfunc;
}

/*****************************************************************************/
/*
**	call module
*/

TUINT
TEKlib_CallModule(TAPTR boot, TAPTR ModBase, TMODINITFUNC entry, struct TTask *task,
	TAPTR mod, TUINT16 version, TTAGITEM *tags)
{
	return (*entry)(task, mod, version, tags);
}
