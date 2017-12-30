#ifndef _TEK_MOD_DISPLAY_H
#define _TEK_MOD_DISPLAY_H

/*
**	tek/mod/display.h - Display driver interface
*/

#include <tek/exec.h>

struct TDisplayBase;

typedef struct
{
	TAPTR Base;
	TAPTR ExecBase;
	TBOOL IsBase;
	struct TModInitNode InitModules;

} TEKDisplay;

#define DISPLAY_STRHELP(x) #x
#define DISPLAY_NAME(x) DISPLAY_STRHELP(x)

#endif
