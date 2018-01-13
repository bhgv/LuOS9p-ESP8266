
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
#define DBG(...)  ;
#endif




char*
fsfileopen(Qid *qid, int mode)
{
	Styxfile *f;

DBG("\nfsopen 1 qid->type = %d, qid.my_type = %d, mode = %x\n\n", qid->type, qid->my_type, mode);
	switch( qid->my_type ){
		case FS_FILE:
		case FS_FILE_DIR:
			break;
		
		case FS_FILE_FILE:
			{
//				char mo[10], *pmo;
				char *otype = NULL;

				FILE* f = NULL;
				char *buf = malloc(256);
				
				int isread = (mode & 0xf) == OREAD | (mode & 0xf) == OEXEC;
				int iswrite =(mode & 0xf) == OWRITE;
				int isrdwr = (mode & 0xf) == ORDWR;
				int istrunc = (mode & 0xf0) == OTRUNC;
				
//				pmo = mo;
				if(isread)
					otype = "r";
				else if(istrunc)
					otype = "w";
				else if(iswrite)
					otype = "r+";
				else if(isrdwr)
					otype = "r+";
				
				int l = file_pathname_from_path(qid->path, buf, 255);
				if(l == 0)
					return "File not found";

				if(otype != NULL)
					f = fopen(buf, otype);

				qid->my_f = f;
				
				free(buf);
			}
			break;

	}
	
	return nil;
}


char*
fsfileclose(Qid qid, int mode)
{
DBG("%s: %d, qid->my_type = %d, mode = %x\n", __func__, __LINE__, qid.my_type, mode);

	switch( qid.my_type ){
		case FS_FILE_FILE:
			{
				FILE* f = NULL;
				
				int isrm = mode & ORCLOSE;
DBG("%s: %d, qid->my_f = %x\n", __func__, __LINE__, qid.my_f);

				f = qid.my_f;
				if(f != NULL)
					fclose(f);
				qid.my_f = NULL;

				if(qid.my_name != NULL)
					styxfree(qid.my_name);

				if(isrm){
					char *buf = malloc(256);
					int l = file_pathname_from_path(qid.path, buf, 255);
					if(l == 0){
						return "File not found";
					}
					
					remove(buf);
				
					free(buf);
				}

			}
			break;
	}
	
	return nil;
}


char *
fsfilecreate(Qid *qid, char *name, int perm, int mode)
{
	int isdir;
	Styxfile *f;

	char *er = "Create error";

	USED(mode);
	isdir = perm & DMDIR;
DBG("%s: %d, qid->my_type = %d, mode = %x, name = %s, perm = %x\n", __func__, __LINE__, 
				qid->my_type, mode, name, perm);

	switch( qid->my_type ){
		case FS_FILE:
		case FS_FILE_DIR:
			{
				char *pth = malloc(256);
				int nm_len = strlen(name);
				int l = file_pathname_from_path(qid->path, pth, 255);
				int pth_l = l;
DBG("\n%s: %d. len = %d, pth = %s\n", __func__, __LINE__, l, pth);

				if(l + 1 + nm_len > 255){
					free(pth);
					return er;
				}
				pth[ l ] = '/';
				l++;
				memcpy( &pth[ l ], name, nm_len);
				l += nm_len;
				pth[ l ] = '\0';
DBG("%s: %d, pth = %s\n", __func__, __LINE__, pth);

				if(isdir){
DBG("%s: %d\n", __func__, __LINE__);
					mkdir(pth, 0);
					qid->my_f = NULL;
//					qid->type = QTDIR;
//					qid->my_type = FS_FILE_DIR;
				}else{
DBG("%s: %d\n", __func__, __LINE__);
					FILE *f = fopen(pth, "w+");
					qid->my_f = f;
//					fclose(f);
//					qid->type = QTFILE;
//					qid->my_type = FS_FILE_FILE;
				}
				qid->my_name = strdup(name);

				pth[ pth_l ] = '\0';
				er = scan_fs_dir(qid, pth, name);
				
				free(pth);
			}
			break;

	}
	
	return er;
}

char *
fsfileremove(Qid qid)
{
	Styxfile *f;

	struct stat statbuf;

DBG("\n%s: %d. qid.my_type = %d, qid.path = %x:%x\n", __func__, __LINE__, qid.my_type, 
												(int)(qid.path>>32), (int)qid.path );
	switch( qid.my_type ){
		case FS_FILE:
			break;
	
		case FS_FILE_DIR:
			{
				char *pth = malloc(256);
				pth[0] = '\0';
				int l = file_pathname_from_path(qid.path, pth, 255);
DBG("\n%s: %d. len = %d, pth = %s\n", __func__, __LINE__, l, pth);
				if (stat(pth, &statbuf) != 0) {
					free(pth);
					return nil;
				}
				
				if (S_ISDIR(statbuf.st_mode)) {
					rmdir(pth);
				}
				free(pth);
			}
			break;

		case FS_FILE_FILE:
			{
				char *pth = malloc(256);
				pth[0] = '\0';
				int l = file_pathname_from_path(qid.path, pth, 255);
DBG("\n%s: %d. len = %d, pth = %s\n", __func__, __LINE__, l, pth);

				if(qid.my_f != NULL){
					fclose(qid.my_f );
					qid.my_f = NULL;
				}
				
				if (stat(pth, &statbuf) != 0) {
					free(pth);
					return nil;
				}
				
				if (!S_ISDIR(statbuf.st_mode)) {
					remove(pth);
				}
				free(pth);
			}
			break;

	}
	
	return nil;
}


char *
fsfilewalk(Qid* qid, char *nm)
{
	char *er = "Not found";

DBG("%s: %d, qid->my_type = %d, nm = %s\n", __func__, __LINE__, qid->my_type, nm);
	switch(qid->my_type){
	case FS_FILE:
		er = scan_fs_dir(qid, "/", nm);
		break;
		
	case FS_FILE_DIR:
		{
			char *buf = malloc(256);
			int l = file_pathname_from_path(qid->path, buf, 255);
			er = scan_fs_dir(qid, buf, nm);
			free(buf);
		}
		break;
		
	case FS_FILE_FILE:
		{
			char *buf = malloc(256);
			int l = file_pathname_from_path(qid->path, buf, 255);
			er = scan_fs_dir(qid, buf, nm);
			free(buf);
		}
		break;
		
	}
	
	return er;
}



char *
fsfileread(Qid qid, char *buf, ulong *n, vlong *off)
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
		case FS_FILE:
			ls_dir_rd_out("/", qid, buf, n, off);
			break;
			
		case FS_FILE_DIR:
			{
				char *pth = malloc(256);
				int l = file_pathname_from_path(qid.path, pth, 255);
//DBG("\n%s: %d. len = %d, pth = %s\n", __func__, __LINE__, l, pth);
				ls_dir_rd_out(pth, qid, buf, n, off);
				free(pth);
			}
			break;
			
		case FS_FILE_FILE:
			{
				FILE *fp;
				int c;
				int i, j;
//				char *pth = malloc(256);
DBG("\n%s:%d path = %x:%x\n", __func__, __LINE__, (int)(qid.path>>32), (int)qid.path);
//				int l = file_pathname_from_path(qid.path, pth, 255);

//DBG("%s: %d. pth = %s, dri = %d, *n = %d\n\n", __func__, __LINE__, pth, dri, *n);
				fp = qid.my_f;  //fopen(pth,"r");

//				free(pth);
				
				if (!fp) {
					*n = 0;
					break;
				}

				i = 0; j = 0;
				while((c = fgetc(fp)) != EOF && j < *n) {
					if(i >= dri){
DBG("%c", c);
						buf[ j ] = c;
						j++;
					}
					i++;
				}
//				fclose(fp);
DBG("\n%s: %d. i = %d, j = %d\n\n", __func__, __LINE__, i, j);
				*n = j;
				//*off = i;
			}
			break;
			
	}

	return nil;
}


char*
fsfilewrite(Qid qid, char *buf, ulong *n, vlong off)
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
		case FS_FILE:
			break;
			
		case FS_FILE_DIR:
			break;
			
		case FS_FILE_FILE:
			{
				FILE *fp;
				int c;
				int i, j;
//				char *pth = malloc(256);
DBG("\n%s:%d path = %x:%x\n", __func__, __LINE__, (int)(qid.path>>32), (int)qid.path);
//				int l = file_pathname_from_path(qid.path, pth, 255);

//DBG("%s: %d. pth = %s, dri = %d, *n = %d\n\n", __func__, __LINE__, pth, dri, *n);
				fp = qid.my_f;  //fopen(pth,"w+");

//				free(pth);
				
				if (!fp) {
					*n = 0;
					break;
				}

				i = 0; j = 0;
				while( j < *n ) {
					if(i >= dri){
						c = buf[ j ];
						fputc(c, fp);
DBG("%c", c);
						j++;
//					}else{
//						c = fgetc(fp);
					}
					i++;
				}
//				fclose(fp);
DBG("\n%s: %d. i = %d, j = %d\n\n", __func__, __LINE__, i, j);
				*n = j;
				//*off = i;
			}
			break;
			
	}
	
	return nil;
}


char*
fsfilestat(Qid qid, Dir *d)
{
	Styxfile *file;

DBG("%s: %d. qid.my_type = %d, my_name = %s\n", __func__, __LINE__, qid.my_type, qid.my_name);

	switch(qid.my_type){
//		case FS_FILE:
		case FS_FILE_DIR:
		case FS_FILE_FILE:
			{
				memset(d, 0, sizeof(Dir) );
				
				//d->type = 'X';
				//d->dev = 'x';
				
				//d->qid.path = qid.path;
				//d->qid.type = 0;
				//d->qid.vers = 0;
				
				//d->qid.my_type = 0;

				d->qid = qid;
				
				d->mode = 0777; //mode;
				d->atime = styxtime(0);
				d->mtime = styxtime(0); //boottime;
				d->length = 0;
				d->name = strdup(qid.my_name); //strdup(qid.my_name);
				//d->uid = strdup(owner);
				//d->gid = strdup(eve);
				d->muid = "";
				
				if(qid.my_type == FS_FILE_DIR){
					d->qid.type |= QTDIR;
					d->mode |= DMDIR;
				}
				else{
					d->qid.type &= ~QTDIR;
					d->mode &= ~DMDIR;
				}
			}
			break;

		case FS_FILE:
			file = styxfindfile(server, qid.path);
			*d = file->d;
			break;
	
	}
	
	return nil;
}

	
char*
fsfilewstat(Qid qid, Dir *d)
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

