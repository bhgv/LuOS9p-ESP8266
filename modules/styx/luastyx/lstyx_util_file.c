
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




char*
ls_dir_rd_out(const char *path, Qid qid, char *buf, ulong *n, vlong *off){
	Dir d;
	int m = 0;
	int dsz = 0;

	int dri = *off;

	int i;
	
	DIR *dir = NULL;
	struct dirent *ent;
	
	
	memset( &d, 0, sizeof(Dir) );

//	d.uid = eve;
//	d.gid = eve;
	
	// Open directory
	dir = opendir(path);
	if (!dir) {
		*n = 0;
		return nil;
	}
	
	// Read entries
	i = 0; m = 0;
	while ((ent = readdir(dir)) != NULL) {
		//d.qid.path = (1<<17) + i;
		
		d.length = 0;
		d.qid.my_type = FS_FILE_DIR;
		
		d.qid.type = QTDIR;
		d.mode = DMDIR;

		//type = 'd';
		if (ent->d_type == DT_REG) {
			d.qid.my_type = FS_FILE_FILE;
			//type = 'f';
			d.length = ent->d_fsize;

			d.qid.type = QTFILE;
 			d.mode = DMREAD | DMWRITE | DMEXEC;

//			sprintf(size,"%8d", ent->d_fsize);
		}

		d.name = ent->d_name;

		if(i >= dri){
			dsz = convD2M(&d, (uchar*)buf, *n-m);
			if(dsz <= BIT16SZ)
					break;
			m += dsz;
			buf += dsz;
		}
	
		i++;
	}
	closedir(dir);
	
	*n = m;
	*off = i;

	return nil;
}


/*
Path
make_file_path(Path new_type, Path oldp, int idx){
	Path newp = 0ULL;
//DBG("\n%s:%d oldp = %x:%x, i = %d\n", __func__, __LINE__, (int)(oldp>>32), (int)oldp, idx);
	Path lvl = PATH_LVL_GET(oldp);
//DBG("%s:%d lvl = %x:%x\n", __func__, __LINE__, (int)(lvl>>32), (int)lvl);

	Path mainpt = oldp & PATH_MAIN_PT_MASK;
//DBG("%s:%d mainpt = %x:%x\n", __func__, __LINE__, (int)(mainpt>>32), (int)mainpt);

	Path curpt = idx & PATH_STEP_MASK;
	curpt <<= lvl * PATH_MAIN_PT_STEP;
//DBG("%s:%d curpt = %x:%x\n", __func__, __LINE__, (int)(curpt>>32), (int)curpt);

	mainpt &= ~( PATH_STEP_MASK << (lvl * PATH_MAIN_PT_STEP) );
//DBG("%s:%d mainpt = %x:%x\n", __func__, __LINE__, (int)(mainpt>>32), (int)mainpt);
	mainpt |= curpt;
//DBG("%s:%d mainpt = %x:%x\n", __func__, __LINE__, (int)(mainpt>>32), (int)mainpt);

	lvl++;
//DBG("%s:%d lvl = %x:%x\n", __func__, __LINE__, (int)(lvl>>32), (int)lvl);

	PATH_LVL_SET(newp, lvl);
//DBG("%s:%d newp = %x:%x\n", __func__, __LINE__, (int)(newp>>32), (int)newp);
	newp |= mainpt;
//DBG("%s:%d newp = %x:%x\n", __func__, __LINE__, (int)(newp>>32), (int)newp);
//DBG("%s:%d new_type = %x:%x\n", __func__, __LINE__, (int)(new_type>>32), (int)new_type);
	PATH_TYPE_COPY(newp, new_type);
//DBG("%s:%d newp = %x:%x\n\n", __func__, __LINE__, (int)(newp>>32), (int)newp);
	
	return newp;
}
*/

char*
scan_fs_dir(Qid *qid, const char *path, char *nm){
	int i;
	char *r = "No exists";
	
	DIR *dir = NULL;
	struct dirent *ent;
	
	// Open directory
	dir = opendir(path);
	if (!dir) {
		return r;
	}
	
	// Read entries
	i = 0; 
DBG("\n%s:%d. old_path = %x:%x\n\n", __func__, __LINE__, (int)(qid->path >> 32), (int)qid->path );
	while ((ent = readdir(dir)) != NULL) {
		if( !strcmp(ent->d_name, nm) ){
//			qid->path = PATH_FILE + (PATH_STEP_MASK & i);
			qid->path = make_file_path( PATH_FILE, (Path)qid->path, i);
DBG("\n%s:%d new_path = %x:%x\n\n", __func__, __LINE__, (int)(qid->path >> 32), (int)qid->path );
			
			//d.length = 0;
			qid->my_type = FS_FILE_DIR;
			qid->type = QTDIR;
			//d.mode = DMDIR;

			//type = 'd';
			if (ent->d_type == DT_REG) {
				qid->my_type = FS_FILE_FILE;
				qid->type = QTFILE;
				//d.mode = DMDIR;

	//			sprintf(size,"%8d", ent->d_fsize);
			}

			qid->my_name = strdup(ent->d_name);

			r = nil;
			break;
		}
		i++;
	}
	closedir(dir);
	
	return r;
}


int
file_pathname_from_path(Path path, char* buf, int max_len){
	int i;
		
	DIR *dir = NULL;
	struct dirent *ent;

//DBG("%s:%d path = %x:%x\n", __func__, __LINE__, (int)(path>>32), (int)path);

	Path lvl = PATH_LVL_GET(path);
	Path mainpt = path & PATH_MAIN_PT_MASK;

//	char *pthnm = "/";
	int cur_len = 0;
	char *cur_nm;

	char *pbuf = buf;

for( ; lvl > 0; lvl--){
	int cur_n = (int)(mainpt & PATH_STEP_MASK);
	mainpt >>= PATH_MAIN_PT_STEP;

	cur_len++;
	if(cur_len > max_len) return 0;
	
	*pbuf = '/';
	pbuf++;
	*pbuf = '\0';

//DBG("file_pathname_from_path path = %s\n", buf );
	// Open directory
	dir = opendir( buf );
	if (!dir) {
		return 0;
	}
	
	// Read entries
	i = 0; 
//DBG("\nfile_pathname_from_path lvl = %d, cur_n = %d\n", lvl, cur_n );
	while ((ent = readdir(dir)) != NULL) {
		if( i == cur_n ){
			int l = ent->d_namlen;
			
			cur_nm = ent->d_name;
//DBG("file_pathname_from_path cur_nm = %s, nm_len = %d, %d\n", cur_nm, l, strlen(cur_nm) );
			if (ent->d_type == DT_REG) {
				if( lvl > 1){
//DBG("\nERROR! file_pathname_from_path. lvl = %d, but fn = %s is a file!\n\n", lvl, cur_nm);
					return 0;
				}
				//d.length = ent->d_fsize;
			}
			
			cur_len += l;
			if(cur_len > max_len){
				closedir(dir);
				return 0;
			}
			
			memcpy(pbuf, cur_nm, l);
			pbuf += l;
			*pbuf = '\0';
			
			break;
		}
		i++;
	}
	closedir(dir);
}

	return cur_len;
}


