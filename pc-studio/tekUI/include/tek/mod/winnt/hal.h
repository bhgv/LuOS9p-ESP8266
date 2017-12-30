
#ifndef _TEK_MOD_WIN32_HAL_H
#define _TEK_MOD_WIN32_HAL_H

/*
**	$Id: hal.h,v 1.1 2006/08/25 21:23:42 tmueller Exp $
**	tek/mod/win32/hal.h - Win32 HAL module internal
*/

#include <tek/exec.h>
#include <tek/mod/hal.h>
#include <stdlib.h>
#include <windows.h>
#include <winreg.h>
#include <process.h>

/*****************************************************************************/

struct HALSpecific
{
	TTAGITEM hsp_Tags[4];
	TSTRPTR hsp_SysDir;
	TSTRPTR hsp_ModDir;
	TSTRPTR hsp_ProgDir;
	DWORD hsp_TLSIndex;
	CRITICAL_SECTION hsp_DevLock;		/* Locking for module base */
	TUINT hsp_RefCount;					/* Open reference counter */
	TAPTR hsp_ExecBase;					/* Inserted at device open */
 	TAPTR hsp_DevTask;					/* Created at device open */
	MMRESULT hsp_Timer;
	struct TList hsp_ReqList;			/* List of pending requests */
	TBOOL hsp_UsePerfCounter;			/* Performance counter available */
	LONGLONG hsp_PerfCStart;			/* Performance counter start */
	LONGLONG hsp_PerfCFreq;				/* Performance counter frequency */
	LONGLONG hsp_PerfCStartDate;		/* Perfcounter calibration date */
};

struct HALThread
{
	TAPTR hth_HALBase;
	HANDLE hth_Thread;
	long hth_ThreadID;
	void *hth_Data;
	void (*hth_Function)(struct TTask *);
	HANDLE hth_SigEvent;				/* Windows Event object */
#ifndef HAL_USE_ATOMICS
	CRITICAL_SECTION hth_SigLock;		/* Protection for sigstate */
	TUINT hth_SigState;					/* State of signals */
#else
	volatile LONG hth_SigState;
#endif
};

struct HALModule
{
	HINSTANCE hmd_Lib;
	TMODINITFUNC hmd_InitFunc;
	TUINT16 hmd_Version;
};

#endif
