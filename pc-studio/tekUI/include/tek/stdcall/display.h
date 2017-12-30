#ifndef _TEK_STDCALL_DISPLAY_H
#define _TEK_STDCALL_DISPLAY_H

/*
**	$Id: display.h $
**	teklib/tek/stdcall/display.h - display module interface
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#define TDisplayAllocReq(display) \
	(*(((TMODCALL struct TVRequest *(**)(TAPTR))(display))[-9]))(display)

#define TDisplayFreeReq(display,req) \
	(*(((TMODCALL void(**)(TAPTR,struct TVRequest *))(display))[-10]))(display,req)

#endif /* _TEK_STDCALL_DISPLAY_H */
