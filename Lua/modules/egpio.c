/*
 * bhgv, pcf8591-egpio Lua module
 *
 * Copyright (C) 2017
 * 
 * Author: bhgv (https://github.com/bhgv)
 * 
 * All rights reserved.  
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */


#include "lua.h"
#include "lauxlib.h"

#include "esp8266.h"
#include <drivers/cpu.h>
#include <drivers/gpio.h>

//#include <common/i2c-def.h>
#include <pcf8574/pcf8575.h>


#define ADDR PCF8575_DEFAULT_ADDRESS

#define I2C_BUS 0
#define SCL_PIN 5
#define SDA_PIN 4

#define INT_PIN 15



void gpio15_interrupt_handler();


static int legpio_setup(lua_State* L) {
	pcf8575_init(ADDR);

//	gpio_pin_output(INT_PIN);
	gpio_pin_opendrain(INT_PIN);
	gpio_pin_set(INT_PIN);
//	gpio_pin_input(INT_PIN);
	gpio_pin_pullup(INT_PIN);	
//	gpio_set_interrupt(INT_PIN, GPIO_INTTYPE_EDGE_NEG);
    return 0;
}

static int legpio_read( lua_State* L ) {
    int n = lua_gettop(L);
	uint8_t ch;
    uint8_t res;

	for(int i = 1; i <= n; i++){
	    ch = luaL_checkinteger(L, i);

    	if(ch < 0 || ch >15){
			lua_pushnil(L);
			continue;
    	}

	    res = pcf8575_gpio_read(ADDR, ch); 
    	lua_pushinteger(L, res);
	}
    return n; 
}

static int legpio_write( lua_State* L ) {
    int n = lua_gettop(L);
	uint8_t ch;
	uint8_t v;

	for(int i = 1; i < n; i+=2){
	    ch = luaL_checkinteger(L, i);
	    v = lua_toboolean(L, i+1);

    	if(ch < 0 || ch >15){
//			lua_pushnil(L);
			continue;
    	}

		pcf8575_gpio_write(ADDR, ch, v); 
	}
    return 0;
}


#if 0
void IRAM gpio15_interrupt_handler(){ 
	printf("gpio 15  intr\n");
}
#endif


static int get_gpio_isr_stat(lua_State* L){
	lua_pushboolean( L, gpio_pin_get(15));
	return 1;
}




static int get_gpio_num(lua_State* L){
	int ch = -1;
	if(lua_type(L, 2) == LUA_TNUMBER) ch = lua_tointeger(L, 2);
	else if(lua_type(L, 2) == LUA_TSTRING) {
		if(lua_getmetatable(L, 1)){
			lua_pushvalue(L,2);
			lua_rawget(L, -2);
			if(lua_type(L, -1) == LUA_TNUMBER) ch = lua_tointeger(L, -1);
			lua_pop(L, 2);
		}
	}
	if(ch < 0 || ch > 15) ch = -1;
	return ch;
}


static int legpio_get_val_meta( lua_State* L ) {
	int ch = get_gpio_num(L);
	if(ch == -1)
		lua_pushinteger(L, pcf8575_port_read(ADDR) );
	else if(ch < 0 || ch > 15) 
		lua_pushnil(L);
	else
		lua_pushboolean(L, (int)(
				pcf8575_gpio_read(ADDR, (unsigned char)ch) != 0
				)
		);
	return 1;
}

static int legpio_set_val_meta( lua_State* L ) {
	unsigned char v;
	int ch = get_gpio_num(L);
	
	if(ch == -1)
		pcf8575_port_write(ADDR, (uint16_t)luaL_optinteger( L, 3, 0) );
	else if(ch >= 0 && ch <= 15) {
		v = lua_toboolean(L, 3);
		pcf8575_gpio_write(ADDR, (char)ch, v);
	}
	return 0;
}




#include "modules.h"

const LUA_REG_TYPE egpio_metatab[] =
{
	{ LSTRKEY( "__newindex" ),       LFUNCVAL( legpio_set_val_meta ) },
	{ LSTRKEY( "__index" ),          LFUNCVAL( legpio_get_val_meta ) },

	{ LSTRKEY( "IO0" ),				LINTVAL( 0 ) },
	{ LSTRKEY( "IO1" ),				LINTVAL( 1 ) },
	{ LSTRKEY( "IO2" ),				LINTVAL( 2 ) },
	{ LSTRKEY( "IO3" ),				LINTVAL( 3 ) },
	{ LSTRKEY( "IO4" ),				LINTVAL( 4 ) },
	{ LSTRKEY( "AC3" ), 			LINTVAL( 5 ) },
	{ LSTRKEY( "AC2" ), 			LINTVAL( 6 ) },
	{ LSTRKEY( "AC1" ), 			LINTVAL( 7 ) },
	{ LSTRKEY( "bOK" ), 			LINTVAL( 8 ) },
	{ LSTRKEY( "bLFT" ), 			LINTVAL( 9 ) },
	{ LSTRKEY( "bRGT" ), 			LINTVAL( 10 ) },
	{ LSTRKEY( "bUP" ), 			LINTVAL( 11 ) },
	{ LSTRKEY( "bDWN" ), 			LINTVAL( 12 ) },
	{ LSTRKEY( "bLPg" ), 			LINTVAL( 13 ) },
	{ LSTRKEY( "bRPg" ), 			LINTVAL( 14 ) },
	{ LSTRKEY( "bExtKey" ), 		LINTVAL( 15 ) },

	{ LSTRKEY( "pins" ), 			LINTVAL( -1 ) },

 	{ LNILKEY, LNILVAL }
};


const LUA_REG_TYPE egpio_tab[] =
{
//  { LSTRKEY( "init" ),		LFUNCVAL( legpio_setup ) },
  { LSTRKEY( "get" ),		LFUNCVAL( legpio_read ) },
  { LSTRKEY( "set" ),		LFUNCVAL( legpio_write ) },

	{ LSTRKEY( "isr" ), 	  LFUNCVAL( get_gpio_isr_stat ) },
  
  { LSTRKEY( "__metatable" ), LROVAL( egpio_metatab ) },
  { LNILKEY, LNILVAL }
};

/* }====================================================== */



LUAMOD_API int luaopen_egpio (lua_State *L) {
  legpio_setup(L);
  return 0;
}

MODULE_REGISTER_MAPPED(EGPIO, gpio, egpio_tab, luaopen_egpio);
