#ifndef _TEK_CONFIG_WINNT_H
#define _TEK_CONFIG_WINNT_H

/*
**	tek/config/winnt.h - Windows platform specific
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <windows.h>

typedef char				TCHR;
typedef void				TVOID;
typedef void *				TAPTR;
typedef signed char			TINT8;
typedef unsigned char		TUINT8;
typedef signed short		TINT16;
typedef unsigned short		TUINT16;
typedef signed int			TINT32;
typedef unsigned int		TUINT32;
typedef LONGLONG			TINT64;
typedef ULONGLONG			TUINT64;
typedef float				TFLOAT;
typedef	double				TDOUBLE;
typedef INT_PTR				TINTPTR;
typedef UINT_PTR			TUINTPTR;

#define TSYS_HAVE_INT64

/*****************************************************************************/
/*
**	Alignment of allocations
*/

struct TMemNodeAlign { TUINT8 tmna_Chunk[8]; };
struct TMemManagerInfoAlign { TUINT8 tmua_Chunk[8]; };
struct TMemHeadAlign { TUINT8 tmha_Chunk[48]; };

/*****************************************************************************/
/*
**	HAL Object container
*/

struct THALObject { TUINTPTR data[4]; };

/*****************************************************************************/
/*
**	Date type container
*/

typedef union { TINT64 tdt_Int64; } TDATE_T;

/*****************************************************************************/
/*
**	Debug support
*/

#include <stdio.h>
#define TDEBUG_PLATFORM_PUTS(s) fputs(s, stderr)
#define TDEBUG_PLATFORM_FATAL() (abort(), 0)

/*****************************************************************************/
/*
**	Inline
*/

#define TINLINE __inline

/*****************************************************************************/
/*
**	Calling conventions and visibility
*/

#ifndef TMODENTRY
#define TMODENTRY __declspec(dllexport)
#endif

#endif
