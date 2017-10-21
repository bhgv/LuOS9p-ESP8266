/*
 * bhgv, pca9685-pwm Lua module
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

//#include <common/i2c-def.h>
#include <pca9685/pca9685.h>


#define ADDR PCA9685_ADDR_BASE

#define I2C_BUS 0
#define SCL_PIN 5
#define SDA_PIN 4

#define PWM_FREQ 500


static int lpwm_setup(lua_State* L) {
    pca9685_init(ADDR);
    pca9685_set_pwm_frequency(ADDR, PWM_FREQ);
	pca9685_set_output_open_drain(ADDR, 1);
    return 0;
}

static int lpwm_setfreq( lua_State* L ) {
	int f = luaL_checkinteger(L, 1);
	pca9685_set_pwm_frequency(ADDR, f);
    return 0;
}

static int lpwm_getfreq( lua_State* L ) {
	lua_pushinteger(L, pca9685_get_pwm_frequency(ADDR) );
    return 1;
}

static int lpwm_restart( lua_State* L ) {
    pca9685_restart(ADDR);
    return 0;
}

static int lpwm_is_sleeping( lua_State* L ) {
    lua_pushboolean(L, pca9685_is_sleeping(ADDR) );
    return 1;
}

static int lpwm_sleep( lua_State* L ) {
	int b = lua_toboolean(L, 1);
    pca9685_sleep(ADDR, b);
    return 0;
}

static int lpwm_is_inverted( lua_State* L ) {
    lua_pushboolean(L, pca9685_is_output_inverted(ADDR) );
    return 1;
}

static int lpwm_invert( lua_State* L ) {
	int b = lua_toboolean(L, 1);
    pca9685_set_output_inverted(ADDR, b);
    return 0;
}

static int lpwm_is_o_drain( lua_State* L ) {
    lua_pushboolean(L, pca9685_get_output_open_drain(ADDR) );
    return 1;
}

static int lpwm_o_drain( lua_State* L ) {
	int b = lua_toboolean(L, 1);
    pca9685_set_output_open_drain(ADDR, b);
    return 0;
}

static int lpwm_set_val( lua_State* L ) {
	int n = lua_gettop(L);
	int ch = luaL_checkinteger(L, 1);
	int v = luaL_checkinteger(L, 2);

	int max = 4096;
	if(n > 2) max = luaL_checkinteger(L, 3);
	
	if(ch < 0 || ch > 15) return 0;
	
	if(max <= 0) max = 1;
	else if(max > 4096) max = 4096;

	if(v < 0) v = 0;
	else if(v > max) v = max;
	
	v = v * 4096 / max;
    pca9685_set_pwm_value(ADDR, ch, v);
    
    lua_pushboolean(L, 1);
    return 1;
}


static int get_pwm_num(lua_State* L){
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

static int lpwm_set_val_meta( lua_State* L ) {
	int ch=-1;
	int pwm = 0;
	float v;
		
	v = luaL_checknumber(L, 3);
//	if(v < 0.0 || v > 100.0) return 0;
	if(v < 0.0) v = 0.0;
	else if( v > 100.0) v = 100.0;

	ch = get_pwm_num(L);
	if(ch < 0 || ch > 15) return 0;
	
	if(ch == 5 || ch == 6) v = 100.0 - v; //inv for SGN0 & SGN1
	pwm = (int)(4095.0 * v / 100.0);
	
	pca9685_set_pwm_value(ADDR, ch, pwm);
	return 0;
}

static int lpwm_get_val_meta( lua_State* L ) {
	int ch=-1;
	int pwm = 0;
	float v;
	
	ch = get_pwm_num(L);
	if(ch < 0 || ch > 15) lua_pushnil( L);
	else{
		v = 100.0*( (float)pca9685_get_pwm_value(ADDR, ch) / 4095.0);
		if(ch == 5 || ch == 6) v = 100.0 - v; //inv for SGN0 & SGN1
		lua_pushnumber( L, v);
	}
	return 1;
}



#include "modules.h"

const LUA_REG_TYPE pwm_metatab[] =
{
  { LSTRKEY( "__newindex" ),	LFUNCVAL( lpwm_set_val_meta ) },
  //{ LSTRKEY( "__index" ),          LROVAL( pwm_metatab ) },
  { LSTRKEY( "__index" ),		LFUNCVAL( lpwm_get_val_meta ) },
  
  { LSTRKEY( "PWM0" ),  	 LINTVAL( 0 ) },
  { LSTRKEY( "PWM1" ),  	 LINTVAL( 1 ) },
  { LSTRKEY( "PWM2" ),  	 LINTVAL( 2 ) },
  { LSTRKEY( "PWM3" ),  	 LINTVAL( 3 ) },
  { LSTRKEY( "PWM4" ),  	 LINTVAL( 4 ) },
  { LSTRKEY( "SGN1" ),  	 LINTVAL( 5 ) },
  { LSTRKEY( "SGN0" ),  	 LINTVAL( 6 ) },
  { LSTRKEY( "DC3" ),  	 LINTVAL( 7 ) },
  { LSTRKEY( "DC2" ),  	 LINTVAL( 8 ) },
  { LSTRKEY( "DC1" ),  	 LINTVAL( 9 ) },
  { LSTRKEY( "LED1" ),  	 LINTVAL( 10 ) },
  { LSTRKEY( "LED2" ),  	 LINTVAL( 11 ) },
  { LSTRKEY( "LED3" ),  	 LINTVAL( 12 ) },
  { LSTRKEY( "LED4" ),  	 LINTVAL( 13 ) },
  { LSTRKEY( "LED5" ),  	 LINTVAL( 14 ) },
  { LSTRKEY( "LED6" ),  	 LINTVAL( 15 ) },
  { LNILKEY, LNILVAL }
};


const LUA_REG_TYPE pwm_tab[] =
{
//  { LSTRKEY( "init" ),       LFUNCVAL( lpwm_setup ) },
  { LSTRKEY( "set_freq" ),		 LFUNCVAL( lpwm_setfreq ) },
  { LSTRKEY( "get_freq" ),      LFUNCVAL( lpwm_getfreq ) },
  { LSTRKEY( "sleep" ),			LFUNCVAL( lpwm_sleep ) },
  { LSTRKEY( "is_sleep" ),		LFUNCVAL( lpwm_is_sleeping ) },
  { LSTRKEY( "open_drn" ),		LFUNCVAL( lpwm_o_drain ) },
  { LSTRKEY( "is_open_drn" ),	LFUNCVAL( lpwm_is_o_drain ) },
  { LSTRKEY( "invert" ),		LFUNCVAL( lpwm_invert ) },
  { LSTRKEY( "is_invert" ),		LFUNCVAL( lpwm_is_inverted ) },
  { LSTRKEY( "restart" ),		LFUNCVAL( lpwm_restart ) },
  { LSTRKEY( "ch_val" ),		LFUNCVAL( lpwm_set_val ) },
  
  { LSTRKEY( "__metatable" ), LROVAL( pwm_metatab ) },
  { LNILKEY, LNILVAL }
};

/* }====================================================== */



LUAMOD_API int luaopen_pwm (lua_State *L) {
  lpwm_setup(L);
  return 0;
}

MODULE_REGISTER_MAPPED(PWM, pwm, pwm_tab, luaopen_pwm);
