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


font_info_t *font = NULL; // current font
font_face_t font_face = 0;

static ssd1306_color_t foreground = OLED_COLOR_WHITE;
static ssd1306_color_t background = OLED_COLOR_BLACK;

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

uint8_t ssd1306_buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8];


static const LUA_REG_TYPE ldisplay_map[];




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

	lua_pushrotable(L, (void *)ldisplay_map); 
    return 1;
}

static int ldisp_mode( lua_State *L )
{
	int i = lua_tointeger( L, 1);

	ssd1306_set_mem_addr_mode(ADDR, i);

	lua_pushrotable(L, (void *)ldisplay_map); 
    return 1;
}


int ldisp_cls( lua_State *L )
{
	int i;
	for(i=0; i<(DISPLAY_WIDTH * DISPLAY_HEIGHT/8); i++)
		ssd1306_buffer[i] = 0;
	ssd1306_set_whole_display_lighting(ADDR, false);
//	ssd1306_clear_screen(ADDR);

	lua_pushrotable(L, (void *)ldisplay_map); 
    return 1;
}

// Lua: oled.setFont( font_num )
static int ldisp_setFont( lua_State *L )
{
	int i = luaL_checkinteger( L, 1);
	if(i >= 0 && i < font_builtin_fonts_count){
		font = font_builtin_fonts[i];
		font_face = i;
		lua_pushinteger(L, i);
	}else{
		lua_pushinteger(L, font_builtin_fonts_count);
	}
		
    return 1;
}

static int ldisp_getFontInfo( lua_State *L )
{
/*
	int i = luaL_checkinteger( L, 1);
	if(i >= 0 && i < font_builtin_fonts_count){
		font = font_builtin_fonts[i];
		font_face = i;
		lua_pushinteger(L, i);
	}else{
		lua_pushinteger(L, font_builtin_fonts_count);
	}
*/
	if(lua_gettop( L)==0){
		lua_pushinteger(L, font_face);
		lua_pushinteger(L, font->height);
	}else{
		int i = luaL_checkinteger( L, 1);
		lua_pushinteger(L, i);
		lua_pushinteger(L, font_builtin_fonts[i]->height);
	}

    return 2;
}


// Lua: u8g.setDefaultBackgroundColor( self )
static int ldisp_setDefaultBackgroundColor( lua_State *L )
{
	background = OLED_COLOR_BLACK;
	
	lua_pushrotable(L, (void *)ldisplay_map); 
    return 1;
}

// Lua: u8g.setDefaultForegroundColor( self )
static int ldisp_setDefaultForegroundColor( lua_State *L )
{
	foreground = OLED_COLOR_WHITE;
	
	lua_pushrotable(L, (void *)ldisplay_map); 
    return 1;
}


// Lua: u8g.setContrast( self, constrast )
static int ldisp_setContrast( lua_State *L )
{
    ssd1306_set_contrast( ADDR, luaL_checkinteger( L, 1 ) );

	lua_pushrotable(L, (void *)ldisplay_map); 
    return 1;
}

// Lua: u8g.setColorIndex( self, color )
static int ldisp_setColorIndex( lua_State *L )
{
    return 0;
}

// Lua: int = u8g.getColorIndex( self )
static int ldisp_getColorIndex( lua_State *L )
{
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

	lua_pushrotable(L, (void *)ldisplay_map); 
	return 1;
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
    u8g_uint_t args[4];
    ldisp_get_int_args( L, 1, 4, args );

	ssd1306_draw_line(ADDR, ssd1306_buffer, args[0], args[1], args[2], args[3], 
							luaL_optinteger( L, 5, foreground)
	);

	lua_pushrotable(L, (void *)ldisplay_map); 
    return 1;
}

// Lua: u8g.drawTriangle( self, x0, y0, x1, y1, x2, y2 )
static int ldisp_drawTriangle( lua_State *L )
{
    u8g_uint_t args[6];
    ldisp_get_int_args( L, 1, 6, args );

	ssd1306_draw_triangle(ADDR, ssd1306_buffer, 
				args[0], args[1], args[2], args[3], args[4], args[5], 
				luaL_optinteger( L, 7, foreground)
				);

	lua_pushrotable(L, (void *)ldisplay_map); 
	return 1;
}

// Lua: u8g.drawBox( self, x, y, width, height )
static int ldisp_drawBox( lua_State *L )
{
    u8g_uint_t args[4];
    ldisp_get_int_args( L, 1, 4, args );

	ssd1306_fill_rectangle(ADDR, ssd1306_buffer, args[0], args[1], args[2], args[3], 
									luaL_optinteger( L, 5, foreground)
	);

	lua_pushrotable(L, (void *)ldisplay_map); 
    return 1;
}


// Lua: u8g.drawFrame( self, x, y, width, height )
static int ldisp_drawFrame( lua_State *L )
{
    u8g_uint_t args[4];
    ldisp_get_int_args( L, 1, 4, args );

	ssd1306_draw_rectangle(ADDR, ssd1306_buffer, args[0], args[1], args[2], args[3], 
									luaL_optinteger( L, 5, foreground)
	);

	lua_pushrotable(L, (void *)ldisplay_map); 
    return 1;
}


// Lua: u8g.drawDisc( self, x0, y0, rad, opt = U8G_DRAW_ALL )
static int ldisp_drawDisc( lua_State *L )
{
    u8g_uint_t args[3];
    ldisp_get_int_args( L, 1, 3, args );

	ssd1306_fill_circle(ADDR, ssd1306_buffer, args[0], args[1], args[2], 
								luaL_optinteger( L, 4, foreground)
	);

	lua_pushrotable(L, (void *)ldisplay_map); 
    return 1;
}

// Lua: u8g.drawCircle( self, x0, y0, rad, opt = U8G_DRAW_ALL )
static int ldisp_drawCircle( lua_State *L )
{
    u8g_uint_t args[3];
    ldisp_get_int_args( L, 1, 3, args );

	ssd1306_draw_circle(ADDR, ssd1306_buffer, args[0], args[1], args[2], 
								luaL_optinteger( L, 4, foreground)
	);

	lua_pushrotable(L, (void *)ldisplay_map); 
    return 1;
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
    u8g_uint_t args[2];
    ldisp_get_int_args( L, 1, 2, args );

	ssd1306_draw_pixel(ADDR, ssd1306_buffer, args[0], args[1], 
								luaL_optinteger( L, 3, foreground)
	);

	lua_pushrotable(L, (void *)ldisplay_map); 
    return 1;
}

// Lua: u8g.drawHLine( self, x, y, width )
static int ldisp_drawHLine( lua_State *L )
{
    u8g_uint_t args[3];
    ldisp_get_int_args( L, 1, 3, args );

	ssd1306_draw_hline(ADDR, ssd1306_buffer, args[0], args[1], args[2], 
								luaL_optinteger( L, 4, foreground)
	);

	lua_pushrotable(L, (void *)ldisplay_map); 
    return 1;
}

// Lua: u8g.drawVLine( self, x, y, width )
static int ldisp_drawVLine( lua_State *L )
{
    u8g_uint_t args[3];
    ldisp_get_int_args( L, 1, 3, args );

	ssd1306_draw_vline(ADDR, ssd1306_buffer, args[0], args[1], args[2], 
								luaL_optinteger( L, 4, foreground)
	);

	lua_pushrotable(L, (void *)ldisplay_map); 
    return 1;
}

// Lua: u8g.drawXBM( self, x, y, width, height, data )
static int ldisp_drawXBM( lua_State *L )
{
    u8g_uint_t args[4];
    ldisp_get_int_args( L, 1, 4, args );

    const char *xbm_data = luaL_checkstring( L, 1+4 );
    if (xbm_data == NULL)
        return 0;

    ssd1306_load_xbm(ADDR, (const uint8_t *)xbm_data, ssd1306_buffer);

	lua_pushrotable(L, (void *)ldisplay_map); 
    return 1;
}

// Lua: u8g.drawBitmap( self, x, y, count, height, data )
static int ldisp_drawBitmap( lua_State *L )
{
    u8g_uint_t args[4];
    ldisp_get_int_args( L, 1, 4, args );

    const char *bm_data = luaL_checkstring( L, 1+4 );
    if (bm_data == NULL)
        return 0;

    ssd1306_load_xbm(ADDR, (const uint8_t *)bm_data, ssd1306_buffer);

	lua_pushrotable(L, (void *)ldisplay_map); 
    return 1;
}

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

	lua_pushrotable(L, (void *)ldisplay_map); 
    return 1;
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
  { LSTRKEY( "XBM" ),						LFUNCVAL( ldisp_drawXBM ) },

  { LSTRKEY( "setContrast" ),				LFUNCVAL( ldisp_setContrast ) },
  { LSTRKEY( "setColorIndex" ),				LFUNCVAL( ldisp_setColorIndex ) },
  { LSTRKEY( "setDefBgClr" ), 				LFUNCVAL( ldisp_setDefaultBackgroundColor ) },
  { LSTRKEY( "setDefFgClr" ),				LFUNCVAL( ldisp_setDefaultForegroundColor ) },
  { LSTRKEY( "setFont" ),					LFUNCVAL( ldisp_setFont ) },
  { LSTRKEY( "getFontInfo" ), 				LFUNCVAL( ldisp_getFontInfo ) },
//  { LSTRKEY( "__gc" ),                         LFUNCVAL( lu8g_close_display ) },
  { LSTRKEY( "__index" ),                      LROVAL( ldisplay_map ) },
  { LNILKEY, LNILVAL }
};


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

