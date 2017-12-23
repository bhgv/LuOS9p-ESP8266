/*
 * bhgv, callback loop module
 *
 * Copyright (C) 2017
 * Author: bhgv (http://github.com/bhgv)
 * 
 * All rights reserved.  
 *
 */

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
extern QueueHandle_t guiqueue;


void runCallbacks(void) {
	static uint16_t cb_delay = 0;
	static uint32_t tv, gtv;
		mtx_lock(&cb_mtx);
		
		if(cbqueue != NULL){
			if(cb_delay >= CB_DELAY){
				tv = cb_delay; 
				xQueueSend(cbqueue, &tv, 0);
				cb_delay = 0;
			}else{
				cb_delay++;
			}
		}
		
		if(guiqueue != NULL){
			gtv = 1;
			xQueueSend(guiqueue, &gtv, 0);
		}
		
		// Called every tick
		mtx_unlock(&cb_mtx);
}


void tloop_cb_noop(){}

void tloop_cb1() __attribute__((weak, alias("tloop_cb_noop")));
void tloop_cb2() __attribute__((weak, alias("tloop_cb_noop")));

static void _cb_task (lua_State *L, int n, int cnt ) { 
	uint32_t v;
	int i = cnt;
	while(cnt == 0 || i > 0) {
		if(!portIN_ISR()){
			if(cnt > 0) i--;
			mtx_lock(&tloop_mtx);
			if(xQueueReceive(cbqueue, &v, 1024)) {
				if(v > 0) { 
					tloop_cb1();
					tloop_cb2();
					
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

