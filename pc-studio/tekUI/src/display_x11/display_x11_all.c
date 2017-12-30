
/*
**	display_x11_all.c - Stub to build module from single source
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

#if !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE 500
#endif

#include "display_x11_mod.c"
#include "display_x11_api.c"
#include "display_x11_inst.c"
#include "display_x11_font.c"
