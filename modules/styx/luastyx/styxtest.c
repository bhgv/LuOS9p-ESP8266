
/*
 * Copyright bhgv 2017
 */

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "ltm.h"



#include <lib9.h>
#include "styxserver.h"
#include "styx.h"

#include "lrotable.h"

/* Externally defined read-only table array */
extern const luaR_entry lua_rotable[];




enum {
	FL_T_NONE = 0,
	FL_T_DEV,
	FL_T_FILE,
	FL_T_C_CB,
	FL_T_L_CB,
};

enum {
	FS_ROOT = 0,
	FS_DEV,
	FS_DEV_FILE,
	FS_FILE,
	FS_FILE_DIR,
	FS_FILE_FILE,
};

enum {
	SC_BYNAME,
	SC_BYPOS,
};

enum {
	RD_ST_LIST,
	RD_ST_RES,
};


/*
 * An in-memory file server
 * allowing truncation, removal on closure, wstat and
 * all other file operations
 */

char *fsremove(Qid);


int nq = Qroot+1;

Styxserver *server = NULL;

int is_styx_srv_run = 1;

lua_State* intL = NULL;

int rd_state = RD_ST_LIST;
char *rd_res = NULL;



int pwm_cb(Styxfile* f, int v){
	return v;
}

int pio_cb(Styxfile* f, int v){
	return v;
}

int adc_cb(Styxfile* f, int v){
	return v;
}








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

//printf("%s: %d\n", __func__, __LINE__);
	if(buf != NULL){
		for(p = buf; *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'; p++);
		r = p;
		
		for(l = 0; *p != ' ' && *p !='\t' && *p != '\r' && *p != '\n' && *p != '\0'; p++, l++);
		*p = 0;
	}

	if(plen != NULL) *plen = l;

//printf("%s: %d. r=%s, l=%d\n", __func__, __LINE__, r, l);
	return r;
}

void fsnewclient(Client *c){
//	switch(c->)
}


char*
fsopen(Qid *qid, int mode)
{
	Styxfile *f;

//printf("fsopen 1 qid->type = %d, qid.my_type = %d\n", qid->type, qid->my_type);
	switch( qid->my_type ){
		case FS_DEV:
		case FS_DEV_FILE:
		case FS_FILE:
		case FS_FILE_DIR:
		case FS_FILE_FILE:
			break;

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

	USED(mode);
	isdir = perm&DMDIR;
	if(isdir)
		f = styxadddir(server, qid->path, nq++, name, perm, "inferno");
	else
		f = styxaddfile(server, qid->path, nq++, name, perm, "inferno");
	if(f == nil)
		return Eexist;
	f->u = nil;
	f->d.length = 0;
	*qid = f->d.qid;
	return nil;
}

char *
fsremove(Qid qid)
{
	Styxfile *f;

	f = styxfindfile(server, qid.path);
	if((f->d.qid.type&QTDIR) && f->child != nil)
		return "directory not empty";
	styxfree(f->u);
	styxrmfile(server, qid.path);	
	return nil;
}


char *
fswalk(Qid* qid, char *nm)
{
	//if(p->d.qid.my_type == 1){
	switch(qid->my_type){
	case FS_DEV:
		{
			int i = (int)scan_devs(lua_rotable, nm, SC_BYNAME);
			if(i > 0){
				qid->path = (1<<16) + i;
				qid->type = 0;
				
				qid->my_type = FS_DEV_FILE;
				
				qid->vers = 0;
				
				return nil;
			}
		}
		break;

	}
	//}else
	return 1;
}



char *
fsread(Qid qid, char *buf, ulong *n, vlong *off)
{
	int m;
	Styxfile *f;
	int i;
	int dri = *off;
	int pth;

	switch( qid.my_type ){
		case FS_DEV:
			{
				Dir d;
				int m = 0;
				int dsz = 0;

				memset( &d, 0, sizeof(Dir) );

				luaR_entry *entry = lua_rotable;

				for( i =0; entry->key.id.strkey && i < dri; ){
					//printf("i = %d  <  off = %d\n", i, dri);
					entry++;
					i++;
				}
				
				for ( ; entry->key.id.strkey && m < *n; entry++, i++ ) {
					if (entry->key.len >= 0) {
						d.name = entry->key.id.strkey;

						dsz = convD2M(&d, (uchar*)buf, *n-m);
						if(dsz <= BIT16SZ)
								break;
						m += dsz;
						buf += dsz;
					}else {
						//printf("  [%d] --> %x\r\n", entry->key.id.numkey,
						//		(unsigned int) rvalue(&entry->value));
					}
				}
				*n = m;
				*off = i;
			}
			break;
			
		case FS_DEV_FILE:
			if(rd_state == RD_ST_RES){
				if(rd_res == NULL) {
					rd_state = RD_ST_LIST;
					return nil;
				}
				m = strlen(rd_res);

//printf("\nrd_res = %s\n\n", rd_res);

				if(*off >= m){
					free(rd_res);
					rd_res = NULL;
				
					rd_state = RD_ST_LIST;
				
					*n = 0;
				}else{
					if(dri + *n > m)
						*n = m-dri;
					memmove(buf, rd_res + dri, *n);
				}
				
			}else{
				const TValue *val;
				luaR_entry *entry, *root_entry;
				
				pth = qid.path & 0xffff;
				
				entry = (luaR_entry*)scan_devs(lua_rotable, (char*)&pth, SC_BYPOS);
				root_entry = entry;

				m = 0;
				for ( ; entry->key.id.strkey && (m - dri) < (*n); entry++ ) {
					if (entry->key.len >= 0) {
						int l = entry->key.len;
						char* nm = entry->key.id.strkey;
						char* t_nm;
						int t_ln;
						int type;

						val = &entry->value;
						type = ttnov(val);

						if(LUA_TNONE <= type && type < LUA_NUMTAGS){
							t_nm = ttypename(type);
						}else{
							t_nm = "";
						}
						t_ln = strlen(t_nm);
					
						//printf("  [%s] --> %x (t = %d)\r\n", nm,
						//		(unsigned int) rvalue(&entry->value), type);
						if( type != LUA_TROTABLE && !(nm[0] == '_' && nm[1] == '_') ){
							if(m + l + 3 + t_ln + 1 > dri){
								if( m + l + 3 + t_ln + 1 - dri > (*n) )
									break;
								memmove(buf, nm, l);
								buf += l;
								memmove(buf, "\t-\t", 3);
								buf += 3;
								if(t_ln > 0) 
									memmove(buf, t_nm, t_ln);
								buf += t_ln;
								memmove(buf, "\n", 1);
								buf += 1;
							}
							m += l + 3 + t_ln + 1;
						}
					}
				}
				*n = m;
			}
			break;
			
		case FS_FILE:
			break;
			
		case FS_FILE_DIR:
			break;
			
		case FS_FILE_FILE:
			break;
			
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
			{
				const TValue *val;
				luaR_entry *entry, *root_entry;

				char* p_buf = buf;
				int l;

//printf("%s: %d\n", __func__, __LINE__);
				if(off == 0){
					char *s, *op;

					if(foo_nm != NULL) {
						free(foo_nm);
						foo_nm = NULL;
					}
					
					op = dev_call_parse_next_par(buf, &l);
					buf += l + 1;
					
					if( l == 0 || !strncmp(op, "list", l) ){
						rd_state = RD_ST_LIST;

						return nil;
					}else{
						rd_state = RD_ST_RES;

						if(rd_res != NULL){
							free(rd_res);
							rd_res = NULL;
						}
						
						s = dev_call_parse_next_par(buf, &l);
						if(intL == NULL || l == 0){
							return nil;
						}

						foo_nm = styxmalloc(l + 1);
						memcpy(foo_nm, s, l);
						foo_nm[ l ] = '\0';

//printf("%s: %d. foo_nm = %s, l = %d\n", __func__, __LINE__, foo_nm, l);
						buf += l + 1;
					}
				}
				pth = qid.path & 0xffff;
				
				entry = (luaR_entry*)scan_devs(lua_rotable, (char*)&pth, SC_BYPOS);
				root_entry = entry;
//printf("%s: %d. ent(%d)=%x\n", __func__, __LINE__, pth, entry);

				m = 0;
				for ( ; entry->key.id.strkey /*&& (m - dri) < (*n)*/; entry++ ) {
					if (
						entry->key.len >= 0 && 
						ttnov(&entry->value) == LUA_TFUNCTION &&
						!strncmp(foo_nm, entry->key.id.strkey, entry->key.len) 
					) {
						int k_ln = entry->key.len;
						char* k_nm = entry->key.id.strkey;
						char* t_nm;
						int t_ln;
						int type;
						int rn, ltop, i, lk, ls = 0;
						char *k, *s;
						float v;

//printf("%s: %d. mmbr_nm = %s, l = %d\n", __func__, __LINE__, k_nm, k_ln);

						val = &entry->value;
						type = ttnov(val);

						if(LUA_TNONE <= type && type < LUA_NUMTAGS){
							t_nm = ttypename(type);
						}else{
							t_nm = "";
						}
						t_ln = strlen(t_nm);
					
//printf("  [%s] --> %x (t = %d, %s), intL = %x\r\n", k_nm, (unsigned int) rvalue(val), type, t_nm, intL);
						ltop = lua_gettop(intL);
//printf("%s: %d\n", __func__, __LINE__);
						
						luaA_pushobject(intL, val);

//printf("%s: %d\n", __func__, __LINE__);
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
								
//printf("%s: %d. k=%s, s=%s\n", __func__, __LINE__, k, s);
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
							//}
						}

						lua_call(intL, i, LUA_MULTRET);

						rn = lua_gettop(intL) - ltop;
//printf("%s: %d. i=%d, rn=%d\n", __func__, __LINE__, i, rn);
						l = 0;
						for(i = ltop+1; i <= ltop + rn; i ++){
							int il;
//printf("%s: %d. i=%d, vtype=%s, %s\n", __func__, __LINE__, i, 
//								lua_typename(intL, lua_type(intL, i) ),
//								lua_tostring(intL, i) );
							lua_tolstring(intL, i, &il);
							l += il + 1;
						}

						rd_res = styxmalloc(l + 1);
						l = 0;
						for(i = ltop+1; i <= ltop + rn; i ++){
							int il;
							char *is;
							is = lua_tolstring(intL, i, &il);
							memcpy(&rd_res[ l ], is, il);
							l += il;
							rd_res[ l ] = '\t';
							l += 1;
							rd_res[ l ] = '\0';
/*
							switch( lua_type(intL, i) ){
								case 
#define LUA_TNONE		(-1)
									
#define LUA_TNIL		0
#define LUA_TBOOLEAN		1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER		3
#define LUA_TSTRING		4
#define LUA_TTABLE		5
#define LUA_TFUNCTION		6
#define LUA_TUSERDATA		7
#define LUA_TTHREAD		8
							}
*/
						}
//printf("%s: %d. rd_res = %s\n", __func__, __LINE__, rd_res);
					}
				}
				*n = m;
			}
			break;
			
		case FS_FILE:
			break;
			
		case FS_FILE_DIR:
			break;
			
		case FS_FILE_FILE:
			break;
			
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
fswstat(Qid qid, Dir *d)
{
	Styxfile *f, *tf;
	Client *c;
	int owner;

	/* the most complicated operation when fully allowed */

	c = styxclient(server);
	f = styxfindfile(server, qid.path);
	owner = strcmp(c->uname, f->d.uid) == 0;
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
		if(!owner)
			return Eperm;
		styxfree(f->d.uid);
		f->d.uid = strdup(d->uid);
	}
	if(d->gid != nil && strcmp(d->gid, f->d.gid) != 0){
		if(!owner)
			return Eperm;
		styxfree(f->d.gid);
		f->d.gid = strdup(d->gid);
	}
	if(d->mode != ~0 && d->mode != f->d.mode){
		if(!owner)
			return Eperm;
		if((d->mode&DMDIR) != (f->d.mode&DMDIR))
			return Eperm;	/* cannot change file->directory or vice-verse */
		f->d.mode = d->mode;
	}
	if(d->mtime != ~0 && d->mtime != f->d.mtime){
		if(!owner)
			return Eperm;
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
	nil,			/* stat */
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



void ro_traverse(luaR_entry *entry){
	const TValue *val;

	luaR_entry *hdr_entry = entry;
	
	while (entry->key.id.strkey) {
		if (entry->key.len >= 0) {
			printf("  [%s] --> %x\r\n", entry->key.id.strkey,
					(unsigned int) rvalue(&entry->value));
		} else {
			printf("  [%d] --> %x\r\n", entry->key.id.numkey,
					(unsigned int) rvalue(&entry->value));
		}

		val = &entry->value;
		if (ttnov(val) == LUA_TROTABLE && (luaR_entry *)val->value_.p != hdr_entry) {
			printf("VVV  %s (%x != %x) VVV\n", entry->key.id.strkey, val->value_.p, hdr_entry);
			ro_traverse( (luaR_entry *)val->value_.p );
			printf("AAA  %s  AAA\n\n", entry->key.id.strkey);
		}

		
		entry++;
	}

}

int luaopen_styx( lua_State *L ) {

	ro_traverse(lua_rotable);

	return 0;
}


MODULE_REGISTER_MAPPED(STYX, styx, styx_map, luaopen_styx);


