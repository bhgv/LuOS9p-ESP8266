
/*
**	teklib/src/visual/visual_all.c - Stub to build module from single source
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

#include "visual_mod.c"
#include "visual_api.c"
