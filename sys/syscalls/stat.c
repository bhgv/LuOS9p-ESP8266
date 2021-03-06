/*-
 * Copyright (c) 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  @(#)stat.c  8.1 (Berkeley) 6/11/93
 */

#include "syscalls.h"

extern struct filedesc *p_fd;

int fstat(int fd, struct stat *sb) {
    register struct file *fp;
    register struct filedesc *fdp = p_fd;
    int res=1;
    
    if(!fd_mtx_set){
        errno = EBADF;
        return -1;
    }

    mtx_lock(&fd_mtx);
    if (
	fdp == NULL ||
	(u_int)fd >= fdp->fd_nfiles ||
        (fp = fdp->fd_ofiles[fd]) == NULL
    ) {
        mtx_unlock(&fd_mtx);
        errno = EBADF;
        return -1;
    }
    mtx_unlock(&fd_mtx);
    
    res = (fp->f_ops->fo_stat)(fp, sb);
    if (res > 0) {
        errno = res;
        return -1;
    }
    
    return (0);
}

int stat(const char *str, struct stat *sb) {
    int fd, rv;

    fd = open(str, 0);
    if (fd < 0)
        return (-1);
    rv = fstat(fd, sb);
    (void)close(fd);
    return (rv);
}

int lstat(const char *str, struct stat *sb) {
    return stat(str, sb);
}
