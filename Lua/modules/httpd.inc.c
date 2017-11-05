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

#include "lwip/api.h"
//#include "ipv4/lwip/ip.h"
#include "lwip/tcp.h"
#include "lwip/tcp_impl.h"

#include "mbedtls/sha1.h"
#include "mbedtls/base64.h"


#if 1
#define DBG(...) printf(__VA_ARGS__)
#else
#define DBG(...)
#endif


#if 1 //LWIP_HTTPD_STRNSTR_PRIVATE
/** Like strstr but does not need 'buffer' to be NULL-terminated */
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

enum{
	HTML_tag=1,
	WS_tag
};

enum{
	REQ_GET=1,
	REQ_WS,
	REQ_WSconn,
};

enum{
	Q_NOT_PROCESSED=0,
	Q_PROCESSED,
	//REQ_WSconn,
};


#define IN_BUF_LEN  512
#define OUT_BUF_LEN  256 //312  //536 //312
//512

#undef MAXPATHLEN
#define MAXPATHLEN  55 //32 //55 
//PATH_MAX

typedef struct _get_par get_par;

struct _get_par {
	char *name;
	char *val;
	
	get_par* next;
};

get_par* get_root = NULL;


const char HTML_HEADER[] ={
"HTTP/1.1 200 OK\r\n"
"Content-type: text/html\r\n\r\n"
};

const char WS_HEADER[] = "Upgrade: websocket"; // \r\n";
const char WS_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const char WS_KEY[] = "Sec-WebSocket-Key: ";
const char WS_RSP[] = "HTTP/1.1 101 Switching Protocols\r\n" \
					  "Upgrade: websocket\r\n" \
					  "Connection: Upgrade\r\n" \
					  "Sec-WebSocket-Accept: %s\r\n\r\n";




    const char html_hdr[] = {
        "HTTP/1.1 200 OK\r\n" 
        "Content-type: text/html; charset=UTF-8\r\n" 
    };

    const char jpg_hdr[] = {
        "HTTP/1.1 200 OK\r\n" 
        "Content-type: image/jpeg\r\n" 
    };

    const char png_hdr[] = {
        "HTTP/1.1 200 OK\r\n" 
        "Content-type: image/png\r\n" 
    };

    const char gif_hdr[] = {
        "HTTP/1.1 200 OK\r\n" 
        "Content-type: image/gif\r\n" 
    };

    const char css_hdr[] = {
        "HTTP/1.1 200 OK\r\n" 
        "Content-type: text/css\r\n" 
    };

    const char js_hdr[] = {
        "HTTP/1.1 200 OK\r\n" 
        "Content-type: application/javascript\r\n" 
    };

    const char lua_hdr[] = {
        "HTTP/1.1 200 OK\r\n" 
	"Content-type: text/html; charset=UTF-8\r\n" 
    };
//       "Content-type: application/lua\r\n\r\n" 
	
	const char hdr_siz[] = "Keep-Alive: timeout=1, max=1\r\nConnection: close\r\nContent-Length: %d\r\n\r\n";
	const char hdr_nosiz[] = "Keep-Alive: timeout=1, max=1\r\nConnection: close\r\n\r\n";

	typedef struct {
		const char* suf;
		const char* hdr;
		const char* siz;
	} suf_hdr;
	
	static suf_hdr suf_hdr_tab[] = {
		{".jpg", jpg_hdr, hdr_siz},
		{".png", png_hdr, hdr_siz},
		{".gif", gif_hdr, hdr_siz},
		{".css", css_hdr, hdr_siz},
		{".js", js_hdr, hdr_siz},
		{".lua", lua_hdr, hdr_siz},
		{NULL, NULL}
	};

char* def_files[]={
	"index.htm",
	"index.html",
	"index.lua",
	"index.cgi",
	"index.ssi",
	NULL
};


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
        "<button onclick=\"location.href='/on'\" type='button'>"
        "LED On</button></p>"
        "<button onclick=\"location.href='/off'\" type='button'>"
        "LED Off</button></p>"
        "</div></body></html>"
    };


int is_httpd_run = 1;


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




void websocket_write(struct netconn *nc, const uint8_t *data, uint16_t len, uint8_t mode){
    if (len > OUT_BUF_LEN-2)
        return;
    unsigned char* buf = malloc(len + 2);
    buf[0] = 0x80 | mode;
    buf[1] = len;
    memcpy(&buf[2], data, len);
    len += 2;
//    tcp_write(nc, buf, len, TCP_WRITE_FLAG_COPY);
	netconn_write(nc, buf, len, NETCONN_COPY);
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
					//DBG("ws rcvd: %d %x %s\n", data_len, opcode, &data[6]);
                    //websocket_cb(nc, &data[6], data_len, opcode);
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
//    return tcp_write(pcb, buf, len, TCP_WRITE_FLAG_COPY);
	return netconn_write(nc, buf, len, NETCONN_COPY);
}

static int websocket_connect(struct netconn *nc, char* data, int data_len/*, char* out, int max_out_len*/){
	int r=0;
	//DBG("wsc 1\n%s\nlen = %d\n", data, data_len);
	if ( strnstr(data, WS_HEADER, data_len) ) {
		#if 1
	    unsigned char encoded_key[32];
	    char key[64];
	    char *key_start = strnstr(data, WS_KEY, data_len);
		//DBG("wsc 2\n");
	    if (key_start) {
	        key_start += 19;
	        char *key_end = strnstr(key_start, "\r\n", data_len);
			//DBG("wsc 3\n");
	        if (key_end) {
	            int len = sizeof(char) * (key_end - key_start);
				//DBG("wsc 4\n");
	            if (len + sizeof(WS_GUID) < sizeof(key) && len > 0) {
					//DBG("wsc 5\n");
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
						//DBG("wsc 6\n");
						char* buf = malloc(OUT_BUF_LEN+1);
						int buf_len = OUT_BUF_LEN;
					
	                    encoded_key[olen] = '\0';
	                    printf("Base64 encoded: %s\n", encoded_key);
	                    /* Send response */
	                    //char buf[256];
	                    u16_t len = snprintf(buf, buf_len, WS_RSP, encoded_key);
						//DBG("wsc 7 \n%s\n", buf);
	                    //http_write(pcb, buf, &len, TCP_WRITE_FLAG_COPY);
	                    netconn_write(nc, buf, len, NETCONN_NOCOPY);
						free(buf);
	                    //return r; // ERR_OK;
	                    //r = REQ_WS;
						r = 1; //REQ_WSconn;
	                }
	            }else {
	                printf("Key overflow\n");
	                return ERR_MEM;
	            }
			}
	    }
		#endif
//    } else {
//        printf("Malformed packet\n");
//        r= ERR_ARG;
    }
	return r;
}


void prep_something( ){

		/* disable LED */
	//	  gpio_enable(2, GPIO_OUTPUT);
	//	  gpio_write(2, true);
}


static int get_uri(char* in, int in_len, char** uri, char** suf, int* suf_len, get_par** ppget_root){
	*uri = NULL;
//	*uri_len = 0;

	char *sp1, *sp2, *sp3, *i, *sufp;
	/* extract URI */
	sp1 = in + 4;
	//sp2 = memchr(sp1, ' ', in_len);
	sufp=NULL;
	for(i = sp2 = sp1; i < sp1 + in_len; i++){
		if(*i == '.' ){
			sufp = i;
		}
		if(*i == ' ' || *i == '?' ){
			sp2 = i;
			break;
		}
	}
	int len = sp2 - sp1;
	if(len > MAXPATHLEN || len <= 0) 
		return 0;
	
	if(*sp2 == '?'){
		char *nm=NULL;
		char *val=NULL;
		int l;
		for(sp3 = i = sp2+1; i < sp1 + in_len; i++ ){
			if( nm == NULL && *i == '=' ){
				l = i - sp3;
				nm = malloc(l + 1);
				memcpy(nm, sp3, l);
				nm[l] = '\0';
				
				sp3 = i + 1;
			}
			if( nm != NULL && (*i == '&' || *i == ' ') ){
				l = i - sp3;
				val = malloc(l + 1);
				memcpy(val, sp3, l);
				val[l] = '\0';
				
				sp3 = i + 1;

				if( !get_list_add(ppget_root, nm, val) ){
					free(nm);
					free(val);
				}
				nm = NULL;
				val = NULL;
			}
			if(*i == ' ') break;
		}
//		if(nm != NULL && val != NULL){
//			get_list_add(ppget_root, nm, val);
//		}else{
			if(nm != NULL) free(nm); 
			if(val != NULL) free(val);
//		}
	}

	*uri = malloc(len+2);
	memcpy(*uri, sp1, len);
	(*uri)[len] = '\0';
	printf("uri: %s\n", *uri);

	if(suf!=NULL){
		if(sufp!=NULL){
			int l = sp2-sufp;
			if(suf_len!=NULL) 
				*suf_len = l;
			*suf = malloc(l+2);
			//memcpy(*suf, sufp, l);
			for( i = *suf; sufp < sp2; i++, sufp++ ){
				if(*sufp >= 'A' && *sufp <= 'Z')
					*i = 'a' + (*sufp - 'A');
				else
					*i = *sufp;
			}
			(*suf)[l] = '\0';
			printf("suffix: %s\n", *suf);
		}else{
			*suf = NULL;
			if(suf_len!=NULL) *suf_len = 0;
		}
	}
	
	return len;
}

static char* suf_to_hdr(char* suf, int suf_len, char** siz){
	char* r = html_hdr; //NULL;
	*siz = hdr_siz;
	int i=0;
	if(suf == NULL)
		return html_hdr; //NULL;
	while(suf_hdr_tab[i].suf != NULL){
		if(!strcmp(suf_hdr_tab[i].suf, suf)){
			r = suf_hdr_tab[i].hdr;
			*siz = suf_hdr_tab[i].siz;
			break;
		}
		i++;
	}
	return r;
}


#include <sys/spiffs/spiffs.h>
#include <sys/spiffs/spiffs_nucleus.h>
#include <sys/spiffs/esp_spiffs.h>


extern spiffs fs;

static /*int*/spiffs_file uri_to_file(char* uri, int len, int *flen){
	*flen=0;
//	int f;
	spiffs_stat stat;
	int res;
//	struct stat sb;
	spiffs_file r = -1;

	DBG("spiffs_file 1\n" );
	if(len + 12 > MAXPATHLEN){
		printf("Too long path: %db, must be < %d\n", len, MAXPATHLEN-12);
		return r;
	}
	char *p = malloc(len+12);
	
	DBG("spiffs_file 2\n");
	if(p==NULL){
		printf("No mem\n");
		return 0; //ERR_MEM;
	}
	p[0]='/'; p[1]='h'; p[2]='t'; p[3]='m'; p[4]='l';
	memcpy(&p[5], uri, len);
	p[len+5] = '\0';

	DBG("pre path = %s %d\n", p, len);
    if (is_dir(p) || p[len+4] == '/') {
		DBG("dir %s\n", p);
		
		char* tp = malloc(len+12+16);
		if(tp==NULL){
			printf("No mem\n");
			free(p);
			return 0; //ERR_MEM;
		}
		memcpy(tp, p, len+5);
		tp[len+5] = '\0';
		
		char* bg = &tp[len+5];
		if(p[len+4] != '/'){
			*bg = '/';
			bg++;
		}
		
		int i=0;
		while(def_files[i] != NULL ){
			char* s = def_files[i];
			memcpy(bg, s, strlen(s) + 1 );
			DBG("test path = %s\n", tp);
			r = SPIFFS_open(&fs, tp, SPIFFS_RDONLY, 0); 
			if(r>0){
				res = SPIFFS_fstat(&fs, r, &stat);
				if (res == SPIFFS_OK && stat.size > 0 ) {
					DBG("path = %s %d\n", tp, len);
					if(flen != NULL) *flen = stat.size;
					break;
				}else{
					SPIFFS_close(&fs, r);
					r = 0;
					if(flen != NULL) *flen = 0;
				}
			}
			i++;
		}
		free(tp);
		free(p);
		
        return r;
    }
	DBG("path = %s %d\n", p, len);
	
	r = SPIFFS_open(&fs, p, SPIFFS_RDONLY, 0); 
	DBG("after open %s r=%d\n", p, r);
	if(r > 0){
		DBG("pre fstat %s %d\n", p, r);
		res = SPIFFS_fstat(&fs, r, &stat);
		if (res == SPIFFS_OK ) {
			if(flen != NULL) *flen = stat.size;
		} else {
			SPIFFS_close(&fs, r);
			r = 0;
			if(flen != NULL) *flen = 0;
		}
	}
//    if (r < 0) {
//        r = spiffs_result(fs.err_code);
//    }

//	if (stat(p, &sb) == 0) {
//		DBG("pre open = %s\n", p);
//		r = open(p, O_RDONLY, 0);
//	}
	free(p);
	
	DBG("uri_to_file exit %d\n", r);
	return r;
}


#if 0
/**
 * Initialize the httpd with the specified local address.
 */
static void
httpd_init_addr(const ip_addr_t *local_addr)
{
  struct tcp_pcb *pcb;
  err_t err;

  pcb = tcp_new();
  LWIP_ASSERT("httpd_init: tcp_new failed", pcb != NULL);
  tcp_setprio(pcb, HTTPD_TCP_PRIO);
  /* set SOF_REUSEADDR here to explicitly bind httpd to multiple interfaces */
  err = tcp_bind(pcb, local_addr, HTTPD_SERVER_PORT);
  LWIP_ASSERT("httpd_init: tcp_bind failed", err == ERR_OK);
  pcb = tcp_listen(pcb);
  LWIP_ASSERT("httpd_init: tcp_listen failed", pcb != NULL);
  /* initialize callback arg and accept callback */
  tcp_arg(pcb, pcb);
  tcp_accept(pcb, http_accept);
}

/**
 * Initialize the httpd: set up a listening PCB and bind it to the defined port
 */
void
httpd_init(void)
{
#if HTTPD_USE_MEM_POOL
  LWIP_ASSERT("memp_sizes[MEMP_HTTPD_STATE] >= sizeof(http_state)",
     memp_sizes[MEMP_HTTPD_STATE] >= sizeof(http_state));
  LWIP_ASSERT("memp_sizes[MEMP_HTTPD_SSI_STATE] >= sizeof(http_ssi_state)",
     memp_sizes[MEMP_HTTPD_SSI_STATE] >= sizeof(http_ssi_state));
#endif
  DBG ("httpd_init\n");

  httpd_init_addr(IP_ADDR_ANY);
}
#endif



struct netconn *ws_client = NULL;
char* ws_uri = NULL;
int ws_len = 0;

void ws_sock_del(){
	if(ws_client != NULL){
		websocket_close(ws_client);
		
		printf("Closing connection (ws_client)\n");
		netconn_close(ws_client);
		netconn_delete(ws_client);
		ws_client = NULL;
	}
	
	if(ws_uri != NULL){
		free(ws_uri);
		ws_uri = NULL;
	}
}

void ws_task(lua_State *L){
	err_t err;

	lua_pushnil(L);
	lua_setglobal(L, "wsData");

	//while(1){
	if(is_httpd_run) {
		struct netbuf *nb=NULL;
		if (ws_client != NULL && (err = netconn_recv(ws_client, &nb)) == ERR_OK) {
			void *data;
			u16_t len;
		
			usleep(50);
		
			netbuf_data(nb, &data, &len);
			if( websocket_parse(ws_client, data, len) == ERR_OK){
				DBG("ws_task: %s %d\n", &data[6], len-6);

				if(ws_uri != NULL){
					char* s;
					int l;
				
					lua_pushlstring(L, &data[6], len-6);
					lua_setglobal(L, "wsData");
					luaC_fullgc(L, 1);
					
					luaL_dofile(L, ws_uri);
					s = lua_tolstring(L, -1, &l);
					
					DBG("res of %s:\n%s\n", ws_uri, s);
					
					websocket_write(ws_client, s, l, 1);
					lua_pop(L, 1);
				}
			} 
		}
		netbuf_delete(nb);
		
		usleep(50);
	} else {
		ws_sock_del();
	}

	luaC_fullgc(L, 1);
}


#define DEF_RECV_TIMEOUT  100
#define DEF_SEND_TIMEOUT  2*60000

int recv_timeout = DEF_RECV_TIMEOUT;
int send_timeout = DEF_SEND_TIMEOUT;

struct netconn *client = NULL;
struct netconn *nc = NULL; //netconn_new(NETCONN_TCP);


int do_something(char **uri, int uri_len, char *hdr, char* hdr_sz, char *out ){

//	if (!strncmp(uri, "/on", uri_len))
//		gpio_write(2, false);
//	else if (!strncmp(uri, "/off", uri_len))
//		gpio_write(2, true);

//	out = NULL;
	return 0;
}


int do_lua(char **uri, int uri_len, char *hdr, char* hdr_sz, char* data, int len, char *out, lua_State* L, get_par** ppget_list ){
	char *pth = malloc(uri_len + 10);
	if( pth == NULL ) return -1;

	pth[0] = '/'; pth[1] = 'h'; pth[2] = 't'; pth[3] = 'm'; pth[4] = 'l'; //pth[] = ''; pth[] = ''; 
	memcpy(&pth[5], *uri, uri_len);
	pth[ uri_len + 5 ] = '\0';
	DBG("do_lua: path = %s, get_list=%x\n", pth, *ppget_list);

	int n_l = lua_gettop(L);
	DBG("0 ");
	
	int r = luaL_loadfile(L, pth);
	DBG("1 ");
	switch( r ){
		case LUA_OK: {
				char *outstr = NULL;
				int outlen = 0;
		DBG("2 ");
				get_par *get_el = *ppget_list;
		DBG("3 ");
				int cgi_lvl = lua_gettop(L);
		//DBG("4 ");
		
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
				netconn_write(client, lua_hdr, strlen(lua_hdr), NETCONN_NOCOPY);
				
				//DBG("pre hdr_lua_tail_wrt %x %d\n", hdr_nosiz, strlen(hdr_nosiz) );
				netconn_write(client, hdr_nosiz, strlen(hdr_nosiz), NETCONN_NOCOPY);
				/*
				DBG("flen = %d\n", flen );
				char *hb = malloc(strlen(hdr_sz)+10);
				snprintf(hb, strlen(hdr_sz)+10-1, hdr_sz, flen);
				DBG("%s\n", hb );
				netconn_write(client, hb, strlen(hb), NETCONN_NOCOPY);
				free(hb);
				*/

				//lua_settop(L, cgi_lvl);
				//luaC_fullgc(L, 1);
				usleep(50);
				do{
					outlen = 0; outstr = NULL;
					int n_c = lua_gettop(L);
					
					lua_pushvalue(L, cgi_lvl);
					DBG("a try to call %s\n", lua_typename(L, lua_type(L, -1) ) );
					r = lua_pcall(L, 0, 1, 0);
					switch(r){
						case LUA_OK: 
							DBG("do_lua: ok run '%s'\n", pth);

							outstr = lua_tolstring(L, -1, &outlen);
							if( outlen > 0 ) {
								if(outlen > OUT_BUF_LEN){
									DBG("do_lua: output string len = %d, but MUST be <= %d\n", 
										outlen, OUT_BUF_LEN);
									
								}
								DBG("pre netconn_write lua %x %d\n", outstr, outlen);
								netconn_write(client, outstr, outlen, NETCONN_NOCOPY);
							}else{
								outstr = NULL;
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
					usleep(50);
				}while(outstr != NULL && outlen > 0);
				lua_settop(L, cgi_lvl-1);
				luaC_fullgc(L, 1);
				usleep(50);
			}
			break;
			
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

	free(pth);

	return 0;
}

int do_file(char **uri, int uri_len, char *hdr, char* hdr_sz, char* data, int len, char *out ){
	int flen;
	int f = uri_to_file(*uri, uri_len, &flen);
	char* buf=NULL;
	int buf_len;
	
	if(f > 0){
	//	free(uri);
	//	uri_len = 0;
		int totl=0;
		
		DBG("pre hdr_net_wrt %x %d f=%d\n", hdr, strlen(hdr), f );
		netconn_write(client, hdr, strlen(hdr), NETCONN_NOCOPY);
		
		DBG("flen = %d\n", flen );
		char *hb = malloc(strlen(hdr_sz)+10);
		snprintf(hb, strlen(hdr_sz)+10-1, hdr_sz, flen);
		DBG("%s\n", hb );
		netconn_write(client, hb, strlen(hb), NETCONN_NOCOPY);
		free(hb);
		usleep(50);
		
		DBG("pre malloc of %d\n", OUT_BUF_LEN+4 );
		buf = malloc(OUT_BUF_LEN+4);
		buf_len = OUT_BUF_LEN;
		
		int tlen;
		if(buf){
			do{
				DBG("pre read %d %x %d totl=%d\n", f, buf, buf_len, totl);
				//tlen = read(f, buf, buf_len);
				tlen = SPIFFS_read(&fs, (spiffs_file)f, buf, buf_len&(~0x3)); 
				totl += tlen;
				DBG("pre netconn_write %d %x %d totl=%d\n", f, buf, tlen, totl);
				if(tlen > 0){
					netconn_write(client, buf, tlen, NETCONN_NOCOPY);
					usleep(50);
				}
			}while( tlen > 0 && !SPIFFS_eof(&fs, (spiffs_file)f ) );
			
			free(buf);
			buf_len = 0;
		}
		//close(f);
		SPIFFS_close(&fs, (spiffs_file)f);

		return totl;
	}
	//*out = NULL;
	return 0;
}

int do_websock(char **uri, int uri_len, char *hdr, char* hdr_sz, char* data, int len, char *out, char *suff ){
	int r=websocket_connect(client, data, len);
	DBG("after websocket_connect %x %d %d\n", data, len, r );
	usleep(50);
	if(r>0){ 
		ws_sock_del();
		
		ws_client = client;
		client = NULL;

		int ws_uri_len = uri_len+5;
		ws_uri = malloc(ws_uri_len + 12);
		ws_uri[0] = '/';
		ws_uri[1] = 'h';
		ws_uri[2] = 't';
		ws_uri[3] = 'm';
		ws_uri[4] = 'l';
		memcpy(&ws_uri[5], *uri, uri_len);
		if(suff != NULL && ( (!strcmp(suff, ".lua")) || (!strcmp(suff, ".cgi") ) ) ){
			ws_uri[ws_uri_len] = '\0';
		}else{
			if( ws_uri[ws_uri_len - 1] == '/' ){
				ws_uri_len--;
			}
			const char index_lua[] = "/index.lua";
			memcpy( &ws_uri[ws_uri_len], index_lua, sizeof(index_lua) );
			ws_uri_len += sizeof(index_lua);
			ws_uri[ws_uri_len] = '\0';
		}
		DBG("ws cgi path = %s\n", ws_uri);

		int f = SPIFFS_open(&fs, ws_uri, SPIFFS_RDONLY, 0); 
		if(f <= 0){
			ws_sock_del();
		}else{
			SPIFFS_close(&fs, f);
		}
		DBG("ws cgi file test = %d\n", f);

		//if(xHandleWs != NULL) 
		//	vTaskDelete( xHandleWs );
		//xTaskCreate(&ws_task, "wsd", 256, NULL, 2, &xHandleWs);
	
		DBG("post websocket_connect %d\n", r );
	}

	DBG("pre exit websocket_connect %x %d %d\n", data, len, r );
	return r > 0;
}

int do_404(char **uri, int uri_len, char *hdr, char* hdr_sz, char* data, int len, char *out ){
	char* buf=NULL;
	int buf_len;
	
	buf_len = sizeof(webpage_404) +20 + 1; //OUT_BUF_LEN;
	buf = malloc(buf_len); // OUT_BUF_LEN+1);

	if(buf){	
		//DBG("pre sprintf %x %d\n", buf, buf_len);
		snprintf(buf, buf_len, webpage_404,
				*uri,
				xTaskGetTickCount() * portTICK_PERIOD_MS / 1000,
				(int) xPortGetFreeHeapSize()
		);
		
		//free(uri);
		//uri_len = 0;
				
		//DBG("pre hdr_net_wrt %x %d\n", html_hdr, strlen(html_hdr) );
		netconn_write(client, hdr, strlen(hdr), NETCONN_NOCOPY);
		
		char *hb = malloc(sizeof(hdr_siz)+10);
		snprintf(hb, sizeof(hdr_siz)+10-1, hdr_sz, strlen(buf));
		netconn_write(client, hb, strlen(hb), NETCONN_NOCOPY);
		free(hb);
		usleep(50);
		
		//DBG("pre net out %x %d\n%s\n", buf, buf_len, buf);
		netconn_write(client, buf, buf_len, NETCONN_NOCOPY);
		usleep(50);
		
		free(buf);
	//	buf_len = 0;
	}

	//*out = NULL;
	return buf_len;
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

int httpd_task(lua_State* L) //void *pvParameters)
{
//	luaC_fullgc(L, 1);
//	static TaskHandle_t xHandleWs = NULL;
	DBG("httpd_task 1 Free mem: %d\n",xPortGetFreeHeapSize());

	//lua_register(L, "GET_cnt", lget_get_params_cnt);
	lua_register(L, "_GET", lget_param);
	
    /*struct netconn * */client = NULL;
    /*struct netconn * */nc = netconn_new(NETCONN_TCP);
	printf("Open connection (main nc)\n");

    if (nc == NULL) {
        printf("Failed to allocate socket.\n");
        vTaskDelete(NULL);
    }
	
	nc->recv_timeout = recv_timeout;
	nc->recv_bufsize = IN_BUF_LEN;

	ip_set_option(nc->pcb.tcp, SOF_REUSEADDR);
	//nc->pcb.tcp->so_options |= SOF_REUSEADDR;
	
	netconn_bind(nc, IP_ADDR_ANY, 80);
    netconn_listen(nc);

	prep_something();

	is_httpd_run = 1;    
    while (is_httpd_run) {
        err_t err = netconn_accept(nc, &client);
        if (client != NULL) 
			printf("Open connection (client) %x\n", client);
		usleep(50);
        if (client != NULL && err == ERR_OK) {
            struct netbuf *nb=NULL;
			client->send_timeout = send_timeout;
			client->recv_timeout = recv_timeout;

			ip_set_option(client->pcb.tcp, SOF_REUSEADDR);
			//client->pcb.tcp->so_options |= SOF_REUSEADDR;
			
			client->recv_bufsize = IN_BUF_LEN;
            if ((err = netconn_recv(client, &nb)) == ERR_OK) {
                void *data;
                u16_t len;
			
				if( !is_httpd_run ){
					if(client != NULL){
						printf("Closing connection (client) on http exit\n");
						netconn_close(client);
						netconn_delete(client);
						client = NULL;
					}
					ws_sock_del();
					if(get_root != NULL){
						get_list_free(&get_root);
					}
					usleep(50);

					break;
				}
				usleep(50);

                netbuf_data(nb, &data, &len);

				//DBG("%s\n", data);
				
                /* check for a GET request */
                if (!strncmp(data, "GET ", 4)) {
                    char *uri, *suf, *hdr, *hdr_sz;
					int suf_len;
					int uri_len;

					ws_sock_del();
					
					uri_len = get_uri(data, len, &uri, &suf, &suf_len, &get_root );

					if(suf != NULL){
						hdr = suf_to_hdr(suf, suf_len, &hdr_sz);
						//free(suf);
					}else{ 
						hdr = html_hdr;
						hdr_sz = hdr_siz;
					}

					//DBG("client->send_timeout %d\n", client->send_timeout );
					if( do_websock(&uri, uri_len, hdr, hdr_sz, data, len, NULL /*char *out*/, suf ) > 0){
					}else
					if(suf != NULL && ( (!strcmp(suf, ".lua")) || (!strcmp(suf, ".cgi")) ) ){
						DBG("lua cgi = %s\n", uri);
						do_lua(&uri, uri_len, hdr, hdr_sz, data, len , NULL /*char *out*/, L, &get_root );
					}else
					if( do_file(&uri, uri_len, hdr, hdr_sz, data, len, NULL/*char *out*/ ) > 0){
					}else
					if( do_something(&uri, uri_len, hdr, hdr_sz, NULL/*&buf_len*/) > 0){
					}else 
					{
						do_404(&uri, uri_len, hdr, hdr_sz, data, len, NULL/*out*/);
					}
					
					if(uri != NULL){
						free(uri);
						uri = NULL;
						uri_len = 0;
					}
					if(suf != NULL){
						free(suf);
						suf = NULL;
					}
					if(get_root != NULL){
						get_list_free(&get_root);
					}
							
                }
            }
            netbuf_delete(nb);
        }
		if(client != NULL){
			printf("Closing connection (client)\n");
			netconn_close(client);
			netconn_delete(client);
			client = NULL;
		}
		usleep(50);
		ws_task(L);
    }
	if(nc != NULL){
		printf("Closing connection (main nc)\n");
		netconn_close(nc);
		netconn_delete(nc);
		nc = NULL;
	}

	DBG("httpd_task 2 Free mem: %d\n",xPortGetFreeHeapSize());
	return 0;
}

int httpd_task_stop(lua_State* L){
	is_httpd_run = 0;

	
//	client->send_timeout=1;
//	client->recv_timeout=1;

//	nc->send_timeout=1;
//	nc->recv_timeout=1;
	
	return 0;
}
#endif


#if 0
#include "mbedtls/sha1.h"
#include "mbedtls/base64.h"



enum{
	HTML_tag=1,
	WS_tag
};

enum{
	REQ_GET=1,
	REQ_WS,
	REQ_WSconn,
};


#define IN_BUF_LEN  512
#define OUT_BUF_LEN  300 
//512

#undef MAXPATHLEN
#define MAXPATHLEN  55 
//PATH_MAX



const char HTML_HEADER[] ={
"HTTP/1.1 200 OK\r\n"
"Content-type: text/html\r\n\r\n"
};

const char WS_HEADER[] = "Upgrade: websocket\r\n";
const char WS_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const char WS_KEY[] = "Sec-WebSocket-Key: ";
const char WS_RSP[] = "HTTP/1.1 101 Switching Protocols\r\n" \
					  "Upgrade: websocket\r\n" \
					  "Connection: Upgrade\r\n" \
					  "Sec-WebSocket-Accept: %s\r\n\r\n";


const char *webpage = {
	"<html><head><title>HTTP Server</title>"
	"<style> div.main {"
	"font-family: Arial;"
	"padding: 0.01em 16px;"
	"box-shadow: 2px 2px 1px 1px #d2d2d2;"
	"background-color: #f1f1f1;}"
	"</style></head>"
	"<body><div class='main'>"
	"<h3>HTTP Server</h3>"
	"<p>URL: %s</p>"
	"<p>Uptime: %d seconds</p>"
	"<p>Free heap: %d bytes</p>"
	"<button onclick=\"location.href='/on'\" type='button'>"
	"LED On</button></p>"
	"<button onclick=\"location.href='/off'\" type='button'>"
	"LED Off</button></p>"
	"</div></body></html>"
};











static void send_HDR(struct netconn *client, char tag){
	switch (tag){
		case HTML_tag:
			netconn_write(client, HTML_HEADER, strlen(HTML_HEADER), NETCONN_COPY);
			break;
			
	}
}


static void CGI_init(){
    /* disable LED */
    gpio_enable(2, GPIO_OUTPUT);
    gpio_write(2, true);
}

static int CGI_run(struct netconn *nc, int rqt, char* uri,  int max_uri_len, char** pout_buf){
	//if(pout_buf != NULL) *pout_buf=0;
	switch(rqt){
		case REQ_GET:
			if (!strncmp(uri, "/on", max_uri_len)){
				gpio_write(2, false);
				return 0;
			}else if (!strncmp(uri, "/off", max_uri_len)){
				gpio_write(2, true);
				return 0;
			}else {
				//FILE* f;
				int f;
				struct stat sb;
				//u16_t len;
				int len;
				char *buf = malloc(OUT_BUF_LEN);
				if(buf==NULL){
					printf("No mem\n");
					return ERR_MEM;
				}
				snprintf(buf, OUT_BUF_LEN-1, "/html%s", uri);
				//printf("fn = %s\n", buf);
				//f=open(buf, "r");
				if (stat(buf, &sb) != 0) { 
				//if(f==NULL){
					len = snprintf(buf, OUT_BUF_LEN-1, webpage,
							uri,
							xTaskGetTickCount() * portTICK_PERIOD_MS / 1000,
							(int) xPortGetFreeHeapSize());
					
					send_HDR(nc, HTML_tag); 
					netconn_write(nc, buf, /*strlen(buf)*/len, NETCONN_COPY);
				}else{
					printf("try open '%s'\n", buf);
					//f=fopen(buf, "r");
					
					f=open(buf, O_RDONLY, 0);
					printf("opened %s fd=%x\n", buf, f);
					send_HDR(nc, HTML_tag); 
					printf("hdr sent'\n");
					do{
					//while ( !feof(f) ) {
						len = read(f, buf, OUT_BUF_LEN-1); //fread(buf, OUT_BUF_LEN-1, 1, f);
						//len = fread(buf, OUT_BUF_LEN/2, 1, f);
						printf("read 'fd=%x len=%d'\n", f, len);
						if(len <= 0) break;
						//buf[ strlen(buf) ]='\0';
						//printf("zero '%s fd=%x'\n", buf, f);
						//printf("%s\n", buf);
						netconn_write(nc, buf, /*strlen(buf)*/len, NETCONN_COPY);
					} while(len>0/*len < OUT_BUF_LEN*//*feof(f)*/ );
					close(f);
				}
				free(buf);
				return HTML_tag;
			}
			break;

		case REQ_WS:
			return WS_tag;
			break;
			
		case REQ_WSconn:
			{
		    unsigned char encoded_key[32];
		    char key[64];
		    char *key_start = strnstr(uri, WS_KEY, max_uri_len);
		    if (key_start) {
		        key_start += 19;
		        char *key_end = strnstr(key_start, "\r\n", max_uri_len);
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
		                    encoded_key[olen] = '\0';
		                    printf("Base64 encoded: %s\n", encoded_key);
		                    /* Send response */
		                    char* buf = malloc(OUT_BUF_LEN);
							if(buf==0){
								printf("No mem\n");
								return ERR_MEM;
							}
		                    u16_t len = snprintf(buf, sizeof(buf), WS_RSP, encoded_key);
		                    //http_write(pcb, buf, &len, TCP_WRITE_FLAG_COPY);
		                    netconn_write(nc, buf, /*strlen(buf)*/len, NETCONN_COPY);
							free(buf);
		                    return WS_tag; // ERR_OK;
		                }
		            } else {
		                printf("Key overflow\n");
		                return ERR_MEM;
		            }
				}
		    }
			return WS_tag;
			}
			break;
				
	}
}


void websocket_write(struct netconn *nc, const uint8_t *data, uint16_t len, uint8_t mode){
    if (len > 125)
        return;
    unsigned char* buf = malloc(len + 2);
    buf[0] = 0x80 | mode;
    buf[1] = len;
    memcpy(&buf[2], data, len);
    len += 2;
//    tcp_write(nc, buf, len, TCP_WRITE_FLAG_COPY);
	netconn_write(nc, buf, len, NETCONN_COPY);
	free(buf);
}


static err_t websocket_parse(struct netconn *nc, unsigned char *data, u16_t data_len){
    if (data != NULL && data_len > 1) {
		if ((data[0] & 0x80) == 0) {
		  printf("Warning: continuation frames not supported\n");
		  return ERR_OK;
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
                    websocket_cb(nc, &data[6], data_len, opcode);
                }
                break;
            case 0x08: // close
                return ERR_CLSD;
                break;
        }
        return ERR_OK;
    }
    return ERR_VAL;
}

static err_t websocket_close(struct netconn *nc)
{
    const char buf[] = {0x88, 0x02, 0x03, 0xe8};
    u16_t len = sizeof(buf);
//    return tcp_write(pcb, buf, len, TCP_WRITE_FLAG_COPY);
	return netconn_write(nc, buf, len, NETCONN_COPY);
}

static int parse_request(char* data, int data_len, char* out, int max_out_len){
	int r = strncmp(data, "GET ", 4) == 0 ? REQ_GET : 0;
	if( r==REQ_GET ){
		char *sp1, *sp2;
		/* extract URI */
		sp1 = data + 4;
		sp2 = memchr(sp1, ' ', max_out_len); //data_len); //
		int len = sp2 - sp1;
		if(len > MAXPATHLEN-1 || len >= max_out_len){
			r = 0;
			printf("uri len shold be < %d: %d\n", MAXPATHLEN-1, len);
		}else{ 
			memcpy(out, sp1, len);
			out[len] = '\0';
			printf("uri: %s\n", out);
		}
	}else if ( strnstr(data, WS_HEADER, data_len) ) {
		#if 0
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
	                    r = REQ_WS;
	                    encoded_key[olen] = '\0';
	                    printf("Base64 encoded: %s\n", encoded_key);
	                    /* Send response */
	                    //char buf[256];
	                    u16_t len = snprintf(out, max_out_len, WS_RSP, encoded_key);
	                    //http_write(pcb, buf, &len, TCP_WRITE_FLAG_COPY);
	                    //return r; // ERR_OK;
	                }
	            } else {
	                printf("Key overflow\n");
	                return ERR_MEM;
	            }
			}
	    }
		#endif
		r = REQ_WSconn;
    } else {
        printf("Malformed packet\n");
        r= ERR_ARG;
    }
	return r;
}

void httpd_task(void *pvParameters){
    struct netconn *client = NULL;
    struct netconn *nc = netconn_new(NETCONN_TCP);
    if (nc == NULL) {
        printf("Failed to allocate socket.\n");
        vTaskDelete(NULL);
    }
    netconn_bind(nc, IP_ADDR_ANY, 80);
    netconn_listen(nc);
//    char *buf /*= malloc(IN_BUF_LEN)*/;

	CGI_init();
    while (1) {
        err_t err = netconn_accept(nc, &client);
        if (err == ERR_OK) {
            struct netbuf *nb;
            if ((err = netconn_recv(client, &nb)) == ERR_OK) {
                void *data;
                u16_t len;

				char* uri=malloc(MAXPATHLEN+8); //16
				int max_uri_len = MAXPATHLEN+7; //255; //16
				
//				buf = malloc(IN_BUF_LEN);
                netbuf_data(nb, &data, &len);
                /* check for a GET request */
				int res_pr = parse_request(data, len, uri, max_uri_len);
				//netbuf_delete(nb);
                switch ( res_pr ) {
					case REQ_GET : 
						netbuf_delete(nb);
	//					free(buf);
						switch( CGI_run(client, REQ_GET, uri, max_uri_len, NULL) ){ // &buf) ){
							case HTML_tag:
//								send_HDR(client, HTML_tag);	
//			                    netconn_write(client, buf, strlen(buf), NETCONN_COPY);
//								free(buf);
								break;

							case WS_tag:
								break;
						}
						break;

					case REQ_WS : 
						netbuf_delete(nb);
//						CGI_run(client, REQ_WS, uri, max_uri_len, &buf);
//						netconn_write(client, uri, strlen(uri), NETCONN_COPY);
//						free(buf);
						websocket_close(client);
						break;
						
					case REQ_WSconn : 
						CGI_run(client, REQ_WSconn, data, len, NULL);
						netbuf_delete(nb);
//						netconn_write(client, uri, strlen(uri), NETCONN_COPY);
						//free(buf);
//						websocket_close(client);
						break;
							
					default: 
//			            netbuf_delete(nb);
						break;
                };
				free(uri);
				max_uri_len=0;
            }
        }
        printf("Closing connection\n");
		websocket_close(client);
        netconn_close(client);
        netconn_delete(client);
    }
}
#endif

/*
int httpd_start(lua_State* L) {
//	static TaskHandle_t xHandle = NULL;
//	if(xHandle != NULL) 
//		vTaskDelete( xHandle );
	lua_settop(L,0);
	lua_pushcfunction(L, &httpd_task);
	int r = new_thread(L, 1);
	DBG("httpd started status=%d\n", r);
//	xTaskCreate(&httpd_task, "httpd", 312, NULL, 2, &xHandle);

	return 0;
}
*/

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

/*
static int httpd_(lua_State* L) {
	return 0;	 
}

static int httpd_(lua_State* L) {
	return 0;	 
}

static int httpd_(lua_State* L) {
	return 0;	 
}

static int httpd_(lua_State* L) {
	return 0;	 
}
*/

#include "modules.h"


const LUA_REG_TYPE httpd_map[] = {
//		{ LSTRKEY( "httpd" ),		LFUNCVAL( net_httpd_start ) },
//		{ LSTRKEY( "httpd" ),		LFUNCVAL( httpd_start ) },
		{ LSTRKEY( "main" ),			LFUNCVAL( httpd_task ) },
		{ LSTRKEY( "stop" ),			LFUNCVAL( httpd_task_stop ) },
		
		{ LSTRKEY( "recv_timeout" ),	LFUNCVAL( httpd_recv_timeout ) },
		{ LSTRKEY( "send_timeout" ),	LFUNCVAL( httpd_send_timeout ) },
		
		{ LNILKEY, LNILVAL }
};

int luaopen_httpd( lua_State *L ) {
	return 0;
}


MODULE_REGISTER_MAPPED(HTTPD, httpd, httpd_map, luaopen_httpd);


