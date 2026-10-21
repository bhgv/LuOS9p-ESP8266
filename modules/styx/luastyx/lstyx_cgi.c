
/*
 * Copyright bhgv 2017
 */

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "ltm.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
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
#define DBG(...) ;
#endif





/* Execute one length-delimited command and retain the caller's Lua stack.
 * Replies keep the existing tab-separated representation. Non-string values
 * occupy an empty field; embedded NUL bytes are counted in the return length.
 * The allocator accepts int, so include the final NUL in its checked bound.
 */
int
call_cgi(char **pbuf, int len)
{
	char *buf, *reply;
	const char *value;
	size_t length, total;
	int base, count, j, status;

	if(pbuf == NULL || *pbuf == NULL || intL == NULL || len < 0)
		return 0;
	buf = *pbuf;
	base = lua_gettop(intL);
	status = luaL_loadbuffer(intL, buf, (size_t)len, "styx command");
	if(status == LUA_OK)
		status = lua_pcall(intL, 0, LUA_MULTRET, 0);
	/* Both compilation and runtime errors leave one error value on the stack. */
	count = lua_gettop(intL) - base;
	total = 0;
	for(j = 0; j < count; j++){
		length = 0;
		lua_tolstring(intL, base + 1 + j, &length);
		if(total >= (size_t)INT_MAX - 1 ||
		   length > (size_t)INT_MAX - total - 2){
			lua_settop(intL, base);
			buf[0] = '\0';
			return 0;
		}
		total += length + 1;
	}

	reply = styxmalloc((int)total + 1);
	if(reply == NULL){
		lua_settop(intL, base);
		buf[0] = '\0';
		return 0;
	}
	total = 0;
	for(j = 0; j < count; j++){
		length = 0;
		value = lua_tolstring(intL, base + 1 + j, &length);
		if(length != 0)
			memcpy(reply + total, value, length);
		total += length;
		reply[total++] = '\t';
	}
	reply[total] = '\0';
	lua_settop(intL, base);
	styxfree(buf);
	*pbuf = reply;
	return (int)total;
}




char*
fscgiopen(Qid *qid, int mode)
{
	Styxfile *f;

DBG("\n%s:%d qid->type = %d, qid.my_type = %d, mode = %x\n\n", __func__, __LINE__, qid->type, qid->my_type, mode);
	switch( qid->my_type ){
		case FS_CGI:
			qid->my_buf = NULL;
			qid->my_buf_idx = 0;

			qid->my_state = 0;
		
//			f = styxfindfile(server, qid->path);

//			if(mode&OTRUNC){	/* truncate on open */
//				styxfree(f->u);
//				f->u = nil;
//				f->d.length = 0;
//			}
			break;
			
	}
	
	return nil;
}


char*
fscgiclose(Qid qid, int mode)
{
	switch( qid.my_type ){
		case FS_CGI:
			if(qid.my_buf)
				styxfree(qid.my_buf);
			
//			if(mode&ORCLOSE)	/* remove on close */
//				//return fsremove(qid);
//				break;
			
	}
	return nil;
}


char *
fscgicreate(Qid *qid, char *name, int perm, int mode)
{
//	int isdir;
//	Styxfile *f;

	char *er = Eperm;

//	USED(mode);
//	isdir = perm & DMDIR;
DBG("%s: %d\n", __func__, __LINE__);

	switch( qid->my_type ){
	case FS_CGI:
			break;

	}
	
	return er;
}


char *
fscgiremove(Qid qid)
{
	Styxfile *f;

	switch( qid.my_type ){
		case FS_CGI:
			return Eperm;

	}
	
	return nil;
}


char *
fscgiwalk(Qid* qid, char *nm)
{
	char *er = Enonexist;

DBG("%s: %d, qid->my_type = %d, nm = %s\n", __func__, __LINE__, qid->my_type, nm);
	switch(qid->my_type){
	case FS_CGI:
		break;
		
	}
	
	return er;
}


char *
fscgiread(Qid *qid, char *buf, ulong *n, vlong *off)
{
	int m;
	Styxfile *f;
	int i;
	int dri = *off;
	int pth;

DBG("\n%s my_type = %d", __func__, qid->my_type);
if(qid->my_name)
	DBG(", my_name = %s", qid->my_name);
DBG("\n\n");

	switch( qid->my_type ){
		case FS_CGI:
			if(qid->my_buf != NULL){
				m = strlen(qid->my_buf );
DBG("\n%s dri=%d, *n=%d, qid.my_buf = %s\n\n", __func__, dri, *n, qid->my_buf );

				dri = 0;
				
				if(dri >= m){
					*n = 0;
				}else{
					if(dri + *n > m)
						*n = m-dri;
					memmove(buf, qid->my_buf + dri, *n);
				}
		
			}
			break;
	}

	return nil;
}


char*
fscgiwrite(Qid *qid, char *buf, ulong *n, vlong off)
{
	Styxfile *f;
//	vlong m, p;
	int p;
	char *u;

	int i;
	int dri = off;
	int pth;
	
//	static char *foo_nm = NULL;

DBG("%s: %d\n", __func__, __LINE__);
	switch( qid->my_type ){
		case FS_CGI:
			{
				char* p_buf = buf;
				int l, m;

				if(intL == NULL){
					return nil;
				}

				int type;
				Styxfile* f = styxfindfile(server, qid->path);
//				const TValue *val;

DBG("%s: %d f=%x\n", __func__, __LINE__, f);
				if(f == NULL)
					break;

				int fi = f->fi;
DBG("%s: %d fi=%d\n", __func__, __LINE__, fi);
				lua_getfield(intL, LUA_REGISTRYINDEX, styx_cgi_reg);
				lua_rawgeti(intL, -1, fi);
				lua_remove(intL, -2);

#if 0
				val = (const TValue *)f->u;
				
				if(val == NULL)
					break;

DBG("%s: %d val=%x\n", __func__, __LINE__, val);
//				m = 0;

				type = ttnov(val);
#endif
				type = lua_type(intL, -1);
DBG("%s: %d type=%d\n", __func__, __LINE__, type);

				if ( type == LUA_TFUNCTION) {
//					char* t_nm;
//					int t_ln;

//DBG("%s: %d. mmbr_nm = %s, l = %d\n", __func__, __LINE__, k_nm, k_ln);

//					if(LUA_TNONE <= type && type < LUA_NUMTAGS){
//						t_nm = ttypename(type);
//					}else{
//						t_nm = "";
//					}
//					t_ln = strlen(t_nm);

					call_virtual_foo(NULL, qid, buf, n, off);
					break;
				}else{
					lua_pop(intL, 1);
				}
				*n = 0; //m;
			}
			break;

	}
	
	return nil;
}


char*
fscgistat(Qid qid, Dir *d)
{
	Styxfile *file;

DBG("%s: %d. qid.my_type = %d, my_name = %s\n", __func__, __LINE__, qid.my_type, qid.my_name);
	switch(qid.my_type){
		case FS_CGI:
			file = styxfindfile(server, qid.path);
			*d = file->d;
			break;
	
	}
	
	return nil;
}


char*
fscgiwstat(Qid qid, Dir *d)
{
	Styxfile *f, *tf;
	Client *c;
//	int owner;

	/* the most complicated operation when fully allowed */

	c = styxclient(server);
	f = styxfindfile(server, qid.path);
//	owner = strcmp(c->uname, f->d.uid) == 0;
	if(d->name != nil && strcmp(d->name, f->d.name) != 0){
		/* need write permission in parent directory */
		if(!styxperm(f->parent, c->uname, OWRITE))
			return Eperm;
		if((tf = styxaddfile(server, f->parent->d.qid.path, -1, d->name, 0, "")) == nil){
			/* file with same name exists */
			return Eexist;
		}
		else{
			/* undo above addfile */
			styxrmfile(server, tf->d.qid.path);
		}
		/* ok to change name now */
		styxfree(f->d.name);
		f->d.name = strdup(d->name);	
	}
	if(d->uid != nil && strcmp(d->uid, f->d.uid) != 0){
//		if(!owner)
//			return Eperm;
		styxfree(f->d.uid);
		f->d.uid = strdup(d->uid);
	}
	if(d->gid != nil && strcmp(d->gid, f->d.gid) != 0){
//		if(!owner)
//			return Eperm;
		styxfree(f->d.gid);
		f->d.gid = strdup(d->gid);
	}
	if(d->mode != ~0 && d->mode != f->d.mode){
//		if(!owner)
//			return Eperm;
		if((d->mode & DMDIR) != (f->d.mode & DMDIR))
			return Eperm;	/* cannot change file->directory or vice-verse */
		f->d.mode = d->mode;
	}
	if(d->mtime != ~0 && d->mtime != f->d.mtime){
//		if(!owner)
//			return Eperm;
		f->d.mtime = d->mtime;
	}
	/* all other file attributes cannot be changed by wstat */
	return nil;
}



