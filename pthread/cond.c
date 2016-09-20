/*
 * Whitecat, pthread implementation ober FreeRTOS
 *
 * Copyright (C) 2015 - 2016
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 * 
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
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

#include "pthread.h"
#include "time.h"

#include <errno.h>
#include <sys/mutex.h>

extern struct mtx cond_mtx;
 
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
    // Init conf, if not
    mtx_lock(&cond_mtx);
    if (cond->mutex.sem == NULL) {
        mtx_init(&cond->mutex, NULL, NULL, 0);
    }
    
    mtx_unlock(&cond_mtx);  
    
    return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond) {
    // Destroy, if config
    mtx_lock(&cond_mtx);
    if (cond->mutex.sem != NULL) {
        mtx_destroy(&cond->mutex);
    }
    
    mtx_unlock(&cond_mtx); 
	
	return 0; 
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {    
    // Init condition, if not
    pthread_cond_init(cond, NULL);
    
    // Wait for condition
    mtx_lock(&cond->mutex);
    pthread_mutex_unlock(mutex);
    
    return 0;
}
  
int pthread_cond_timedwait(pthread_cond_t *cond, 
    pthread_mutex_t *mutex, const struct timespec *abstime) { 
    
    // Init condition, if not
    pthread_cond_init(cond, NULL);
    
    // Wait for condition
    if (xSemaphoreTake(cond->mutex.sem, portTICK_PERIOD_MS * 1000 * abstime->tv_sec) != pdTRUE) {
        errno = ETIMEDOUT;
        return ETIMEDOUT;
    }
    
    mtx_lock(&cond->mutex);
    pthread_mutex_unlock(mutex);
    
    return 0;
}

int pthread_cond_signal(pthread_cond_t *cond) {
    // Init condition, if not
    pthread_cond_init(cond, NULL);

    return 0;
}
