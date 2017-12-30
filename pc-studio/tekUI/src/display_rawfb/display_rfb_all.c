
/*
**	display_rfb_all.c - Stub to build module from single source
**	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#ifndef EXPORT
#define EXPORT static TMODAPI
#endif

#ifndef LOCAL
#define LOCAL static
#endif

#include "display_rfb_mod.c"
#include "display_rfb_api.c"
#include "display_rfb_font.c"
#include "display_rfb_draw.c"
#if defined(ENABLE_LINUXFB)
#include "display_rfb_linux.c"
#endif
#if defined(ENABLE_VNCSERVER)
#include "vnc/display_rfb_vnc.c"
#endif
