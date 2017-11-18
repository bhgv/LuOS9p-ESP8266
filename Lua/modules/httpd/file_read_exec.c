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


static spiffs_file cur_f = 0;


int do_file_pas(spiffs_file cur_f){
	err_t err;
	char* buf=NULL;
	int buf_len = 0;
	int tlen = 0;
	
	DBG("pre malloc of %d\n", OUT_BUF_LEN );
	buf = malloc(OUT_BUF_LEN);
	buf_len = OUT_BUF_LEN-4;
	
	DBG("pre read %d %x %d\n", cur_f, buf, buf_len );
	//tlen = read(f, buf, buf_len);
	tlen = SPIFFS_read(&fs, cur_f, buf, buf_len); //&(~0x3));
	DBG("after read %d %x %d\n", cur_f, buf, buf_len );
	if(tlen < 0)
		return 0;
	//totl += tlen;
	DBG("pre netconn_write %d %x %d\n", cur_f, buf, tlen );
	if(tlen > 0){
		//usleep(10);
		int t_is_httpd_run = is_httpd_run;
		if( check_conn(__func__, __LINE__) ){
			err = netconn_write(client, buf, tlen, NETCONN_NOCOPY);
			print_err(err, __func__, __LINE__);
		}
		//if(err != ERR_OK) 
		if(is_httpd_run == 4){
			if(err == ERR_ABRT) {
				nc_free(&client, "Closing connection (client) aborted\n");
				is_httpd_run = t_is_httpd_run;
			}
			tlen = 0;
			//break;
		}
		//usleep(50);
	}

	if(buf != NULL){
		DBG("after do while read %d %x %d totl=%d\n", f, buf, buf_len, totl);
		
		free(buf);
		buf_len = 0;
	}

	return tlen;
}


int do_file(char **uri, int uri_len, char *hdr, char* hdr_sz, /*char* data, int len,*/ char *out ){
	int flen = 0;
	err_t err = ERR_OK;

	if(cur_f > 0)
		SPIFFS_close(&fs, cur_f);
	
	cur_f = uri_to_file(*uri, uri_len, &flen);
	
	if(cur_f > 0){
		//ws_sock_del();
	//	free(uri);
	//	uri_len = 0;
		int totl=0;

		//nb_free();
		
		DBG("pre hdr_net_wrt %x %d f=%d\n", hdr, strlen(hdr), cur_f );
		//usleep(10);
		if( check_conn(__func__, __LINE__) ){
			err = netconn_write(client, hdr, strlen(hdr), NETCONN_NOCOPY);
			print_err(err, __func__, __LINE__);
		}
		//if(err != ERR_OK) 
		if(is_httpd_run == 4) {
			SPIFFS_close(&fs, cur_f);
			cur_f = 0;
			return 0;
		}
		
		if( check_conn(__func__, __LINE__) ){
			DBG("flen = %d\n", flen );
			char *hb = malloc(strlen(hdr_sz)+10);
			snprintf(hb, strlen(hdr_sz)+10-1, hdr_sz, flen);
			DBG("%s\n", hb );
			err = netconn_write(client, hb, strlen(hb), NETCONN_NOCOPY);
			free(hb);

			print_err(err, __func__, __LINE__);
		}
		//if(err != ERR_OK) 
		if(is_httpd_run == 4) {
			SPIFFS_close(&fs, cur_f);
			cur_f = 0;
			return 0;
		}
		
		int tlen = 0;
		do{
			tlen = do_file_pas( cur_f );
			totl += tlen;
		}while( tlen > 0 /*&& !SPIFFS_eof(&fs, (spiffs_file)f )*/ );

		SPIFFS_close(&fs, cur_f );
		cur_f = 0;

		return totl;
	}
	return 0;
}



