#ifndef _TEK_CONFIG_H
#define	_TEK_CONFIG_H

/*
**	teklib/tek/config.h - Platform and compiler specific
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#if defined(TSYS_POSIX) || defined(__unix__) || defined(unix) \
	|| defined(__linux) || defined(__FreeBSD__) || defined(__NetBSD__) \
	|| defined(__APPLE__)
	#ifndef TSYS_POSIX
		#define TSYS_POSIX
	#endif
	#include <tek/config/posix.h>
#elif defined(TSYS_WINNT) || defined(_WIN32)
	#ifndef TSYS_WINNT
		#define TSYS_WINNT
	#endif
	#include <tek/config/winnt.h>
#elif defined(TSYS_PS2)
	#include <tek/config/ps2.h>
#endif

/*****************************************************************************/
/*
**	Module interface
**
**	- Depending on your kind of module build, you may possibly want to
**	  set TMODAPI to "static". Note that module functions are not exported
**	  symbolically in TEKlib.
**
**	- Depending on your platform, you may have to override TMODENTRY with
**	  declarations like __declspec(dllexport). See config/ for examples.
**
**	- Depending on your preferred calling conventions, define TMODCALL to
**	  declare __stdargs, __fastcall etc. Some platforms allow (or require)
**	  to distinguish register and stack-based calling. When porting TEKlib
**	  to a new platform, try to ascertain the calling conventions for the
**	  entire platform, not only for a single compiler.
*/

#ifndef TMODAPI
#define TMODAPI
#endif

#ifndef TLIBAPI
#define TLIBAPI
#endif

#ifndef TMODCALL
#define TMODCALL
#endif

#ifndef TMODENTRY
#define TMODENTRY
#endif

#ifndef TMODINTERN
#define TMODINTERN
#endif

/*****************************************************************************/
/*
**	Callback, hook and task entries
**
**	In config/yourplatform.h, override these with your preferred
**	platform-specific calling conventions. See also the annotations
**	for the module interface above. Stack/register calling conventions
**	may be an issue here.
*/

#ifndef TCALLBACK
#define TCALLBACK
#endif

#ifndef THOOKENTRY
#define THOOKENTRY
#endif

#ifndef TTASKENTRY
#define TTASKENTRY
#endif

/*****************************************************************************/
/*
**	Inline
**	Override with compiler-specific declarations if available
*/

#ifndef TINLINE
#define TINLINE
#endif

#endif
