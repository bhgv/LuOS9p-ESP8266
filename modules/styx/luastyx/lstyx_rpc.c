
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
#define DBG(...) ;
#endif





int
call_rpc(char**pbuf, int len){
	char *buf = *pbuf;

	if(buf == NULL)
		return 0;

DBG("\n%s: %d\n%s\n\n", __func__, __LINE__, buf);

	int n = lua_gettop(intL);
	
	int er = luaL_loadstring(intL, buf);
	if(er == LUA_OK){
		lua_call(intL, 0, LUA_MULTRET);
//	}else{
//		lua_pushstring(intL, "Error!");
	}

	int m, l, j, i = lua_gettop(intL) - n;

	m = 0;
	for(j = 0; j < i; j++){
		lua_tolstring(intL, n + 1 + j, &l);
		m += l + 1;
	}

DBG("%s: %d, i=%d, n=%d, j=%d, m=%d\n", __func__, __LINE__, i, n, j, m);
	char *tb = styxmalloc(m + 1);
	tb[0] = '\0';
	
	m = 0;
	for(j = 0; j < i; j++){
		char *s = lua_tolstring(intL, n + 1 + j, &l);
		memcpy(&tb[ m ], s, l);
		m += l;
		tb[m] ='\t';
		m++;
		tb[m] = '\0';
	}
	

	styxfree(buf);
	*pbuf = tb;

	lua_settop(intL, n);
	
	return m;
}




char*
fsrpcopen(Qid *qid, int mode)
{
	Styxfile *f;

DBG("\n%s:%d qid->type = %d, qid.my_type = %d, mode = %x\n\n", __func__, __LINE__, qid->type, qid->my_type, mode);
	switch( qid->my_type ){
		case FS_RPC:
			qid->my_buf = NULL;
			qid->my_buf_idx = 0;

			qid->my_state = 0;
		
			f = styxfindfile(server, qid->path);

			if(mode&OTRUNC){	/* truncate on open */
				styxfree(f->u);
				f->u = nil;
				f->d.length = 0;
			}
			break;
			
	}
	
	return nil;
}


char*
fsrpcclose(Qid qid, int mode)
{
	switch( qid.my_type ){
		case FS_RPC:
			if(qid.my_buf)
				styxfree(qid.my_buf);
			
			if(mode&ORCLOSE)	/* remove on close */
				//return fsremove(qid);
				break;
			
	}
	return nil;
}


char *
fsrpccreate(Qid *qid, char *name, int perm, int mode)
{
	int isdir;
	Styxfile *f;

	char *er = "Create error";

	USED(mode);
	isdir = perm & DMDIR;
DBG("%s: %d\n", __func__, __LINE__);

	switch( qid->my_type ){
	case FS_RPC:
			break;

	}
	
	return er;
}


char *
fsrpcremove(Qid qid)
{
	Styxfile *f;

	switch( qid.my_type ){
		case FS_RPC:
			return "not possible";

	}
	
	return nil;
}


char *
fsrpcwalk(Qid* qid, char *nm)
{
	char *er = nil; //"Not found";

DBG("%s: %d, qid->my_type = %d, nm = %s\n", __func__, __LINE__, qid->my_type, nm);
	switch(qid->my_type){
	case FS_RPC:
		break;
		
	}
	
	return er;
}


char *
fsrpcread(Qid *qid, char *buf, ulong *n, vlong *off)
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
		case FS_RPC:
			{
				char st = qid->my_state;
				if(st != 1 && st != 2)
					break;
				
				if(st == 1){
					qid->my_buf_idx = call_rpc(&qid->my_buf, qid->my_buf_idx);
					*off = 0;
					dri = 0;
				}
				st = 2;
				
				i = qid->my_buf_idx;
				m = strlen( qid->my_buf ); //qid->my_buf_idx; //f->d.length;
			
DBG("%s: %d, qid->my_buf = %x, off = %d, *n = %d\n", __func__, __LINE__, qid->my_buf, dri, *n);
				if(dri >= m){
					*n = 0;
				}else{
					if(dri + *n > m)
						*n = m - dri;
					memmove(buf, qid->my_buf + dri, *n);
				}
DBG("%s: %d, qid->my_buf = '%s', off = %d, *n = %d\n", __func__, __LINE__, qid->my_buf, dri, *n);
			}
			break;
	}

	return nil;
}


char*
fsrpcwrite(Qid *qid, char *buf, ulong *n, vlong off)
{
	Styxfile *f;
//	vlong m, p;
	int p;
	char *u;

	int i;
	int dri = off;
	int pth;
	
	static char *foo_nm = NULL;

DBG("%s: %d\n", __func__, __LINE__);
	switch( qid->my_type ){
		case FS_RPC:
			{
				char *tb = qid->my_buf;
DBG("%s: %d, tb = %x, off = %d, *n = %d\n", __func__, __LINE__, tb, off, *n);
				if(*n > 0){
					p = (int)off + (int)(*n);
DBG("%s: %d, p = %d\n", __func__, __LINE__, p);
					qid->my_buf = styxmalloc(p+1);
					if(tb != NULL){
DBG("%s: %d\n", __func__, __LINE__);
						if(off > 0)
							memmove(qid->my_buf, tb, off);
						styxfree(tb);
					}

					tb = qid->my_buf;
DBG("%s: %d, tb = %x\n", __func__, __LINE__, tb);
#if 0
			if(p > m){	/* just grab a larger piece of memory */
				u = styxmalloc(p);
				if(u == nil)
					return "out of memory";
				memset(u, 0, p);
				memmove(u, f->u, m);
				styxfree(f->u);
				f->u = u;
				f->d.length = p;
			}
#endif
					memmove(tb + off, buf, *n);
					tb[p] = '\0';
					qid->my_buf_idx = p; 

					qid->my_state = 1;
				}
			}
			break;

	}
	
	return nil;
}


char*
fsrpcstat(Qid qid, Dir *d)
{
	Styxfile *file;

DBG("%s: %d. qid.my_type = %d, my_name = %s\n", __func__, __LINE__, qid.my_type, qid.my_name);
	switch(qid.my_type){
		case FS_RPC:
			file = styxfindfile(server, qid.path);
			*d = file->d;
			break;
	
	}
	
	return nil;
}

	
char*
fsrpcwstat(Qid qid, Dir *d)
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



