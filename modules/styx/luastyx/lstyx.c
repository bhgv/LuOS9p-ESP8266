
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


char *fsremove(Qid);


int nq = Qroot+1;

Styxserver *server = NULL;

int is_styx_srv_run = 1;



#if 0
Path
make_file_path(Path new_type, Path oldp, int idx){
	Path newp = 0ULL;
//printf("\n%s:%d oldp = %x:%x, i = %d\n", __func__, __LINE__, (int)(oldp>>32), (int)oldp, idx);
	Path lvl = PATH_LVL_GET(oldp);
//printf("%s:%d lvl = %x:%x\n", __func__, __LINE__, (int)(lvl>>32), (int)lvl);

	Path mainpt = oldp & PATH_MAIN_PT_MASK;
//printf("%s:%d mainpt = %x:%x\n", __func__, __LINE__, (int)(mainpt>>32), (int)mainpt);

	Path curpt = idx & PATH_STEP_MASK;
	curpt <<= lvl * PATH_MAIN_PT_STEP;
//printf("%s:%d curpt = %x:%x\n", __func__, __LINE__, (int)(curpt>>32), (int)curpt);

	mainpt &= ~( PATH_STEP_MASK << (lvl * PATH_MAIN_PT_STEP) );
//printf("%s:%d mainpt = %x:%x\n", __func__, __LINE__, (int)(mainpt>>32), (int)mainpt);
	mainpt |= curpt;
//printf("%s:%d mainpt = %x:%x\n", __func__, __LINE__, (int)(mainpt>>32), (int)mainpt);

	lvl++;
//printf("%s:%d lvl = %x:%x\n", __func__, __LINE__, (int)(lvl>>32), (int)lvl);

	PATH_LVL_SET(newp, lvl);
//printf("%s:%d newp = %x:%x\n", __func__, __LINE__, (int)(newp>>32), (int)newp);
	newp |= mainpt;
//printf("%s:%d newp = %x:%x\n", __func__, __LINE__, (int)(newp>>32), (int)newp);
//printf("%s:%d new_type = %x:%x\n", __func__, __LINE__, (int)(new_type>>32), (int)new_type);
	PATH_TYPE_COPY(newp, new_type);
//printf("%s:%d newp = %x:%x\n\n", __func__, __LINE__, (int)(newp>>32), (int)newp);
	
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

printf("\nfsopen 1 qid->type = %d, qid.my_type = %d, mode = %x\n\n", qid->type, qid->my_type, mode);
	switch( qid->my_type ){
		case FS_DEV:
		case FS_DEV_FILE:
			return fsdevopen(qid, mode);

		case FS_FILE:
		case FS_FILE_DIR:
		case FS_FILE_FILE:
			return fsfileopen(qid, mode);

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
fsclose(Qid qid, int mode)
{
	if(mode&ORCLOSE)	/* remove on close */
		return fsremove(qid);
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
printf("%s: %d\n", __func__, __LINE__);

	switch( qid->my_type ){
		case FS_FILE:
		case FS_FILE_DIR:
			return fsfilecreate(qid, name, perm, mode);

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
	char *er = "Not found";

printf("%s: %d, qid->my_type = %d, nm = %s\n", __func__, __LINE__, qid->my_type, nm);
	switch(qid->my_type){
	case FS_DEV:
		return fsdevwalk(qid, nm);

	case FS_FILE:
	case FS_FILE_DIR:
	case FS_FILE_FILE:
		return fsfilewalk(qid, nm);
		
	}
	
	return er;
}



char *
fsread(Qid qid, char *buf, ulong *n, vlong *off)
{
	int m;
	Styxfile *f;
	int i;
	int dri = *off;
	int pth;

printf("\nfsread my_type = %d", qid.my_type);
if(qid.my_name)
	printf(", my_name = %s", qid.my_name);
printf("\n\n");

	switch( qid.my_type ){
		case FS_DEV:
		case FS_DEV_FILE:
			return fsdevread(qid, buf, n, off);
			
		case FS_FILE:
		case FS_FILE_DIR:
		case FS_FILE_FILE:
			return fsfileread(qid, buf, n, off);
			
		default:
			f = styxfindfile(server, qid.path);
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
fswrite(Qid qid, char *buf, ulong *n, vlong off)
{
	Styxfile *f;
	vlong m, p;
	char *u;

	int i;
	int dri = off;
	int pth;
	
	static char *foo_nm = NULL;

//printf("%s: %d\n", __func__, __LINE__);
	switch( qid.my_type ){
		case FS_DEV_FILE:
			return fsdevwrite(qid, buf, n, off);
			
		case FS_FILE:
		case FS_FILE_DIR:
		case FS_FILE_FILE:
			return fsfilewrite(qid, buf, n, off);
			
		default:
			f = styxfindfile(server, qid.path);
			m = f->d.length;
			p = off + *n;
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
			memmove((char*)f->u+off, buf, *n);
			break;

	}
	
	return nil;
}


char*
fsstat(Qid qid, Dir *d)
{
	Styxfile *file;

//printf("%s: %d. qid.my_type = %d, my_name = %s\n", __func__, __LINE__, qid.my_type, qid.my_name);

	switch(qid.my_type){
		case FS_FILE:
		case FS_FILE_DIR:
			return fsfilestat(qid, d);

		case FS_DEV:
		case FS_ROOT:
			file = styxfindfile(server, qid.path);
			*d = file->d;
			break;
	
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
	int i, j;
	Styxfile* f;
	char nm[5];
	
	//styxadddir(server, Qroot, Qroot, "/", 0555, "inferno");
	
	f = styxadddir(server, Qroot, 1, "dev", 0555, "inferno");
	//f->d.type = FS_DEV;
	f->d.qid.my_type = FS_DEV;
	
	f = styxaddfile(server, 1, 1000, "tst", 0666, "inferno");

	f = styxadddir(server, Qroot, 2, "fs", 0555, "inferno");
	//f->d.type = FS_FILE;
	f->d.qid.my_type = FS_FILE;
	

	j = 3;

	

/*
	styxadddir(s, 1, 2, "pwm", 0555, "inferno");
	j = 2;
	for(i = 0; i < 16; i++){
		snprintf(nm, 5, "%d", i);
		f = styxaddfile(s, j, j+1+i, nm, 0666, "inferno");
		f->type = FL_T_DEV;
		f->par.p = (void*)pwm_cb;
	}
	j = j + 1 + i;
	
	styxadddir(s, 1, j, "pio", 0555, "inferno");
	for(i = 0; i < 16; i++){
		snprintf(nm, 5, "%d", i);
		f = styxaddfile(s, j, j+1+i, nm, 0666, "inferno");
		f->type = FL_T_DEV;
		f->par.p = (void*)pio_cb;
	}
	j = j + 1 + i;
	
	styxadddir(s, 1, j, "adc", 0555, "inferno");
	for(i = 0; i < 4; i++){
		snprintf(nm, 5, "%d", i);
		f = styxaddfile(s, j, j + i + 1, nm, 0666, "inferno");
		f->type = FL_T_DEV;
		f->par.p = (void*)adc_cb;
	}
	j = j + 1 + i;
*/

//	styxaddfile(s, Qroot, 1, "fred", 0664, "inferno");
//	styxaddfile(s, Qroot, 2, "joe", 0664, "q56");
//	styxadddir(s, Qroot, 3, "adir", 0775, "inferno");
//	styxaddfile(s, 3, 4, "bill", 0664, "inferno");
//	styxadddir(s, Qroot, 5, "new", 0775, "inferno");
//	styxadddir(s, 5, 6, "cdir", 0775, "inferno");
//	styxaddfile(s, 6, 7, "cfile", 0664, "inferno");
	nq = j;
}



int
lstyx_add_file(lua_State* L){
	int ln;
	char *path = luaL_checklstring(L, 1, &ln);

	if(path == NULL || ln <= 0) return 0;

//	char* p = malloc(ln+1);
//	memcpy(p, path, ln);
//	p[ln] = '\0';
	
	Styxfile* f = styxaddfile(server, Qroot, nq, path, 0666, "inferno");
	f->type = FL_T_FILE;
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
	
	styxdebug();
	
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
//		{ LSTRKEY( "add_to_callbacks" ),	LFUNCVAL( httpd_task_cb_run ) },
		
		{ LSTRKEY( "stop" ),			LFUNCVAL( lstyx_stop ) },
		
		{ LSTRKEY( "add_file" ),			LFUNCVAL( lstyx_add_file ) },
		
//		{ LSTRKEY( "recv_timeout" ),	LFUNCVAL( httpd_recv_timeout ) },
//		{ LSTRKEY( "send_timeout" ),	LFUNCVAL( httpd_send_timeout ) },
		
		{ LNILKEY, LNILVAL }
};



int luaopen_styx( lua_State *L ) {

	//ro_traverse(lua_rotable);

	return 0;
}


MODULE_REGISTER_MAPPED(STYX, styx, styx_map, luaopen_styx);


