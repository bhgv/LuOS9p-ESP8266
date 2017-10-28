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


#include "mbedtls/sha1.h"
#include "mbedtls/base64.h"



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
//printf("strnstr 1 %s %s %d\n", buffer, token, n);
  for (p = buffer; *p && (p + tokenlen <= buffer + n); p++) {
    if ((*p == *token) && (strncmp(p, token, tokenlen) == 0)) {
//printf("strnstr 2 %s\n", p);
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

#define IN_BUF_LEN  512
#define OUT_BUF_LEN  311 
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




    const char html_hdr[] = {
        "HTTP/1.1 200 OK\r\n" 
        "Content-type: text/html\r\n\r\n" 
    };

    const char webpage[] = {
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

static int websocket_connect(struct netconn *nc, char* data, int data_len/*, char* out, int max_out_len*/){
	int r=0;
	if ( strnstr(data, WS_HEADER, data_len) ) {
		#if 1
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
						char* buf = malloc(OUT_BUF_LEN+1);
						int buf_len = OUT_BUF_LEN;
					
	                    encoded_key[olen] = '\0';
	                    printf("Base64 encoded: %s\n", encoded_key);
	                    /* Send response */
	                    //char buf[256];
	                    u16_t len = snprintf(buf, buf_len, WS_RSP, encoded_key);
	                    //http_write(pcb, buf, &len, TCP_WRITE_FLAG_COPY);
	                    netconn_write(nc, buf, len, NETCONN_NOCOPY);
						free(buf);
	                    //return r; // ERR_OK;
	                    //r = REQ_WS;
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


char* do_something(char *uri, int uri_len, int *out_len ){

	if (!strncmp(uri, "/on", uri_len))
		gpio_write(2, false);
	else if (!strncmp(uri, "/off", uri_len))
		gpio_write(2, true);

	*out_len = 0;
	return NULL;
}


static int get_uri(char* in, int in_len, char** uri/*, int* uri_len*/){
	*uri = NULL;
//	*uri_len = 0;

	char *sp1, *sp2;
	/* extract URI */
	sp1 = in + 4;
	sp2 = memchr(sp1, ' ', in_len);
	int len = sp2 - sp1;
	if(len > MAXPATHLEN || len <= 0) 
		return 0;

	*uri = malloc(len+2);
	memcpy(*uri, sp1, len);
	(*uri)[len] = '\0';
	printf("uri: %s\n", *uri);
	
	return len;
}



#include <sys/spiffs/spiffs.h>
#include <sys/spiffs/spiffs_nucleus.h>
#include <sys/spiffs/esp_spiffs.h>


extern spiffs fs;

static /*int*/spiffs_file uri_to_file(char* uri, int len){
//	int f;
	struct stat sb;
	spiffs_file r = -1;
	char *p = malloc(len+10);
	
	if(p==NULL){
		printf("No mem\n");
		return 0; //ERR_MEM;
	}
	p[0]='/'; p[1]='h'; p[2]='t'; p[3]='m'; p[4]='l';
	memcpy(&p[5], uri, len);
	p[len+5] = '\0';
	printf("path = %s %d\n", p, len);

    if (is_dir(p)) {
        return 0;
    }
	r = SPIFFS_open(&fs, p, SPIFFS_RDONLY, 0); 
//    if (r < 0) {
//        r = spiffs_result(fs.err_code);
//    }

//	if (stat(p, &sb) == 0) {
//		printf("pre open = %s\n", p);
//		r = open(p, O_RDONLY, 0);
//	}
	free(p);
	
	printf("uri_to_file exit %d\n", r);
	return r;
}

int httpd_task(lua_State* L) //void *pvParameters)
{
    struct netconn *client = NULL;
    struct netconn *nc = netconn_new(NETCONN_TCP);
    if (nc == NULL) {
        printf("Failed to allocate socket.\n");
        vTaskDelete(NULL);
    }
    netconn_bind(nc, IP_ADDR_ANY, 80);
    netconn_listen(nc);
    char* buf;
    int buf_len;
    
    /* disable LED */
//    gpio_enable(2, GPIO_OUTPUT);
//    gpio_write(2, true);
    
    while (1) {
        err_t err = netconn_accept(nc, &client);
        if (err == ERR_OK) {
            struct netbuf *nb;
            if ((err = netconn_recv(client, &nb)) == ERR_OK) {
                void *data;
                u16_t len;
                netbuf_data(nb, &data, &len);
                /* check for a GET request */
                if (!strncmp(data, "GET ", 4)) {
                    char* uri;
                    int uri_len = get_uri(data, len, &uri/*, &uri_len*/);

					if(!strcmp(uri, "/ws") ){
						int r=websocket_connect(client, data, len);
						printf("post websocket_connect %d\n", r );
						
					}else{
	                    int f = uri_to_file(uri, uri_len);
	                    
	                    if(f > 0){
							free(uri);
							uri_len = 0;
							
							printf("pre hdr_net_wrt %x %d f=%d\n", html_hdr, strlen(html_hdr), f );
							netconn_write(client, html_hdr, strlen(html_hdr), NETCONN_NOCOPY);
							
							printf("pre malloc of %d\n", OUT_BUF_LEN+1 );
							buf = malloc(OUT_BUF_LEN+1);
							buf_len = OUT_BUF_LEN;
							
							int tlen;
							do{
								printf("pre read %d %x %d\n", f, buf, buf_len );
								//tlen = read(f, buf, buf_len);
								tlen = SPIFFS_read(&fs, (spiffs_file)f, buf, buf_len); 
								printf("pre netconn_write %d %x %d\n", f, buf, tlen );
								if(tlen > 0){
									netconn_write(client, buf, tlen, NETCONN_NOCOPY);
								}
							}while(tlen > 0);
							//close(f);
							SPIFFS_close(&fs, (spiffs_file)f);

							free(buf);
							buf_len = 0;
						}else{ 
							buf = do_something(uri, uri_len, &buf_len);
						   
							//printf("post do_smt %x %d\n", buf, buf_len);
							
							//printf("pre hdr_net_wrt %x %d\n", html_hdr, strlen(html_hdr) );
							netconn_write(client, html_hdr, strlen(html_hdr), NETCONN_NOCOPY);
							
							if(buf == NULL){
								//printf("pre malloc %d\n", OUT_BUF_LEN+1);
								
								buf = malloc(OUT_BUF_LEN+1);
								buf_len = OUT_BUF_LEN;
								
								//printf("pre sprintf %x %d\n", buf, buf_len);
								
								snprintf(buf, OUT_BUF_LEN, webpage,
									uri,
									xTaskGetTickCount() * portTICK_PERIOD_MS / 1000,
									(int) xPortGetFreeHeapSize()
								);
							}
							free(uri);
							uri_len = 0;
							
							//printf("pre net out %x %d\n%s\n", buf, buf_len, buf);
							
							netconn_write(client, buf, buf_len, NETCONN_NOCOPY);
							
							free(buf);
							buf_len = 0;
						}
					}
                }
            }
            netbuf_delete(nb);
        }
        printf("Closing connection\n");
        netconn_close(client);
        netconn_delete(client);
    }
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


void httpd_start(lua_State* L) {
//	static TaskHandle_t xHandle = NULL;
//	if(xHandle != NULL) 
//		vTaskDelete( xHandle );
	lua_settop(L,0);
	lua_pushcfunction(L, &httpd_task);
	int r = new_thread(L, 1);
	printf("httpd started status=%d\n", r);
//	xTaskCreate(&httpd_task, "httpd", 312, NULL, 2, &xHandle);
}


