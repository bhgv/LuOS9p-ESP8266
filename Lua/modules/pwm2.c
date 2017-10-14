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

#include <common/i2c-def.h>
#include <pca9685/pca9685.h>


#define ADDR PCA9685_ADDR_BASE

#define I2C_BUS 0
#define SCL_PIN 5
#define SDA_PIN 4

#define PWM_FREQ 500


i2c_dev_t* dev=NULL;


static int lpwm_setup(lua_State* L) {
	if(dev == NULL) dev = (i2c_dev_t*)malloc(sizeof(i2c_dev_t));
	
//    i2c_init(I2C_BUS, SCL_PIN, SDA_PIN, I2C_FREQ_100K);
	dev->addr = ADDR;
	dev->bus = I2C_BUS;

    pca9685_init(dev);

    pca9685_set_pwm_frequency(dev, PWM_FREQ);
    //printf("Freq 1000Hz, real %d\n", pca9685_get_pwm_frequency(&dev));

    return 0;
}

static int lpwm_setfreq( lua_State* L ) {
	int f = luaL_checkinteger(L, 1);
	pca9685_set_pwm_frequency(dev, f);
    return 0;
}

static int lpwm_getfreq( lua_State* L ) {
	lua_pushinteger(L, pca9685_get_pwm_frequency(dev) );
    return 1;
}

static int lpwm_restart( lua_State* L ) {
    pca9685_restart(dev);
    return 0;
}

static int lpwm_is_sleeping( lua_State* L ) {
    lua_pushboolean(L, pca9685_is_sleeping(dev) );
    return 1;
}

static int lpwm_sleep( lua_State* L ) {
	int b = lua_toboolean(L, 1);
    pca9685_sleep(dev, b);
    return 0;
}

static int lpwm_is_inverted( lua_State* L ) {
    lua_pushboolean(L, pca9685_is_output_inverted(dev) );
    return 1;
}

static int lpwm_invert( lua_State* L ) {
	int b = lua_toboolean(L, 1);
    pca9685_set_output_inverted(dev, b);
    return 0;
}

static int lpwm_is_o_drain( lua_State* L ) {
    lua_pushboolean(L, pca9685_get_output_open_drain(dev) );
    return 1;
}

static int lpwm_o_drain( lua_State* L ) {
	int b = lua_toboolean(L, 1);
    pca9685_set_output_open_drain(dev, b);
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
    pca9685_set_pwm_value(dev, ch, v);
    
    lua_pushboolean(L, 1);
    return 1;
}



#include "modules.h"

const LUA_REG_TYPE pwm_tab[] =
{
  { LSTRKEY( "init" ),       LFUNCVAL( lpwm_setup ) },
  { LSTRKEY( "set_freq" ),   LFUNCVAL( lpwm_setfreq ) },
  { LSTRKEY( "get_freq" ),      LFUNCVAL( lpwm_getfreq ) },
  { LSTRKEY( "sleep" ),     LFUNCVAL( lpwm_sleep ) },
  { LSTRKEY( "is_sleep" ),     LFUNCVAL( lpwm_is_sleeping ) },
  { LSTRKEY( "open_drn" ),     LFUNCVAL( lpwm_o_drain ) },
  { LSTRKEY( "is_open_drn" ),       LFUNCVAL( lpwm_is_o_drain ) },
  { LSTRKEY( "invert" ),    LFUNCVAL( lpwm_invert ) },
  { LSTRKEY( "is_invert" ),       LFUNCVAL( lpwm_is_inverted ) },
  { LSTRKEY( "restart" ),    LFUNCVAL( lpwm_restart ) },
  { LSTRKEY( "ch_val" ),  LFUNCVAL( lpwm_set_val ) },
  { LSTRKEY( "PWM0" ),  	 LINTVAL( 0 ) },
  { LSTRKEY( "PWM1" ),  	 LINTVAL( 1 ) },
  { LSTRKEY( "PWM2" ),  	 LINTVAL( 2 ) },
  { LSTRKEY( "PWM3" ),  	 LINTVAL( 3 ) },
  { LSTRKEY( "PWM4" ),  	 LINTVAL( 4 ) },
  { LSTRKEY( "SGN1" ),  	 LINTVAL( 5 ) },
  { LSTRKEY( "SGN0" ),  	 LINTVAL( 6 ) },
  { LSTRKEY( "OUT3" ),  	 LINTVAL( 7 ) },
  { LSTRKEY( "OUT2" ),  	 LINTVAL( 8 ) },
  { LSTRKEY( "OUT1" ),  	 LINTVAL( 9 ) },
  { LSTRKEY( "LED0" ),  	 LINTVAL( 10 ) },
  { LSTRKEY( "LED1" ),  	 LINTVAL( 11 ) },
  { LSTRKEY( "LED2" ),  	 LINTVAL( 12 ) },
  { LSTRKEY( "LED3" ),  	 LINTVAL( 13 ) },
  { LSTRKEY( "LED4" ),  	 LINTVAL( 14 ) },
  { LSTRKEY( "LED5" ),  	 LINTVAL( 15 ) },
  { LNILKEY, LNILVAL }
};

/* }====================================================== */



LUAMOD_API int luaopen_pwm (lua_State *L) {
  return 0;
}

MODULE_REGISTER_MAPPED(PWM, pwm, pwm_tab, luaopen_pwm);
