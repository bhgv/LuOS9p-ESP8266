/**
 * ESP8266 SPIFFS HAL configuration.
 *
 * Part of esp-open-rtos
 * Copyright (c) 2016 sheinz https://github.com/sheinz
 * MIT License
 */
#include "esp_spiffs.h"
#include "spiffs.h"
//#include <spiflash.h>
#include <spi_flash.h>
#include <stdbool.h>
#include <esp/uart.h>
#include <fcntl.h>

extern spiffs fs;

typedef struct {
    void *buf;
    uint32_t size;
} fs_buf_t;

static spiffs_config config = {0};
static fs_buf_t work_buf = {0};
static fs_buf_t fds_buf = {0};
static fs_buf_t cache_buf = {0};

/**
 * Number of file descriptors opened at the same time
 */
#define ESP_SPIFFS_FD_NUMBER       5

#define ESP_SPIFFS_CACHE_PAGES     5

#if 0
//static 
s32_t esp_spiffs_read(u32_t addr, u32_t size, u8_t *dst)
{
    if (!sdk_spi_flash_read(addr, dst, size)) {
        return SPIFFS_ERR_INTERNAL;
    }

    return SPIFFS_OK;
}

//static 
s32_t esp_spiffs_write(u32_t addr, u32_t size, u8_t *src)
{
    if (!sdk_spi_flash_write(addr, src, size)) {
        return SPIFFS_ERR_INTERNAL;
    }

    return SPIFFS_OK;
}

//static 
s32_t esp_spiffs_erase(u32_t addr, u32_t size)
{
    uint32_t sectors = size / SPI_FLASH_SEC_SIZE; //SPI_FLASH_SECTOR_SIZE;

    for (uint32_t i = 0; i < sectors; i++) {
        if (!sdk_spi_flash_erase_sector(addr + (SPI_FLASH_SEC_SIZE * i))) {
            return SPIFFS_ERR_INTERNAL;
        }
    }

    return SPIFFS_OK;
}
#else
s32_t esp_spiffs_read(u32_t addr, u32_t size, u8_t *dst) {
    u32_t aaddr;
    u8_t *buff = NULL;
    u8_t *abuff = NULL;
    u32_t asize;

//printf("esp_spiffs_read enter adr=%x, sz=%d, dst=%x\n", addr, size, dst);	 

    asize = size;
    
    // Align address to 4 byte
    aaddr = (addr + 3) & (u32_t)(-4);
    if (aaddr != addr) {
		aaddr -= 4;
		asize += (addr - aaddr);
    }

    // Align size to 4 byte
    asize = (asize + 3) & (u32_t)(-4);
	if(asize < size) asize += 4+4;

    if ((aaddr != addr) || (asize != size)) {
		// Align buffer
		buff = malloc(asize + 8);
//printf("esp_spiffs_read size = %d, asize = %d, buf = %x\n", size, asize, buff);
		if (!buff) {
		    return SPIFFS_ERR_INTERNAL;
		}

		abuff = (u8_t *)(((ptrdiff_t)buff + 3) & (u32_t)(-4));

//usleep(10);
		if (sdk_spi_flash_read(aaddr, (void *)abuff, asize) != 0) {
		    free(buff);
		    return SPIFFS_ERR_INTERNAL;
		}

		memcpy(dst, abuff + (addr - aaddr), size);

		free(buff);
    } else {
//usleep(10);
//printf("esp_spiffs_read regular a=%x, b=%x, l=%d\n",  addr, (void *)dst, size);
		if (sdk_spi_flash_read(addr, (void *)dst, size) != 0) {
			printf("esp_spiffs_read regular - ERR_INTERNAL\n" );
		    return SPIFFS_ERR_INTERNAL;
		}
    }

//usleep(10);
//printf("esp_spiffs_read exit\n");    
    return SPIFFS_OK;
}

s32_t esp_spiffs_write(u32_t addr, u32_t size, u8_t *src) {
    u32_t aaddr;
    u8_t *buff = NULL;
    u8_t *abuff = NULL;
    u32_t asize;

    asize = size;
    
    // Align address to 4 byte
    aaddr = (addr + (4 - 1)) & -4;
    if (aaddr != addr) {
		aaddr -= 4;
		asize += (addr - aaddr);
    }

    // Align size to 4 byte
    asize = (asize + (4 - 1)) & -4; 

    if ((aaddr != addr) || (asize != size)) {
		// Align buffer
		buff = malloc(asize + 4);
//printf("esp_spiffs_write asize = %d, buf = %x\n", asize, buff);
		if (!buff) {
		    return SPIFFS_ERR_INTERNAL;
		}

		abuff = (u8_t *)(((ptrdiff_t)buff + (4 - 1)) & -4);

//usleep(1);
		if (sdk_spi_flash_read(aaddr, (void *)abuff, asize) != 0) {
		    free(buff);
		    return SPIFFS_ERR_INTERNAL;
		}

		memcpy(abuff + (addr - aaddr), src, size);

//usleep(10);
		if (sdk_spi_flash_write(aaddr, (uint32_t *)abuff, asize) != 0) {
		    free(buff);
		    return SPIFFS_ERR_INTERNAL;
		}

		free(buff);
    } else {
//usleep(10);
		if (sdk_spi_flash_write(addr, (uint32_t *)src, size) != 0) {
		    return SPIFFS_ERR_INTERNAL;
		}
    }
	//usleep(10);
    
    return SPIFFS_OK;
}

s32_t IRAM esp_spiffs_erase(u32_t addr, u32_t size) {
//usleep(1);
    if (sdk_spi_flash_erase_sector(addr >> 12) != 0) {
		return SPIFFS_ERR_INTERNAL;
    }
    
    return SPIFFS_OK;
}
#endif




#if SPIFFS_SINGLETON == 1
void esp_spiffs_init()
{
#else
void esp_spiffs_init(uint32_t addr, uint32_t size)
{
    config.phys_addr = addr;
    config.phys_size = size;

    config.phys_erase_block = SPIFFS_ESP_ERASE_SIZE;
    config.log_page_size = SPIFFS_LOG_PAGE_SIZE;
    config.log_block_size = SPIFFS_LOG_BLOCK_SIZE;

    // Initialize fs.cfg so the following helper functions work correctly
    memcpy(&fs.cfg, &config, sizeof(spiffs_config));
#endif
    work_buf.size = 2 * SPIFFS_LOG_PAGE_SIZE;
    fds_buf.size = SPIFFS_buffer_bytes_for_filedescs(&fs, ESP_SPIFFS_FD_NUMBER);
    cache_buf.size= SPIFFS_buffer_bytes_for_cache(&fs, ESP_SPIFFS_CACHE_PAGES);

    work_buf.buf = malloc(work_buf.size);
    fds_buf.buf = malloc(fds_buf.size);
    cache_buf.buf = malloc(cache_buf.size);

    config.hal_read_f = esp_spiffs_read;
    config.hal_write_f = esp_spiffs_write;
    config.hal_erase_f = esp_spiffs_erase;

    config.fh_ix_offset = 3;

}

void esp_spiffs_deinit()
{
    free(work_buf.buf);
    work_buf.buf = 0;

    free(fds_buf.buf);
    fds_buf.buf = 0;

    free(cache_buf.buf);
    cache_buf.buf = 0;
}

int32_t esp_spiffs_mount()
{
    printf("SPIFFS memory, work_buf_size=%d, fds_buf_size=%d, cache_buf_size=%d\n",
            work_buf.size, fds_buf.size, cache_buf.size);

    int32_t err = SPIFFS_mount(&fs, &config, (uint8_t*)work_buf.buf,
            (uint8_t*)fds_buf.buf, fds_buf.size,
            cache_buf.buf, cache_buf.size, 0);

    if (err != SPIFFS_OK) {
        printf("Error spiffs mount: %d\n", err);
    }

    return err;
}

#if 0
// This implementation replaces implementation in core/newlib_syscals.c
//long _write_filesystem_r(struct _reent *r, int fd, const char *ptr, int len )
long _write_r(struct _reent *r, int fd, const char *ptr, int len )
{
    return SPIFFS_write(&fs, (spiffs_file)fd, (char*)ptr, len);
}

// This implementation replaces implementation in core/newlib_syscals.c
//long _read_filesystem_r( struct _reent *r, int fd, char *ptr, int len )
long _read_r( struct _reent *r, int fd, char *ptr, int len )
{
    return SPIFFS_read(&fs, (spiffs_file)fd, ptr, len);
}

int _open_r(struct _reent *r, const char *pathname, int flags, int mode)
{
    uint32_t spiffs_flags = SPIFFS_RDONLY;

    if (flags & O_CREAT)    spiffs_flags |= SPIFFS_CREAT;
    if (flags & O_APPEND)   spiffs_flags |= SPIFFS_APPEND;
    if (flags & O_TRUNC)    spiffs_flags |= SPIFFS_TRUNC;
    if (flags & O_RDONLY)   spiffs_flags |= SPIFFS_RDONLY;
    if (flags & O_WRONLY)   spiffs_flags |= SPIFFS_WRONLY;
    if (flags & O_EXCL)     spiffs_flags |= SPIFFS_EXCL;
    /* if (flags & O_DIRECT)   spiffs_flags |= SPIFFS_DIRECT; no support in newlib */

    return SPIFFS_open(&fs, pathname, spiffs_flags, mode);
}

// This implementation replaces implementation in core/newlib_syscals.c
//int _close_filesystem_r(struct _reent *r, int fd)
int _close_r(struct _reent *r, int fd)
{
    return SPIFFS_close(&fs, (spiffs_file)fd);
}

int _unlink_r(struct _reent *r, const char *path)
{
    return SPIFFS_remove(&fs, path);
}

int _fstat_r(struct _reent *r, int fd, void *buf)
{
    spiffs_stat s;
    struct stat *sb = (struct stat*)buf;

    int result = SPIFFS_fstat(&fs, (spiffs_file)fd, &s);
    sb->st_size = s.size;

    return result;
}

int _stat_r(struct _reent *r, const char *pathname, void *buf)
{
    spiffs_stat s;
    struct stat *sb = (struct stat*)buf;

    int result = SPIFFS_stat(&fs, pathname, &s);
    sb->st_size = s.size;

    return result;
}

off_t _lseek_r(struct _reent *r, int fd, off_t offset, int whence)
{
    return SPIFFS_lseek(&fs, (spiffs_file)fd, offset, whence);
}
#endif
