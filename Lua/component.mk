INC_DIRS += $(Lua_ROOT)adds $(Lua_ROOT)common $(Lua_ROOT)modules $(Lua_ROOT)src $(Lua_ROOT)platform

Lua_INC_DIR = $(Lua_ROOT)adds $(Lua_ROOT)common $(Lua_ROOT)modules $(Lua_ROOT)src \
	$(ROOT)sys \
	$(ROOT)modules/ssd1306 \
	$(HOME)/esp-open-rtos/include/espressif \
	$(HOME)/esp-open-rtos/include \
	$(ROOT)modules/core/include \
	$(ROOT)include/platform/esp8266/espressif/esp8266 \
	$(ROOT)modules/lwip/lwip/espressif/include \
	$(ROOT)FreeRTOS/Source/include \
	$(ROOT)include \
	$(ROOT)config/lua \
	 $(Lua_ROOT)httpd \

#	$(HOME)/esp-open-rtos/extras \


#	$(HOME)/esp-open-rtos/core/include

CFLAGS += -DKERNEL -DLUA_USE_CTYPE -DLUA_32BITS -DLUA_USE_ROTABLE=1 -DDEBUG_FREE_MEM=1 \
	-DLUA_USE_OS=1 \
	-DLUA_USE_UTF8=1 \
	-DLUA_USE_I2C=1 \

#	-DFREERTOS=1 \


#-DLUA_USE_U8G=1 \
#	-DLUA_USE_PACKAGE=1 
#	-DLUA_USE_PIO=0 

Lua_SRC_DIR = $(Lua_ROOT)src $(Lua_ROOT)modules $(Lua_ROOT)modules/httpd $(Lua_ROOT)common $(Lua_ROOT)platform 
#$(Lua_ROOT)httpd
#EXTRA_COMPONENTS = extras/ssd1306 extras/i2c

#EXTRA_CFLAGS=-DLWIP_HTTPD_CGI=1 
#-I./fsdata

#Enable debugging
#EXTRA_CFLAGS+=-DLWIP_DEBUG=1 -DHTTPD_DEBUG=LWIP_DBG_ON
#EXTRA_COMPONENTS = extras/dhcpserver extras/rboot-ota extras/libesphttpd
ESP_IP ?= 192.168.4.1

#Tag for OTA images. 0-27 characters. Change to eg your projects title.
#LIBESPHTTPD_OTA_TAGNAME ?= generic

LIBESPHTTPD_MAX_CONNECTIONS ?= 2
#LIBESPHTTPD_STACKSIZE ?= 2048

CFLAGS += -DFREERTOS=1 
#-DLIBESPHTTPD_OTA_TAGNAME="\"$(LIBESPHTTPD_OTA_TAGNAME)\"" 
#-DFLASH_SIZE=$(FLASH_SIZE)
EXTRA_CFLAGS += -DMEMP_NUM_NETCONN=$(LIBESPHTTPD_MAX_CONNECTIONS)







LDFLAGS += 
#-Wl,-u,lua_libs_os1 
#-Wl,--whole-archive $(BUILD_DIR)/lua.a -Wl,--no-whole-archive

$(eval $(call component_compile_rules,Lua))
