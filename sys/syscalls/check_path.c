#include <FreeRTOS.h>

#include <string.h>
#include <stdio.h>
#include <limits.h>

#include <sys/stat.h>

#include "esp_vfs.h"
#include "esp_attr.h"
#include <errno.h>

#include <spiffs.h>
#include <esp_spiffs.h>
#include <spiffs_nucleus.h>
#include <sys/syslog.h>
#include <sys/mount.h>
#include <sys/list/list.h>
#include <sys/fcntl.h>
#include <dirent.h>

#undef IRAM_ATTR
#define IRAM_ATTR 


#if !defined(max)
#define max(A,B) ( (A) > (B) ? (A):(B))
#endif

#if !defined(min)
#define min(A,B) ( (A) < (B) ? (A):(B))
#endif

//#define VFS_SPIFFS_FLAGS_READONLY  (1 << 0)



//static spiffs fs;
//static struct list files;

//static u8_t *my_spiffs_work_buf;
//static u8_t *my_spiffs_fds;
//static u8_t *my_spiffs_cache;

void dir_path(char *npath, uint8_t base) {
	int len = strlen(npath);

	if (base) {
		char *c;

		c = &npath[len - 1];
		while (c >= npath) {
			if (*c == '/') {
				break;
			}

			len--;
			c--;
		}
	}

    if (len > 1) {
    	if (npath[len - 1] == '.') {
    		npath[len - 1] = '\0';

        	if (npath[len - 2] == '/') {
        		npath[len - 2] = '\0';
        	}
    	} else {
        	if (npath[len - 1] == '/') {
        		npath[len - 1] = '\0';
        	}
    	}
    } else {
    	if ((npath[len - 1] == '/') ||  (npath[len - 1] == '.')) {
    		npath[len - 1] = '\0';
    	}
    }

    strlcat(npath,"/.", PATH_MAX);
}

#if 0
void check_path(const char *path, uint8_t *base_is_dir, uint8_t *full_is_dir, uint8_t *is_file, int *files) {
//    char bpath[PATH_MAX + 1]; // Base path
//    char fpath[PATH_MAX + 1]; // Full path
//    struct spiffs_dirent e;
//    spiffs_DIR d;
//    int file_num = 0;

    struct file *fp;
    char *npath;
    int res;
    int fd;
    
    struct dirent ent;

    *files = 0;
    *base_is_dir = 0;
    *full_is_dir = 0;
    *is_file = 0;

    // Get base directory name
//    strlcpy(bpath, path, PATH_MAX);
//    dir_path(bpath, 1);

    // Get full directory name
//    strlcpy(fpath, path, PATH_MAX);
//    dir_path(fpath, 0);


    if ((fd = open("/", O_WRONLY)) == -1)
        return -1;
    
    if (!(fp = getfile(fd))) {
        close(fd);
        return -1;
    }

    res = (*fp->f_ops->fo_opendir)(fp);
  
//    SPIFFS_opendir(&fs, "/", &d);
	while ((*fp->f_ops->fo_readdir)(fp, &ent)) {
//	while (SPIFFS_readdir(&d, &e)) {
		if (!strcmp(bpath, (const char *)e.name)) {
			*base_is_dir = 1;
		}

		if (!strcmp(fpath, (const char *)e.name)) {
			*full_is_dir = 1;
		}

		if (!strcmp(path, (const char *)e.name)) {
			*is_file = 1;
		}

		if (!strncmp(fpath, (const char *)e.name, min(strlen((char *)e.name), strlen(fpath) - 1))) {
			if (strcmp(fpath, (const char *)e.name)) {
				file_num++;
			}
		}
	}
	SPIFFS_closedir(&d);
	
    close(fd);

	*files = file_num;
}
#endif

/*
 * This function translate error codes from SPIFFS to errno error codes
 *
 */
/*
static int spiffs_result(int res) {
    switch (res) {
        case SPIFFS_OK:
        case SPIFFS_ERR_END_OF_OBJECT:
            return 0;

        case SPIFFS_ERR_NOT_FOUND:
        case SPIFFS_ERR_CONFLICTING_NAME:
            return ENOENT;

        case SPIFFS_ERR_NOT_WRITABLE:
        case SPIFFS_ERR_NOT_READABLE:
            return EACCES;

        case SPIFFS_ERR_FILE_EXISTS:
            return EEXIST;

        default:
            return res;
    }
}
*/
