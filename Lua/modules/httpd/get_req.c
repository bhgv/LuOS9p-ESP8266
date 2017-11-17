/*
 * Copyright bhgv 2017
 */

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "espressif/esp_common.h"


#include "httpd/httpd.h"


#if 0
#define DBG(...) printf(__VA_ARGS__)
#else
#define DBG(...)
#endif




get_par* get_root = NULL;



int get_list_add(get_par** first, char* name, char* val){
	get_par* par = malloc( sizeof(get_par) );
	if( par == NULL ) return 0;

	par->name = name;
	par->val = val;
	par->next = *first;
	*first = par;

	return 1;
}

void get_list_free(get_par** first){
	get_par* par = *first;
	get_par* nxt;
	for( ; par != NULL; par = nxt){
		free(par->name);
		free(par->val);
		nxt = par->next;
		free(par);
	}
	*first = NULL;
}




int lget_params_cnt(lua_State* L) {
	int i = 0;
	get_par* el = get_root;
	for( ; el != NULL; el = el->next){
		i++;
	}
	lua_pushinteger(L, i);
	return 1;
}

int lget_param(lua_State* L) {
	get_par* el = get_root;
	if(lua_gettop(L) == 0 || lua_type(L, 1) == LUA_TNIL){
		return lget_params_cnt(L);
	}else if(lua_type(L, 1) == LUA_TNUMBER){
		int i = lua_tointeger(L, 1);
		for( ; el != NULL && i > 1; el = el->next, i--);
		if(el == NULL){
			return 0;
		}else{
			lua_pushstring(L, el->name);
			lua_pushstring(L, el->val);
			return 2;
		}
	}else if(lua_type(L, 1) == LUA_TSTRING){
		char *s = lua_tostring(L, 1);
		for( ; el != NULL && !strcmp(s, el->name); el = el->next);
		if(el == NULL){
			return 0;
		}else{
			//lua_pushstring(L, el->name);
			lua_pushstring(L, el->val);
			return 1; //2;
		}
	}
	
	return 0;
}




