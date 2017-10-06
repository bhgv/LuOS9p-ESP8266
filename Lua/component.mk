INC_DIRS += $(Lua_ROOT)adds $(Lua_ROOT)common $(Lua_ROOT)modules $(Lua_ROOT)src

Lua_INC_DIR = $(Lua_ROOT)adds $(Lua_ROOT)common $(Lua_ROOT)modules $(Lua_ROOT)src \
	$(ROOT)sys \
	$(HOME)/esp-open-rtos/extras \
	$(ROOT)platform/esp8266/ssd1306 \
	$(ROOT)platform/esp8266/i2c \
	$(ROOT)platform/esp8266/platform \
	$(HOME)/esp-open-rtos/include/espressif \
	$(HOME)/esp-open-rtos/include \
	$(ROOT)platform/esp8266/core/include \
	$(ROOT)include/platform/esp8266/espressif/esp8266 \
	$(ROOT)platform/esp8266/lwip/lwip/espressif/include \
	$(ROOT)FreeRTOS/Source/include \
	$(ROOT)include \
	$(ROOT)config/lua \


#	$(HOME)/esp-open-rtos/core/include

CFLAGS += -DKERNEL -DLUA_USE_CTYPE -DLUA_32BITS -DLUA_USE_ROTABLE=1 -DDEBUG_FREE_MEM=1 \
	-DLUA_USE_PACKAGE=1 -DLUA_USE_OS=1 \
	-DLUA_USE_PIO=1 -DLUA_USE_UTF8=1 \
	-DLUA_USE_I2C=1 -DLUA_USE_U8G=1 \


Lua_SRC_DIR = $(Lua_ROOT)src $(Lua_ROOT)modules $(Lua_ROOT)common 

EXTRA_COMPONENTS = extras/ssd1306 extras/i2c

LDFLAGS += 
#-Wl,-u,lua_libs_os1 
#-Wl,--whole-archive $(BUILD_DIR)/lua.a -Wl,--no-whole-archive

$(eval $(call component_compile_rules,Lua))
