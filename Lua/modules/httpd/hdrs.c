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





const char HTML_HEADER[] ={
	"HTTP/1.1 200 OK\r\n"
	"Content-type: text/html\r\n\r\n"
};


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

#if 1
const char css_hdr[] = {
        "HTTP/1.1 200 OK\r\n" 
        "Content-type: text/css; charset=UTF-8\r\n" 
};
#else
const char css_hdr[] = {
        "HTTP/1.1 200 OK\r\n" 
        "Content-type: text/html; charset=UTF-8\r\n" 
};
#endif

const char js_hdr[] = {
        "HTTP/1.1 200 OK\r\n" 
        "Content-type: application/javascript\r\n" 
};

const char lua_hdr[] = {
        "HTTP/1.1 200 OK\r\n" 
	"Content-type: text/html; charset=UTF-8\r\n" 
};
//       "Content-type: application/lua\r\n\r\n" 
	
//	const char hdr_siz[] = "Keep-Alive: timeout=1, max=1\r\nConnection: close\r\nContent-Length: %d\r\n\r\n";
//	const char hdr_nosiz[] = "Keep-Alive: timeout=1, max=1\r\nConnection: close\r\n\r\n";
const char hdr_siz[] = "Content-Length: %d\r\nConnection: close\r\n\r\n";
const char hdr_nosiz[] = "Connection: close\r\n\r\n";



static suf_hdr suf_hdr_tab[] = {
	{".jpg", jpg_hdr, hdr_siz},
	{".png", png_hdr, hdr_siz},
	{".gif", gif_hdr, hdr_siz},
	{".css", css_hdr, hdr_siz},
	{".js", js_hdr, hdr_siz},
	{".lua", lua_hdr, hdr_nosiz},
	{NULL, NULL}
};


char* suf_to_hdr(char* suf, int suf_len, char** siz){
	char* r = html_hdr; //NULL;
	*siz = hdr_siz;
	int i=0;
	
	if(suf == NULL){
		*siz = hdr_nosiz;
		return html_hdr; //NULL;
	}
	
	while(suf_hdr_tab[i].suf != NULL){
		char *tsuf = suf_hdr_tab[i].suf;
		if( !strcmp(tsuf, suf) ){ //, strlen( tsuf) ) ){
			r = suf_hdr_tab[i].hdr;
			*siz = suf_hdr_tab[i].siz;
			break;
		}
		i++;
	}
	return r;
}



