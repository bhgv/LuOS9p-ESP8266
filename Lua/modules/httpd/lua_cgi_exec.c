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



static int totl = 0;

extern get_par* get_root;


int do_lua_pas(lua_State* L, char** outstr, nc_node *node ){
	err_t err;
	int outlen = 0; 
	int n_c = lua_gettop(L);
	int r;
	
	*outstr = NULL;

	get_root = node->get_root;

	lua_pushvalue(L, node->cgi_lvl);
	DBG("a try to call %s\n", lua_typename(L, lua_type(L, -1) ) );
	r = lua_pcall(L, 0, 1, 0);
	switch(r){
		case LUA_OK: 
			DBG("do_lua: ok run '%s'\n", node->pth);

			if( !lua_isnoneornil(L, -1) ){
				*outstr = lua_tolstring(L, -1, &outlen);
				if( outlen > 0 && *outstr != NULL ) {
					if(outlen > OUT_BUF_LEN){
						DBG("do_lua: output string len = %d, but MUST be <= %d\n", 
							outlen, OUT_BUF_LEN);
					
					}else{
						DBG("pre netconn_write lua %x %d\n", *outstr, outlen);
						if( check_conn(__func__, __LINE__) ){
							err = netconn_write(node->clnt, *outstr, outlen, NETCONN_NOCOPY);

							print_err(err, __func__, __LINE__);
						}
						//if(is_httpd_run == 4) 
						//	break;
					}
				}else{
					//outlen = 0;
					//lua_settop(L, n_c)
					*outstr = NULL;
				}
			}
			lua_settop(L, n_c);
			break;
		
		case LUA_ERRRUN: 
			DBG("do_lua: can't run '%s'\n", node->pth);
			break;
		
		case LUA_ERRMEM: 
			DBG("do_lua: no mem to run '%s'\n", node->pth);
			break;
		
		case LUA_ERRERR: 
			DBG("do_lua: runtime error in '%s'\n", node->pth);
			break;
		
		case LUA_ERRGCMM: 
			DBG("do_lua: gc error in '%s'\n", node->pth);
			break;
	};

	lua_settop(L, n_c);
	luaC_fullgc(L, 1);

	get_root = NULL;
	
	return outlen;
}


int do_lua_begin(char **uri, int uri_len, char *hdr, char* hdr_sz, lua_State* L, nc_node *node ){
	err_t err = ERR_OK;
	get_par** ppget_list = &node->get_root;

	if(node->pth != NULL) free( node->pth );

	char *pth = malloc(uri_len + 10);
	node->pth = pth;

	if( pth == NULL ) return ERR_MEM;

	pth[0] = '/'; pth[1] = 'h'; pth[2] = 't'; pth[3] = 'm'; pth[4] = 'l'; //pth[] = ''; pth[] = ''; 
	memcpy(&pth[5], *uri, uri_len);
	pth[ uri_len + 5 ] = '\0';
	DBG("%s: %d path = %s, get_list=%x\n", __func__, __LINE__, pth, *ppget_list);

	int n_l = lua_gettop(L);
	
	int r = luaL_loadfile(L, pth);
	switch( r ){
		case LUA_OK: {
				char *outstr = NULL;
				int outlen = 0;
				node->cgi_lvl = lua_gettop(L);
		
				DBG("do_lua: file '%s' loaded\n", pth);

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
				if( check_conn(__func__, __LINE__) ){
					err = netconn_write(node->clnt, hdr, strlen(hdr), NETCONN_NOCOPY);
					print_err(err, __func__, __LINE__);
				}
				if(is_httpd_run == 4) 
					break;
				
				//DBG("pre hdr_lua_tail_wrt %x %d\n", hdr_nosiz, strlen(hdr_nosiz) );
				if( check_conn(__func__, __LINE__) ){
					err = netconn_write(node->clnt, hdr_sz, strlen(hdr_sz), NETCONN_NOCOPY);
					print_err(err, __func__, __LINE__);
				}
				if(is_httpd_run == 4) 
					break;

			}
			return 1;
			//break;
			
		case LUA_ERRSYNTAX: 
			DBG("do_lua: syntax error in '%s'\n", pth);
			break;
			
		case LUA_ERRMEM: 
			DBG("do_lua: can't alloc mem to load '%s'\n", pth);
			break;
			
		case LUA_ERRGCMM:
			DBG("do_lua: can't gc in '%s'\n", pth);
			break;
			
		case LUA_ERRFILE:
			DBG("do_lua: not found file path = %s\n", pth);
			break;
	};

	lua_settop(L, n_l);
	luaC_fullgc(L, 1);

	return 0;
}


void do_lua_end(lua_State* L, nc_node *node){
	if( node->cgi_lvl > 0 ){
		lua_settop(L, node->cgi_lvl - 1 );
	}
	node->cgi_lvl = 0;
	luaC_fullgc(L, 1);
}



extern nc_node* nc_clients[];
extern int nc_clients_cnt;

int do_lua(char **uri, int uri_len, char *hdr, char* hdr_sz, lua_State* L, int nd_idx ){
	nc_node *node = nc_clients[ nd_idx ];
	
	DBG( "%s: %d nc_state=%d\n", __func__, __LINE__, node->state );
	switch(node->state){
		case NC_BEGIN:
			totl = 0;
			int r = do_lua_begin(uri, uri_len, hdr, hdr_sz, L, node );

			if( r > 0 ){
				node->state = NC_PAS;
			}else if( r == ERR_MEM ){
				node->state = NC_CLOSE;
			}else{
				node->state = NC_END;
			}
			break;
	
		case NC_PAS:
			{
				char *outstr;
				
				int tlen = do_lua_pas(L, &outstr, node );

				if(is_httpd_run == 4){
//					do_lua_end( L, node );
//					node->state = NC_CLOSE;
					node->state = NC_END;
					break;
				}

				if(tlen > 0 && outstr != NULL ){
					totl += tlen;
				}else{
					node->state = NC_END;
				}
			}
			//return 0;
			break;
		
		case NC_END:
			do_lua_end( L, node );
			node->state = NC_CLOSE;
			break;
			
		case NC_CLOSE:
			nc_sock_del( nd_idx );
			return -1;
			
	}
	return 0;
}


