#ifndef _TEK_PROTO_EXEC_H
#define _TEK_PROTO_EXEC_H

/*
**	$Id: exec.h,v 1.1.1.1 2006/08/20 22:15:26 tmueller Exp $
**	teklib/tek/proto/exec.h - Exec module prototypes
**
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>
#include <tek/mod/ioext.h>
#include <tek/stdcall/exec.h>

extern TMODENTRY TUINT
tek_init_exec(struct TTask *, struct TModule *, TUINT16, TTAGITEM *);

#endif /* _TEK_PROTO_EXEC_H */
