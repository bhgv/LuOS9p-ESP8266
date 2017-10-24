// Module for ssd1306 oled

#include "modules.h"
#include "lauxlib.h"
#include "lmem.h"

#include "ssd1306_2/ssd1306.h"

#include <drivers/i2c-platform.h>

#include <wdt_regs.h>

#include <fonts/fonts.h>


typedef uint8_t u8g_uint_t;
typedef int8_t u8g_int_t;


#define ADDR   SSD1306_I2C_ADDR_0


#define DEFAULT_FONT FONT_FACE_TERMINUS_6X12_ISO8859_1


const font_info_t *font = NULL; // current font
const font_face_t font_face = 0;

static ssd1306_color_t foreground = OLED_COLOR_WHITE;
static ssd1306_color_t background = OLED_COLOR_BLACK;

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

uint8_t ssd1306_buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8];



// helper function: retrieve and check userdata argument
/*
static lu8g_userdata_t *get_lud( lua_State *L )
{
//    lu8g_userdata_t *lud = (lu8g_userdata_t *)luaL_checkudata(L, 1, "u8g.display");
    lu8g_userdata_t *lud = (lu8g_userdata_t *)lua_touserdata(L, 1);
    luaL_argcheck(L, lud, 1, "u8g.display expected");
    return lud;
}
*/
// helper function: retrieve given number of integer arguments
static void ldisp_get_int_args( lua_State *L, uint8_t stack, uint8_t num, u8g_uint_t *args)
{
    while (num-- > 0)
    {
        *args++ = luaL_checkinteger( L, stack++ );
    }
}

static int ldisp_dir( lua_State *L )
{
	int i = lua_toboolean( L, 1);

	ssd1306_set_scan_direction_fwd(ADDR, i);

    return 0;
}

static int ldisp_mode( lua_State *L )
{
	int i = lua_tointeger( L, 1);

	ssd1306_set_mem_addr_mode(ADDR, i);

    return 0;
}


static int ldisp_cls( lua_State *L )
{
	int i;
	for(i=0; i<(DISPLAY_WIDTH * DISPLAY_HEIGHT/8); i++)
		ssd1306_buffer[i] = 0;
	ssd1306_set_whole_display_lighting(ADDR, false);
//	ssd1306_clear_screen(ADDR);

    return 0;
}

// Lua: u8g.setFont( self, font )
static int ldisp_setFont( lua_State *L )
{
/*
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_fntpgm_uint8_t *font = (u8g_fntpgm_uint8_t *)lua_touserdata( L, 2 );
    if (font != NULL)
        u8g_SetFont( LU8G, font );
    else
        luaL_argerror(L, 2, "font data expected");
*/
    return 0;
}

#if 0
// Lua: u8g.setFontRefHeightAll( self )
static int ldisp_setFontRefHeightAll( lua_State *L )
{
/*
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_SetFontRefHeightAll( LU8G );
*/

    return 0;
}

// Lua: u8g.setFontRefHeightExtendedText( self )
static int ldisp_setFontRefHeightExtendedText( lua_State *L )
{
/*
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_SetFontRefHeightExtendedText( LU8G );
*/
    return 0;
}

// Lua: u8g.setFontRefHeightText( self )
static int ldisp_setFontRefHeightText( lua_State *L )
{
/*
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_SetFontRefHeightText( LU8G );
*/
    return 0;
}
#endif

// Lua: u8g.setDefaultBackgroundColor( self )
static int ldisp_setDefaultBackgroundColor( lua_State *L )
{
	background = OLED_COLOR_BLACK;
/*
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_SetDefaultBackgroundColor( LU8G );
*/
    return 0;
}

// Lua: u8g.setDefaultForegroundColor( self )
static int ldisp_setDefaultForegroundColor( lua_State *L )
{
	foreground = OLED_COLOR_WHITE;
/*
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_SetDefaultForegroundColor( LU8G );
*/
    return 0;
}

#if 0
// Lua: u8g.setFontPosBaseline( self )
static int ldisp_setFontPosBaseline( lua_State *L )
{
/*
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_SetFontPosBaseline( LU8G );
*/
    return 0;
}

// Lua: u8g.setFontPosBottom( self )
static int ldisp_setFontPosBottom( lua_State *L )
{
/*
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_SetFontPosBottom( LU8G );
*/
    return 0;
}

// Lua: u8g.setFontPosCenter( self )
static int ldisp_setFontPosCenter( lua_State *L )
{
/*
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_SetFontPosCenter( LU8G );
*/
    return 0;
}

// Lua: u8g.setFontPosTop( self )
static int ldisp_setFontPosTop( lua_State *L )
{
/*
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_SetFontPosTop( LU8G );
*/
    return 0;
}

// Lua: int = u8g.getFontAscent( self )
static int ldisp_getFontAscent( lua_State *L )
{
/*
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    lua_pushinteger( L, u8g_GetFontAscent( LU8G ) );

    return 1;
 */
 	return 0;
}

// Lua: int = u8g.getFontDescent( self )
static int ldisp_getFontDescent( lua_State *L )
{
/*
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    lua_pushinteger( L, u8g_GetFontDescent( LU8G ) );
*/
    return 0; //1;
}

// Lua: int = u8g.getFontLineSpacing( self )
static int ldisp_getFontLineSpacing( lua_State *L )
{
/*
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    lua_pushinteger( L, u8g_GetFontLineSpacing( LU8G ) );
*/
    return 0; //1;
}

// Lua: int = u8g.getMode( self )
static int ldisp_getMode( lua_State *L )
{
/*
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    lua_pushinteger( L, u8g_GetMode( LU8G ) );
*/
    return 0; //1;
}
#endif

// Lua: u8g.setContrast( self, constrast )
static int ldisp_setContrast( lua_State *L )
{
//    lu8g_userdata_t *lud;

//    if ((lud = get_lud( L )) == NULL)
//        return 0;

//    u8g_SetContrast( LU8G, luaL_checkinteger( L, 2 ) );
    ssd1306_set_contrast( ADDR, luaL_checkinteger( L, 1 ) );

    return 0;
}

// Lua: u8g.setColorIndex( self, color )
static int ldisp_setColorIndex( lua_State *L )
{
/*
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_SetColorIndex( LU8G, luaL_checkinteger( L, 2 ) );
*/
    return 0;
}

// Lua: int = u8g.getColorIndex( self )
static int ldisp_getColorIndex( lua_State *L )
{
/*
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    lua_pushinteger( L, u8g_GetColorIndex( LU8G ) );
*/
    return 0; //1;
}

/*
static int lu8g_generic_drawStr( lua_State *L, uint8_t rot )
{
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_uint_t args[2];
    lu8g_get_int_args( L, 2, 2, args );

    const char *s = luaL_checkstring( L, (1+2) + 1 );
    if (s == NULL)
        return 0;

    switch (rot)
    {
    case 1:
        lua_pushinteger( L, u8g_DrawStr90( LU8G, args[0], args[1], s ) );
        break;
    case 2:
        lua_pushinteger( L, u8g_DrawStr180( LU8G, args[0], args[1], s ) );
        break;
    case 3:
        lua_pushinteger( L, u8g_DrawStr270( LU8G, args[0], args[1], s ) );
        break;
    default:
        lua_pushinteger( L, u8g_DrawStr( LU8G, args[0], args[1], s ) );
        break;
    }

    return 1;
}
*/

// Lua: pix_len = u8g.drawStr( self, x, y, string )
static int ldisp_drawStr( lua_State *L )
{
    u8g_uint_t args[2];
    ldisp_get_int_args( L, 1, 2, args );

    const char *s = luaL_checkstring( L, 1+2 );
    if (s == NULL)
        return 0;

//    return lu8g_generic_drawStr( L, 0 );
	
	ssd1306_draw_string(ADDR, ssd1306_buffer, font, 
						args[0], args[1], s, 
						luaL_optinteger( L, 4, foreground), 
						luaL_optinteger( L, 5, background)
						);
    return 0;
}

/*
// Lua: pix_len = u8g.drawStr90( self, x, y, string )
static int lu8g_drawStr90( lua_State *L )
{
    return lu8g_generic_drawStr( L, 1 );
}

// Lua: pix_len = u8g.drawStr180( self, x, y, string )
static int lu8g_drawStr180( lua_State *L )
{
    return lu8g_generic_drawStr( L, 2 );
}

// Lua: pix_len = u8g.drawStr270( self, x, y, string )
static int lu8g_drawStr270( lua_State *L )
{
    return lu8g_generic_drawStr( L, 3 );
}
*/
// Lua: u8g.drawLine( self, x1, y1, x2, y2 )
static int ldisp_drawLine( lua_State *L )
{
//    lu8g_userdata_t *lud;

//    if ((lud = get_lud( L )) == NULL)
//        return 0;

    u8g_uint_t args[4];
    ldisp_get_int_args( L, 1, 4, args );

//    u8g_DrawLine( LU8G, args[0], args[1], args[2], args[3] );
	ssd1306_draw_line(ADDR, ssd1306_buffer, args[0], args[1], args[2], args[3], 
							luaL_optinteger( L, 5, foreground)
	);

    return 0;
}

// Lua: u8g.drawTriangle( self, x0, y0, x1, y1, x2, y2 )
static int ldisp_drawTriangle( lua_State *L )
{
//    lu8g_userdata_t *lud;

//    if ((lud = get_lud( L )) == NULL)
//        return 0;

    u8g_uint_t args[6];
    ldisp_get_int_args( L, 1, 6, args );

//    u8g_DrawTriangle( LU8G, args[0], args[1], args[2], args[3], args[4], args[5] );
	ssd1306_draw_triangle(ADDR, ssd1306_buffer, 
				args[0], args[1], args[2], args[3], args[4], args[5], 
				luaL_optinteger( L, 7, foreground)
				);

    return 0;
}

// Lua: u8g.drawBox( self, x, y, width, height )
static int ldisp_drawBox( lua_State *L )
{
//    lu8g_userdata_t *lud;

//    if ((lud = get_lud( L )) == NULL)
//        return 0;

    u8g_uint_t args[4];
    ldisp_get_int_args( L, 1, 4, args );

//    u8g_DrawBox( LU8G, args[0], args[1], args[2], args[3] );
	ssd1306_fill_rectangle(ADDR, ssd1306_buffer, args[0], args[1], args[2], args[3], 
									luaL_optinteger( L, 5, foreground)
	);

    return 0;
}

#if 0
// Lua: u8g.drawRBox( self, x, y, width, height, radius )
static int ldisp_drawRBox( lua_State *L )
{
//    lu8g_userdata_t *lud;

//    if ((lud = get_lud( L )) == NULL)
//        return 0;

    u8g_uint_t args[5];
    ldisp_get_int_args( L, 1, 5, args );

//    u8g_DrawRBox( LU8G, args[0], args[1], args[2], args[3], args[4] );
	ssd1306_fill_rectangle(ADDR, ssd1306_buffer, args[0], args[1], args[2], args[3], foreground);

    return 0;
}
#endif

// Lua: u8g.drawFrame( self, x, y, width, height )
static int ldisp_drawFrame( lua_State *L )
{
//    lu8g_userdata_t *lud;

//    if ((lud = get_lud( L )) == NULL)
//        return 0;

    u8g_uint_t args[4];
    ldisp_get_int_args( L, 1, 4, args );

//    u8g_DrawFrame( LU8G, args[0], args[1], args[2], args[3] );
	ssd1306_draw_rectangle(ADDR, ssd1306_buffer, args[0], args[1], args[2], args[3], 
									luaL_optinteger( L, 5, foreground)
	);

    return 0;
}

#if 0
// Lua: u8g.drawRFrame( self, x, y, width, height, radius )
static int ldisp_drawRFrame( lua_State *L )
{
//    lu8g_userdata_t *lud;

//    if ((lud = get_lud( L )) == NULL)
//        return 0;

    u8g_uint_t args[5];
    ldisp_get_int_args( L, 1, 5, args );

//    u8g_DrawRFrame( LU8G, args[0], args[1], args[2], args[3], args[4] );
	ssd1306_draw_rectangle(ADDR, ssd1306_buffer, args[0], args[1], args[2], args[3], foreground);

    return 0;
}
#endif

// Lua: u8g.drawDisc( self, x0, y0, rad, opt = U8G_DRAW_ALL )
static int ldisp_drawDisc( lua_State *L )
{
//    lu8g_userdata_t *lud;

//    if ((lud = get_lud( L )) == NULL)
//        return 0;

    u8g_uint_t args[3];
    ldisp_get_int_args( L, 1, 3, args );

//    u8g_uint_t opt = luaL_optinteger( L, (1+3) + 1, U8G_DRAW_ALL );

//    u8g_DrawDisc( LU8G, args[0], args[1], args[2], opt );
	ssd1306_fill_circle(ADDR, ssd1306_buffer, args[0], args[1], args[2], 
								luaL_optinteger( L, 4, foreground)
	);

    return 0;
}

// Lua: u8g.drawCircle( self, x0, y0, rad, opt = U8G_DRAW_ALL )
static int ldisp_drawCircle( lua_State *L )
{
//    lu8g_userdata_t *lud;

//    if ((lud = get_lud( L )) == NULL)
//        return 0;

    u8g_uint_t args[3];
    ldisp_get_int_args( L, 1, 3, args );

//    u8g_uint_t opt = luaL_optinteger( L, (1+3) + 1, U8G_DRAW_ALL );

//    u8g_DrawCircle( LU8G, args[0], args[1], args[2], opt );
	ssd1306_draw_circle(ADDR, ssd1306_buffer, args[0], args[1], args[2], 
								luaL_optinteger( L, 4, foreground)
	);

    return 0;
}

/*
// Lua: u8g.drawEllipse( self, x0, y0, rx, ry, opt = U8G_DRAW_ALL )
static int lu8g_drawEllipse( lua_State *L )
{
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_uint_t args[4];
    lu8g_get_int_args( L, 2, 4, args );

    u8g_uint_t opt = luaL_optinteger( L, (1+4) + 1, U8G_DRAW_ALL );

    u8g_DrawEllipse( LU8G, args[0], args[1], args[2], args[3], opt );

    return 0;
}

// Lua: u8g.drawFilledEllipse( self, x0, y0, rx, ry, opt = U8G_DRAW_ALL )
static int lu8g_drawFilledEllipse( lua_State *L )
{
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_uint_t args[4];
    lu8g_get_int_args( L, 2, 4, args );

    u8g_uint_t opt = luaL_optinteger( L, (1+4) + 1, U8G_DRAW_ALL );

    u8g_DrawFilledEllipse( LU8G, args[0], args[1], args[2], args[3], opt );

    return 0;
}
*/

// Lua: u8g.drawPixel( self, x, y )
static int ldisp_drawPixel( lua_State *L )
{
//    lu8g_userdata_t *lud;

//    if ((lud = get_lud( L )) == NULL)
//        return 0;

    u8g_uint_t args[2];
    ldisp_get_int_args( L, 1, 2, args );

//    u8g_DrawPixel( LU8G, args[0], args[1] );
	ssd1306_draw_pixel(ADDR, ssd1306_buffer, args[0], args[1], 
								luaL_optinteger( L, 3, foreground)
	);

    return 0;
}

// Lua: u8g.drawHLine( self, x, y, width )
static int ldisp_drawHLine( lua_State *L )
{
//   lu8g_userdata_t *lud;

//    if ((lud = get_lud( L )) == NULL)
//        return 0;

    u8g_uint_t args[3];
    ldisp_get_int_args( L, 1, 3, args );

//    u8g_DrawHLine( LU8G, args[0], args[1], args[2] );
	ssd1306_draw_hline(ADDR, ssd1306_buffer, args[0], args[1], args[2], 
								luaL_optinteger( L, 4, foreground)
	);

    return 0;
}

// Lua: u8g.drawVLine( self, x, y, width )
static int ldisp_drawVLine( lua_State *L )
{
//    lu8g_userdata_t *lud;

//    if ((lud = get_lud( L )) == NULL)
//        return 0;

    u8g_uint_t args[3];
    ldisp_get_int_args( L, 1, 3, args );

//    u8g_DrawVLine( LU8G, args[0], args[1], args[2] );
	ssd1306_draw_vline(ADDR, ssd1306_buffer, args[0], args[1], args[2], 
								luaL_optinteger( L, 4, foreground)
	);

    return 0;
}

// Lua: u8g.drawXBM( self, x, y, width, height, data )
static int ldisp_drawXBM( lua_State *L )
{
//    lu8g_userdata_t *lud;

//    if ((lud = get_lud( L )) == NULL)
//        return 0;

    u8g_uint_t args[4];
    ldisp_get_int_args( L, 1, 4, args );

    const char *xbm_data = luaL_checkstring( L, 1+4 );
    if (xbm_data == NULL)
        return 0;

//    u8g_DrawXBM( LU8G, args[0], args[1], args[2], args[3], (const uint8_t *)xbm_data );
    ssd1306_load_xbm(ADDR, (const uint8_t *)xbm_data, ssd1306_buffer);

    return 0;
}

// Lua: u8g.drawBitmap( self, x, y, count, height, data )
static int ldisp_drawBitmap( lua_State *L )
{
//    lu8g_userdata_t *lud;

//    if ((lud = get_lud( L )) == NULL)
//        return 0;

    u8g_uint_t args[4];
    ldisp_get_int_args( L, 1, 4, args );

    const char *bm_data = luaL_checkstring( L, 1+4 );
    if (bm_data == NULL)
        return 0;

//    u8g_DrawBitmap( LU8G, args[0], args[1], args[2], args[3], (const uint8_t *)bm_data );
    ssd1306_load_xbm(ADDR, (const uint8_t *)bm_data, ssd1306_buffer);

    return 0;
}

/*
// Lua: u8g.setScale2x2( self )
static int lu8g_setScale2x2( lua_State *L )
{
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_SetScale2x2( LU8G );

    return 0;
}

// Lua: u8g.undoScale( self )
static int lu8g_undoScale( lua_State *L )
{
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_UndoScale( LU8G );

    return 0;
}

// Lua: u8g.firstPage( self )
static int lu8g_firstPage( lua_State *L )
{
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_FirstPage( LU8G );

    return 0;
}

// Lua: bool = u8g.nextPage( self )
static int lu8g_nextPage( lua_State *L )
{
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    lua_pushboolean( L, u8g_NextPage( LU8G ) );

    return 1;
}

// Lua: u8g.sleepOn( self )
static int lu8g_sleepOn( lua_State *L )
{
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_SleepOn( LU8G );

    return 0;
}

// Lua: u8g.sleepOff( self )
static int lu8g_sleepOff( lua_State *L )
{
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_SleepOff( LU8G );

    return 0;
}
*/

/*
// Lua: u8g.setRot90( self )
static int lu8g_setRot90( lua_State *L )
{
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_SetRot90( LU8G );

    return 0;
}

// Lua: u8g.setRot180( self )
static int lu8g_setRot180( lua_State *L )
{
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_SetRot180( LU8G );

    return 0;
}

// Lua: u8g.setRot270( self )
static int lu8g_setRot270( lua_State *L )
{
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_SetRot270( LU8G );

    return 0;
}

// Lua: u8g.undoRotation( self )
static int lu8g_undoRotation( lua_State *L )
{
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_UndoRotation( LU8G );

    return 0;
}

// Lua: width = u8g.getWidth( self )
static int lu8g_getWidth( lua_State *L )
{
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    lua_pushinteger( L, u8g_GetWidth( LU8G ) );

    return 1;
}

// Lua: height = u8g.getHeight( self )
static int lu8g_getHeight( lua_State *L )
{
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    lua_pushinteger( L, u8g_GetHeight( LU8G ) );

    return 1;
}

// Lua: width = u8g.getStrWidth( self, string )
static int lu8g_getStrWidth( lua_State *L )
{
   lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    const char *s = luaL_checkstring( L, 2 );
    if (s == NULL)
        return 0;

    lua_pushinteger( L, u8g_GetStrWidth( LU8G, s ) );

    return 1;
}

// Lua: u8g.setFontLineSpacingFactor( self, factor )
static int lu8g_setFontLineSpacingFactor( lua_State *L )
{
   lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    u8g_uint_t factor = luaL_checkinteger( L, 2 );

    u8g_SetFontLineSpacingFactor( LU8G, factor );

    return 0;
}


// device destructor
static int lu8g_close_display( lua_State *L )
{
    lu8g_userdata_t *lud;

    if ((lud = get_lud( L )) == NULL)
        return 0;

    if (lud->cb_ref != LUA_NOREF) {
        // this is the fb_rle device
        u8g_dev_t *fb_dev = LU8G->dev;
        u8g_pb_t *fb_dev_pb = (u8g_pb_t *)(fb_dev->dev_mem);
        uint8_t *fb_dev_buf = fb_dev_pb->buf;

        luaM_free( L, fb_dev_buf );
        luaM_free( L, fb_dev_pb );
        luaM_free( L, fb_dev );

        luaL_unref( L, lud->cb_ref, LUA_REGISTRYINDEX );
    }

    return 0;
}
*/

// ***************************************************************************
// Device constructors
//
//
// I2C based devices will use this function template to implement the Lua binding.
#if 0
#undef U8G_DISPLAY_TABLE_ENTRY
#define U8G_DISPLAY_TABLE_ENTRY(device)                                 \
    static int lu8g_ ## device( lua_State *L )                          \
    {                                                                   \
        uint8_t sda = 4;                                                    \
        uint8_t scl = 5;                                                   \
        uint32_t speed = 100;                                                \
        int spd /*= platform_i2c_setup(0, sda, scl, speed)*/;                         \
        /*delay(700);*/         \
                                                                        \
        unsigned addr = /*luaL_checkinteger*/luaL_optinteger( L, 1, /*0x3c*/0 );                      \
        unsigned del  = luaL_optinteger( L, 2, 0 );                     \
                                                                        \
        if (addr == 0)                                                  \
            addr = 0x3c;                                                \
            /*return luaL_error( L, "i2c address required" );*/         \
                                                                        \
        lu8g_userdata_t *lud = (lu8g_userdata_t *) lua_newuserdata( L, sizeof( lu8g_userdata_t ) ); \
        lud->cb_ref = LUA_NOREF;                                        \
                                                                        \
        lud->i2c_addr = (uint8_t)addr;                                  \
        lud->use_delay = del > 0 ? 1 : 0;                               \
                                                                        \
        u8g_InitI2C( LU8G, &u8g_dev_ ## device, U8G_I2C_OPT_NONE);      \
                                                                        \
        /* set its metatable */                                         \
        luaL_getmetatable(L, "u8g.display");                           \
        lua_setmetatable(L, -2);                                        \
                                                                        \
        /*printf("i2c speed=%d\n", spd);*/                                      \
        return 1;                                                       \
    }
//
// Unroll the display table and insert binding functions for I2C based displays.
U8G_DISPLAY_TABLE_I2C
#endif
//
//
//
// SPI based devices will use this function template to implement the Lua binding.
#if 0
#undef U8G_DISPLAY_TABLE_ENTRY
#define U8G_DISPLAY_TABLE_ENTRY(device)                                 \
    static int lu8g_ ## device( lua_State *L )                          \
    {                                                                   \
        unsigned cs = luaL_checkinteger( L, 1 );                        \
        if (cs == 0)                                                    \
            return luaL_error( L, "CS pin required" );                  \
        unsigned dc = luaL_checkinteger( L, 2 );                        \
        if (dc == 0)                                                    \
            return luaL_error( L, "D/C pin required" );                 \
        unsigned res = luaL_optinteger( L, 3, U8G_PIN_NONE );           \
        unsigned del = luaL_optinteger( L, 4, 0 );                      \
                                                                        \
        lu8g_userdata_t *lud = (lu8g_userdata_t *) lua_newuserdata( L, sizeof( lu8g_userdata_t ) ); \
        lud->cb_ref = LUA_NOREF;                                        \
                                                                        \
        lud->use_delay = del > 0 ? 1 : 0;                               \
                                                                        \
        u8g_InitHWSPI( LU8G, &u8g_dev_ ## device, cs, dc, res );        \
                                                                        \
        /* set its metatable */                                         \
        luaL_getmetatable(L, "u8g.display");                            \
        lua_setmetatable(L, -2);                                        \
                                                                        \
        return 1;                                                       \
    }
//
// Unroll the display table and insert binding functions for SPI based displays.
U8G_DISPLAY_TABLE_SPI
#endif
//
//
//
// This display forwards the framebuffer contents as run-length encoded chunks to a Lua callback
#if 0
static int lu8g_fb_rle( lua_State *L ) {
    lu8g_userdata_t *lud;

    if ((lua_type( L, 1 ) != LUA_TFUNCTION) 
//        && (lua_type( L, 1 ) != LUA_TLIGHTFUNCTION)
    ) {
        luaL_typerror( L, 1, "function" );
    }

    int width = luaL_checkinteger( L, 2 );
    int height = luaL_checkinteger( L, 3 );

    luaL_argcheck( L, (width > 0) && (width < 256) && ((width % 8) == 0), 2, "invalid width" );
    luaL_argcheck( L, (height > 0) && (height < 256) && ((height % 8) == 0), 3, "invalid height" );

    // construct display device structures manually because width and height are configurable
    uint8_t *fb_dev_buf = (uint8_t *)luaM_malloc( L, width );

    u8g_pb_t *fb_dev_pb = (u8g_pb_t *)luaM_malloc( L, sizeof( u8g_pb_t ) );
    fb_dev_pb->p.page_height  = 8;
    fb_dev_pb->p.total_height = height;
    fb_dev_pb->p.page_y0      = 0;
    fb_dev_pb->p.page_y1      = 0;
    fb_dev_pb->p.page         = 0;
    fb_dev_pb->width = width;
    fb_dev_pb->buf   = fb_dev_buf;

    u8g_dev_t *fb_dev = (u8g_dev_t *)luaM_malloc( L, sizeof( u8g_dev_t ) );
    fb_dev->dev_fn = u8g_dev_gen_fb_fn;
    fb_dev->dev_mem = fb_dev_pb;
    fb_dev->com_fn = u8g_com_esp8266_fbrle_fn;

    lud = (lu8g_userdata_t *) lua_newuserdata( L, sizeof( lu8g_userdata_t ) );
    lua_pushvalue( L, 1 );  // copy argument (func) to the top of stack
    lud->cb_ref = luaL_ref( L, LUA_REGISTRYINDEX );

    /* set metatable for userdata */
    luaL_getmetatable(L, "u8g.display");
    lua_setmetatable(L, -2);

    u8g_Init8BitFixedPort( LU8G, fb_dev, width, height, 0, 0, 0);

    return 1;
}
#endif

static int ldisp_show( lua_State *L ) {
	lua_gc(L, LUA_GCCOLLECT, 0);

	ssd1306_load_frame_buffer(ADDR, ssd1306_buffer);
	return 0;
}

//
// ***************************************************************************


// Module function map
static const LUA_REG_TYPE ldisplay_map[] = {
//	{ LSTRKEY( "init" ),						LFUNCVAL( ldisp_init ) },
//    { LSTRKEY( "dir" ),						  LFUNCVAL( ldisp_dir ) },
//    { LSTRKEY( "mode" ),						  LFUNCVAL( ldisp_mode ) },
  
  { LSTRKEY( "draw" ),                        LFUNCVAL( ldisp_show ) },
  { LSTRKEY( "cls" ),						 LFUNCVAL( ldisp_cls ) },
  { LSTRKEY( "bitmap" ),                   LFUNCVAL( ldisp_drawBitmap ) },
  { LSTRKEY( "box" ),                      LFUNCVAL( ldisp_drawBox ) },
  { LSTRKEY( "circle" ),                   LFUNCVAL( ldisp_drawCircle ) },
  { LSTRKEY( "disc" ),                     LFUNCVAL( ldisp_drawDisc ) },
//  { LSTRKEY( "drawEllipse" ),                  LFUNCVAL( ldisp_drawEllipse ) },
//  { LSTRKEY( "drawFilledEllipse" ),            LFUNCVAL( ldisp_drawFilledEllipse ) },
  { LSTRKEY( "frame" ),                    LFUNCVAL( ldisp_drawFrame ) },
  { LSTRKEY( "hline" ),                    LFUNCVAL( ldisp_drawHLine ) },
  { LSTRKEY( "line" ),                     LFUNCVAL( ldisp_drawLine ) },
  { LSTRKEY( "pixel" ),                    LFUNCVAL( ldisp_drawPixel ) },
//  { LSTRKEY( "rBox" ),                     LFUNCVAL( ldisp_drawRBox ) },
//  { LSTRKEY( "rFrame" ),                   LFUNCVAL( ldisp_drawRFrame ) },
  { LSTRKEY( "print" ),                      LFUNCVAL( ldisp_drawStr ) },
//  { LSTRKEY( "drawStr90" ),                    LFUNCVAL( ldisp_drawStr90 ) },
//  { LSTRKEY( "drawStr180" ),                   LFUNCVAL( ldisp_drawStr180 ) },
//  { LSTRKEY( "drawStr270" ),                   LFUNCVAL( ldisp_drawStr270 ) },
  { LSTRKEY( "triangle" ),                 LFUNCVAL( ldisp_drawTriangle ) },
  { LSTRKEY( "vline" ),                    LFUNCVAL( ldisp_drawVLine ) },
  { LSTRKEY( "XBM" ),                      LFUNCVAL( ldisp_drawXBM ) },
//  { LSTRKEY( "firstPage" ),                    LFUNCVAL( ldisp_firstPage ) },
//  { LSTRKEY( "getColorIndex" ),                LFUNCVAL( ldisp_getColorIndex ) },
//  { LSTRKEY( "getFontAscent" ),                LFUNCVAL( ldisp_getFontAscent ) },
//  { LSTRKEY( "getFontDescent" ),               LFUNCVAL( ldisp_getFontDescent ) },
//  { LSTRKEY( "getFontLineSpacing" ),           LFUNCVAL( ldisp_getFontLineSpacing ) },
//  { LSTRKEY( "getHeight" ),                    LFUNCVAL( ldisp_getHeight ) },
//  { LSTRKEY( "getMode" ),                      LFUNCVAL( ldisp_getMode ) },
//  { LSTRKEY( "getStrWidth" ),                  LFUNCVAL( ldisp_getStrWidth ) },
//  { LSTRKEY( "getWidth" ),                     LFUNCVAL( ldisp_getWidth ) },
//  { LSTRKEY( "nextPage" ),                     LFUNCVAL( ldisp_nextPage ) },
  { LSTRKEY( "setContrast" ),                  LFUNCVAL( ldisp_setContrast ) },
  { LSTRKEY( "setColorIndex" ),                LFUNCVAL( ldisp_setColorIndex ) },
  { LSTRKEY( "setDefaultBackgroundColor" ),    LFUNCVAL( ldisp_setDefaultBackgroundColor ) },
  { LSTRKEY( "setDefaultForegroundColor" ),    LFUNCVAL( ldisp_setDefaultForegroundColor ) },
  { LSTRKEY( "setFont" ),                      LFUNCVAL( ldisp_setFont ) },
//  { LSTRKEY( "setFontLineSpacingFactor" ),     LFUNCVAL( ldisp_setFontLineSpacingFactor ) },
//  { LSTRKEY( "setFontPosBaseline" ),           LFUNCVAL( lu8g_setFontPosBaseline ) },
//  { LSTRKEY( "setFontPosBottom" ),             LFUNCVAL( lu8g_setFontPosBottom ) },
//  { LSTRKEY( "setFontPosCenter" ),             LFUNCVAL( lu8g_setFontPosCenter ) },
//  { LSTRKEY( "setFontPosTop" ),                LFUNCVAL( lu8g_setFontPosTop ) },
//  { LSTRKEY( "setFontRefHeightAll" ),          LFUNCVAL( lu8g_setFontRefHeightAll ) },
//  { LSTRKEY( "setFontRefHeightExtendedText" ), LFUNCVAL( lu8g_setFontRefHeightExtendedText ) },
//  { LSTRKEY( "setFontRefHeightText" ),         LFUNCVAL( lu8g_setFontRefHeightText ) },
//  { LSTRKEY( "setRot90" ),                     LFUNCVAL( lu8g_setRot90 ) },
//  { LSTRKEY( "setRot180" ),                    LFUNCVAL( lu8g_setRot180 ) },
//  { LSTRKEY( "setRot270" ),                    LFUNCVAL( lu8g_setRot270 ) },
//  { LSTRKEY( "setScale2x2" ),                  LFUNCVAL( lu8g_setScale2x2 ) },
//  { LSTRKEY( "sleepOff" ),                     LFUNCVAL( lu8g_sleepOff ) },
//  { LSTRKEY( "sleepOn" ),                      LFUNCVAL( lu8g_sleepOn ) },
//  { LSTRKEY( "undoRotation" ),                 LFUNCVAL( lu8g_undoRotation ) },
//  { LSTRKEY( "undoScale" ),                    LFUNCVAL( lu8g_undoScale ) },
//  { LSTRKEY( "__gc" ),                         LFUNCVAL( lu8g_close_display ) },
  { LSTRKEY( "__index" ),                      LROVAL( ldisplay_map ) },
  { LNILKEY, LNILVAL }
};

/*
#undef U8G_DISPLAY_TABLE_ENTRY
#undef U8G_FONT_TABLE_ENTRY

//  { LSTRKEY( #device ),            LFUNCVAL ( lu8g_ ##device ) },
static const LUA_REG_TYPE lu8g_map[] = {
#define U8G_DISPLAY_TABLE_ENTRY(device) \
  { LSTRKEY( "disp" ),            LFUNCVAL ( lu8g_ ##device ) },
  U8G_DISPLAY_TABLE_I2C
//  U8G_DISPLAY_TABLE_SPI
// Register fonts
#define U8G_FONT_TABLE_ENTRY(font) \
  { LSTRKEY( #font ),              LUDATA( (void *)(u8g_ ## font) ) },
  U8G_FONT_TABLE
  //
  { LSTRKEY( "fb_rle" ),  LFUNCVAL( lu8g_fb_rle ) },
  // Options for circle/ ellipse drawing
  { LSTRKEY( "DRAW_UPPER_RIGHT" ), LNUMVAL( U8G_DRAW_UPPER_RIGHT ) },
  { LSTRKEY( "DRAW_UPPER_LEFT" ),  LNUMVAL( U8G_DRAW_UPPER_LEFT ) },
  { LSTRKEY( "DRAW_LOWER_RIGHT" ), LNUMVAL( U8G_DRAW_LOWER_RIGHT ) },
  { LSTRKEY( "DRAW_LOWER_LEFT" ),  LNUMVAL( U8G_DRAW_LOWER_LEFT ) },
  { LSTRKEY( "DRAW_ALL" ),         LNUMVAL( U8G_DRAW_ALL ) },
  // Display modes
  { LSTRKEY( "MODE_BW" ),          LNUMVAL( U8G_MODE_BW ) },
  { LSTRKEY( "MODE_GRAY2BIT" ),    LNUMVAL( U8G_MODE_GRAY2BIT ) },
  { LSTRKEY( "__metatable" ), LROVAL( lu8g_map ) },
  { LNILKEY, LNILVAL }
};
*/

int luaopen_disp( lua_State *L ) {
  ssd1306_init(ADDR);
  
  ssd1306_set_whole_display_lighting(ADDR, true);
  font = font_builtin_fonts[font_face];

  int i;
  for(i=0; i<(DISPLAY_WIDTH * DISPLAY_HEIGHT/8); i++)
	  ssd1306_buffer[i] = 0;
  ssd1306_clear_screen(ADDR);
  ssd1306_set_whole_display_lighting(ADDR, false);
  

//  luaL_newmetarotable(L, "u8g.display", (void *)lu8g_display_map);  // create metatable
//  lua_pop(L, 1);
  //luaL_newmetatable(L, "u8g.display", (void *)lu8g_display_map);
//  lua_pushrotable(L, (void *)lu8g_map);  // create metatable	
	//lua_pushtable(L,  (void *)lu8g_map);
  
//  luaL_newlib(L, lu8g_map);
  return 0; //1;
}

MODULE_REGISTER_MAPPED(SSD1306, oled, ldisplay_map, luaopen_disp );

