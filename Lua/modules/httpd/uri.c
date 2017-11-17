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

#include "lwip/api.h"

#include "httpd/httpd.h"


#if 0
#define DBG(...) printf(__VA_ARGS__)
#else
#define DBG(...)
#endif


const char* def_files[]={
	"index.htm",
	"index.html",
	"index.lua",
	"index.cgi",
	"index.ssi",
	NULL
};


int get_uri(char* in, int in_len, char** uri, char** suf, int* suf_len, get_par** ppget_root){
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

		get_list_free(&get_root);

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
		if(*suf != NULL){
			free(*suf);
		}
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
			*i = '\0';
			//(*suf)[l] = '\0';
			printf("suffix: %s\n", *suf);
		}else{
			*suf = NULL;
			if(suf_len!=NULL) *suf_len = 0;
		}
	}
	DBG("%s: %d\n", __func__, __LINE__);
	
	return len;
}


spiffs_file uri_to_file(char* uri, int len, int *flen){
	*flen=0;
//	int f;
	spiffs_stat stat;
	int res;
//	struct stat sb;
	spiffs_file r = -1;

	DBG("%s: %d\n", __func__, __LINE__);
	if(len + 12 > MAXPATHLEN){
		printf("Too long path: %db, must be < %d\n", len, MAXPATHLEN-12);
		return r;
	}
	char *p = malloc(len+12);
	
	DBG("%s: %d\n", __func__, __LINE__);
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
			usleep(10);
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
	
	usleep(10);
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
	free(p);
	
	DBG("uri_to_file exit %d\n", r);
	return r;
}


