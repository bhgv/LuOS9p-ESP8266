
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




#if 0
#define DBG(...) printf(__VA_ARGS__)
#else
#define DBG(...)
#endif




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

//DBG("%s: %d\n", __func__, __LINE__);
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

//DBG("%s: %d. r=%s, l=%d\n", __func__, __LINE__, r, l);
	return r;
}




char*
call_virtual_foo(const TValue *foo, Qid *qid, char *buf, ulong *n, vlong off)
{
	int type;
	int l, rn, ltop, i, lk, ls = 0;
	char *k, *s;
	float v;
	
//DBG("  [%s] --> %x (t = %d, %s), intL = %x\r\n", k_nm, (unsigned int) rvalue(val), type, t_nm, intL);
	ltop = lua_gettop(intL);
//DBG("%s: %d\n", __func__, __LINE__);

	if(foo != NULL)
		luaA_pushobject(intL, foo);
	else
		ltop--;

//DBG("%s: %d\n", __func__, __LINE__);
	k = dev_call_parse_next_par(buf, &lk);
	//buf += l + 1;

	for(
		i = 0 ; 
		lk > 0 && k[0] == '-'; 
		k = dev_call_parse_next_par(buf, &lk)
	){
		//if(lk > 0 && k[0] == '-'){
		if(lk > 2)
			buf += 2;
		else
			buf += lk + 1;
	
		s = dev_call_parse_next_par(buf, &ls);
		if(ls == 0)
			break;
		buf += ls + 1;

//DBG("%s: %d. k=%s, s=%s\n", __func__, __LINE__, k, s);
		switch(k[1]){
			case 'n':
				lua_pushnumber(intL, atof(s) );
				i++;
				break;

			case 'i':
				lua_pushinteger(intL, atoi(s) );
				i++;
				break;

			case 's':
			default:
				lua_pushstring(intL, s);
				i++;
				break;

		}
	}
	
	lua_call(intL, i, LUA_MULTRET);
	
	rn = lua_gettop(intL) - ltop;
DBG("%s: %d. i=%d, rn=%d, qid->my_buf = %x\n", __func__, __LINE__, i, rn, qid->my_buf);
	l = 0;
	for(i = ltop+1; i <= ltop + rn; i ++){
		int il;
//DBG("%s: %d. i=%d, vtype=%s, %s\n", __func__, __LINE__, i, 
//								lua_typename(intL, lua_type(intL, i) ),
//								lua_tostring(intL, i) );
		lua_tolstring(intL, i, &il);
		l += il + 1;
	}
	
	if(qid->my_buf == NULL){
		qid->my_buf = styxmalloc( 256 );
//		qid->my_buf[0] = '\0';
		qid->my_buf_idx = 0;
	}
	
	l = qid->my_buf_idx;
	qid->my_buf[ l ] = '\0';

	for(i = ltop+1; i <= ltop + rn; i ++){
		int il;
		char *is;
		is = lua_tolstring(intL, i, &il);
	
		if(l + il + 2 > 256)
			break;

		memcpy(&qid->my_buf[ l ], is, il);
		l += il;
		qid->my_buf [ l ] = '\t';
		l += 1;
		qid->my_buf [ l ] = '\0';
/*
		switch( lua_type(intL, i) ){
			case 
		}
*/
//DBG("%s: %d\n", __func__, __LINE__);
	}
	qid->my_buf_idx = l;

	lua_settop(intL, ltop);
	luaC_fullgc(intL, 1);
	
DBG("%s: %d. qid->my_buf  = %s\n", __func__, __LINE__, qid->my_buf );

	return nil;
}
