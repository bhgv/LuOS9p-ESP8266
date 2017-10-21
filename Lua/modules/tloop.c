/*
 * bhgv, callback loop module
 *
 * Copyright (C) 2017
 * Author: bhgv (http://github.com/bhgv)
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

//#include "whitecat.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <sys/mutex.h>

#include "lua.h"
#include "lauxlib.h"
#include "modules.h"

#include "tloop.h"


#define CB_DELAY 15

struct mtx cb_mtx;
struct mtx tloop_mtx;

QueueHandle_t cbqueue=NULL;



void runCallbacks(void) {
	static uint16_t cb_delay = 0;
	uint32_t tv;
	if(cbqueue != NULL){
		mtx_lock(&cb_mtx);
		if(cb_delay >= CB_DELAY){
			tv = cb_delay; 
			xQueueSend(cbqueue, &tv, 0);
			cb_delay = 0;
		}else{
			cb_delay++;
		}
		// Called every tick
		mtx_unlock(&cb_mtx);
	}
}


static void _cb_task (lua_State *L, int n, int cnt ) { 
	uint32_t v;
	int i = cnt;
	while(cnt == 0 || i > 0) {
		if(!portIN_ISR()){
			if(cnt > 0) i--;
			mtx_lock(&tloop_mtx);
			if(xQueueReceive(cbqueue, &v, 1024)) {
				if(v > 0) { 
					lua_pushvalue(L, n);
					lua_call(L, 0, 0);
					lua_gc(L, LUA_GCCOLLECT, 0);
				}
			}
			mtx_unlock(&tloop_mtx);
		}
	}
}

void _cb_init(void) {
    mtx_init(&cb_mtx, NULL, NULL, 0);  
    mtx_init(&tloop_mtx, NULL, NULL, 0);  
	cbqueue = xQueueCreate(1, sizeof(uint32_t));
}


int doLoop(lua_State *L){
	if(lua_type(L, 1) == LUA_TFUNCTION){
		_cb_task(L, 1, luaL_optinteger(L, 2, 0));
	}
	return 0;
}


static const LUA_REG_TYPE tloop_map[] = {
  { LSTRKEY( "run" ),           LFUNCVAL( doLoop) },
  { LNILKEY, LNILVAL }
};


int luaopen_tloop( lua_State *L ) {
  return 0;
}


MODULE_REGISTER_MAPPED(TLOOP, tloop, tloop_map, luaopen_tloop);

