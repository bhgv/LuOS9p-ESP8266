#ifndef _TEK_MOD_VISUAL_H
#define _TEK_MOD_VISUAL_H

/*
**  teklib/tek/mod/visual.h - Visual module definitions
**  See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>
#include <tek/mod/time.h>

/*****************************************************************************/
/*
**  Forward declarations
*/

/* Visual module base structure: */
struct TVisualBase;

/*****************************************************************************/
/*
**  Types
*/

typedef TTAG TVPEN;
#define TVPEN_UNDEFINED             ((TVPEN)(-1))

/*****************************************************************************/
/*
**  Tags
*/

#define TVISTAGS_                   (TTAG_USER + 0x600)

#define TVisual_Display             (TVISTAGS_ + 0x001)
#define TVisual_DisplayName         (TVISTAGS_ + 0x002)
#define TVisual_NewInstance         (TVISTAGS_ + 0x003)
#define TVisual_Width               (TVISTAGS_ + 0x004)
#define TVisual_Height              (TVISTAGS_ + 0x005)
#define TVisual_Title               (TVISTAGS_ + 0x006)
#define TVisual_WinLeft             (TVISTAGS_ + 0x007)
#define TVisual_WinTop              (TVISTAGS_ + 0x008)
#define TVisual_MinWidth            (TVISTAGS_ + 0x009)
#define TVisual_MinHeight           (TVISTAGS_ + 0x00a)
#define TVisual_MaxWidth            (TVISTAGS_ + 0x00b)
#define TVisual_MaxHeight           (TVISTAGS_ + 0x00c)
#define TVisual_Attach              (TVISTAGS_ + 0x00d)
#define TVisual_Center              (TVISTAGS_ + 0x00e)
#define TVisual_Borderless          (TVISTAGS_ + 0x00f)
#define TVisual_FullScreen          (TVISTAGS_ + 0x010)
#define TVisual_EventMask           (TVISTAGS_ + 0x011)
#define TVisual_BlankCursor         (TVISTAGS_ + 0x012)
#define TVisual_HaveSelection       (TVISTAGS_ + 0x013)
#define TVisual_HaveClipboard       (TVISTAGS_ + 0x014)
#define TVisual_SelectionType       (TVISTAGS_ + 0x015)
#define TVisual_PixelFormat         (TVISTAGS_ + 0x016)
#define TVisual_AlphaChannel        (TVISTAGS_ + 0x017)
#define TVisual_PopupWindow         (TVISTAGS_ + 0x018)
#define TVisual_ScreenWidth         (TVISTAGS_ + 0x019)
#define TVisual_ScreenHeight        (TVISTAGS_ + 0x01a)
#define TVisual_SelectionLength     (TVISTAGS_ + 0x01b)

#define TVisual_FontName            (TVISTAGS_ + 0x100)
#define TVisual_FontPxSize          (TVISTAGS_ + 0x101)
#define TVisual_FontItalic          (TVISTAGS_ + 0x102)
#define TVisual_FontBold            (TVISTAGS_ + 0x103)
#define TVisual_FontScaleable       (TVISTAGS_ + 0x104)
#define TVisual_FontAscent          (TVISTAGS_ + 0x105)
#define TVisual_FontDescent         (TVISTAGS_ + 0x106)
#define TVisual_FontUlPosition      (TVISTAGS_ + 0x107)
#define TVisual_FontUlThickness     (TVISTAGS_ + 0x108)
#define TVisual_FontNumResults      (TVISTAGS_ + 0x109)
#define TVisual_FontHeight          (TVISTAGS_ + 0x10a)
#define TVisual_FontWidth           (TVISTAGS_ + 0x10b)

#define TVisual_Pen                 (TVISTAGS_ + 0x10c)
#define TVisual_PenArray            (TVISTAGS_ + 0x10d)

#define TVisual_ExposeHook          (TVISTAGS_ + 0x10e)

#define TVisual_Modulo              (TVISTAGS_ + 0x10f)
#define TVisual_Format              (TVISTAGS_ + 0x110)
#define TVisual_BufPtr              (TVISTAGS_ + 0x111)
#define TVisual_DriverName          (TVISTAGS_ + 0x112)
#define TVisual_Device              (TVISTAGS_ + 0x113)
#define TVisual_Window              (TVISTAGS_ + 0x114)
#define TVisual_UserData            (TVISTAGS_ + 0x115)
#define TVisual_CmdRPort            (TVISTAGS_ + 0x116)
#define TVisual_IMsgPort            (TVISTAGS_ + 0x117)
#define TVisual_CacheRequest        (TVISTAGS_ + 0x118)
#define TVisual_ExtraArgs           (TVISTAGS_ + 0x119)
#define TVisual_HaveWindowManager   (TVISTAGS_ + 0x11a)
#define TVisual_WindowHints         (TVISTAGS_ + 0x11b)

/* Tagged rendering: */

#define TVisualDraw_Command         (TVISTAGS_ + 0x300)
#define TVisualDraw_X0              (TVISTAGS_ + 0x301)
#define TVisualDraw_Y0              (TVISTAGS_ + 0x302)
#define TVisualDraw_X1              (TVISTAGS_ + 0x303)
#define TVisualDraw_Y1              (TVISTAGS_ + 0x304)
#define TVisualDraw_NewX            (TVISTAGS_ + 0x305)
#define TVisualDraw_NewY            (TVISTAGS_ + 0x306)
#define TVisualDraw_FgPen           (TVISTAGS_ + 0x307)
#define TVisualDraw_BgPen           (TVISTAGS_ + 0x308)

/*****************************************************************************/
/*
**  Visual request
*/

struct TVRequest
{
    struct TIORequest tvr_Req;
    struct {
        TINT x; TINT y; TINT w; TINT h; TINT iscursor; 
    } tvr_cursor;
    union
    {
        struct { TAPTR Window; TTAGITEM *Tags; struct TMsgPort *IMsgPort; }
            OpenWindow;
        struct { TAPTR Window; } CloseWindow;
        struct { TAPTR Font; TTAGITEM *Tags; } OpenFont;
        struct { TAPTR Font; } CloseFont;
        struct { TAPTR Font; TTAGITEM *Tags; TUINT Num; } GetFontAttrs;
        struct { TAPTR Font; TSTRPTR Text; TINT NumChars; TINT Width; } 
            TextSize;
        struct { TAPTR Handle; TTAGITEM *Tags; } QueryFonts;
        struct { TAPTR Handle; TTAGITEM *Attrs; } GetNextFont;
        struct { TAPTR Window; TUINT Mask; TUINT OldMask; } SetInput;
        struct { TAPTR Window; TTAGITEM *Tags; TUINT Num; } GetAttrs;
        struct { TAPTR Window; TTAGITEM *Tags; TUINT Num; } SetAttrs;
        struct { TAPTR Window; TUINT RGB; TVPEN Pen; } AllocPen;
        struct { TAPTR Window; TVPEN Pen; } FreePen;
        struct { TAPTR Font; TAPTR Window; } SetFont;
        struct { TAPTR Window; TVPEN Pen; } Clear;
        struct { TAPTR Window; TINT Rect[4]; TVPEN Pen; } Rect;
        struct { TAPTR Window; TINT Rect[4]; TVPEN Pen; } FRect;
        struct { TAPTR Window; TINT Rect[4]; TVPEN Pen; } Line;
        struct { TAPTR Window; TINT Rect[2]; TVPEN Pen; } Plot;
        struct { TAPTR Window; TINT X, Y; TSTRPTR Text; TINT Length;
            TVPEN FgPen, BgPen; } Text;
        struct { TAPTR Window; TINT *Array; TINT Num; 
//                        TINT x; TINT y; TINT w; TINT h; TINT iscursor; 
                        TTAGITEM *Tags; } Strip;
        struct { TAPTR Window; TTAGITEM *Tags; } DrawTags;
        struct { TAPTR Window; TINT *Array; TINT Num; 
//                        TINT x; TINT y; TINT w; TINT h; TINT iscursor; 
                        TTAGITEM *Tags; } Fan;
        struct { TAPTR Window; TINT Rect[4]; TINT DestX; TINT DestY;
            TTAGITEM *Tags; } CopyArea;
        struct { TAPTR Window; TINT Rect[4]; TTAGITEM *Tags; } ClipRect;
        struct { TAPTR Window; TINT RRect[4]; TAPTR Buf; TINT TotWidth;
            TTAGITEM *Tags; } DrawBuffer;
        struct { TAPTR Window; TINT Rect[4]; } Flush;
        struct { TAPTR Window; TUINT Type; TAPTR Data; TSIZE Length; } GetSelection;
        struct { TAPTR Window; TUINT Type; TAPTR Data; TSIZE Length; } SetSelection;
    } tvr_Op;
};

/* Command codes: */
#define TVCMD_OPENWINDOW    0x1001
#define TVCMD_CLOSEWINDOW   0x1002
#define TVCMD_OPENFONT      0x1003
#define TVCMD_CLOSEFONT     0x1004
#define TVCMD_GETFONTATTRS  0x1005
#define TVCMD_TEXTSIZE      0x1006
#define TVCMD_QUERYFONTS    0x1007
#define TVCMD_GETNEXTFONT   0x1008
#define TVCMD_SETINPUT      0x1009
#define TVCMD_GETATTRS      0x100a
#define TVCMD_SETATTRS      0x100b
#define TVCMD_ALLOCPEN      0x100c
#define TVCMD_FREEPEN       0x100d
#define TVCMD_SETFONT       0x100e
#define TVCMD_CLEAR         0x100f
#define TVCMD_RECT          0x1010
#define TVCMD_FRECT         0x1011
#define TVCMD_LINE          0x1012
#define TVCMD_PLOT          0x1013
#define TVCMD_TEXT          0x1014
#define TVCMD_DRAWSTRIP     0x1015
#define TVCMD_DRAWTAGS      0x1016
#define TVCMD_DRAWFAN       0x1017
#define TVCMD_DRAWARC       0x1018
#define TVCMD_COPYAREA      0x1019
#define TVCMD_SETCLIPRECT   0x101a
#define TVCMD_UNSETCLIPRECT 0x101b
#define TVCMD_DRAWFARC      0x101c
#define TVCMD_DRAWBUFFER    0x101d
#define TVCMD_FLUSH         0x101e
#define TVCMD_GETSELECTION  0x101f
#define TVCMD_SETSELECTION  0x1020
#define TVCMD_EXTENDED      0x2000

/*****************************************************************************/
/*
**  Pixel formats
**
**  "native" endianness (register of said width stored in memory)
**  unless otherwise noted
**                                 _ order of color components 
**                                | _ bytes per pixel
**                                || _ flags: 8=reserved 4=has_alpha
**                                ||| _ format identifying number
**                                ||||
**                                vvvv   */
#define TVPIXFMT_UNDEFINED  0x00000000
#define TVPIXFMT_CLUT8      0x00000100
#define TVPIXFMT_08R8G8B8   0x88801401
#define TVPIXFMT_A8R8G8B8   0x88801442
#define TVPIXFMT_08B8G8R8   0x88802403
#define TVPIXFMT_A8B8G8R8   0x88802444
#define TVPIXFMT_R8G8B808   0x88803405
#define TVPIXFMT_R8G8B8A8   0x88803446
#define TVPIXFMT_B8G8R808   0x88804407
#define TVPIXFMT_B8G8R8A8   0x88804448
#define TVPIXFMT_R5G6B5     0x5650120b
#define TVPIXFMT_0R5G5B5    0x5550120c
#define TVPIXFMT_0B5G5R5    0x5550220d

/* access macros: */

#define TVPIXFMT_BITS_RED(fmt) (((fmt) & 0xf0000000) >> 28)
#define TVPIXFMT_BITS_GREEN(fmt) (((fmt) & 0xf0000000) >> 24)
#define TVPIXFMT_BITS_BLUE(fmt) (((fmt) & 0xf0000000) >> 20)
#define TVPIXFMT_COL_ORDER(fmt) (((fmt) & 0xf000) >> 12)
#define TVPIXFMT_BYTES_PER_PIXEL(fmt) (((fmt) & 0x0f00) >> 8)
#define TVPIXFMT_HAS_ALPHA(fmt) (((fmt) & 0x0040) >> 6)
#define TVPIXFMT_ORDER_UNDEFINED    0
#define TVPIXFMT_ORDER_XRGB         1
#define TVPIXFMT_ORDER_XBGR         2
#define TVPIXFMT_ORDER_RGBX         3
#define TVPIXFMT_ORDER_BGRX         4

#define TVPIXFMT_ARGB32_GET_ALPHA8(p)   ((p) >> 24)
#define TVPIXFMT_ARGB32_GET_RED8(p)     (((p) >> 16) & 0xff)
#define TVPIXFMT_ARGB32_GET_GREEN8(p)   (((p) >> 8) & 0xff)
#define TVPIXFMT_ARGB32_GET_BLUE8(p)    ((p) & 0xff)

#define TVPIXFMT_ABGR32_GET_ALPHA8(p)   ((p) >> 24)
#define TVPIXFMT_ABGR32_GET_RED8(p)     ((p) & 0xff)
#define TVPIXFMT_ABGR32_GET_GREEN8(p)   (((p) >> 8) & 0xff)
#define TVPIXFMT_ABGR32_GET_BLUE8(p)    (((p) >> 16) & 0xff)

#define TVPIXFMT_RGB15_GET_RED8(p)      ((((p) & 0x7c00) >> 7) | (((p) & 0x7000) >> 12))
#define TVPIXFMT_RGB15_GET_GREEN8(p)    ((((p) & 0x03e0) >> 2) | (((p) & 0x0380) >> 7))
#define TVPIXFMT_RGB15_GET_BLUE8(p)     ((((p) & 0x001f) << 3) | (((p) & 0x001c) >> 2))

#define TVPIXFMT_RGB16_GET_RED8(p)      ((((p) & 0xf800) >> 8) | (((p) & 0xe000) >> 13))
#define TVPIXFMT_RGB16_GET_GREEN8(p)    ((((p) & 0x07e0) >> 3) | (((p) & 0x0600) >> 9))
#define TVPIXFMT_RGB16_GET_BLUE8(p)     ((((p) & 0x001f) << 3) | (((p) & 0x001c) >> 2))

#define TVPIXFMT_BGR15_GET_RED8(p)      ((((p) & 0x001f) << 3) | (((p) & 0x001c) >> 2))
#define TVPIXFMT_BGR15_GET_GREEN8(p)    ((((p) & 0x03e0) >> 2) | (((p) & 0x0380) >> 7))
#define TVPIXFMT_BGR15_GET_BLUE8(p)     ((((p) & 0x7c00) >> 7) | (((p) & 0x7000) >> 12))

#define TVPIXFMT_R_G_B_TO_ARGB32(r,g,b) (((r) << 16) | ((g) << 8) | (b))
#define TVPIXFMT_R_G_B_TO_ABGR32(r,g,b) (((b) << 16) | ((g) << 8) | (r))
#define TVPIXFMT_R_G_B_TO_RGB15(r,g,b)  ((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3))
#define TVPIXFMT_R_G_B_TO_RGB16(r,g,b)  ((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))
#define TVPIXFMT_R_G_B_TO_BGR15(r,g,b)  ((((b) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((r) & 0xf8) >> 3))

#define TVPIXFMT_RGB15_TO_ARGB32(p)     TVPIXFMT_R_G_B_TO_ARGB32(TVPIXFMT_RGB15_GET_RED8(p), TVPIXFMT_RGB15_GET_GREEN8(p), TVPIXFMT_RGB15_GET_BLUE8(p))
#define TVPIXFMT_RGB16_TO_ARGB32(p)     TVPIXFMT_R_G_B_TO_ARGB32(TVPIXFMT_RGB16_GET_RED8(p), TVPIXFMT_RGB16_GET_GREEN8(p), TVPIXFMT_RGB16_GET_BLUE8(p))
#define TVPIXFMT_BGR15_TO_ARGB32(p)     TVPIXFMT_R_G_B_TO_ARGB32(TVPIXFMT_BGR15_GET_RED8(p), TVPIXFMT_BGR15_GET_GREEN8(p), TVPIXFMT_BGR15_GET_BLUE8(p))

#define TVPIXFMT_BGR15_TO_RGB16(p)      ((((p) & 0x7c00) >> 10) | (((p) & 0x03e0) << 1) | (((p) & 0x001f) << 11))

#define TVPIXFMT_ARGB32_TO_RGB15(p)     TVPIXFMT_R_G_B_TO_RGB15(TVPIXFMT_ARGB32_GET_RED8(p), TVPIXFMT_ARGB32_GET_GREEN8(p), TVPIXFMT_ARGB32_GET_BLUE8(p))
#define TVPIXFMT_ARGB32_TO_RGB16(p)     TVPIXFMT_R_G_B_TO_RGB16(TVPIXFMT_ARGB32_GET_RED8(p), TVPIXFMT_ARGB32_GET_GREEN8(p), TVPIXFMT_ARGB32_GET_BLUE8(p))
#define TVPIXFMT_ARGB32_TO_BGR15(p)     TVPIXFMT_R_G_B_TO_BGR15(TVPIXFMT_ARGB32_GET_RED8(p), TVPIXFMT_ARGB32_GET_GREEN8(p), TVPIXFMT_ARGB32_GET_BLUE8(p))

#define TVPIXFMT_ABGR32_TO_RGB16(p)     TVPIXFMT_R_G_B_TO_RGB16(TVPIXFMT_ABGR32_GET_RED8(p), TVPIXFMT_ABGR32_GET_GREEN8(p), TVPIXFMT_ABGR32_GET_BLUE8(p))
#define TVPIXFMT_ABGR32_TO_RGB15(p)     TVPIXFMT_R_G_B_TO_RGB15(TVPIXFMT_ABGR32_GET_RED8(p), TVPIXFMT_ABGR32_GET_GREEN8(p), TVPIXFMT_ABGR32_GET_BLUE8(p))

#define TVPIXFMT_ARGB32_TO_ABGR32(p)    (((p) & 0xff000000) | (((p) & 0xff0000) >> 16) | ((p) & 0xff00) | (((p) & 0xff) << 16))
#define TVPIXFMT_0RGB32_TO_0BGR32(p)    ((((p) & 0xff0000) >> 16) | ((p) & 0xff00) | (((p) & 0xff) << 16))

#define TVPIXFMT_RGB16_SWAP(p)          ((((p) & 0xff00) >> 8) | (((p) & 0xff) << 8))
#define TVPIXFMT_ARGB32_SWAP(p)         ((((p) & 0xff000000) >> 24) | (((p) & 0xff0000) >> 8) | (((p) & 0xff00) << 8) | (((p) & 0xff) << 24))
#define TVPIXFMT_0RGB32_SWAP(p)         ((((p) & 0xff0000) >> 8) | (((p) & 0xff00) << 8) | (((p) & 0xff) << 24))

/*****************************************************************************/
/*
**  Input message
*/

typedef struct TInputMessage
{
    /* Node header: */
    struct TNode timsg_Node;
    /* Size of extra data following the message body: */
    TTAG timsg_ExtraSize;
    /* Instance/Window pointer: */
    TAPTR timsg_Instance;
    /* Copy of instance/window userdata: */
    TTAG timsg_UserData;
    /* Timestamp: */
    TTIME timsg_TimeStamp;
    /* Message type: */
    TUINT timsg_Type;
    /* Message code: */
    TUINT timsg_Code;
    /* Keyboard qualifiers: */
    TUINT timsg_Qualifier;
    /* Window relative mouse position: */
    TINT timsg_MouseX, timsg_MouseY;
    /* Screen absolute mouse position: */
    TINT timsg_ScreenMouseX, timsg_ScreenMouseY;
    /* Damage rect: */
    TINT timsg_X, timsg_Y, timsg_Width, timsg_Height;
    /* UTF-8 representation of keycode: */
    unsigned char timsg_KeyCode[8];
    
    /* Some identification for originator of message: */
    TTAG timsg_Requestor;
    /* Msgtype-dependant reply data: */
    TTAG timsg_ReplyData;
    /* Tag buffer: */
    struct TTagItem timsg_Tags[16];
    
    /* more fields may follow in the future. Do not use
    ** sizeof(TIMSG); if necessary use TGetSize(imsg) */

} TIMSG;

#define TIMsgReply_UTF8Selection    (TVISTAGS_ + 0x080)
#define TIMsgReply_UTF8SelectionLen (TVISTAGS_ + 0x081)

/*****************************************************************************/
/*
**  Input types
*/

#define TITYPE_NONE             0x00000000
#define TITYPE_ALL              0xffffffff

/* Closed: */
#define TITYPE_CLOSE            0x00000001
/* Focused/unfocused: */
#define TITYPE_FOCUS            0x00000002
/* Resized: */
#define TITYPE_NEWSIZE          0x00000004
/* Needs refreshing: */
#define TITYPE_REFRESH          0x00000008
/* Mouse moved in/out: */
#define TITYPE_MOUSEOVER        0x00000010
/* key down: */
#define TITYPE_KEYDOWN          0x00000100
/* Mouse movement: */
#define TITYPE_MOUSEMOVE        0x00000200
/* Mouse button: */
#define TITYPE_MOUSEBUTTON      0x00000400
/* 1/50s interval: */
#define TITYPE_INTERVAL         0x00000800
/* key up: */
#define TITYPE_KEYUP            0x00001000
/* "Cooked" keystroke, an alias to keydown: */
#define TITYPE_COOKEDKEY        TITYPE_KEYDOWN
/* User message: */
#define TITYPE_USER             0x00002000
/* Selection Request message: */
#define TITYPE_REQSELECTION     0x00004000
/* Signal message: */
#define TITYPE_SIGNAL           0x00008000

/*****************************************************************************/
/*
**  Mouse button codes
**  Styleguide notes: Some mice have only one or two buttons.
**  usage of the middle and right buttons without alternatives
**  is disencouraged.
*/

#define TMBCODE_LEFTDOWN        0x00000001
#define TMBCODE_LEFTUP          0x00000002
#define TMBCODE_RIGHTDOWN       0x00000004
#define TMBCODE_RIGHTUP         0x00000008
#define TMBCODE_MIDDLEDOWN      0x00000010
#define TMBCODE_MIDDLEUP        0x00000020
#define TMBCODE_WHEELUP         0x00000040
#define TMBCODE_WHEELDOWN       0x00000080

/*****************************************************************************/
/*
**  Keycodes (cooked)
*/

/* no key (but may still have a qualifier): */
#define TKEYC_NONE              0x00000000

/*
**  Named special key codes
*/

/* backspace key: */
#define TKEYC_BCKSPC            0x00000008
/* tab key: */
#define TKEYC_TAB               0x00000009
/* return: */
#define TKEYC_RETURN            0x0000000d
/* escape: */
#define TKEYC_ESC               0x0000001b
/* del key: */
#define TKEYC_DEL               0x0000007f

/*
**  Function key codes
**  Styleguide notes: Some keyboards have only 10 F-Keys.
**  Hardcoded key bindings should never rely on F11 or F12
**  without offering some kind of alternative.
*/

#define TKEYC_F1                0x0000f001
#define TKEYC_F2                0x0000f002
#define TKEYC_F3                0x0000f003
#define TKEYC_F4                0x0000f004
#define TKEYC_F5                0x0000f005
#define TKEYC_F6                0x0000f006
#define TKEYC_F7                0x0000f007
#define TKEYC_F8                0x0000f008
#define TKEYC_F9                0x0000f009
#define TKEYC_F10               0x0000f00a
#define TKEYC_F11               0x0000f00b
#define TKEYC_F12               0x0000f00c

/*
**  Cursor key codes
*/

#define TKEYC_CRSRLEFT          0x0000f010
#define TKEYC_CRSRRIGHT         0x0000f011
#define TKEYC_CRSRUP            0x0000f012
#define TKEYC_CRSRDOWN          0x0000f013

/*
**  Proprietary key codes
**  Styleguide notes: Whenever your application binds actions
**  to a key from this section, you should offer at least one
**  non-proprietary alternative
*/

/* help key: */
#define TKEYC_HELP              0x0000f020
/* insert key: */
#define TKEYC_INSERT            0x0000f021
/* overwrite key: */
#define TKEYC_OVERWRITE         0x0000f022
/* page up: */
#define TKEYC_PAGEUP            0x0000f023
/* page down: */
#define TKEYC_PAGEDOWN          0x0000f024
/* position one key: */
#define TKEYC_POSONE            0x0000f025
/* position end key: */
#define TKEYC_POSEND            0x0000f026
/* print key: */
#define TKEYC_PRINT             0x0000f027
/* scroll down: */
#define TKEYC_SCROLL            0x0000f028
/* pause key: */
#define TKEYC_PAUSE             0x0000f029

/*
**  Keyboard qualifiers
**  Styleguide notes: Your application's default or hardcoded key
**  bindings should never rely on TKEYQ_PROP alone. Some keyboards
**  do not have a numblock, so usage of the TKEYQ_NUMBLOCK qualifier
**  without an alternative is disencouraged, too. Also note that some
**  keyboards don't have a right control key, etc.
*/

/* no qualifier: */
#define TKEYQ_NONE              0x00000000
#define TKEYQ_LSHIFT            0x00000001
#define TKEYQ_RSHIFT            0x00000002
#define TKEYQ_LCTRL             0x00000004
#define TKEYQ_RCTRL             0x00000008
#define TKEYQ_LALT              0x00000010
#define TKEYQ_RALT              0x00000020
/* proprietary qualifier: */
#define TKEYQ_LPROP             0x00000040
#define TKEYQ_RPROP             0x00000080
/* is a qualifier here: */
#define TKEYQ_NUMBLOCK          0x00000100

/* Combinations: Use e.g. msg->timsg_Qualifier & TKEYQ_SHIFT
** to find out whether any SHIFT key was held */

#define TKEYQ_SHIFT             (TKEYQ_LSHIFT|TKEYQ_RSHIFT)
#define TKEYQ_CTRL              (TKEYQ_LCTRL|TKEYQ_RCTRL)
#define TKEYQ_ALT               (TKEYQ_LALT|TKEYQ_RALT)
#define TKEYQ_PROP              (TKEYQ_LPROP|TKEYQ_RPROP)

/*****************************************************************************/

struct TVPixBuf
{
    TUINT8 *tpb_Data;
    TUINT tpb_Format;
    TINT tpb_BytesPerLine;
};

#define TVPB_GETADDRESS(b, x, y) \
    ((b)->tpb_Data + (y) * (b)->tpb_BytesPerLine + \
    (x) * TVPIXFMT_BYTES_PER_PIXEL((b)->tpb_Format))

/*****************************************************************************/
/*
**  Cache request
*/

struct TVImageCacheRequest
{
    struct THandle *tvc_CacheManager;
    TUINT8 *tvc_Key;
    TSIZE tvc_KeyLen;
    TINT tvc_OrigX, tvc_OrigY;
    TINT tvc_Result;
};

#define TVIMGCACHE_FOUND        0
#define TVIMGCACHE_NOTFOUND     1
#define TVIMGCACHE_STORED       2
#define TVIMGCACHE_STORE_FAILED 3

#endif
