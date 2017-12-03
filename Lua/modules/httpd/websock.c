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

#include "lwip/ip_addr.h"
#include "lwip/tcp.h"
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



ws_node* ws_clients[WS_MAX];
int ws_clients_cnt = 0;


const char WS_HEADER[] = "Upgrade: websocket"; // \r\n";
const char WS_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const char WS_KEY[] = "Sec-WebSocket-Key: ";
const char WS_RSP[] = "HTTP/1.1 101 Switching Protocols\r\n" \
					  "Upgrade: websocket\r\n" \
					  "Connection: Upgrade\r\n" \
					  "Sec-WebSocket-Accept: %s\r\n\r\n";


void websocket_write(struct netconn *nc, const uint8_t *data, uint16_t len, uint8_t mode){
    if (len > OUT_BUF_LEN-2)
        return;
    unsigned char* buf = malloc(len + 2);
    buf[0] = 0x80 | mode;
    buf[1] = len;
    memcpy(&buf[2], data, len);
    len += 2;
	usleep(10);
	netconn_write(nc, buf, len, NETCONN_NOCOPY);
	free(buf);
}


static err_t websocket_parse(struct netconn *nc, unsigned char *data, u16_t data_len){
    if (data != NULL && data_len > 1) {
		if ((data[0] & 0x80) == 0) {
		  printf("Warning: continuation frames not supported\n");
		  return ERR_VAL;
		}
        uint8_t opcode = data[0] & 0x0F;
        switch (opcode) {
            case 0x01: // text
            case 0x02: // bin
                if (data_len > 6) {
                    data_len -= 6;
                    /* unmask */
                    for (int i = 0; i < data_len; i++)
                        data[i + 6] ^= data[2 + i % 4];
                    /* user callback */
					return ERR_OK;
                }
				return ERR_VAL;
                break;
            case 0x08: // close
                return ERR_CLSD;
                break;
        }
        return ERR_VAL;
    }
    return ERR_VAL;
}


static err_t websocket_close(struct netconn *nc)
{
    const char buf[] = {0x88, 0x02, 0x03, 0xe8};
    u16_t len = sizeof(buf);

	return netconn_write(nc, buf, len, NETCONN_COPY);
}



extern int nc_clients_cnt;

static int websocket_connect(struct netconn *nc, char* data, int data_len/*, char* out, int max_out_len*/){
	int r=0;
	if ( 
		strnstr(data, WS_HEADER, data_len) 
	) {
		if(nc_clients_cnt > 1) return 2;
		
	    unsigned char encoded_key[32];
	    char key[64];
	    char *key_start = strnstr(data, WS_KEY, data_len);
	    if (key_start) {
	        key_start += 19;
	        char *key_end = strnstr(key_start, "\r\n", data_len);
	        if (key_end) {
	            int len = sizeof(char) * (key_end - key_start);
	            if (len + sizeof(WS_GUID) < sizeof(key) && len > 0) {
	                /* Concatenate key */
	                memcpy(key, key_start, len);
	                strlcpy(&key[len], WS_GUID, sizeof(key));
	                printf("Resulting key: %s\n", key);
	                /* Get SHA1 */
	                unsigned char sha1sum[20];
	                mbedtls_sha1((unsigned char *) key, sizeof(WS_GUID) + len - 1, sha1sum);
	                /* Base64 encode */
	                unsigned int olen;
	                mbedtls_base64_encode(NULL, 0, &olen, sha1sum, 20); //get length
	                int ok = mbedtls_base64_encode(encoded_key, sizeof(encoded_key), &olen, sha1sum, 20);
	                if (ok == 0) {
						int buf_len = sizeof(WS_RSP) + olen + 4;
						char* buf = malloc(buf_len+1);
					
	                    encoded_key[olen] = '\0';
	                    printf("Base64 encoded: %s\n", encoded_key);
	                    /* Send response */
	                    u16_t len = snprintf(buf, buf_len, WS_RSP, encoded_key);
						err_t err = ERR_OK;
						if( check_conn(__func__, __LINE__) ){
	                    	err = netconn_write(nc, buf, len, NETCONN_NOCOPY);
							print_err(err, __func__, __LINE__);
						}
						free(buf);
						
						r = 1;
	                }
	            }else {
	                printf("Key overflow\n");
	                return ERR_MEM;
	            }
			}
	    }
//    } else {
//        printf("Malformed packet\n");
//        r= ERR_ARG;
    }
	return r;
}



typedef float (*ws_dev_foo)(int, float);

struct ws_dev_tab {
	char* name;
	ws_dev_foo foo;
};



#include <pca9685/pca9685.h>

static float dev_pwm(int ch, float v){
	float ov = -1.0;
	int pwm = 0;

//	printf("from dev_pwm ch=%d, v=%f\n", ch, v);

//	if(ch < 0 || ch > 15) return -1.;

	if(ch >= 0 && ch <= 15){
		if(v < 0.0){
			pwm = pca9685_get_pwm_value(PCA9685_ADDR_BASE, ch);
			ov = 100.0*( (float)pwm / 4095.0);
			if(ch == 5 || ch == 6) ov = 100.0 - ov; //inv for SGN0 & SGN1
		}else{
			if(v >= 0.0 && v <= 100.0){
				if(ch == 5 || ch == 6) v = 100.0 - v; //inv for SGN0 & SGN1
				pwm = (int)(4095.0 * v / 100.0);
				pca9685_set_pwm_value(PCA9685_ADDR_BASE, ch, pwm);
			}
			
			pwm = pca9685_get_pwm_value(PCA9685_ADDR_BASE, ch);
			ov = 100.0*( (float)pwm / 4095.0);
			if(ch == 5 || ch == 6) ov = 100.0 - ov; //inv for SGN0 & SGN1
		}
	}

//	printf("exit dev_pwm ov=%f\n", ov);
	return ov;
}



#include <pcf8591/pcf8591.h>
extern unsigned char dac;

static float dev_adc(int ch, float v){
	float ov = -1.0;

	//printf("from dev_adc ch=%d, v=%f\n", ch, v);

	if(ch == -2) 
		ov = ( 100.0*(float)dac )/255.0;
	else if(ch == -3) 
		ov = ( 100.0*(float)sdk_system_adc_read() )/1023.0;
	else if(ch >= 0 && ch <= 3)
		ov = ( 100.0*
			(float)pcf8591_read(PCF8591_DEFAULT_ADDRESS, (unsigned char)ch) 
			)/255.0;
	else return -1.0;

	
	if(ch == -2 && v >= 0.0 && v <=100.0) {
		dac = (unsigned char)(255.0 * v / 100.0);
		pcf8591_write(PCF8591_DEFAULT_ADDRESS, dac);
	
		ov = ( 100.0*(float)dac )/255.0;
	}

	//printf("exit dev_adc ov=%f\n", ov);

	return ov;
}

static float dev_dac(int idx, float par){
	return 0.;
}



#include <pcf8574/pcf8575.h>

static float dev_pio(int ch, float par){
	float ov = 0.0;

	if(par < 0.0){
		if(ch == -2)
			ov = (float)pcf8575_port_read(PCF8575_DEFAULT_ADDRESS);
		//else if(ch < 0 || ch > 15) 
		//	lua_pushnil(L);
		else
			ov = ( pcf8575_gpio_read(PCF8575_DEFAULT_ADDRESS, (uint8_t)ch) != 0 ? 1.0 : 0.0 );
	}else{
		uint16_t v = (par > 0.0) ? 1 : 0;
		
		if(ch == -2){
			pcf8575_port_write(PCF8575_DEFAULT_ADDRESS, v );
			ov = (float)pcf8575_port_read(PCF8575_DEFAULT_ADDRESS);
		}else if(ch >= 0 && ch <= 15) {
			pcf8575_gpio_write(PCF8575_DEFAULT_ADDRESS, (uint8_t)ch, v);
			ov = ( pcf8575_gpio_read(PCF8575_DEFAULT_ADDRESS, (uint8_t)ch) != 0 ? 1.0 : 0.0 );
		}
	}
	
	return ov;
}


struct ws_dev_tab dev_tab[] = {
	{"pwm", dev_pwm},
	{"pio", dev_pio},
	{"adc", dev_adc},
	{"dac", dev_adc},
	{NULL, NULL}
};



void ws_sock_del(int idx){
	ws_node* el;

	DBG("%s: %d i=%d, cnt=%d\n", __func__, __LINE__, idx, ws_clients_cnt);
	if(idx < 0 || idx >= WS_MAX || idx >= ws_clients_cnt) return;

	el = ws_clients[idx];
	if( el != NULL ){
		if(el->clnt != NULL){
			DBG("%s: %d pre ws close\n", __func__, __LINE__);
			websocket_close(el->clnt);
			printf("%s: %d post ws close\n", __func__, __LINE__);
			char *s = malloc(81);
			snprintf(s, 80,  "Closing connection (ws_client %d from %d)\n", idx, ws_clients_cnt);
			nc_free(&(el->clnt), s);
			free(s);
		}
		
		if(el->uri != NULL){
			free(el->uri);
			el->uri = NULL;
		}
		
		free(el);
	}
	
	for( ; idx < ws_clients_cnt-1; idx++){
		ws_clients[idx] = ws_clients[idx+1];
	}
	
	ws_clients_cnt--;
	ws_clients[ ws_clients_cnt ] = NULL;
}


void ws_sock_del_doubles(struct netconn *ws_client){
	if(ws_client == NULL) return;

	u32_t ip = ws_client->pcb.tcp->remote_ip.addr;

	int i;
	for(i = ws_clients_cnt-1; i >= 0; i--){
		ws_node* nd = ws_clients[ i ];
		struct netconn *clnt = nd->clnt;
		if(clnt->pcb.tcp->remote_ip.addr == ip){
			ws_sock_del( i );
		}
	}
}


ws_node* ws_sock_add(struct netconn *new_client){
	int idx;
	ws_node* el = NULL;

	if(ws_clients_cnt == WS_MAX ){
		ws_sock_del( WS_MAX-1 );
	}

	ws_sock_del_doubles( new_client );
	
	el = malloc( sizeof(ws_node) );
	if( el == NULL ) return NULL;
	
	for(idx = ws_clients_cnt; idx > 0; idx--){
		ws_clients[idx] = ws_clients[idx-1];
	}
	ws_clients_cnt++;
	
	el->clnt = new_client;
	el->uri = NULL;
	el->age = 0;

	ws_clients[0] = el;
	
	return el;
}


void ws_sock_gotop(int idx){
	if( idx < 0 || idx >= ws_clients_cnt ) return;
//	if( idx == 0 ) return;

	ws_node *el = ws_clients[ idx ];

	for( ; idx > 0; idx-- ){
		ws_clients[ idx ] = ws_clients[ idx-1 ];
	}
	ws_clients[0] = el;
}


void ws_socks_del(){
	while( ws_clients_cnt > 0 ){		
//		DBG("%s: %d  cnt=%d\n", __func__, __LINE__, ws_clients_cnt);
		ws_sock_del( 0 );
	}
//	ws_clients_cnt = 0;
}



extern get_par* get_root;

void ws_sub_task(lua_State *L, int idx){
	err_t err = ERR_OK;
	int t_is_httpd_run = is_httpd_run;

	if( idx < 0 || idx >= WS_MAX || idx >= ws_clients_cnt ) return;

	ws_node *el = ws_clients[idx];

	if(el == NULL) return;

	el->age++;

	//lua_pushnil(L);
	//lua_setglobal(L, "wsData");

	if(is_httpd_run == 1 || is_httpd_run == 2) {
		struct netbuf *nb=NULL;

		if (
			el->clnt != NULL && 
			check_conn(__func__, __LINE__) &&
			(err = print_err( netconn_recv(el->clnt, &nb), __func__, __LINE__ ) ) == ERR_OK
		) {
			unsigned char *data;
			u16_t len;
		
			if( check_conn(__func__, __LINE__) ){
				err = netbuf_data(nb, &data, &len);
				print_err(err, __func__, __LINE__);
			}
			if(is_httpd_run == 4) 
				return;
			
			if( websocket_parse(el->clnt, data, len) == ERR_OK){
				unsigned char* ws_data = &data[6];
				int ws_data_len = len-6;
				DBG("ws_task: %s %d\n", &data[6], len-6);

				if(el->uri != NULL){
					if(!strcmp(el->uri, "/dev")){
						char c1, c2, c3;
						int idx;
						float par;
						char *s = malloc(ws_data_len + 1);
						memcpy(s, ws_data, ws_data_len);
						s[ws_data_len] = '\0';
						
						//netbuf_delete(nb);
						//nb = NULL;
						ws_data = NULL;
						
						int r = sscanf(s, "%c%c%c[%d]=%f", &c1, &c2, &c3, &idx, &par);
						free(s);

						DBG("after scanf res = %d %c%c%c[%d]=%f\n", r, c1, c2, c3, idx, par);
						if( r == 5 ){
							struct ws_dev_tab* p_dev_tab = dev_tab;
							for( ; p_dev_tab->name != NULL; p_dev_tab++ ){
								if(
									c1 == p_dev_tab->name[0] &&
									c2 == p_dev_tab->name[1] &&
									c3 == p_dev_tab->name[2] 
								){
									DBG("try run dev = %s (%x)\n", p_dev_tab->name, p_dev_tab->foo);
									float res = p_dev_tab->foo(idx, par);
									char *res_s = malloc(30);
									DBG("after run dev = %s, res = %f\n", p_dev_tab->name, res);
									if(res_s){
										int l = snprintf(res_s, 29, "%s[%d]=%.2f", 
											p_dev_tab->name, idx, res
										);
										websocket_write(el->clnt, res_s, l, 1);
										free(res_s);

										el->age = 0;
										ws_sock_gotop(idx);
									}

									break;
								}
							}
							
						}
					}else{
						const char* s;
						int l;
						int n = lua_gettop(L);

//						get_root = el->get_root;
					
						luaC_fullgc(L, 1);
						
						lua_pushlstring(L, ws_data, ws_data_len);
						lua_setglobal(L, "wsData");

						luaL_dofile(L, el->uri);
						s = lua_tolstring(L, -1, &l);
						
						DBG("res of %s:\n%s\n", el->uri, s);
						
						websocket_write(el->clnt, (const uint8_t*)s, l, 1);
						lua_settop(L, n);
					
						luaC_fullgc(L, 1);

						el->age = 0;
						ws_sock_gotop(idx);
					}
				}
//				netbuf_delete(nb);
//				nb = NULL;
			} 
		}else{
			if(is_httpd_run == 4 ){
				if(
					err == ERR_ABRT ||
					err == ERR_CLSD
				){
					ws_sock_del(idx);
					el = NULL;
					is_httpd_run = t_is_httpd_run;
				}else{
					ws_socks_del();
					el = NULL;
				}
			}
		}
		
		if(nb != NULL){
			netbuf_delete(nb);
			nb = NULL;
		}

		if(el != NULL && el->age >= WS_TIMEOUT_UNCONNECT){
			ws_sock_del(idx);
			//ws_timeout = 0;
		}
	}

}


void ws_task(lua_State *L){
	int i;
	
	if(is_httpd_run == 1 || is_httpd_run == 2) {
		for(i = 0; i < ws_clients_cnt; i++){
			ws_sub_task(L, i);
		}
	}else{
		ws_socks_del();
	}
}


int do_websock(char **uri, int uri_len, char *hdr, char* hdr_sz, char* data, int len, nc_node *node ){
	if(node == NULL) return 0;
	
	char *suff = node->suf;
	
	int r=websocket_connect(node->clnt, data, len);
	
	DBG("after websocket_connect %x %d %d\n", data, len, r );
	if(r == 2){
		node->type = NC_TYPE_GET;
		return 2;
	}else 
	if(r == 1){ 
		node->type = NC_TYPE_WS;
		ws_node *el = ws_sock_add(node->clnt);

		node->clnt = NULL;

		if(!strncmp(*uri, "/dev", uri_len)){
			char *u = malloc(5);
			u[0] = '/';
			u[1] = 'd';
			u[2] = 'e';
			u[3] = 'v';
			u[4] = '\0';

			el->uri = u;

			return 1;
		}

		int ws_uri_len = uri_len+5 + 1;
		char *u = malloc(ws_uri_len + 12);
		el->uri = u;
		
		u[0] = '/';
		u[1] = 'h';
		u[2] = 't';
		u[3] = 'm';
		u[4] = 'l';
		memcpy(&u[5], *uri, uri_len);

		if(suff != NULL && ( (!strcmp(suff, ".lua")) || (!strcmp(suff, ".cgi") ) ) ){
			u[ws_uri_len] = '\0';
		}else{
			if( u[ws_uri_len - 1] == '/' ){
				ws_uri_len--;
			}
			const char index_lua[] = "/index.lua";
			memcpy( &u[ws_uri_len], index_lua, sizeof(index_lua) );
			ws_uri_len += sizeof(index_lua);
			u[ws_uri_len] = '\0';
		}
		DBG("ws cgi path = %s\n", u);

		int f = SPIFFS_open(&fs, u, SPIFFS_RDONLY, 0); 
		DBG("ws cgi file test = %d\n", f);
		if(f <= 0){
			ws_socks_del();
		}else{
			SPIFFS_close(&fs, f);
		}

		DBG("post websocket_connect %d\n", r );
	}

	DBG("pre exit websocket_connect %d\n", r );
	return r > 0;
}



