

#ifndef _HTTPD_H
#define _HTTPD_H

#include <spiffs.h>
#include <esp_spiffs.h>




#define IN_BUF_LEN  548 //400 //512
#define OUT_BUF_LEN  548 //312 //256 //312  //536 //312
//512

#undef MAXPATHLEN
#define MAXPATHLEN  55 //32 //55 
//PATH_MAX



#define DEF_RECV_TIMEOUT			50
#define DEF_SEND_TIMEOUT			30000

#define NC_MAX						3

#define WS_MAX						5

#define WS_TIMEOUT_NO_RECONNECT		10
#define WS_TIMEOUT_UNCONNECT		500



#ifndef ERR_IS_FATAL
#define ERR_IS_FATAL(e) (e < ERR_ISCONN)
#endif


typedef struct _get_par get_par;

struct _get_par {
	char *name;
	char *val;
	
	get_par* next;
};



typedef struct {
	const char* suf;
	const char* hdr;
	const char* siz;
} suf_hdr;

enum {
	NC_TYPE_GET,
	NC_TYPE_WS,
};

typedef struct {
	int type;
	
	struct netconn* clnt;
	char* uri;
	char* suf;

	get_par *get_root;
	
//	struct netbuf *nb;

	spiffs_file cur_f;

	// VVV lua cgi
	int cgi_lvl;
	char *pth;
	// AAA lua cgi

	int state;
} nc_node;


enum {
	NC_CLOSE = 0,
	NC_EMPTY = 0,
	NC_BEGIN,
	NC_PAS,
	NC_END,
};

typedef struct {
	struct netconn* clnt;
	char* uri;
	int age;
} ws_node;


char* strnstr(const char* buffer, const char* token, size_t n);
char* strstr(const char* buffer, const char* token);



void nc_free(struct netconn **nc, char* msg);
void nb_free();



int check_conn(char* fn, int ln);
err_t print_err(err_t err, char* fn, int ln);

void ws_socks_del();
void ws_sock_del(int i);
void ws_task(lua_State *L);
int do_websock(char **uri, int uri_len, char *hdr, char* hdr_sz, char* data, int len, nc_node *node );

int get_list_add(get_par** first, char* name, char* val);
void get_list_free(get_par** first);
int lget_params_cnt(lua_State* L);
int lget_param(lua_State* L);

char* suf_to_hdr(char* suf, char** siz);

int get_uri(char* in, int in_len, char** uri, char** suf, int* suf_len, get_par** ppget_root);
spiffs_file uri_to_file(char* uri, int len, int *flen);

int do_lua(char **uri, int uri_len, char *hdr, char* hdr_sz, lua_State* L, int nd_idx );
int do_file(char **uri, int uri_len, char *hdr, char* hdr_sz, int nd_idx );




extern int is_httpd_run;

extern get_par* get_root;

extern spiffs fs;

#endif

