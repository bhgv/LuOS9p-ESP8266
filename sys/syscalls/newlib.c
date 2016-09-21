#include "FreeRTOS.h"
#include "task.h"

#include "lua.h"
#include <stdio.h>
#include <sys/reent.h>
#include <sys/types.h>
#include <sys/times.h>

#include <sys/drivers/clock.h>
#include <sys/drivers/uart.h>

extern lua_State *gLuaState;
extern int _syscalls_inited;
extern int __rename(const char *old_filename, const char *new_filename);
extern pid_t getpid(void);
extern void luaC_fullgc (lua_State *L, int isemergency);

extern int read(int fd, void *buf, size_t nbyte);
extern int write(int fd, const void *buf, size_t nbyte);

void vPortFree(void *pv);
void *pvPortMalloc(size_t xWantedSize);
void *pvPortRealloc(void *pv, size_t size);
long long ticks();
 
clock_t _clock_(void) {
	return (clock_t)ticks();
}

int __remove_r(struct _reent *r, const char *_path) {
	return (unlink(_path));
}

int _rename_r(struct _reent *r, const char *_old, const char *_new) {
	return __rename(_old, _new);
}

pid_t _getpid_r(struct _reent *r) {
	return getpid();
}

int _kill_r(struct _reent *r, int pid, int sig) {
	return 0;
}

clock_t _times_r(struct _reent *r, struct tms *ptms) {
	return ticks();
}

#if (!USE_CUSTOM_HEAP)
// WRAP for malloc
//
// If memory cannot be allocated, launch Lua garbage collector and
// try again with the hope that then more memory will be available
void* __real_malloc(size_t bytes);
void* __wrap_malloc(size_t bytes) {
	void *ptr = __real_malloc(bytes);
	
	if (!ptr) {
        if (gLuaState) {
           luaC_fullgc(gLuaState, 1);
           ptr = __real_malloc(bytes);
        }		
	}
	
	return ptr;
}

void* __real_calloc(size_t bytes);
void* __wrap_calloc(size_t bytes) {
	void *ptr = __real_calloc(bytes);
	
	if (!ptr) {
        if (gLuaState) {
           luaC_fullgc(gLuaState, 1);
           ptr = __real_calloc(bytes);
        }		
	}
	
	return ptr;
}

void* __real_realloc(void *ptr, size_t bytes);
void* __wrap_realloc(void *ptr, size_t bytes) {
	void *nptr = __real_realloc(ptr, bytes);
	
	if (!nptr) {
        if (gLuaState) {
           luaC_fullgc(gLuaState, 1);
           nptr = __real_realloc(ptr, bytes);
        }		
	}
	
	return nptr;
}
#else
void __free(void *cp) {
    vPortFree(cp);
}

void* __malloc(size_t bytes) {
    void *p;

    p = (void *)pvPortMalloc(bytes);
    if (!p) {
        if (gLuaState) {
           luaC_fullgc(gLuaState, 1);
           p = pvPortMalloc(bytes);
        }
    }        
    
    return p;
}

void *__realloc(void *ptr, size_t size) {
    void *p;
    
    // If ptr is NULL, then the call is equivalent to malloc(size), for all 
    // values of size
    if (!ptr) {
        return __malloc(size);
    }
    
    // If size is equal to zero, and ptr is not NULL, then the call is 
    // equivalent to free(ptr)
    if ((size == 0) && (ptr)) {
        __free(ptr);
        return NULL;
    }
    
    // Realloc
    p = (void *)pvPortRealloc(ptr, size);
    if (!p) {
        if (gLuaState) {
           luaC_fullgc(gLuaState, 1);
           p = (void *)pvPortRealloc(ptr, size);
        }
    }
    
    return p;
}

void __free_r(struct _reent *r, void *cp) {
    __free(cp);
}

void* __malloc_r(struct _reent *r, size_t bytes) {
	return __malloc(bytes);
}

void *__realloc_r(struct _reent *r, void *ptr, size_t size) {
	return __realloc(ptr, size);
}

void *__calloc(size_t items, size_t size) {
   void *result;

   result = (void *) __malloc(items * size);
   if (!result)
       return NULL;

   bzero(result, items * size);

   return result;
}

void *__calloc_r(struct _reent *r, size_t items, size_t size) {
	return __calloc(items, size);
}
#endif

int _write_r(struct _reent *r, int fd, const void *buf, size_t nbyte) {
	if (_syscalls_inited) {
		return write(fd, buf, nbyte);		
	}
	
	return nbyte;
}

int _read_r(struct _reent *r, int fd, void *buf, size_t nbyte) {
	if (_syscalls_inited) {
		return read(fd, buf, nbyte);		
	}
	
	return nbyte;
}

unsigned sleep(unsigned int secs) {
    vTaskDelay( (secs * 1000) / portTICK_PERIOD_MS );
    
    return 0;
}

int usleep(useconds_t usec) {
	vTaskDelay(usec / portTICK_PERIOD_US);
	
	return 0;
}

