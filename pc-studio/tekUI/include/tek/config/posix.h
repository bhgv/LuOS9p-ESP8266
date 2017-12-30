#ifndef _TEK_CONFIG_POSIX_H
#define _TEK_CONFIG_POSIX_H

/*
**	teklib/tek/config/posix.h - POSIX configuration
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

/*****************************************************************************/
/*
**	Elementary types
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef char				TCHR;
typedef void				TVOID;
typedef void *				TAPTR;
typedef int8_t				TINT8;
typedef uint8_t				TUINT8;
typedef int16_t				TINT16;
typedef uint16_t			TUINT16;
typedef int32_t				TINT32;
typedef uint32_t			TUINT32;
typedef int64_t				TINT64;
typedef	uint64_t			TUINT64;
typedef float				TFLOAT;
typedef	double				TDOUBLE;
typedef intptr_t			TINTPTR;
typedef uintptr_t			TUINTPTR;

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

struct THALObject { TUINTPTR tho_Chunk[4]; };

/*****************************************************************************/
/*
**	Date type container
*/

typedef union { TINT64 tdt_Int64; } TDATE_T;

/*****************************************************************************/
/*
**	Debug support
*/

#define TDEBUG_PLATFORM_PUTS(s) do { fputs(s, stderr); fflush(stderr); } while(0)
#define TDEBUG_PLATFORM_FATAL() (abort(), 0)

/*****************************************************************************/
/*
**	Default locations
*/

#if !defined(TEKHOST_SYSDIR)
#define TEKHOST_SYSDIR		"/usr/local/tek/"
#endif /* !defined(TEKHOST_SYSDIR) */

#if !defined(TEKHOST_MODDIR)
#define TEKHOST_MODDIR		TEKHOST_SYSDIR "mod/"
#endif /* !defined(TEKHOST_MODDIR) */

#if !defined(TEKHOST_PROGDIR)
#define TEKHOST_PROGDIR		TNULL
#endif /* !defined(TEKHOST_PROGDIR) */

/*****************************************************************************/
/*
**	Module name extension
*/

#define TEKHOST_EXTSTR		".so"
#define TEKHOST_EXTLEN		3

/*****************************************************************************/
/*
**	Inline
*/

#define TINLINE __inline

/*****************************************************************************/
/*
**	Calling conventions and visibility
*/

#if defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && \
    defined(__ELF__)
#define TMODENTRY __attribute__ ((visibility("default")))
#define TMODINTERN __attribute__ ((visibility("hidden")))
#endif

#endif
