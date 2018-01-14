
/*
 * Copyright bhgv 2017
 */

#ifndef _LSTYX_H
#define _LSTYX_H


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

	FS_RPC,

	FS_CGI,
};

enum {
	SC_BYNAME,
	SC_BYPOS,
};

enum {
	RD_ST_LIST,
	RD_ST_RES,
};

//enum {
#define 	PATH_MAIN_PT_STEP	7ULL

#define 	PATH_DEV			(Path)( 1ULL << 60 )
#define 	PATH_FILE			(Path)( 2ULL << 60 )
	
#define 	PATH_BODY_MASK		(Path)( (1ULL<<60) - 1ULL )
	
#define 	PATH_STEP_MASK		(Path)( ( 1ULL << PATH_MAIN_PT_STEP )  - 1ULL)

#define 	PATH_LVL_MASK		(Path)0xfULL

#define 	PATH_MAIN_PT_MASK	(Path)( ( 1ULL << (60-4) ) - 1ULL )

#define		PATH_TYPE_MASK		(Path)(0xfULL << 60)
//};
#define PATH_LVL_GET(p)			( ( (Path)p >> (60 - 4) ) & PATH_LVL_MASK)

#define PATH_LVL_SET(p, lvl)	p &= ~(PATH_LVL_MASK << (60 - 4));  \
							p |= ( ( (Path)lvl & PATH_LVL_MASK) << (60 - 4) )

#define PATH_TYPE_COPY(p, t)	p &= ~PATH_TYPE_MASK; \
							p |= ( (Path)t & PATH_TYPE_MASK)


extern Styxserver *server;

extern lua_State* intL;

extern char *eve;

extern char* styx_cgi_reg;



/*
 * An in-memory file server
 * allowing truncation, removal on closure, wstat and
 * all other file operations
 */

char *fsremove(Qid);


Path make_file_path(Path new_type, Path oldp, int idx);

char* ls_dir_rd_out(const char *path, Qid qid, char *buf, ulong *n, vlong *off);
char* scan_fs_dir(Qid *qid, const char *path, char *nm);
int file_pathname_from_path(Path path, char* buf, int max_len);

void* scan_devs(luaR_entry *hdr_entry, char* nm, int type);
char* dev_call_parse_next_par(char* buf, int *plen);


#endif

