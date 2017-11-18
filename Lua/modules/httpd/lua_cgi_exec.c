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

//#include <etstimer.h>

/*
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
*/

#include "espressif/esp_common.h"

#include "lwip/api.h"
/*
//#include "ipv4/lwip/ip.h"
#include "lwip/tcp.h"
//#include "lwip/tcp_impl.h"
*/

#include "mbedtls/sha1.h"
#include "mbedtls/base64.h"


#include "httpd/httpd.h"



#if 0
#define DBG(...) printf(__VA_ARGS__)
#else
#define DBG(...)
#endif


extern struct netconn *client;




int do_lua_pas(lua_State* L,
	//int outlen,
	char *pth,
	get_par *get_el,
	int cgi_lvl,
	char** outstr
){
	err_t err;
	int outlen = 0; 
	int n_c = lua_gettop(L);
	int r;
	
	*outstr = NULL;

	lua_pushvalue(L, cgi_lvl);
	DBG("a try to call %s\n", lua_typename(L, lua_type(L, -1) ) );
	r = lua_pcall(L, 0, 1, 0);
	switch(r){
		case LUA_OK: 
			DBG("do_lua: ok run '%s'\n", pth);

			*outstr = lua_tolstring(L, -1, &outlen);
			if( outlen > 0 ) {
				if(outlen > OUT_BUF_LEN){
					DBG("do_lua: output string len = %d, but MUST be <= %d\n", 
						outlen, OUT_BUF_LEN);
				
				}
				DBG("pre netconn_write lua %x %d\n", *outstr, outlen);
				//usleep(10);
				if( check_conn(__func__, __LINE__) ){
					err = netconn_write(client, *outstr, outlen, NETCONN_NOCOPY);
					
					lua_settop(L, n_c);

					print_err(err, __func__, __LINE__);
				}
				//if(err != ERR_OK) 
				//if(is_httpd_run == 4) 
				//	break;
			}else{
				//outlen = 0;
				//lua_settop(L, n_c)
				*outstr = NULL;
			}

			break;
		
		case LUA_ERRRUN: 
			DBG("do_lua: can't run '%s'\n", pth);
			break;
		
		case LUA_ERRMEM: 
			DBG("do_lua: no mem to run '%s'\n", pth);
			break;
		
		case LUA_ERRERR: 
			DBG("do_lua: runtime error in '%s'\n", pth);
			break;
		
		case LUA_ERRGCMM: 
			DBG("do_lua: gc error in '%s'\n", pth);
			break;
	};

	lua_settop(L, n_c);
	luaC_fullgc(L, 1);
	//usleep(50);

	return outlen;
}



int do_lua(char **uri, int uri_len, char *hdr, char* hdr_sz, /*char* data, int len,*/ char *out, lua_State* L, get_par** ppget_list ){
	char *pth = malloc(uri_len + 10);
	err_t err = ERR_OK;
	
	if( pth == NULL ) return -1;

	pth[0] = '/'; pth[1] = 'h'; pth[2] = 't'; pth[3] = 'm'; pth[4] = 'l'; //pth[] = ''; pth[] = ''; 
	memcpy(&pth[5], *uri, uri_len);
	pth[ uri_len + 5 ] = '\0';
	DBG("do_lua: path = %s, get_list=%x\n", pth, *ppget_list);

	int n_l = lua_gettop(L);
	
	int r = luaL_loadfile(L, pth);
	switch( r ){
		case LUA_OK: {
				char *outstr = NULL;
				int outlen = 0;
				get_par *get_el = *ppget_list;
				int cgi_lvl = lua_gettop(L);
		
				DBG("do_lua: file '%s' loaded\n", pth);

				//ws_sock_del();

				/**
				lua_newtable(L);
				
				lua_pushliteral(L, "_G");
				lua_getupvalue(L, cgi_lvl, 1);
				lua_rawset(L, -3);
				//lua_setfield(L, -2, "_G");

				lua_pushliteral(L, "_GET");
				lua_newtable(L);
				while(get_el != NULL){
					lua_pushstring(L, get_el->name);
					lua_pushstring(L, get_el->val);
					lua_rawset(L, -3);

					DBG("par: %s = %s\n", get_el->name, get_el->val);

					get_el = get_el->next;
				}
				get_list_free(ppget_list);

				lua_rawset(L, -3);
				//TODO: set CGI table here
				lua_setupvalue(L, cgi_lvl, 1);
				**/

				//DBG("pre hdr_lua_wrt %x %d\n", lua_hdr, strlen(lua_hdr) );
				//usleep(10);
				if( check_conn(__func__, __LINE__) ){
					err = netconn_write(client, hdr, strlen(hdr), NETCONN_NOCOPY);
					print_err(err, __func__, __LINE__);
				}
				//if(err != ERR_OK) 
				if(is_httpd_run == 4) 
					return 0;
				
				//DBG("pre hdr_lua_tail_wrt %x %d\n", hdr_nosiz, strlen(hdr_nosiz) );
				if( check_conn(__func__, __LINE__) ){
					err = netconn_write(client, hdr_sz, strlen(hdr_sz), NETCONN_NOCOPY);
					print_err(err, __func__, __LINE__);
				}
				//if(err != ERR_OK) 
				if(is_httpd_run == 4) 
					return 0;

				do{
					outlen = do_lua_pas( L,
									pth,
									//outlen,
									get_el,
									cgi_lvl,
									&outstr
					);
					if(is_httpd_run == 4) 
						break;
				
				}while(outstr != NULL && outlen > 0);
				lua_settop(L, cgi_lvl-1);
				luaC_fullgc(L, 1);
				//usleep(50);
			}
			break;
			
		case LUA_ERRSYNTAX: 
			//ws_sock_del();
			DBG("do_lua: syntax error in '%s'\n", pth);
			break;
			
		case LUA_ERRMEM: 
			//ws_sock_del();
			DBG("do_lua: can't alloc mem to load '%s'\n", pth);
			break;
			
		case LUA_ERRGCMM:
			//ws_sock_del();
			DBG("do_lua: can't gc in '%s'\n", pth);
			break;
			
		case LUA_ERRFILE:
			DBG("do_lua: not found file path = %s\n", pth);
			break;
	};

	lua_settop(L, n_l);
	luaC_fullgc(L, 1);

	free(pth);

	return 0;
}




