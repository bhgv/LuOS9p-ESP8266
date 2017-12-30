#ifndef _TEK_STDCALL_VISUAL_H
#define _TEK_STDCALL_VISUAL_H

/*
**  teklib/tek/stdcall/visual.h - visual module interface
**
**  See copyright notice in teklib/COPYRIGHT
*/

#define TVisualOpen(visual,tags) \
    (*(((TMODCALL TAPTR(**)(TAPTR,TTAGITEM *))(visual))[-9]))(visual,tags)

#define TVisualClose(visual,inst) \
    (*(((TMODCALL TAPTR(**)(TAPTR,TAPTR))(visual))[-10]))(visual,inst)

#define TVisualAttach(visual,tags) \
    (*(((TMODCALL TAPTR(**)(TAPTR,TTAGITEM *))(visual))[-11]))(visual,tags)

#define TVisualOpenFont(visual,tags) \
    (*(((TMODCALL TAPTR(**)(TAPTR,TTAGITEM *))(visual))[-12]))(visual,tags)

#define TVisualCloseFont(visual,font) \
    (*(((TMODCALL void(**)(TAPTR,TAPTR))(visual))[-13]))(visual,font)

#define TVisualGetFontAttrs(visual,font,tags) \
    (*(((TMODCALL TINT(**)(TAPTR,TAPTR,TTAGITEM *))(visual))[-14]))(visual,font,tags)

#define TVisualTextSize(visual,font,text,numchars) \
    (*(((TMODCALL TINT(**)(TAPTR,TAPTR,TSTRPTR,TINT))(visual))[-15]))(visual,font,text,numchars)

#define TVisualQueryFonts(visual,tags) \
    (*(((TMODCALL TAPTR(**)(TAPTR,TTAGITEM *))(visual))[-16]))(visual,tags)

#define TVisualGetNextFont(visual,fqhandle) \
    (*(((TMODCALL TTAGITEM *(**)(TAPTR,TAPTR))(visual))[-17]))(visual,fqhandle)

#define TVisualGetPort(visual) \
    (*(((TMODCALL TAPTR(**)(TAPTR))(visual))[-18]))(visual)

#define TVisualSetInput(visual,clearflags,setflags) \
    (*(((TMODCALL TUINT(**)(TAPTR,TUINT,TUINT))(visual))[-19]))(visual,clearflags,setflags)

#define TVisualGetAttrs(visual,tags) \
    (*(((TMODCALL TUINT(**)(TAPTR,TTAGITEM *))(visual))[-20]))(visual,tags)

#define TVisualSetAttrs(visual,tags) \
    (*(((TMODCALL TUINT(**)(TAPTR,TTAGITEM *))(visual))[-21]))(visual,tags)

#define TVisualAllocPen(visual,rgb) \
    (*(((TMODCALL TVPEN(**)(TAPTR,TUINT))(visual))[-22]))(visual,rgb)

#define TVisualFreePen(visual,pen) \
    (*(((TMODCALL void(**)(TAPTR,TVPEN))(visual))[-23]))(visual,pen)

#define TVisualSetFont(visual,font) \
    (*(((TMODCALL void(**)(TAPTR,TAPTR))(visual))[-24]))(visual,font)

#define TVisualClear(visual,pen) \
    (*(((TMODCALL void(**)(TAPTR,TVPEN))(visual))[-25]))(visual,pen)

#define TVisualRect(visual,x,y,w,h,pen) \
    (*(((TMODCALL void(**)(TAPTR,TINT,TINT,TINT,TINT,TVPEN))(visual))[-26]))(visual,x,y,w,h,pen)

#define TVisualFRect(visual,x,y,w,h,pen) \
    (*(((TMODCALL void(**)(TAPTR,TINT,TINT,TINT,TINT,TVPEN))(visual))[-27]))(visual,x,y,w,h,pen)

#define TVisualLine(visual,x1,y1,x2,y2,pen) \
    (*(((TMODCALL void(**)(TAPTR,TINT,TINT,TINT,TINT,TVPEN))(visual))[-28]))(visual,x1,y1,x2,y2,pen)

#define TVisualPlot(visual,x,y,pen) \
    (*(((TMODCALL void(**)(TAPTR,TINT,TINT,TVPEN))(visual))[-29]))(visual,x,y,pen)

#define TVisualText(visual,x,y,text,len,fg) \
    (*(((TMODCALL void(**)(TAPTR,TINT,TINT,TSTRPTR,TINT,TVPEN))(visual))[-30]))(visual,x,y,text,len,fg)

#define TVisualDrawStrip(visual,array,num,x,y,w,h,iscursor,tags) \
    (*(((TMODCALL void(**)(TAPTR,TINT *,TINT,TINT,TINT,TINT,TINT,TINT,TTAGITEM *))(visual))[-31]))(visual,array,num,x,y,w,h,iscursor,tags)

#define TVisualDrawTags(visual,tags) \
    (*(((TMODCALL void(**)(TAPTR,TTAGITEM *))(visual))[-32]))(visual,tags)

#define TVisualDrawFan(visual,array,num,x,y,w,h,iscursor,tags) \
    (*(((TMODCALL void(**)(TAPTR,TINT *,TINT,TINT,TINT,TINT,TINT,TINT,TTAGITEM *))(visual))[-33]))(visual,array,num,x,y,w,h,iscursor,tags)

#define TVisualCopyArea(visual,x,y,w,h,dx,dy,tags) \
    (*(((TMODCALL void(**)(TAPTR,TINT,TINT,TINT,TINT,TINT,TINT,TTAGITEM *))(visual))[-34]))(visual,x,y,w,h,dx,dy,tags)

#define TVisualDrawBuffer(visual,x,y,buf,w,h,totw,tags) \
    (*(((TMODCALL void(**)(TAPTR,TINT,TINT,TAPTR,TINT,TINT,TINT,TTAGITEM *))(visual))[-35]))(visual,x,y,buf,w,h,totw,tags)

#define TVisualSetClipRect(visual,x,y,w,h,tags) \
    (*(((TMODCALL void(**)(TAPTR,TINT,TINT,TINT,TINT,TTAGITEM *))(visual))[-36]))(visual,x,y,w,h,tags)

#define TVisualUnsetClipRect(visual) \
    (*(((TMODCALL void(**)(TAPTR))(visual))[-37]))(visual)

#define TVisualOpenDisplay(visual,tags) \
    (*(((TMODCALL TAPTR(**)(TAPTR,TTAGITEM *))(visual))[-38]))(visual,tags)

#define TVisualCloseDisplay(visual,display) \
    (*(((TMODCALL void(**)(TAPTR,TAPTR))(visual))[-39]))(visual,display)

#define TVisualQueryDisplays(visual,tags) \
    (*(((TMODCALL TAPTR(**)(TAPTR,TTAGITEM *))(visual))[-40]))(visual,tags)

#define TVisualGetNextDisplay(visual,handle) \
    (*(((TMODCALL TTAGITEM *(**)(TAPTR,TAPTR))(visual))[-41]))(visual,handle)

#define TVisualGetSelection(visual,tags) \
    (*(((TMODCALL TAPTR *(**)(TAPTR,TTAGITEM *))(visual))[-42]))(visual,tags)

#define TVisualSetSelection(visual,sel,len,tags) \
    (*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TSIZE,TTAGITEM *))(visual))[-43]))(visual,sel,len,tags)

#endif /* _TEK_STDCALL_VISUAL_H */
