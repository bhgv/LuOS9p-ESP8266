#ifndef _TEK_DEBUG_H
#define _TEK_DEBUG_H

/*
**	tek/debug.h - Debug support
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/type.h>

#if defined(TDEBUG) && TDEBUG > 0
	TLIBAPI TINT TDebugPutS(TSTRPTR s);
	TLIBAPI TINT TDebugPrintF(TSTRPTR fmt, ...);
	TLIBAPI TINT TDebugFatal(void);
	#define TDB(level, x) ((level) >= (TDEBUG) ? ((x), (void)0) : (void)0)
	#define TDBPUTS(level, s) TDB(level, TDebugPutS(s))
	#define TDBFATAL() TDEBUG_PLATFORM_FATAL()
	#define TDBPRINTF(level, x) \
		TDB(level, (TDebugPrintF("(%02d %s:%d) ", level, __FILE__, __LINE__), \
		TDebugPrintF x))
	#define TDBASSERT(level, expr) TDB(level, (expr) ? (void)0 : \
		((TDebugPrintF("(%02d %s:%d) ", level, __FILE__, __LINE__), \
		TDebugPrintF(#expr " : assertion failed\n"), TDEBUG_PLATFORM_FATAL()), \
		(void)0))
#else
	#define TDB(level, x) ((void)0)
	#define TDBPRINTF(level, x) ((void)0)
	#define TDBFATAL() ((void)0)
	#define TDBASSERT(level, expr) ((void)0)
#endif

#define TDB_DEBUG	1
#define TDB_TRACE	2
#define TDB_INFO	4
#define TDB_WARN	5
#define TDB_ERROR	10
#define TDB_FAIL	20

#endif
