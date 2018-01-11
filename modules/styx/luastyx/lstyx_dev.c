
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




/* Externally defined read-only table array */
extern const luaR_entry lua_rotable[];


lua_State* intL = NULL;

int rd_state = RD_ST_LIST;
char *rd_res = NULL;




void fsdevnewclient(Client *c){
//	switch(c->)
}


char*
fsdevopen(Qid *qid, int mode)
{
	Styxfile *f;

DBG("\nfsopen 1 qid->type = %d, qid.my_type = %d, mode = %x\n\n", qid->type, qid->my_type, mode);
	switch( qid->my_type ){
		case FS_DEV:
		case FS_DEV_FILE:
			break;

	}
	
	return nil;
}


char*
fsdevclose(Qid qid, int mode)
{
	return nil;
}


char *
fsdevcreate(Qid *qid, char *name, int perm, int mode)
{
	int isdir;
	Styxfile *f;

	char *er = "Create error";

	USED(mode);
	isdir = perm & DMDIR;
DBG("%s: %d\n", __func__, __LINE__);

	switch( qid->my_type ){

	}
	
	return er;
}

char *
fsdevremove(Qid qid)
{
	Styxfile *f;

	switch( qid.my_type ){

	}
	
	return nil;
}


char *
fsdevwalk(Qid* qid, char *nm)
{
	char *er = "Not found";

DBG("%s: %d, qid->my_type = %d, nm = %s\n", __func__, __LINE__, qid->my_type, nm);
	switch(qid->my_type){
	case FS_DEV:
		{
			int i = (int)scan_devs(lua_rotable, nm, SC_BYNAME);
			if(i > 0){
				//qid->path = PATH_DEV + (PATH_STEP_MASK & i);
				qid->path = make_file_path( PATH_DEV, qid->path, i);
				qid->type = 0; //QTDIR;
				qid->vers = 0;

				qid->my_type = FS_DEV_FILE;
				qid->my_name = strdup(nm);
				
				return nil;
			}
		}
		break;

	}
	
	return er;
}



char *
fsdevread(Qid qid, char *buf, ulong *n, vlong *off)
{
	int m;
	Styxfile *f;
	int i;
	int dri = *off;
	int pth;

DBG("\nfsread my_type = %d", qid.my_type);
if(qid.my_name)
	DBG(", my_name = %s", qid.my_name);
DBG("\n\n");

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
//DBG("  [%d] --> %x\r\n", entry->key.id.numkey,
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

//DBG("\nrd_res = %s\n\n", rd_res);

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
				
				pth = (int)(qid.path & PATH_STEP_MASK);
				
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
			
	}

	return nil;
}


char*
fsdevwrite(Qid qid, char *buf, ulong *n, vlong off)
{
	Styxfile *f;
	vlong m, p;
	char *u;

	int i;
	int dri = off;
	int pth;
	
	static char *foo_nm = NULL;

//DBG("%s: %d\n", __func__, __LINE__);
	switch( qid.my_type ){
		case FS_DEV_FILE:
			{
				const TValue *val;
				luaR_entry *entry, *root_entry;

				char* p_buf = buf;
				int l;

//DBG("%s: %d\n", __func__, __LINE__);
				if(off == 0){
					char *s, *op;

					if(foo_nm != NULL) {
						free(foo_nm);
						foo_nm = NULL;
					}

/*
					op = dev_call_parse_next_par(buf, &l);
					buf += l + 1;
					
					if( l == 0 || !strncmp(op, "list", l) ){
						rd_state = RD_ST_LIST;

						return nil;
					}else
*/
					{
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

//DBG("%s: %d. foo_nm = %s, l = %d\n", __func__, __LINE__, foo_nm, l);
						buf += l + 1;
					}
				}
				pth = qid.path & 0xffff;
				
				entry = (luaR_entry*)scan_devs(lua_rotable, (char*)&pth, SC_BYPOS);
				root_entry = entry;
//DBG("%s: %d. ent(%d)=%x\n", __func__, __LINE__, pth, entry);

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

//DBG("%s: %d. mmbr_nm = %s, l = %d\n", __func__, __LINE__, k_nm, k_ln);

						val = &entry->value;
						type = ttnov(val);

						if(LUA_TNONE <= type && type < LUA_NUMTAGS){
							t_nm = ttypename(type);
						}else{
							t_nm = "";
						}
						t_ln = strlen(t_nm);
					
//DBG("  [%s] --> %x (t = %d, %s), intL = %x\r\n", k_nm, (unsigned int) rvalue(val), type, t_nm, intL);
						ltop = lua_gettop(intL);
//DBG("%s: %d\n", __func__, __LINE__);
						
						luaA_pushobject(intL, val);

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
							//}
						}

						lua_call(intL, i, LUA_MULTRET);

						rn = lua_gettop(intL) - ltop;
//DBG("%s: %d. i=%d, rn=%d\n", __func__, __LINE__, i, rn);
						l = 0;
						for(i = ltop+1; i <= ltop + rn; i ++){
							int il;
//DBG("%s: %d. i=%d, vtype=%s, %s\n", __func__, __LINE__, i, 
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
//DBG("%s: %d. rd_res = %s\n", __func__, __LINE__, rd_res);
					}
				}
				*n = m;
			}
			break;
			

	}
	
	return nil;
}


char*
fsdevstat(Qid qid, Dir *d)
{
	Styxfile *file;

//DBG("%s: %d. qid.my_type = %d, my_name = %s\n", __func__, __LINE__, qid.my_type, qid.my_name);

	switch(qid.my_type){
		case FS_DEV:
			break;
	
	}
	
	return nil;
}

	
char*
fsdevwstat(Qid qid, Dir *d)
{
#if 0
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
#endif
	return nil;
}


#if 0
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
#endif

