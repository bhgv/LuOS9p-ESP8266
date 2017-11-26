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

//#include <espressif/esp_common.h>
//#include <etstimer.h>

//typedef uint8_t uint8;

//#include <dhcpserver.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "espressif/esp_common.h"
//#include "espressif/phy_info.h"

#include "lwip/api.h"
//#include "ipv4/lwip/ip.h"
#include "lwip/tcp.h"
//#include "lwip/tcp_impl.h"

#include "mbedtls/sha1.h"
#include "mbedtls/base64.h"

#include "httpd/httpd.h"



#if 0
#define DBG(...) printf(__VA_ARGS__)
#else
#define DBG(...)
#endif



int is_httpd_run = 0;

char *hdr, *hdr_sz;


#if 1 //LWIP_HTTPD_STRNSTR_PRIVATE
/** Like strstr but does not need 'buffer' to be NULL-terminated */

void system_soft_wdt_feed(){
//	wdt_flg = false;
//	WDT.FEED = WDT_FEED_MAGIC;
	sdk_wdt_feed();
}

char*
strnstr(const char* buffer, const char* token, size_t n)
{
  const char* p;
  int tokenlen = (int)strlen(token);
  if (tokenlen == 0) {
    return (char *)buffer;
  }
  if (n == 0) {
    n = (int)strlen(buffer);
  }
//DBG("strnstr 1 %s %s %d\n", buffer, token, n);
  for (p = buffer; *p && (p + tokenlen <= buffer + n); p++) {
    if ((*p == *token) && (strncmp(p, token, tokenlen) == 0)) {
//DBG("strnstr 2 %s\n", p);
      return (char *)p;
    }
  }
  return NULL;
}

char*
strstr(const char* buffer, const char* token)
{
  return strnstr(buffer, token, 0);
}

#endif /* LWIP_HTTPD_STRNSTR_PRIVATE */



#if 1

const char webpage_404[] = {
        "<html><head><title>luos9 v0.1 Server</title>"
        "<style> div.main {"
        "font-family: Arial;"
        "padding: 0.01em 16px;"
        "box-shadow: 2px 2px 1px 1px #d2d2d2;"
        "background-color: #f1f1f1;}"
        "</style></head>"
        "<body><div class='main'>"
        "<h3>404</h3>"
        "<p>URL: %s</p>"
        "<p>Uptime: %d seconds</p>"
        "<p>Free heap: %d bytes</p>"
        "</div></body></html>"
};


int recv_timeout = DEF_RECV_TIMEOUT;
int send_timeout = DEF_SEND_TIMEOUT;

struct netconn *nc = NULL;

nc_node* nc_clients[ NC_MAX ];
int nc_clients_cnt = 0;


void nb_free(struct netbuf **nb){
	if(*nb != NULL){
		netbuf_delete(*nb);
		*nb = NULL;
	}
}


void nc_free(struct netconn **nc, char* msg){
	if(*nc != NULL){
		if(msg != NULL) 
			printf(msg);
		netconn_close(*nc);
		netconn_delete(*nc);
		*nc = NULL;
	}
}


void nc_sock_del(int idx){
	nc_node* el;
	
	if(idx < 0 || idx >= NC_MAX || idx >= nc_clients_cnt) return;

	el = nc_clients[idx];
	
	DBG("Removing connection (%d from %d)\n", idx, nc_clients_cnt);
	if( el != NULL ){
		if( el->clnt != NULL ){
			char *s = malloc(81);
			snprintf(s, 80, "Closing connection (client %d from %d)\n", idx, nc_clients_cnt);
			nc_free(&(el->clnt), s);
			free(s);
		}
		if(el->uri != NULL){
			free(el->uri);
			el->uri = NULL;
		}
		if(el->suf != NULL){
			free(el->suf);
		}
		
		if(el->cur_f > 0){
			SPIFFS_close(&fs, el->cur_f);
		}
		
		if(el->get_root != NULL){
			get_list_free(&el->get_root);
		}
//		if(el->nb != NULL){
//			nb_free(&el->nb);
//		}
		if(el->pth != NULL){
			free(el->pth);
		}
		
		free(el);
	}
	
	for( ; idx < nc_clients_cnt-1; idx++){
		nc_clients[idx] = nc_clients[idx+1];
	}

	nc_clients_cnt--;
	nc_clients[ nc_clients_cnt ] = NULL;
}


int nc_sock_add(struct netconn *new_client){
	int idx = -1;
	nc_node* el = NULL;

	if( nc_clients_cnt >= NC_MAX ){
		return -1;
	}
		
	el = malloc( sizeof(nc_node) );
	if(el == NULL)
		return -1;
	
	el->clnt = new_client;
	el->uri = NULL;
	el->suf = NULL;
	el->get_root = NULL;
//	el->nb = NULL;
	el->state = NC_CLOSE;
	el->cur_f = 0;
	
	// VVV lua cgi
	el->cgi_lvl = 0;
	el->pth = NULL;
	// AAA lua cgi

	idx = nc_clients_cnt;
	
	nc_clients[ idx ] = el;
	nc_clients_cnt++;
	
	return idx;
}


int do_404(char **uri, int uri_len, char *hdr, char* hdr_sz, nc_node *node ){
	char* buf=NULL;
	int buf_len = 0;
	err_t err = ERR_OK;
	
	buf_len = sizeof(webpage_404) +20 + 1; //OUT_BUF_LEN;
	buf = malloc(buf_len); // OUT_BUF_LEN+1);

	if(buf){	
		DBG("pre sprintf %x %d\n", buf, buf_len);
		int out_len = snprintf(buf, buf_len, webpage_404,
				*uri,
				xTaskGetTickCount() * portTICK_PERIOD_MS / 1000,
				(int) xPortGetFreeHeapSize()
		);
		
		DBG("pre hdr_net_wrt %x %d\n", hdr, strlen(hdr) );
		if( check_conn(__func__, __LINE__) ){
			err = netconn_write(node->clnt, hdr, strlen(hdr)+1, NETCONN_NOCOPY);
			print_err(err, __func__, __LINE__);
		}
		if(is_httpd_run == 4) 
		{
			free(buf);
			return 0;
		}
		
		if( check_conn(__func__, __LINE__) ){
			//char *hb = malloc(sizeof(hdr_sz)+10);
			//snprintf(hb, sizeof(hdr_siz)+10-1, hdr_sz, strlen(buf));
			//err = netconn_write(client, hb, strlen(hb), NETCONN_NOCOPY);
			err = netconn_write(node->clnt, hdr_sz, strlen(hdr_sz), NETCONN_NOCOPY);
			//free(hb);
			
			print_err(err, __func__, __LINE__);
		}
		if(is_httpd_run == 4) 
		{
			free(buf);
			return 0;
		}
		
		DBG("pre net out %x %d\n%s\n", buf, buf_len, buf);
		if( check_conn(__func__, __LINE__) ){
			err = netconn_write(node->clnt, buf, out_len, NETCONN_NOCOPY);
			print_err(err, __func__, __LINE__);
		}
		free(buf);

		if(is_httpd_run == 4) 
			return 0;
	}

	return buf_len;
}


int httpd_pas(lua_State* L, int idx ){
	DBG( "%s: %d (%d from %d)\n", __func__, __LINE__, idx, nc_clients_cnt );
	if(idx < 0 || idx >= NC_MAX || idx >= nc_clients_cnt )
		return 0;
	
	nc_node *node = nc_clients[idx];
	DBG( "%s: %d node=%x\n", __func__, __LINE__, node );
	
	if(node == NULL) return 0;
	
	DBG( "%s: %d node->state=%d, node->uri=%x\n", __func__, __LINE__, node->state, node->uri );

	if( node->state == NC_CLOSE || node->uri == NULL ){
		nc_sock_del(idx);
		return 0;
	}
	
	DBG( "%s: %d node=%x\n", __func__, __LINE__, node );
	
	char *uri = node->uri;
	char *suf = node->suf;
	
	DBG( "%s: %d node=%d, uri=%x, suf=%x, clt=%x\n", __func__, __LINE__, node, uri, suf, node->clnt );
	
	int uri_len = uri == NULL ? 0 : strlen(uri);

	DBG( "%s: %d node=%d, uri=%s, suf=%s, clt=%x\n", __func__, __LINE__, node, uri, suf ? suf : "", node->clnt );

	hdr = suf_to_hdr(suf, &hdr_sz);
	
	//DBG("client->send_timeout %d\n", client->send_timeout );
	DBG( "%s: %d\n", __func__, __LINE__ );

#if 1
	if(uri != NULL && suf != NULL && ( (!strcmp(suf, ".lua")) || (!strcmp(suf, ".cgi")) ) ){
		node->type = NC_TYPE_GET;
		
		DBG("lua cgi = %s\n", uri);
		do_lua(&uri, uri_len, hdr, hdr_sz, L, idx );

		return 1;
	}else
#endif
	if( uri != NULL && do_file(&uri, uri_len, hdr, hdr_sz, /*node*/idx ) > 0){
		DBG( "%s: %d\n", __func__, __LINE__ );
		node->type = NC_TYPE_GET;
		
		return 1;
	}

	return 0;
}


int httpd_task(lua_State* L) {
	err_t err = ERR_OK;

	//DBG("httpd_task 1 Free mem: %d\n",xPortGetFreeHeapSize());

	lua_register(L, "_GET", lget_param);

	if( check_conn(__func__, __LINE__) ){
		nc = netconn_new(NETCONN_TCP);
		printf("Open connection (main nc)\n");
	}else{
		nc_free(&nc, NULL);
	}

    if (nc == NULL || is_httpd_run == 4) {
        printf("Failed to allocate socket.\n");
        return 0;
    }
	
	nc->recv_timeout = recv_timeout;
	nc->recv_bufsize = IN_BUF_LEN;

	ip_set_option(nc->pcb.tcp, SOF_REUSEADDR);
	//nc->pcb.tcp->so_options |= SOF_REUSEADDR;

	if( check_conn(__func__, __LINE__) ){
		err = netconn_bind(nc, IP_ADDR_ANY, 80);
		print_err(err, __func__, __LINE__);
	}

	if(is_httpd_run != 4){ 
		if( check_conn(__func__, __LINE__) ){
			err = netconn_listen(nc);
			print_err(err, __func__, __LINE__);
		}
	}

    while (is_httpd_run == 1 || is_httpd_run == 2) {
		struct netconn *clnt = NULL;
	
		//printf("httpd_task: Free mem: %d\n",xPortGetFreeHeapSize());

		if( check_conn(__func__, __LINE__) ){
			err = netconn_accept(nc, &clnt);
			print_err(err, __func__, __LINE__);
		}
		if(is_httpd_run == 4) {
			break;
		}
			
        if (clnt != NULL) 
			printf("Open connection (client) %x\n", clnt);

		nc_node* node = NULL;
		
        if (clnt != NULL && err == ERR_OK) {
            struct netbuf *nb=NULL;

			clnt->send_timeout = send_timeout;
			clnt->recv_timeout = recv_timeout;
			clnt->recv_bufsize = IN_BUF_LEN;

			ip_set_option(clnt->pcb.tcp, SOF_REUSEADDR);
			//client->pcb.tcp->so_options |= SOF_REUSEADDR;
			
			int nd_idx = nc_sock_add(clnt);
			if( nd_idx < 0){
				nc_free(&clnt, "client closed. too many connections\n" );
				clnt = NULL;
				node = NULL;
			}else{
				node = nc_clients[ nd_idx ];
			}
			
			if( 
				nd_idx >= 0 &&
				node != NULL &&
				check_conn(__func__, __LINE__) &&
				(err = print_err( netconn_recv(node->clnt, &nb), __func__, __LINE__ ) ) == ERR_OK
			) {
                void *data;
                u16_t len;
			
				if( !(is_httpd_run == 1 || is_httpd_run == 2) ){
					nb_free(&nb);
					node->state = NC_CLOSE;
					break;
				}

				if( check_conn(__func__, __LINE__) ){
	                err = netbuf_data(nb, &data, &len);
					print_err(err, __func__, __LINE__);
				}
				if(is_httpd_run == 4) {
					nb_free(&nb);
					node->state = NC_CLOSE;
					break;
				}
\				
                /* check for a GET request */
				if (!strncmp(data, "GET ", 4)) {
                    char *uri = NULL;
					char *suf = NULL;
					int suf_len = 0;
					int uri_len = 0;
					get_par* get_root = NULL;

					uri_len = get_uri(data, len, &uri, &suf, &suf_len, &get_root );

					node->uri = uri;
					node->suf = suf;
					node->get_root = get_root;
					get_root = NULL;
					
			//		node->nb = nb;
					node->state = NC_BEGIN;

					hdr = suf_to_hdr(suf, &hdr_sz);

#if 1
					if( do_websock(&uri, uri_len, hdr, hdr_sz, data, len, node ) > 0){
						nb_free(&nb);
						
						node->type = NC_TYPE_WS;
						node->state = NC_CLOSE;
						nc_sock_del( nd_idx );
					}else
#endif
					{
						ws_socks_del();

						nb_free(&nb);

						if( !httpd_pas(L, nd_idx) )
						{
							node->type = NC_TYPE_GET;
							
							do_404(&uri, uri_len, hdr, hdr_sz, node );
							
							node->state = NC_CLOSE;
							
							//nc_sock_del( nd_idx );
						}
					}

					if(node->state == NC_BEGIN)
						node->state = NC_CLOSE;

//					if(node->state == NC_CLOSE){
//						nc_sock_del(nd_idx);
//					}
                }
            }
			nb_free(&nb);
        }

		ws_task(L);
		
		if(nc_clients_cnt > 0){
			httpd_pas(L, 0);
		}

		if(is_httpd_run == 2)
			is_httpd_run = 3;
    }
	ws_socks_del();

	while(nc_clients_cnt > 0){
		nc_sock_del(0);
	}
	nc_clients_cnt = 0;

	nc_free(&nc, "Closing connection (main nc)\n");
	
	if(is_httpd_run == 3)
		is_httpd_run = 2;

	DBG("httpd_task 2 Free mem: %d\n",xPortGetFreeHeapSize());

	return 0;
}


int httpd_task_loop_run(lua_State* L){
	is_httpd_run = 1;
	int res;

	do{
		if(is_httpd_run == 4) {
			is_httpd_run = 1;
			sleep(2);
		}
		
		res = httpd_task(L);
	}while( is_httpd_run == 4 );
	
	return res;
}


int (*cb_httpd)(lua_State *L) = NULL;

int httpd_task_cb_run(lua_State* L){
	is_httpd_run = 2;

	cb_httpd = httpd_task;
	return 0;
}


void sdk_sta_status_set(int);

int httpd_task_stop(lua_State* L){
	sdk_sta_status_set(0);
	
	is_httpd_run = 0;

	cb_httpd = NULL;

//	client->send_timeout=1;
//	client->recv_timeout=1;

//	nc->send_timeout=1;
//	nc->recv_timeout=1;
	
	return 0;
}
#endif


static int httpd_recv_timeout(lua_State* L) {
	int timeout = recv_timeout;
	if(lua_gettop(L) > 0){
		recv_timeout = luaL_optinteger(L, 1, DEF_RECV_TIMEOUT);
		if(recv_timeout < 0) recv_timeout = DEF_RECV_TIMEOUT;
	}
	lua_pushinteger(L, timeout);
	return 1;
}


static int httpd_send_timeout(lua_State* L) {
	int timeout = send_timeout;
	if(lua_gettop(L) > 0){
		send_timeout = luaL_optinteger(L, 1, DEF_SEND_TIMEOUT);
		if(send_timeout < 0) send_timeout = DEF_SEND_TIMEOUT;
	}
	lua_pushinteger(L, timeout);
	return 1;
}



#include "modules.h"

const LUA_REG_TYPE httpd_map[] = {
//		{ LSTRKEY( "httpd" ),		LFUNCVAL( net_httpd_start ) },
//		{ LSTRKEY( "httpd" ),		LFUNCVAL( httpd_start ) },
		{ LSTRKEY( "loop" ),		LFUNCVAL( httpd_task_loop_run ) },
		{ LSTRKEY( "add_to_callbacks" ),	LFUNCVAL( httpd_task_cb_run ) },
		
		{ LSTRKEY( "stop" ),			LFUNCVAL( httpd_task_stop ) },
		
		{ LSTRKEY( "recv_timeout" ),	LFUNCVAL( httpd_recv_timeout ) },
		{ LSTRKEY( "send_timeout" ),	LFUNCVAL( httpd_send_timeout ) },
		
		{ LNILKEY, LNILVAL }
};


extern ws_node* ws_clients[];
extern int ws_clients_cnt;

int luaopen_httpd( lua_State *L ) {
	int i;
	for(i=0; i < WS_MAX; i++){
		ws_clients[ i ] = NULL;
	}
	ws_clients_cnt = 0;
	
	for(i=0; i < NC_MAX; i++){
		nc_clients[ i ] = NULL;
	}
	nc_clients_cnt = 0;
	
	cb_httpd = NULL;
	
	return 0;
}


MODULE_REGISTER_MAPPED(HTTPD, httpd, httpd_map, luaopen_httpd);


