#ifndef _TEK_TYPE_H
#define	_TEK_TYPE_H

/*
**	teklib/tek/type.h - Basic types, constants, macros
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/config.h>

/*****************************************************************************/
/*
**	Aliased types
*/

/* 32bit signed integer: */
typedef TINT32 TINT;
/* 32bit unsigned integer: */
typedef TUINT32 TUINT;
/* Boolean: */
typedef TUINT TBOOL;
/* Tag type; integers and pointers: */
typedef TINTPTR TTAG;
/* Size type: */
typedef TUINTPTR TSIZE;
/* Stringptr type: */
typedef TCHR * TSTRPTR;

/* Module function pointer type: */
typedef TMODCALL TINT (*TMFPTR)(TAPTR);

/*****************************************************************************/
/*
**	TagItem - key, value pair
*/

struct TTagItem
{
	/* Tag identifier: */
	TUINT tti_Tag;
	/* Tag value item: */
	TTAG tti_Value;
};

typedef struct TTagItem TTAGITEM;

/* User tag item */
#define TTAG_USER 0x80000000
/* Taglist ends with this item */
#define TTAG_DONE 0x00000000
/* This item is being ignored */
#define TTAG_IGNORE 0x00000001
/* List continues at tti_Value */
#define TTAG_MORE 0x00000002
/* Skip this plus tti_Value items */
#define TTAG_SKIP 0x00000003
/* Traverse sublist in tti_Value */
#define TTAG_GOSUB 0x00000004

/*****************************************************************************/
/*
**	Constants
*/

#define TNULL 0
#define TTRUE 1
#define TFALSE 0
#define TPI (3.14159265358979323846)

/*****************************************************************************/
/*
**	Macros
*/

#define TABS(a) ((a) > 0 ? (a) : -(a))
#define TMIN(a,b) ((a) < (b) ? (a) : (b))
#define TMAX(a,b) ((a) > (b) ? (a) : (b))
#define TCLAMP(min,x,max) ((x) > (max) ? (max) : ((x) < (min) ? (min) : (x)))

#endif
