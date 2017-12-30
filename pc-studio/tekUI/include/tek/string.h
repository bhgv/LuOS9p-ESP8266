#ifndef _TEK_STRING_H
#define _TEK_STRING_H

/*
**	teklib/tek/string.h - string link library functions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>

#ifdef __cplusplus
extern "C" {
#endif

TLIBAPI TSIZE TStrLen(TSTRPTR s);
TLIBAPI TSTRPTR TStrCpy(TSTRPTR d, TSTRPTR s);
TLIBAPI TSTRPTR TStrNCpy(TSTRPTR d, TSTRPTR s, TSIZE maxlen);
TLIBAPI TSTRPTR TStrCat(TSTRPTR d, TSTRPTR s);
TLIBAPI TSTRPTR TStrNCat(TSTRPTR d, TSTRPTR s, TSIZE maxlen);
TLIBAPI TINT TStrCmp(TSTRPTR s, TSTRPTR s2);
TLIBAPI TINT TStrNCmp(TSTRPTR s, TSTRPTR s2, TSIZE maxlen);
TLIBAPI TINT TStrCaseCmp(TSTRPTR s1, TSTRPTR s2);
TLIBAPI TINT TStrNCaseCmp(TSTRPTR s1, TSTRPTR s2, TSIZE maxlen);
TLIBAPI TSTRPTR TStrStr(TSTRPTR a, TSTRPTR b);
TLIBAPI TSTRPTR TStrChr(TSTRPTR s, TINT c);
TLIBAPI TSTRPTR TStrRChr(TSTRPTR s, TINT c);
TLIBAPI TINT TStrToI(TSTRPTR s, TINT *valp);

#ifdef __cplusplus
}
#endif

#endif /* _TEK_STRING */
