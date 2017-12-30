#ifndef _TEKUI_H
#define _TEKUI_H

#include <tek/exec.h>

#define TEKUI_HUGE				1000000

#define TEKUI_FL_LAYOUT			0x00001
#define TEKUI_FL_REDRAW			0x00002
#define TEKUI_FL_REDRAWBORDER	0x00004
#define TEKUI_FL_SETUP			0x00008
#define TEKUI_FL_SHOW			0x00010
#define TEKUI_FL_CHANGED		0x00020
#define TEKUI_FL_POPITEM		0x00040
#define TEKUI_FL_UPDATE			0x00080
#define TEKUI_FL_RECVINPUT		0x00100
#define TEKUI_FL_RECVMOUSEMOVE	0x00200
#define TEKUI_FL_CURSORFOCUS	0x00400
#define TEKUI_FL_AUTOPOSITION	0x00800
#define TEKUI_FL_ERASEBG		0x01000
#define TEKUI_FL_TRACKDAMAGE	0x02000
#define TEKUI_FL_ACTIVATERMB	0x04000
#define TEKUI_FL_INITIALFOCUS	0x08000
#define TEKUI_FL_ISWINDOW		0x10000
#define TEKUI_FL_DONOTBLIT		0x20000

#define TEK_UI_OVERLAP(d0, d1, d2, d3, s0, s1, s2, s3) \
((s2) >= (d0) && (s0) <= (d2) && (s3) >= (d1) && (s1) <= (d3))

#define TEK_UI_OVERLAPRECT(d, s) \
TEK_UI_OVERLAP((d)[0], (d)[1], (d)[2], (d)[3], (s)[0], (s)[1], (s)[2], (s)[3])

#endif
