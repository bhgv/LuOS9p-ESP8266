
#ifndef _TEK_MOD_POSIX_HAL_H
#define _TEK_MOD_POSIX_HAL_H

/*
**	$Id: hal.h,v 1.2 2006/09/10 14:36:03 tmueller Exp $
**	teklib/tek/mod/hal/posix/hal.h - HAL internal structures on POSIX
**
**	Written by Timm S. Mueller <tmueller@neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
**
**	These structures should be accessed only in platform-specific
**	driver code
*/

#include <tek/exec.h>
#include <pthread.h>
#include <sys/time.h>
#include <tek/mod/time.h>

/*****************************************************************************/

struct HALSpecific
{
	TTAGITEM hsp_Tags[4];				/* Host properties container */
	TSTRPTR hsp_SysDir;					/* Global system directory */
	TSTRPTR hsp_ModDir;					/* Global module directory */
	TSTRPTR hsp_ProgDir;				/* Application local directory */
	pthread_key_t hsp_TSDKey;			/* Thread specific data key */
	pthread_mutex_t hsp_DevLock;		/* Locking for module base */
	pthread_mutex_t hsp_TimeLock;		/* Thanks to localtime() */
	TUINT hsp_RefCount;					/* Open reference counter */
	TAPTR hsp_ExecBase;					/* Inserted at device open */
	TAPTR hsp_DevTask;					/* Created at device open */
	struct TList hsp_ReqList;					/* List of pending requests */
	TINT hsp_TZSec;						/* Seconds west of GMT */
};

struct HALThread
{
	pthread_t hth_PThread;				/* Thread handle */
	void *hth_Data;						/* Task data ptr */
	void (*hth_Function)(struct TTask *);	/* Task function */
	TAPTR hth_HALBase;					/* HAL module base ptr */
	pthread_mutex_t hth_SigMutex;		/* Signal mutex */
	pthread_cond_t hth_SigCond;			/* Signal conditional */
	TUINT hth_SigState;					/* Signal state */
};

struct HALModule
{
	void *hmd_Lib;						/* Host-specific module handle */
	TMODINITFUNC hmd_InitFunc;			/* Initfunction ptr */
	TUINT16 hmd_Version;				/* Module major version */
};

/*****************************************************************************/
/*
**	Revision History
**	$Log: hal.h,v $
**	Revision 1.2  2006/09/10 14:36:03  tmueller
**	removed TNODE, TLIST and THNDL
**
**	Revision 1.1.1.1  2006/08/20 22:15:26  tmueller
**	intermediate import
**
**	Revision 1.8  2006/06/25 22:23:48  tmueller
**	removed TZDays
**
**	Revision 1.7  2006/06/25 15:31:02  tmueller
**	cosmetic
**
**	Revision 1.6  2006/03/11 16:19:45  tmueller
**	AbortIO now interrupts running TimeRequests as well, not just queued ones
**
**	Revision 1.5  2005/11/20 16:08:39  tmueller
**	added stricter funcptr declarations for modentries
**
**	Revision 1.4  2005/09/13 02:45:09  tmueller
**	updated copyright reference
**
**	Revision 1.3  2003/12/20 14:00:18  tmueller
**	hsp_TZSecDays renamed to hsp_TZDays
**
**	Revision 1.2  2003/12/19 14:16:18  tmueller
**	Added hsp_TZSecDays field
**
**	Revision 1.1.1.1  2003/12/11 07:18:00  tmueller
**	Krypton import
**
**	Revision 1.2  2003/10/28 08:52:46  tmueller
**	Reworked HAL-internal structures
**
**	Revision 1.1.1.1  2003/03/08 18:28:40  tmueller
**	Import to new chrooted pserver repository.
**
**	Revision 1.1.1.1  2002/11/30 05:15:33  bifat
**	import
*/

#endif
