
/*
**	$Id: exec_all.c,v 1.2 2006/09/10 14:39:46 tmueller Exp $
**	teklib/src/exec/exec_all.c - Stub to build module from single source
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#ifndef EXPORT
#define EXPORT static TMODAPI
#endif

#ifndef LOCAL
#define LOCAL static
#endif

#ifndef TLIBAPI
#define TLIBAPI static
#endif

#include "../teklib/teklib.c"
#include "../teklib/string.c"
#if defined(TDEBUG) && TDEBUG > 0
#include "../teklib/debug.c"
#endif

#include "exec_mod.c"
#include "exec_api.c"
#include "exec_memory.c"
#include "exec_doexec.c"
#include "exec_time.c"
