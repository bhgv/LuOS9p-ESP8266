
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


/*
 * An in-memory file server
 * allowing truncation, removal on closure, wstat and
 * all other file operations
 */


char*
fsfileopen(Qid *qid, int mode)
{
	Styxfile *f;

printf("\nfsopen 1 qid->type = %d, qid.my_type = %d, mode = %x\n\n", qid->type, qid->my_type, mode);
	switch( qid->my_type ){
		case FS_DEV:
		case FS_DEV_FILE:
		case FS_FILE:
		case FS_FILE_DIR:
		case FS_FILE_FILE:
			break;

	}
	
	return nil;
}


char*
fsfileclose(Qid qid, int mode)
{
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
printf("%s: %d\n", __func__, __LINE__);

	switch( qid->my_type ){
		case FS_FILE:
		case FS_FILE_DIR:
			{
				char *pth = malloc(256);
				int nm_len = strlen(name);
				int l = file_pathname_from_path(qid->path, pth, 255);
				int pth_l = l;
printf("\n%s: %d. len = %d, pth = %s\n", __func__, __LINE__, l, pth);

				if(l + 1 + nm_len > 255){
					free(pth);
					return er;
				}
				pth[ l ] = '/';
				l++;
				memcpy( &pth[ l ], name, nm_len);
				l += nm_len;
				pth[ l ] = '\0';
printf("%s: %d, pth = %s\n", __func__, __LINE__, pth);

				if(isdir){
printf("%s: %d\n", __func__, __LINE__);
					mkdir(pth, 0);
//					qid->type = QTDIR;
//					qid->my_type = FS_FILE_DIR;
				}else{
printf("%s: %d\n", __func__, __LINE__);
					FILE *f = fopen(pth, "a");
					fclose(f);
//					qid->type = QTFILE;
//					qid->my_type = FS_FILE_FILE;
				}
				qid->my_name = strdup(name);

				pth[ pth_l ] = '\0';
				er = scan_fs_dir(qid, pth, name);
				
				free(pth);
//				er = nil;
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

	switch( qid.my_type ){
		case FS_FILE:
			break;
	
		case FS_FILE_DIR:
			{
				char *pth = malloc(256);
				int l = file_pathname_from_path(qid.path, pth, 255);
//printf("\n%s: %d. len = %d, pth = %s\n", __func__, __LINE__, l, pth);
				if (stat(pth, &statbuf) != 0) {
					return 1;
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
				int l = file_pathname_from_path(qid.path, pth, 255);
//printf("\n%s: %d. len = %d, pth = %s\n", __func__, __LINE__, l, pth);
				if (stat(pth, &statbuf) != 0) {
					return 1;
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

printf("%s: %d, qid->my_type = %d, nm = %s\n", __func__, __LINE__, qid->my_type, nm);
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

printf("\nfsread my_type = %d", qid.my_type);
if(qid.my_name)
	printf(", my_name = %s", qid.my_name);
printf("\n\n");

	switch( qid.my_type ){
		case FS_FILE:
			ls_dir_rd_out("/", qid, buf, n, off);
			break;
			
		case FS_FILE_DIR:
			{
				char *pth = malloc(256);
				int l = file_pathname_from_path(qid.path, pth, 255);
//printf("\n%s: %d. len = %d, pth = %s\n", __func__, __LINE__, l, pth);
				ls_dir_rd_out(pth, qid, buf, n, off);
				free(pth);
			}
			break;
			
		case FS_FILE_FILE:
			{
				FILE *fp;
				int c;
				int i, j;
				char *pth = malloc(256);
printf("\n%s:%d path = %x:%x\n", __func__, __LINE__, (int)(qid.path>>32), (int)qid.path);
				int l = file_pathname_from_path(qid.path, pth, 255);

printf("%s: %d. pth = %s, dri = %d, *n = %d\n\n", __func__, __LINE__, pth, dri, *n);
				fp = fopen(pth,"r");

				free(pth);
				
				if (!fp) {
					*n = 0;
					break;
				}

				i = 0; j = 0;
				while((c = fgetc(fp)) != EOF && j < *n) {
					if(i >= dri){
printf("%c", c);
						buf[ j ] = c;
						j++;
					}
					i++;
				}
				fclose(fp);
printf("\n%s: %d. i = %d, j = %d\n\n", __func__, __LINE__, i, j);
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

//printf("%s: %d\n", __func__, __LINE__);
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
				char *pth = malloc(256);
printf("\n%s:%d path = %x:%x\n", __func__, __LINE__, (int)(qid.path>>32), (int)qid.path);
				int l = file_pathname_from_path(qid.path, pth, 255);

printf("%s: %d. pth = %s, dri = %d, *n = %d\n\n", __func__, __LINE__, pth, dri, *n);
				fp = fopen(pth,"w+");

				free(pth);
				
				if (!fp) {
					*n = 0;
					break;
				}

				i = 0; j = 0;
				while( j < *n ) {
					if(i >= dri){
						c = buf[ j ];
						fputc(c, fp);
printf("%c", c);
						j++;
					}else{
						c = fgetc(fp);
					}
					i++;
				}
				fclose(fp);
printf("\n%s: %d. i = %d, j = %d\n\n", __func__, __LINE__, i, j);
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

//printf("%s: %d. qid.my_type = %d, my_name = %s\n", __func__, __LINE__, qid.my_type, qid.my_name);

	switch(qid.my_type){
		case FS_FILE_DIR:
			{
				memset(d, 0, sizeof(Dir) );
				
				//d->type = 'X';
				//d->dev = 'x';
				
				//d->qid.path = qid.path;
				//d->qid.type = 0;
				//d->qid.vers = 0;
				
				//d->qid.my_type = 0;

				d->qid = qid;
				
				d->mode = 0; //mode;
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

