
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




char *fsremove(Qid);


int nq = Qroot+1;

Styxserver *server = NULL;

int is_styx_srv_run = 1;

char* styx_cgi_reg = "styx_cgi";



#if 1
Path
make_file_path(Path new_type, Path oldp, int idx){
	Path newp = 0ULL;
DBG("\n%s:%d oldp = %x:%x, i = %d\n", __func__, __LINE__, (int)(oldp>>32), (int)oldp, idx);
	Path lvl = PATH_LVL_GET(oldp);
DBG("%s:%d lvl = %x:%x\n", __func__, __LINE__, (int)(lvl>>32), (int)lvl);

	Path mainpt = oldp & PATH_MAIN_PT_MASK;
DBG("%s:%d mainpt = %x:%x\n", __func__, __LINE__, (int)(mainpt>>32), (int)mainpt);

	Path curpt = idx & PATH_STEP_MASK;
	curpt <<= lvl * PATH_MAIN_PT_STEP;
DBG("%s:%d curpt = %x:%x\n", __func__, __LINE__, (int)(curpt>>32), (int)curpt);

	mainpt &= ~( PATH_STEP_MASK << (lvl * PATH_MAIN_PT_STEP) );
DBG("%s:%d mainpt = %x:%x\n", __func__, __LINE__, (int)(mainpt>>32), (int)mainpt);
	mainpt |= curpt;
DBG("%s:%d mainpt = %x:%x\n", __func__, __LINE__, (int)(mainpt>>32), (int)mainpt);

	lvl++;
DBG("%s:%d lvl = %x:%x\n", __func__, __LINE__, (int)(lvl>>32), (int)lvl);

	PATH_LVL_SET(newp, lvl);
DBG("%s:%d newp = %x:%x\n", __func__, __LINE__, (int)(newp>>32), (int)newp);
	newp |= mainpt;
DBG("%s:%d newp = %x:%x\n", __func__, __LINE__, (int)(newp>>32), (int)newp);
DBG("%s:%d new_type = %x:%x\n", __func__, __LINE__, (int)(new_type>>32), (int)new_type);
	PATH_TYPE_COPY(newp, new_type);
DBG("%s:%d newp = %x:%x\n\n", __func__, __LINE__, (int)(newp>>32), (int)newp);
	
	return newp;
}
#endif



void fsnewclient(Client *c){
//	switch(c->)
}


char*
fsopen(Qid *qid, int mode)
{
	Styxfile *f;

DBG("\nfsopen 1 qid->type = %d, qid.my_type = %d, mode = %x\n\n", qid->type, qid->my_type, mode);
	switch( qid->my_type ){
		case FS_DEV:
		case FS_DEV_FILE:
			return fsdevopen(qid, mode);

		case FS_FILE:
		case FS_FILE_DIR:
		case FS_FILE_FILE:
			return fsfileopen(qid, mode);

		case FS_RPC:
			return fsrpcopen(qid, mode);
			
		case FS_CGI:
			return fscgiopen(qid, mode);
			
		default:
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
fsclose(Qid *qid, int mode)
{

	switch( qid->my_type ){
		case FS_DEV:
		case FS_DEV_FILE:
			return fsdevclose(qid, mode);
		
//		case FS_FILE:
		case FS_FILE_DIR:
		case FS_FILE_FILE:
			return fsfileclose(*qid, mode);
	
		case FS_RPC:
			return fsrpcclose(*qid, mode);
			
		case FS_CGI:
			return fscgiclose(*qid, mode);
			
		case FS_ROOT:
		//default:
			if(mode&ORCLOSE)	/* remove on close */
				return fsremove(*qid);
			
	}
	return nil;
}


char *
fscreate(Qid *qid, char *name, int perm, int mode)
{
	int isdir;
	Styxfile *f;

	char *er = "Create error";

	USED(mode);
	isdir = perm & DMDIR;
DBG("%s: %d\n", __func__, __LINE__);

	switch( qid->my_type ){
		case FS_FILE:
		case FS_FILE_DIR:
			return fsfilecreate(qid, name, perm, mode);

		case FS_RPC:
			return fsrpccreate(qid, name, perm, mode);
			
		case FS_CGI:
			return fscgicreate(qid, name, perm, mode);
			
		case FS_ROOT:
			if(isdir)
				f = styxadddir(server, qid->path, nq++, name, perm, "inferno");
			else
				f = styxaddfile(server, qid->path, nq++, name, perm, "inferno");
			if(f == nil)
				return Eexist;
			f->u = nil;
			f->d.length = 0;
			*qid = f->d.qid;

			er = nil;
			
			break;

	}
	
	return er;
}

char *
fsremove(Qid qid)
{
	Styxfile *f;

	struct stat statbuf;

	switch( qid.my_type ){
		case FS_FILE:
		case FS_FILE_DIR:
		case FS_FILE_FILE:
			return fsfileremove(qid);

		case FS_RPC:
			return fsrpcremove(qid);
			
		case FS_CGI:
			return fscgiremove(qid);
			
		case FS_ROOT:
			f = styxfindfile(server, qid.path);
			if((f->d.qid.type&QTDIR) && f->child != nil)
				return "directory not empty";
			styxfree(f->u);
			styxrmfile(server, qid.path);
			break;

	}
	
	return nil;
}


char *
fswalk(Qid* qid, char *nm)
{
	char *er = Enonexist;

DBG("%s: %d, qid->my_type = %d, nm = %s\n", __func__, __LINE__, qid->my_type, nm);
	switch(qid->my_type){
	case FS_DEV:
		return fsdevwalk(qid, nm);

	case FS_FILE:
	case FS_FILE_DIR:
	case FS_FILE_FILE:
		return fsfilewalk(qid, nm);
		
	case FS_RPC:
		return fsrpcwalk(qid, nm);

	case FS_CGI:
		return fscgiwalk(qid, nm);

	default:
//		er = nil;
		break;
		
	}
	
	return er;
}



char *
fsread(Qid *qid, char *buf, ulong *n, vlong *off)
{
	int m;
	Styxfile *f;
	int i;
	int dri = *off;
	int pth;

DBG("\n%s:%d, qid->my_buf = %x\n", __func__, __LINE__, qid->my_buf  );

DBG("\nfsread my_type = %d", qid->my_type);
if(qid->my_name){
	DBG(", my_name = %s", qid->my_name);
}
DBG("\n\n");

	switch( qid->my_type ){
		case FS_DEV:
		case FS_DEV_FILE:
			return fsdevread(*qid, buf, n, off);
			
		case FS_FILE:
		case FS_FILE_DIR:
		case FS_FILE_FILE:
			return fsfileread(*qid, buf, n, off);
			
		case FS_RPC:
			return fsrpcread(qid, buf, n, off);
		
		case FS_CGI:
			return fscgiread(qid, buf, n, off);
		
		default:
			f = styxfindfile(server, qid->path);
			m = f->d.length;
			if(*off >= m)
				*n = 0;
			else{
				if(*off + *n > m)
					*n = m-(*off);
				memmove(buf, (char*)f->u + (*off), *n);
			}
			break;
	}

	return nil;
}


char*
fswrite(Qid *qid, char *buf, ulong *n, vlong off)
{
	Styxfile *f;
	vlong m, p;
	char *u;

	int i;
	int dri = off;
	int pth;
	
	static char *foo_nm = NULL;

DBG("%s: %d, qid->my_type = %d, *n=%d, off=%d\n", __func__, __LINE__, qid->my_type, *n, off);
	switch( qid->my_type ){
		case FS_DEV_FILE:
DBG("%s: %d\n", __func__, __LINE__);
			return fsdevwrite(qid, buf, n, off);
			
		case FS_FILE:
		case FS_FILE_DIR:
		case FS_FILE_FILE:
DBG("%s: %d\n", __func__, __LINE__);
			return fsfilewrite(*qid, buf, n, off);
		
		case FS_RPC:
DBG("%s: %d\n", __func__, __LINE__);
			return fsrpcwrite(qid, buf, n, off);
		
		case FS_CGI:
DBG("%s: %d\n", __func__, __LINE__);
			return fscgiwrite(qid, buf, n, off);
		
		default:
DBG("%s: %d\n", __func__, __LINE__);
			f = styxfindfile(server, qid->path);
			m = f->d.length;
			p = off + *n;
			if(p > m){	/* just grab a larger piece of memory */
				u = styxmalloc(p);
				if(u == nil)
					return "2 out of memory";
				memset(u, 0, p);
				memmove(u, f->u, m);
				styxfree(f->u);
				f->u = u;
				f->d.length = p;
			}
			memmove((char*)f->u+off, buf, *n);
			break;

	}
	
	return nil;
}


char*
fsstat(Qid qid, Dir *d)
{
	Styxfile *file;

DBG("%s: %d. qid.my_type = %d, my_name = %s\n", __func__, __LINE__, 
			(int)qid.my_type, qid.my_name ? qid.my_name : "");
	switch(qid.my_type){
		case FS_FILE:
		case FS_FILE_DIR:
			return fsfilestat(qid, d);

		case FS_DEV:
		case FS_ROOT:
		//default:
			file = styxfindfile(server, qid.path);
			*d = file->d;
			break;
	
		case FS_RPC:
			return fsrpcstat(qid, d);
		
		case FS_CGI:
			return fscgistat(qid, d);
		
	}
	
	return nil;
}

	
char*
fswstat(Qid qid, Dir *d)
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



Styxops p9_root_ops = {
	fsnewclient,			/* newclient */
	nil,			/* freeclient */

	nil,			/* attach */
	fswalk,			/* walk */
	fsopen,		/* open */
	fscreate,		/* create */
	fsread,		/* read */
	fswrite,		/* write */
	fsclose,		/* close */
	fsremove,	/* remove */
	fsstat,			/* stat */
	fswstat,		/* wstat */
};




void
myinit(/*Styxserver *s*/)
{
//	int i, j;
	Styxfile* f;
//	char nm[5];

	server->root->d.qid.my_type = FS_ROOT;
	
	f = styxadddir(server, Qroot, 1, "dev", 0555, eve);
	f->d.qid.my_type = FS_DEV;

	f = styxadddir(server, Qroot, 2, "fs", 0777, eve);
	f->d.qid.my_type = FS_FILE;

	f = styxaddfile(server, Qroot, 3, "rpc", 0666, eve);
	f->d.qid.my_type = FS_RPC;
	
	nq = 4;
}



TValue *index2addr (lua_State *L, int idx) ;


#if 0
LUA_API void* lua_tolfunction (lua_State *L, int idx) {
  StkId o = index2addr(L, idx);
  //if (ttislcf(o)) return fvalue(o);
  //else 
  if (ttisLclosure(o))
    return clLvalue(o)->f;
  else return NULL;  /* not a C function */
}
#endif






int
lstyx_add_file(lua_State* L){
	int ln, i;
	int n = lua_gettop(L);
	char *path = luaL_checklstring(L, 1, &ln);

	if(path == NULL || ln <= 0 || n < 2 || !lua_isfunction(L, 2) ) return 0;
	
DBG("%s: %d\n", __func__, __LINE__);
	Styxfile* f = styxaddfile(server, Qroot, nq++, path, 0666, eve);
	if(f == NULL)
		return 0;
	
	f->d.qid.my_type = FS_CGI;
DBG("%s: %d\n", __func__, __LINE__);

	lua_getfield(L, LUA_REGISTRYINDEX, styx_cgi_reg);
DBG("%s: %d reg_tbl_cgi_type=%s\n", __func__, __LINE__, luaL_typename(L, -1));
	int ti = lua_gettop(L);
	
	lua_len(L, -1);
	int al = lua_tointeger(L, -1);
DBG("%s: %d al=%d\n", __func__, __LINE__, al);
	lua_settop(L, ti);
	
	for(i = 1; i <= al; i++){
		lua_rawgeti(L, -1, i);
		int isf = lua_isfunction(L, -1);
		lua_settop(L, ti);

		if(!isf){
			break;
		}
	}
DBG("%s: %d i=%d\n", __func__, __LINE__, i);
	if(i == 0) i++;
	
	lua_pushvalue(L, 2);
DBG("%s: %d reg_tbl_cgi_type=%s, val=%s\n", __func__, __LINE__, 
			luaL_typename(L, ti), luaL_typename(L, -1));
	lua_rawseti(L, ti, i);

	lua_settop(L, ti - 1);
DBG("%s: %d i=%d\n", __func__, __LINE__, i);

	f->fi = i;

#if 0
	 TValue *val = index2addr(L, 2);
	//luaA_pushobject(intL, val);
//	f->par.p = (void*)p;

	f->u = val_(val).gc;
	
DBG("%s: %d, f->u = %x\n", __func__, __LINE__, f->u);

	int type;

	//val->tt_ &= ~BIT_ISCOLLECTABLE;
	
	type = ttnov(val);
DBG("%s: %d\n", __func__, __LINE__);

	char* t_nm;
	int t_ln;
	
	if(LUA_TNONE <= type && type < LUA_NUMTAGS){
		t_nm = ttypename(type);
	}else{
		t_nm = "";
	}
	t_ln = strlen(t_nm);

DBG("%s: %d. type_nm = %s, l = %d\n", __func__, __LINE__, t_nm, t_ln);
#endif
		
	return 0;
}


int
lstyx_add_folder(lua_State* L){
	int ln;
	char *path = luaL_checklstring(L, 1, &ln);

	if(path == NULL || ln <= 0) return 0;
	
	Styxfile* f = styxadddir(server, Qroot, nq++, path, 0777, eve);
//	f->type = FL_T_FILE;
//	f->par.p = (void*)p;
	
	return 0;
}



int
lstyx_stop(lua_State* L){
	is_styx_srv_run = 0;
	return 0;
}
	
int
lstyx_loop(lua_State* L){
	if(server != NULL){
		free(server);
	}
	server = malloc(sizeof(Styxserver));

	intL = L;
	
//	styxdebug();
	
	styxinit(server, &p9_root_ops, "6701", 0777, 0/*1*/);
	
	myinit();

	is_styx_srv_run = 1;
	while(is_styx_srv_run){
		//printf( "my_main: Free heap %d bytes\r\n", (int)xPortGetFreeHeapSize());
		usleep(10);

		styxwait(server);
		styxprocess(server);
	}
	styxend(server);

	free(server);
	server = NULL;

	intL = NULL;
	
	return 0;
}




#include "modules.h"

const LUA_REG_TYPE styx_map[] = {
		{ LSTRKEY( "loop" ),		LFUNCVAL( lstyx_loop ) },		
		{ LSTRKEY( "stop" ),		LFUNCVAL( lstyx_stop ) },
		
		{ LSTRKEY( "add_file" ),	LFUNCVAL( lstyx_add_file ) },
		{ LSTRKEY( "add_dir" ),		LFUNCVAL( lstyx_add_folder ) },
		
		{ LNILKEY, LNILVAL }
};



int luaopen_styx( lua_State *L ) {
	lua_createtable(L, 0, 0); 
	lua_setfield(L, LUA_REGISTRYINDEX, styx_cgi_reg);
	//ro_traverse(lua_rotable);

	return 0;
}


MODULE_REGISTER_MAPPED(STYX, styx, styx_map, luaopen_styx);


