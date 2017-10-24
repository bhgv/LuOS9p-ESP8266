/*
 * Lua RTOS, NET vfs operations
 *
 * Copyright (C) 2017
 *
 * Author: bhgv (http://github.com/bhgv)
 *
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "whitecat.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/filedesc.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <pthread.h>


#if (USE_ETHERNET || USE_GPRS || USE_WIFI)

#include "FreeRTOS.h"
#include "lwip/sockets.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>

int net_open(struct file *fp, int flags);
ssize_t net_write(struct file *fp, struct uio *uio, struct ucred *cred);
ssize_t net_read(struct file *fp, struct uio *uio, struct ucred *cred);
int net_close(struct file *fp);

int net_open(struct file *fp, int flags) {
    char *path = fp->f_path;
 	int *s;
	int b[4];
	int len=strlen(path);
	int i, j=0;
	char c, oc=0;
	//int i, j=0;
	
 	s=malloc(sizeof(int));
	*s=0;

	for(i=0; i<len; i++){
		c=path[i];
		if(c == '/'){
			j=0;
			b[0]=0; b[1]=0; b[2]=0; b[3]=0;
		}else if(j>=0 && j<4 && c>='0' && c<='9'){
			b[j] *=10;
			b[j] += c-'0';
		}else if(c=='.'){
			j++;
		}else{
			j = -1;
		}
	}
	if(j>=0){
		for(i=0; i<=j && i<4; i++){
			*s <<= 8;
			*s += b[i];
		}
	}
	// Get socket number
	//sscanf(path,"/%d", &s1); //, &s2, &s3, &s4);
	//*s =s1; //(s1<<24) + (s2<<16) + (s3<<8) + s4;

	//printf("net_open %s  -> %d\n", path /*, s2, s3, s4*/, *s);
	
	fp->f_fs = s;
    return 0; //s;
}

ssize_t net_write(struct file *fp, struct uio *uio, struct ucred *cred) {
		 int res;
		 char *buf = uio->uio_iov->iov_base;
		 unsigned int size = uio->uio_iov->iov_len;
		
//		res = SPIFFS_read(&fs, *(spiffs_file *)fp->f_fs, buf, size); 
	
		res = lwip_send(*(int*)fp->f_fs, buf, size, 0);
		if (res >= 0) {
			uio->uio_resid = uio->uio_resid - res;
			res = 0;
	//	} else {
	//		res = spiffs_result(fs.err_code);
		}
	
		return res;
//	return (ssize_t)lwip_send(fd, data, size, 0);
}

ssize_t net_read(struct file *fp, struct uio *uio, struct ucred *cred) {
	 int res;
	 char *buf = uio->uio_iov->iov_base;
	 unsigned int size = uio->uio_iov->iov_len;
	
//	res = SPIFFS_read(&fs, *(spiffs_file *)fp->f_fs, buf, size); 

	res = lwip_recv(*(int*)fp->f_fs, buf, size, 0);
	if (res >= 0) {
		uio->uio_resid = uio->uio_resid - res;
		res = 0;
//	} else {
//		res = spiffs_result(fs.err_code);
	}

    return res;
}

int net_close(struct file *fp) {
	closesocket(*(int*)fp->f_fs);
	free(fp->f_fs);
	fp->f_fs=NULL;
	return 0;
}

/*
#if LWIP_COMPAT_SOCKETS
#define accept(a,b,c)         lwip_accept(a,b,c)
#define bind(a,b,c)           lwip_bind(a,b,c)
#define shutdown(a,b)         lwip_shutdown(a,b)
#define closesocket(s)        lwip_close(s)
#define connect(a,b,c)        lwip_connect(a,b,c)
#define getsockname(a,b,c)    lwip_getsockname(a,b,c)
#define getpeername(a,b,c)    lwip_getpeername(a,b,c)
#define setsockopt(a,b,c,d,e) lwip_setsockopt(a,b,c,d,e)
#define getsockopt(a,b,c,d,e) lwip_getsockopt(a,b,c,d,e)
#define listen(a,b)           lwip_listen(a,b)
#define recv(a,b,c,d)         lwip_recv(a,b,c,d)
#define recvfrom(a,b,c,d,e,f) lwip_recvfrom(a,b,c,d,e,f)
#define send(a,b,c,d)         lwip_send(a,b,c,d)
#define sendto(a,b,c,d,e,f)   lwip_sendto(a,b,c,d,e,f)
#define socket(a,b,c)         lwip_socket(a,b,c)
#define select(a,b,c,d,e)     lwip_select(a,b,c,d,e)
#define ioctlsocket(a,b,c)    lwip_ioctl(a,b,c)

*/

#endif
