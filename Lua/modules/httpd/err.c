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

//#include "lwip/api.h"

//#include <lwip/err.h>

#include "httpd/httpd.h"


#if 0
#define DBG(...) printf(__VA_ARGS__)
#else
#define DBG(...)
#endif


//extern const char *lwip_strerr(err_t err);


static char * err_msgs[] = {
	[-ERR_MEM] = "Out of memory error.",
	[-ERR_BUF] = "Buffer error.",
	[-ERR_TIMEOUT] = "Timeout.",
	[-ERR_RTE] = "Routing problem.",
	[-ERR_INPROGRESS] = "Operation in progress",
	[-ERR_VAL] = "Illegal value.",
	[-ERR_WOULDBLOCK] = "Operation would block.",
	[-ERR_USE] = "Address in use.",
	// ERR_ALREADY = "Already connecting.",
	[-ERR_ISCONN] = "Conn already established.",
				//case ERR_IS_FATAL(e) ((e) < ERR_ISCONN)
	[-ERR_ABRT] = "Connection aborted.",
	[-ERR_RST] = "Connection reset.",
	[-ERR_CLSD] = "Connection closed.",
	[-ERR_CONN] = "Not connected.",
	[-ERR_ARG] = "Illegal argument.",
	[-ERR_IF] = "Low-level netif error",
};



int check_conn(char* fn, int ln){
	int r = 1;

	usleep(50);
	uint8_t st = sdk_wifi_station_get_connect_status();

	DBG( "%s, %d: Wifi status = %d\n", fn, ln, st );
	
	switch(st){
//		case STATION_IDLE:
		case STATION_CONNECTING:
		case STATION_WRONG_PASSWORD:
		case STATION_NO_AP_FOUND:
		case STATION_CONNECT_FAIL:
			is_httpd_run = 4;
			r = 0;
			break;
		
		case STATION_IDLE:
		case STATION_GOT_IP:
			r = 1;
			break;
		
	};
	return r;
}


err_t print_err(err_t err){
	if(err != ERR_OK){
		char* s = 0; //lwip_strerr(err); //0;
		/**/
		switch(err){
			case ERR_MEM		:
//				s = "Out of memory error.";
//				break;
			case ERR_BUF		:
//				s = "Buffer error.";
//				break;
			//case ERR_TIMEOUT	:
			//	s = "Timeout.";
			//	break;
			case ERR_RTE		:
//				s = "Routing problem.";
//				break;
			case ERR_INPROGRESS:
//				s = "Operation in progress";
//				break;
			case ERR_VAL		:
//				s = "Illegal value.";
//				break;
			case ERR_WOULDBLOCK:
//				s = "Operation would block.";
//				break;
			case ERR_USE	   :
//				s = "Address in use.";
//				break;
//			case ERR_ALREADY	:
//				s = "Already connecting.";
//					break;
			case ERR_ISCONN 	:
//				s = "Conn already established.";
//				break;
			//case ERR_IS_FATAL(e) ((e) < ERR_ISCONN)
			case ERR_ABRT		:
//				s = "Connection aborted.";
//				break;
			case ERR_RST		:
//				s = "Connection reset.";
//				break;
			case ERR_CLSD		:
//				s = "Connection closed.";
//				break;
			case ERR_CONN		:
//				s = "Not connected.";
//				break;
			case ERR_ARG		:
//				s = "Illegal argument.";
//				break;
			case ERR_IF 		:
//				s = "Low-level netif error";
//				break;
				s = err_msgs[-err];
				break;
			
		};
		/**/
		if(s != 0) printf("! %s: %s\n", (ERR_IS_FATAL(err) ? "fatal-err" : "err" ), s);
		if(ERR_IS_FATAL(err)){
			is_httpd_run = 4;
		}
	}
	usleep(50);
	
	return err;
}



