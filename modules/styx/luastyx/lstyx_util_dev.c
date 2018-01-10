
/*
 * Copyright bhgv 2017
 */

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "ltm.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include <lib9.h>
#include "styxserver.h"
#include "styx.h"

#include "lstyx.h"



void*
scan_devs(luaR_entry *hdr_entry, char* nm, int type){
	int i = 1;
	luaR_entry *entry = hdr_entry; //lua_rotable;
	const TValue *val;

	for ( ; entry->key.id.strkey; entry++, i++ ) {
		if (entry->key.len >= 0) {
			if(type == SC_BYNAME && !strncmp(nm, entry->key.id.strkey, entry->key.len) )
				return (void*)i;
			else if( type == SC_BYPOS && i == *(int*)nm ){
				val = &entry->value;
				if (ttnov(val) == LUA_TROTABLE && (luaR_entry *)val->value_.p != hdr_entry) {
					//printf("VVV  %s (%x != %x) VVV\n", entry->key.id.strkey, val->value_.p, hdr_entry);
					return (void*) val->value_.p;
				}else{
					return NULL;
				}
			}
		}
	}

	return NULL;
}


char*
dev_call_parse_next_par(char* buf, int *plen){
	int l = 0;
	char *p, *r = NULL;
	char c, prt = 0, slsh = 0;;

//printf("%s: %d\n", __func__, __LINE__);
	if(buf != NULL){
		for(p = buf; *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'; p++);
		r = p;
		
		for(l = 0; *p != '\0'; p++, l++){
			c = *p;
			if(prt == 0 && r == p && ( c == '"' ||c == '\'' ) ){
				prt = c;
				r++;
			}else if(prt != 0 && slsh == 0 && c == '\\'){
				slsh = c;
			}else if(slsh == '\\'){
				slsh = 0;
			}else if(prt != 0 && c == prt){
				prt = 0;
				break;
			}else if( prt == 0 && (c == ' ' || c =='\t' || c == '\r' || c == '\n') )
				break;
		}
		*p = 0;
	}

	if(plen != NULL) *plen = l;

//printf("%s: %d. r=%s, l=%d\n", __func__, __LINE__, r, l);
	return r;
}

