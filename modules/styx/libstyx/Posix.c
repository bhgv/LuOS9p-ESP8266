#define	__EXTENSIONS__
#define	_BSD_COMPAT
#include <lib9.h>
#include 	<sys/types.h>
//#include	<sys/time.h>
//#include	<sys/socket.h>
//#include	<netinet/in.h>

#include	<lwip/sockets.h>


#include "espressif/esp_common.h"
//#include "espressif/phy_info.h"

#include "lwip/api.h"
//#include "ipv4/lwip/ip.h"
#include "lwip/tcp.h"


#include "styxserver.h"
#include "styxaux.h"


#define NC_MAX 5

typedef struct Fdset Fdset;

struct Fdset
{
	fd_set infds, outfds, excfds, r_infds, r_outfds, r_excfds;
};



//static struct netconn* nc_lsn = NULL;

static struct netconn* nc_pool[NC_MAX];
static struct netbuf* nb_pool[NC_MAX];
static int nc_pool_idx = 0;


int is_nc_pool_have_place(){
printf("%s: %d\n", __func__, __LINE__);
	int i;
	for(i=0; i<nc_pool_idx; i++){
		if(nc_pool[i] == NULL)
			return 1;
	}
	if(nc_pool_idx >= NC_MAX)
		return 0;
	else
		return 1;
}

int add_to_nc_pool(struct netconn *nc){
printf("%s: %d\n", __func__, __LINE__);
	int i;
	if(nc_pool_idx >= NC_MAX)
		return -1;
	for(i=0; i<nc_pool_idx; i++){
		if(nc_pool[i] == NULL){
			nc_pool[i] = nc;
			if(nb_pool[i] != NULL){
				netbuf_delete( nb_pool[i] );
			}
			nb_pool[i] = NULL;
			return i;
		}
	}
	i = nc_pool_idx;
	nc_pool_idx++;
	nc_pool[i] = nc;
	return i;
}

struct netconn * get_from_nc_pool(int skt){
printf("%s: %d\n", __func__, __LINE__);
	//skt--;
	if(skt >= nc_pool_idx) return NULL;
	
	return nc_pool[skt];
}

struct netbuf * get_from_nb_pool(int skt){
printf("%s: %d\n", __func__, __LINE__);
	//skt--;
	if(skt >= nc_pool_idx) return NULL;
	
	return nb_pool[skt];
}

void set_to_nb_pool(int skt, struct netbuf *nb){
	printf("%s: %d\n", __func__, __LINE__);
	//skt--;
	if(skt >= nc_pool_idx) return;
	
	nb_pool[skt] = nb;
}

void del_from_nc_pool(int skt){
	printf("%s: %d\n", __func__, __LINE__);
	//skt--;
	if(skt >= nc_pool_idx) return;
	
	nc_pool[skt] = NULL;
	if(nb_pool[skt] != NULL){
		netbuf_delete( nb_pool[skt] );
		nb_pool[skt] = NULL;
	}
	
	if(skt == nc_pool_idx-1)
		nc_pool_idx--;
}




int
styxinitsocket(void)
{
	printf("%s: %d\n", __func__, __LINE__);
	int i;
	for(i=0; i<NC_MAX; i++){
		nc_pool[i] = NULL;
		nb_pool[i] = NULL;
	}
	nc_pool_idx = 0;
	
	return 0;
}

void
styxendsocket(void)
{
	printf("%s: %d\n", __func__, __LINE__);
}

void
styxclosesocket(int fd)
{
	printf("%s: %d\n", __func__, __LINE__);
	/*
	struct netconn* nc = get_from_nc_pool(fd);

	if(nc != NULL){
		netconn_close(nc);
		netconn_delete(nc);
	}
	del_from_nc_pool(fd);
	*/
	lwip_close(fd);
	
}

int
styxannounce(Styxserver *server, char *port)
{
	printf("%s: %d\n", __func__, __LINE__);
	struct sockaddr_in sin;
	int s, one;

	USED(server);
/*
	struct netconn *nc = NULL;

	if(!is_nc_pool_have_place() ) return -1;
	
	nc = netconn_new(NETCONN_TCP);
	netconn_set_nonblocking(nc, TRUE);	   // Don't block!
	s = add_to_nc_pool(nc);
	//s = lwip_socket(AF_INET, SOCK_STREAM, 0);
	if(s < 0)
		return s;
	one = 1;

	nc->send_timeout = 10000;
	nc->recv_timeout = 3000;
	nc->recv_bufsize = MSGMAX;
	ip_set_option(nc->pcb.tcp, SOF_REUSEADDR);
	*/
	
	s = lwip_socket(AF_INET, SOCK_STREAM, 0);
	if(s < 0)
		return s;
	one = 1;

	if(lwip_setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one)) < 0)
		printf("setsockopt failed\n");
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	sin.sin_port = lwip_htons(atoi(port));
	if(lwip_bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
/*
	if( netconn_bind(nc, IP_ADDR_ANY, 999) != ERR_OK ){
		netconn_close(nc);
		netconn_delete(nc);
	
		del_from_nc_pool(s);
*/
		lwip_close(s);
		return -1;
	}
	if(lwip_listen(s, /*20*/ 4) < 0){
/*
	if(netconn_listen(nc) != ERR_OK){
		netconn_close(nc);
		netconn_delete(nc);
	
		del_from_nc_pool(s);
*/
		lwip_close(s);
		return -1;
	}
	return s;
}

int
styxaccept(Styxserver *server)
{
	printf("%s: %d\n", __func__, __LINE__);
	struct sockaddr_in sin;
	int s;
	socklen_t len;

//	struct netconn* nc, *clnt = NULL;

	len = sizeof(sin);
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	s = lwip_accept(server->connfd, (struct sockaddr *)&sin, &len);
	if(s < 0){
		if(errno != EINTR)
			printf("error in accept: %s\n", strerror(errno));
	}
	/*
	if(!is_nc_pool_have_place()) return -1;

	nc = get_from_nc_pool(server->connfd);
	if(nc == NULL || netconn_accept(nc, &clnt) != ERR_OK) return -1;
	netconn_set_nonblocking(clnt, TRUE);	   // Don't block!
	s = add_to_nc_pool(clnt);

	if(s < 0 || clnt == NULL){
		if(clnt){
			netconn_close(clnt);
			netconn_delete(clnt);
		}
		if(errno != EINTR)
			printf("error in accept: %s\n", strerror(errno));
		return -1;
	}

	clnt->send_timeout = 10000;
	clnt->recv_timeout = 3000;
	clnt->recv_bufsize = MSGMAX;
*/

	return s;
}

void
styxinitwait(Styxserver *server)
{
	printf("%s: %d\n", __func__, __LINE__);
	Fdset *fs;

	server->priv = fs = malloc(sizeof(Fdset));
	FD_ZERO(&fs->infds);
	FD_ZERO(&fs->outfds);
	FD_ZERO(&fs->excfds);
	FD_SET(server->connfd, &fs->infds);
}

int
styxnewcall(Styxserver *server)
{
//	printf("%s: %d\n", __func__, __LINE__);
	Fdset *fs;

	fs = server->priv;
	return FD_ISSET(server->connfd, &fs->r_infds);
}

void
styxnewclient(Styxserver *server, int s)
{
	printf("%s: %d\n", __func__, __LINE__);
	Fdset *fs;

	fs = server->priv;
	FD_SET(s, &fs->infds);
}

void
styxfreeclient(Styxserver *server, int s)
{
	printf("%s: %d\n", __func__, __LINE__);
	Fdset *fs;

	fs = server->priv;
	FD_CLR(s, &fs->infds);
}

int
styxnewmsg(Styxserver *server, int s)
{
//	printf("%s: %d\n", __func__, __LINE__);
	Fdset *fs;

	fs = server->priv;
	
//	struct netbuf* nb = get_from_nb_pool(s);

	return /*(nb != NULL); */ FD_ISSET(s, &fs->r_infds) || FD_ISSET(s, &fs->r_excfds);
}

char*
styxwaitmsg(Styxserver *server)
{
//	printf("%s: %d\n", __func__, __LINE__);
	struct timeval seltime;
	int nfds;
	Fdset *fs;

	fs = server->priv;
	fs->r_infds = fs->infds;
	fs->r_outfds = fs->outfds;
	fs->r_excfds = fs->excfds;
	seltime.tv_sec = 10;
	seltime.tv_usec = 0L;
	nfds = lwip_select(sizeof(fd_set)*8, &fs->r_infds, &fs->r_outfds, &fs->r_excfds, &seltime);
	if(nfds < 0 && errno != EINTR)
		return"error in select";

/**
	struct netconn* nc = NULL;
	struct netbuf* nb =NULL;

	int i;
	for(i = 0; i < nc_pool_idx; i++){
		if(i != server->connfd){
			nc = get_from_nc_pool(i);
			nb = get_from_nb_pool(i);
			if(nb != NULL){
				netbuf_delete(nb);
				nb = NULL;
			}
			if(nc != NULL){
				nc->send_timeout = 1000;
				nc->recv_timeout = 100;
				nc->recv_bufsize = MSGMAX;

				netconn_recv(nc, &nb);

				if(nb != NULL)
					printf("%s: %d fd = %d\n", __func__, __LINE__, i);
			}
			set_to_nb_pool(i, nb);
		}
	}
/**/

	return nil;
}

int
styxrecv(Styxserver *server, int fd, char *buf, int n, int m)
{
	printf("%s: %d\n", __func__, __LINE__);
/*
	//struct netconn* nc = NULL;
	struct netbuf* nb =NULL;

	//nc = get_from_nc_pool(fd);
	nb = get_from_nb_pool(fd);
	if(nb != NULL){
		char *str;
		u16_t len;
		int l;
		
		netbuf_data(nb, &str, &len);
		l = (int)len < n ? len : n;
		memcpy(buf, str, l);
		
		netbuf_delete(nb);
		set_to_nb_pool(fd, NULL);

		return l;
	}else{
		buf[0] = '\0';
		return 0;
	}
*/
	return lwip_recv(fd, buf, n, m);
}

int
styxsend(Styxserver *server, int fd, char *buf, int n, int m)
{
	printf("%s: %d fd=%d, buf=%x, l=%d\n", __func__, __LINE__, fd, buf, n);

//	if(buf == NULL || n == 0) return 0;
/*
	struct netconn* nc = get_from_nc_pool(fd);

	if(nc != NULL && netconn_write(nc, (const void*)buf, n, NETCONN_NOCOPY) == ERR_OK){
		return n;
	}else{
		return -1;
	}
*/
	int r = lwip_send(fd, buf, n, m);
	printf("%s: %d fd = %d, ret = %d\n", __func__, __LINE__, fd, r);
	return r; //lwip_send(fd, buf, n, m);
}

void
styxexit(int n)
{
	printf("%s: %d\n", __func__, __LINE__);
	//exit(n);
	printf("exit %d\n", n);
	while(1);
}

ulong styxtime(int ulong){
	printf("%s: %d\n", __func__, __LINE__);
	return sdk_system_get_time();
}
