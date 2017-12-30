#ifndef _TEK_TEKLIB_H
#define _TEK_TEKLIB_H

/*
**	teklib/tek/teklib.h - teklib link library functions
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>

#ifdef __cplusplus
extern "C" {
#endif

TLIBAPI struct TTask *TEKCreate(struct TTagItem *tags);
TLIBAPI void TAddHead(struct TList *list, struct TNode *node);
TLIBAPI void TAddTail(struct TList *list, struct TNode *node);
TLIBAPI void TRemove(struct TNode *node);
TLIBAPI struct TNode *TRemHead(struct TList *list);
TLIBAPI struct TNode *TRemTail(struct TList *list);
TLIBAPI void TDestroy(struct THandle *handle);
TLIBAPI void TInitList(struct TList *list);
TLIBAPI void TInsert(struct TList *list, struct TNode *node, struct TNode *prednode);
TLIBAPI void TNodeUp(struct TNode *node);
TLIBAPI TTAG TGetTag(TTAGITEM *taglist, TUINT tag, TTAG defvalue);
TLIBAPI void TDestroyList(struct TList *list);
TLIBAPI struct TModule *TNewInstance(struct TModule *mod, TSIZE possize, TSIZE negsize);
TLIBAPI void TFreeInstance(struct TModule *mod);
TLIBAPI void TInitVectors(struct TModule *mod, const TMFPTR *vectors, TUINT numv);
TLIBAPI struct THandle *TFindHandle(struct TList *list, TSTRPTR name);
TLIBAPI TBOOL TForEachTag(struct TTagItem *taglist, struct THook *hook);
TLIBAPI void TInitHook(struct THook *hook, THOOKFUNC func, TAPTR data);
TLIBAPI TTAG TCallHookPkt(struct THook *hook, TAPTR obj, TTAG msg);
TLIBAPI void TAddTime(TTIME *a, TTIME *b);
TLIBAPI void TSubTime(TTIME *a, TTIME *b);
TLIBAPI TINT TCmpTime(TTIME *a, TTIME *b);
TLIBAPI void TAddDate(TDATE *d, TINT ndays, TTIME *tm);
TLIBAPI void TSubDate(TDATE *d, TINT ndays, TTIME *tm);
TLIBAPI TINT TDiffDate(TDATE *d1, TDATE *d2, TTIME *tm);
TLIBAPI TBOOL TCreateTime(TTIME *t, TINT d, TINT s, TINT us);
TLIBAPI TBOOL TExtractTime(TTIME *t, TINT *d, TINT *s, TINT *us);
TLIBAPI void TInitInterface(struct TInterface *iface, struct TModule *mod, TSTRPTR name, TUINT16 version);
TLIBAPI TAPTR TGetNextEntry(struct THandle *handle);
TLIBAPI TAPTR TEKlib_DoRef(struct TTask *(*)(struct TTagItem *), struct TTagItem *tags);
TLIBAPI void TEKlib_DoUnref(void (*func)(TAPTR), TAPTR handle);

#ifdef __cplusplus
}
#endif

#endif /* _TEK_TEKLIB */
