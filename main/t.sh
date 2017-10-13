#!/bin/sh

xtensa-lx106-elf-gcc \
    -E \
    -I./ -I./include -I../../../include/platform/esp8266 -I../../../libc/platform/esp8266/include \
    -I../../../FreeRTOS/Source/include -I../../../FreeRTOS/Source/portable/esp8266 -I../../../ -I../../../Lua/adds \
    -I../../../Lua/common -I../../../Lua/modules -I../../../Lua/src -I../../../platform/esp8266/core/include \
    -I../../../platform/esp8266/lwip/lwip/src/include -I../../../lwip/include -I../../../platform/esp8266/lwip/include \
    -I../../../platform/esp8266/lwip/lwip/src/include/posix -I../../../platform/esp8266/lwip/lwip/src/include/ipv4 \
    -I../../../platform/esp8266/lwip/lwip/src/include/ipv4/lwip -I../../../platform/esp8266/lwip/lwip/src/include/lwip \
    -Ilwip/espressif/include -I../../../platform/esp8266/open_esplibs/include -I../../../Lua/adds -I../../../Lua/common \
    -I../../../Lua/modules -I../../../Lua/src -I../../../sys -I/home/q56/esp-open-rtos/extras -I../../../platform/esp8266/ssd1306 \
    -I../../../platform/esp8266/i2c -I../../../platform/esp8266/platform -I/home/q56/esp-open-rtos/include/espressif \
    -I/home/q56/esp-open-rtos/include -I../../../platform/esp8266/core/include -I../../../include/platform/esp8266/espressif/esp8266 \
    -I../../../platform/esp8266/lwip/lwip/espressif/include -I../../../FreeRTOS/Source/include -I../../../include \
    -mlongcalls -mtext-section-literals -DGITSHORTREV=\"06d2000\" -Wall -Wl,-EL -nostdlib  -ffunction-sections -fdata-sections -g -O2 \
    -std=gnu99  -D__XMK__ -fno-builtin -DRBOOT_GPIO_ENABLED=0 -DPLATFORM_ESP8266 -DCPU_HZ=80000000L -DCORE_TIMER_HZ=CPU_HZ \
    -D_CLOCKS_PER_SEC_=100 -DUSE_NETWORKING=1 -DMTX_USE_EVENTS=0 -DluaTaskStack=192*5 -DtskDEF_PRIORITY=0 -DdefaultThreadStack=192*5 \
    -DUSE_CFI=1 -DSPIFFS_ERASE_SIZE=4096 -DSPIFFS_LOG_PAGE_SIZE=256 -DSPIFFS_LOG_BLOCK_SIZE=8192 -DSPIFFS_BASE_ADDR=0x100000 \
    -DSPIFFS_SIZE=0x80000 -DUSE_CONSOLE=1 -DCONSOLE_BR=115200 -DCONSOLE_UART=1 -DCONSOLE_SWAP_UART=2 -DCONSOLE_BUFFER_LEN=255 \
    -DLORA_UART=3 -DLORA_UART_BR=57600 -DLORA_UART_BUFF_SIZE=255 -DLORA_RST_PIN=14 -DDEBUG_FREE_MEM=1 -DLUA_USE_LUA_LOCK=0 \
    -DLUA_USE_SAFE_SIGNAL=1 -DSTRCACHE_N=1 -DSTRCACHE_M=1 -DMINSTRTABSIZE=32 -DLUA_USE__G=1 -DLUA_USE_OS=1 -DLUA_USE_MATH=1 -DLUA_USE_TABLE=1 \
    -DLUA_USE_IO=1 -DLUA_USE_STRING=1 -DLUA_USE_COROUTINE=1 -DLUA_USE_DEBUG=1 -DLUA_USE_TMR=1 -DLUA_USE_PIO=1 -DLUA_USE_LORA=0 \
    -DLUA_USE_PACK=1 -DLUA_USE_THREAD=1 -DLUA_USE_EDITOR=0 -DLUA_USE_I2C=0 -DUSE_CUSTOM_HEAP=0 \
    -I../../../Lua/src -I../../../FreeRTOS/Source/include -I../../../include/platform/esp8266/espressif -I../../../include/platform/esp8266/espressif/esp8266 -I../../../platform/esp8266/lwip/lwip/espressif/include -I../../../platform/esp8266/core/include/esp -I../../../platform/esp8266/core/include -I../../../sys  -DKERNEL -DKERNEL -DLUA_USE_CTYPE -DLUA_32BITS -DLUA_USE_ROTABLE=1 -DDEBUG_FREE_MEM=1  -c \
    /home/q56/bld/esp8266/Lua-RTOS-ESP8266/Lua/src/loslib.c -o linit.o
