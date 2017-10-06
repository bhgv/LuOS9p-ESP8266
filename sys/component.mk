INC_DIRS += $(ROOT)
CFLAGS += -DKERNEL

#LDFLAGS += -Wl,--wrap=_realloc_r
#LDFLAGS += -Wl,--wrap=_malloc_r
#LDFLAGS += -Wl,--wrap=_calloc_r
#LDFLAGS += -Wl,--wrap=_open_r
#LDFLAGS += -Wl,--wrap=_unlink_r
#LDFLAGS += -Wl,--wrap=_rename_r
#LDFLAGS += -Wl,--wrap=_stat_r
#LDFLAGS += -Wl,--wrap=mkdir
#LDFLAGS += -Wl,--wrap=opendir
#LDFLAGS += -Wl,--wrap=rmdir

#LDFLAGS += -Wl,--wrap=lwip_socket
#LDFLAGS += -Wl,--wrap=lwip_accept_r
#LDFLAGS += -Wl,--wrap=lwip_bind_r
#LDFLAGS += -Wl,--wrap=lwip_shutdown_r
#LDFLAGS += -Wl,--wrap=lwip_getpeername_r 
#LDFLAGS += -Wl,--wrap=lwip_getsockname_r
#LDFLAGS += -Wl,--wrap=lwip_getsockopt_r
#LDFLAGS += -Wl,--wrap=lwip_setsockopt_r
#LDFLAGS += -Wl,--wrap=lwip_connect_r
#LDFLAGS += -Wl,--wrap=lwip_listen_r
#LDFLAGS += -Wl,--wrap=lwip_recv_r
#LDFLAGS += -Wl,--wrap=lwip_read_r
#LDFLAGS += -Wl,--wrap=lwip_recvfrom_r
#LDFLAGS += -Wl,--wrap=lwip_send_r
#LDFLAGS += -Wl,--wrap=lwip_sendmsg_r
#LDFLAGS += -Wl,--wrap=lwip_sendto_r
#LDFLAGS += -Wl,--wrap=lwip_write_r
#LDFLAGS += -Wl,--wrap=lwip_writev_r
#LDFLAGS += -Wl,--wrap=lwip_select
#LDFLAGS += -Wl,--wrap=lwip_ioctl_r
#LDFLAGS += -Wl,--wrap=lwip_fcntl_r
#LDFLAGS += -Wl,--wrap=lwip_close_r

LDFLAGS += -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=free

sys_SRC_DIR = $(sys_ROOT) \
    $(sys_ROOT)platform/$(PLATFORM) \
    $(sys_ROOT)syscalls \
    $(sys_ROOT)list \
    $(sys_ROOT)drivers \
    $(sys_ROOT)drivers/platform/$(PLATFORM) \
    $(sys_ROOT)unix \
    $(sys_ROOT)spiffs \
    $(sys_ROOT)vfs \
    $(sys_ROOT)freertos \
    $(sys_ROOT)sys \

#    $(sys_ROOT)lwip \

sys_INC_DIR = $(ROOT)include $(sys_ROOT)spiffs 

$(eval $(call component_compile_rules,sys))
