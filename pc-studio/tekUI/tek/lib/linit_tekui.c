
#include <stdio.h>

#define loslib_c
#define lobject_c
#define lvm_c
#define lua_c

/* LoadString() conflicts with Windows */
#include "lundump.c"

#if defined(LUA_TEKUI_INCLUDE_CLASS_LIBRARY)

#define EXPORT static TMODAPI
#define LOCAL static
#define TLIBAPI static

#include "../../src/teklib/init.c"
#if defined(TSYS_POSIX)
#include "../../src/teklib/posix/host.c"
#elif defined(TSYS_WINNT)
#include "../../src/teklib/winnt/host.c"
#endif
#include "../../src/teklib/debug.c"
#include "../../src/teklib/teklib.c"
#include "../../src/teklib/string.c"

#include "../../src/visual/visual_all.c"
#include "../../src/exec/exec_all.c"
#include "../../src/hal/hal_mod.c"
#if defined(TSYS_POSIX)
#include "../../src/hal/posix/hal.c"
#elif defined(TSYS_WINNT)
#include "../../src/hal/winnt/hal.c"
#endif

#if defined(TSYS_POSIX)
#include "../../src/display_x11/display_x11_all.c"
/*#include "../../src/display_rawfb/display_rfb_all.c"*/
#elif defined(TSYS_WINNT)
#include "../../src/display_windows/display_win_api.c"
#include "../../src/display_windows/display_win_font.c"
#include "../../src/display_windows/display_win_mod.c"
#endif

#endif


#include "lapi.c"
#include "lauxlib.c"
#include "lbaselib.c"

#if LUA_VERSION_NUM >= 502
#if LUA_VERSION_NUM == 502 || LUA_COMPAT_BITLIB
#include "lbitlib.c"
#endif
#include "lcorolib.c"
#include "lctype.c"
#if LUA_VERSION_NUM >= 503
#include "lutf8lib.c"
#endif
#endif

#include "lcode.c"
#include "ldblib.c"
#include "ldebug.c"
#include "ldo.c"
#include "ldump.c"
#include "lfunc.c"
#include "lgc.c"
#include "liolib.c"
#include "llex.c"
#include "lmathlib.c"
#include "lmem.c"
#include "loadlib.c"
#include "lobject.c"
#include "lopcodes.c"
#include "loslib.c"
#include "lparser.c"
#include "lstate.c"
#include "lstring.c"
#include "lstrlib.c"
#include "ltable.c"
#include "ltablib.c"
#include "ltm.c"
#include "lvm.c"
#include "lzio.c"

#if defined(LUA_TEKUI_INCLUDE_CLASS_LIBRARY)

#include "exec_lua.c"
#include "visual_api.c"
#include "visual_io.c"
#include "visual_lua.c"
#include "region.c"
#include "string.c"
#include "support.c"

#include "../../src/misc/utf8.c"
#include "../../src/misc/region.c"
#include "../../src/misc/pixconv.c"
#include "../../src/misc/cachemanager.c"
#include "../../src/misc/imgcache.c"
#include "../../src/misc/imgload.c"

#if defined(TSYS_POSIX)
#include "display/x11_lua.c"
/*#include "display/rawfb_lua.c"*/
#elif defined(TSYS_WINNT)
#include "display/windows_lua.c"
#endif

#include "../ui/layout/default.c"
#include "../ui/class/area.c"
#include "../ui/class/frame.c"

#endif

#include "linit.c"
