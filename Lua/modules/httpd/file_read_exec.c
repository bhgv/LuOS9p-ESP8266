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



static int totl=0;


int strip_st = 0;

static int do_text_strip(char* buf, int len){
	int i, j;
	int st = strip_st;
	
	for(i=0, j = 0; i < len; i++){
		char c = buf[i];
		switch(st){
			case 0:
			case 1:
				if(c == ' ' || c == '\t' || c == '\r' || c == '\n'){
					if(st == 0){
						buf[ j++ ] = ' ';
						st = 1;
					}
				}else if(c == '/'){
					st = 2;
				}else if(c == '\"'){
					buf[ j++ ] = c;
					st = 7;
				}else if(c == '\''){
					buf[ j++ ] = c;
					st = 8;
				}else{
					buf[ j++ ] = c;
					st = 0;
				}
				break;

			case 2:
				if(c == '/')
					st = 3;
				else if(c == "*")
					st = 4;
				else{
					buf[ j++ ] = '/';
					buf[ j++ ] = c;
					st = 0;
				}
				break;

			case 3:
				if(c == '\r' || c == '\n'){
					st = 0;
				}
				break;
				
			case 4:
				if(c == '*')
					st = 5;
				break;
					
			case 5:
				if(c == '/')
					st = 0;
				else
					st = 4;
				break;
						
			case 7:
				buf[ j++ ] = c;
				if(c == '\"')
					st = 0;
				break;
				
			case 8:
				buf[ j++ ] = c;
				if(c == '\'')
					st = 0;
				break;
					
		}
	}

	strip_st = st;

	return j;
}


int do_file_pas(nc_node *node){
	err_t err;
	char* buf=NULL;
	int buf_len = 0;
	int tlen = 0;
	
	DBG("pre malloc of %d\n", OUT_BUF_LEN );
	buf = malloc(OUT_BUF_LEN);
	buf_len = OUT_BUF_LEN -4;

	if(buf == NULL) return ERR_MEM;
	
	DBG("pre read %d %x %d\n", node->cur_f, buf, buf_len );
	//tlen = read(f, buf, buf_len);
	tlen = SPIFFS_read(&fs, node->cur_f, buf, buf_len); //&(~0x3));
	DBG("after read %d %x %d\n", node->cur_f, buf, tlen );
	if(tlen < 0){
		free(buf);
		buf = NULL;
		
		return 0;
	}

	DBG("pre netconn_write %d %x %d\n", node->cur_f, buf, tlen );
	if(tlen > 0){
		int t_is_httpd_run = is_httpd_run;
		if( check_conn(__func__, __LINE__) ){
			DBG("%s: %d client=%x\n", __func__, __LINE__, node->clnt );

			tlen = do_text_strip(buf, tlen);
		
			err = netconn_write(node->clnt, buf, tlen, NETCONN_NOCOPY);
			DBG("post netconn_write %d, err=%d\n", node->cur_f, err );
			print_err(err, __func__, __LINE__);
		}
		if(is_httpd_run == 4){
			if(err == ERR_ABRT || err == ERR_CLSD) {
				//free(buf);
				//buf = NULL;
			
				//nc_free(&node->clnt, "Closing connection (client) aborted\n");
				//node->state = NC_END;
				is_httpd_run = t_is_httpd_run;
				//return 0;
			}
			tlen = 0;
			//break;
		}
	}

	if(buf != NULL){
		DBG("after do while read %d %x %d totl=%d\n", node->cur_f, buf, buf_len, totl);
		
		free(buf);
		buf = NULL;
		buf_len = 0;
	}

	return tlen;
}


int do_file_begin(char **uri, int uri_len, char *hdr, char* hdr_sz, nc_node *node ){
	int flen = 0;
	err_t err = ERR_OK;
	
	node->cur_f = uri_to_file(node->uri, strlen(node->uri), &flen);
	
	if(node->cur_f > 0){
		totl = 0;

		
		char* buf = malloc(OUT_BUF_LEN);
		int buf_len = OUT_BUF_LEN -4;
		int tlen, fl_len;

		strip_st = 0;
		
		if(buf != NULL){
			flen = 0;
			do{
				tlen = SPIFFS_read(&fs, node->cur_f, buf, buf_len); 
				if(tlen <= 0){
					break;
				}

				//if(tlen > 0){
					tlen = do_text_strip(buf, tlen);
				//}
				flen += tlen;
			}while(tlen > 0);
			free(buf);
			buf = NULL;
		}
		SPIFFS_lseek(&fs, node->cur_f, 0, SPIFFS_SEEK_SET);

		strip_st = 0;
		
		DBG("pre hdr_net_wrt %x %d f=%d\n", hdr, strlen(hdr), node->cur_f );
		if( check_conn(__func__, __LINE__) ){
			err = netconn_write(node->clnt, hdr, strlen(hdr), NETCONN_NOCOPY);
			print_err(err, __func__, __LINE__);
		}
		if(is_httpd_run == 4) {
			SPIFFS_close(&fs, node->cur_f);
			node->cur_f = -1;
			node->state = NC_CLOSE;
			return 0;
		}
		
		if( check_conn(__func__, __LINE__) ){
			DBG("flen = %d\n", flen );
			char *hb = malloc(strlen(hdr_sz)+20);
			snprintf(hb, strlen(hdr_sz)+20-1, hdr_sz, flen);
			DBG("%s\n", hb );
			err = netconn_write(node->clnt, hb, strlen(hb), NETCONN_NOCOPY);
			free(hb);

			print_err(err, __func__, __LINE__);
		}
		if(is_httpd_run == 4) {
			SPIFFS_close(&fs, node->cur_f);
			node->cur_f = -1;
			node->state = NC_CLOSE;
			return 0;
		}
		
		node->state = NC_PAS;
		
		return 1;
	}
	return 0;
}


void do_file_end(nc_node *node){
	if( node->cur_f > 0){
		SPIFFS_close(&fs, node->cur_f );
		node->cur_f = -1;
	}
	
	node->state = NC_CLOSE;
}



extern nc_node* nc_clients[];
extern int nc_clients_cnt;

int do_file(char **uri, int uri_len, char *hdr, char* hdr_sz, int nd_idx ){
	nc_node *node = nc_clients[nd_idx];
	
	DBG( "\n%s: %d nc_state=%d uri=%s, %s (%d from %d)\n", 
		__func__, __LINE__, node->state, node->uri, *uri,
		nd_idx, nc_clients_cnt);
	
	switch( node->state ){
		case NC_BEGIN:
			totl = 0;
			int r = do_file_begin(uri, uri_len, hdr, hdr_sz, node);
			return r;
			//break;
	
		case NC_PAS:
			{
				int tlen = 0;
				tlen = do_file_pas( node );
				if(tlen > 0){
					totl += tlen;
				}else{
					//nc_state = NC_END;
					node->state = NC_END;
				}
				return 1;
			}
			//break;
	
		case NC_END:
			do_file_end( node );
			//nc_state = NC_CLOSE;
			node->state = NC_CLOSE;
			break;

		case NC_CLOSE:
			nc_sock_del( nd_idx );
			return -1;
		
	}
	return 1;
}


