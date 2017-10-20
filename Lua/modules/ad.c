/*
 * bhgv, pcf8591-adc-dac Lua module
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

#include <pcf8591/pcf8591.h>


#define ADDR PCF8591_DEFAULT_ADDRESS

#define I2C_BUS 0
#define SCL_PIN 5
#define SDA_PIN 4

#define PWM_FREQ 500


unsigned char dac = 0;

static int leadc_setup(lua_State* L) {
//    if(adcd == NULL) adcd = (i2c_dev_t*)malloc(sizeof(i2c_dev_t));
    
//    i2c_init(I2C_BUS, SCL_PIN, SDA_PIN, I2C_FREQ_100K);
//    adcd->addr = ADDR;
//    adcd->bus = I2C_BUS;
    return 0;
}

static int leadc_adc( lua_State* L ) {
//    int n = lua_gettop(L);
    uint8_t ch = luaL_checkinteger(L, 1);
    uint8_t res;
//    unsigned char a0, a1, a2, a3;

    if(ch < 0 || ch >3) return 0;

    res = pcf8591_read(ADDR, ch); // &a0, &a1, &a2, &a3);
    
    lua_pushinteger(L, res);
    return 1; 
}

static int leadc_dac( lua_State* L ) {
    int n = lua_gettop(L);
    int dat = luaL_checkinteger(L, 1);

    pcf8591_write(ADDR, dat);
    return 0;
}


static int get_adc_num(lua_State* L){
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
	return ch;
}


static int leadc_get_val_meta( lua_State* L ) {
	int ch = get_adc_num(L);
	if(ch == -2) 
		lua_pushnumber(L, ( 100.0*(float)dac )/255.0 );
	else if(ch < 0 || ch > 3) 
		lua_pushnil(L);
	else
		lua_pushnumber(L, ( 100.0*(float)pcf8591_read(ADDR, (unsigned char)ch) )/255.0 );
	return 1;
}

static int leadc_set_val_meta( lua_State* L ) {
	//unsigned char dac = 0;
	int ch;
	float v;
	
	if(get_adc_num(L) != -2) return 0;
	
	v = luaL_checknumber(L, 3);
	if(v < 0.0) v = 0.0;
	else if(v > 100.0) v = 100.0;
	
	dac = (unsigned char)(255.0 * v / 100.0);
	pcf8591_write(ADDR, dac);

	return 0;
}




#include "modules.h"

const LUA_REG_TYPE eadc_metatab[] =
{
  { LSTRKEY( "__newindex" ),       LFUNCVAL( leadc_set_val_meta ) },
  { LSTRKEY( "__index" ),          LFUNCVAL( leadc_get_val_meta ) },

  { LSTRKEY( "Light" ),			LINTVAL( 2 ) },
  { LSTRKEY( "DAC" ),			LINTVAL( -2 ) },
  { LNILKEY, LNILVAL }
};


const LUA_REG_TYPE eadc_tab[] =
{
//  { LSTRKEY( "init" ),		LFUNCVAL( ladc_setup ) },
  { LSTRKEY( "adc" ),		LFUNCVAL( leadc_adc ) },
  { LSTRKEY( "dac" ),		LFUNCVAL( leadc_dac ) },
  
  { LSTRKEY( "__metatable" ), LROVAL( eadc_metatab ) },
  { LNILKEY, LNILVAL }
};

/* }====================================================== */



LUAMOD_API int luaopen_eadc (lua_State *L) {
  leadc_setup(L);
  return 0;
}

MODULE_REGISTER_MAPPED(AD, adc, eadc_tab, luaopen_eadc);
