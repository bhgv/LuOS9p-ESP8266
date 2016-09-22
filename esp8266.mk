CFLAGS += -DRBOOT_GPIO_ENABLED=0

CFLAGS += -DCPU_HZ=80000000L           # CPU frequency in hertz
CFLAGS += -DCORE_TIMER_HZ=CPU_HZ       # CPU core timer frequency
CFLAGS += -D_CLOCKS_PER_SEC_=100       # Number of interrupt ticks for reach 1 second

#
# File system configuration
# 
CFLAGS += -DUSE_CFI=1                  # Enable CFI (common flash interface)
CFLAGS += -DSPIFFS_ERASE_SIZE=4096     # SPI FLASH sector size (see your flah's datasheet)
CFLAGS += -DSPIFFS_LOG_PAGE_SIZE=256   # SPI FLASH page size (see your flah's datasheet)
CFLAGS += -DSPIFFS_LOG_BLOCK_SIZE=8192 # Logical block size, must be a miltiple of the page size
CFLAGS += -DSPIFFS_BASE_ADDR=0x100000  # SPI FLASH start adress for SPIFFS
CFLAGS += -DSPIFFS_SIZE=0x80000        # SPIFFS size

#
# Console configuration
#
CFLAGS += -DUSE_CONSOLE=1		       # Enable console
CFLAGS += -DCONSOLE_BR=115200	       # Console baud rate
CFLAGS += -DCONSOLE_UART=1		       # Console UART unit
CFLAGS += -DCONSOLE_SWAP_UART=2	       # Console alternative UART unit (0 = don't use alternative UART)
CFLAGS += -DCONSOLE_BUFFER_LEN=1024    # Console buffer length in bytes

#
# LoraWAN driver connfiguration for RN2483
#
CFLAGS += -DLORA_UART=3				   # RN2483 UART unit
CFLAGS += -DLORA_UART_BR=57600         # RN2483 UART speed
CFLAGS += -DLORA_UART_BUFF_SIZE=1024   # Buffer size for RX
CFLAGS += -DLORA_RST_PIN=14			   # RN2483 hardware reset pin


CFLAGS += -DDEBUG_FREE_MEM=1           # Enable LUA free mem debug utility

#
# Standard Lua modules to include
#
CFLAGS += -DLUA_USE__G=1			   # base
CFLAGS += -DLUA_USE_OS=1			   # os
CFLAGS += -DLUA_USE_MATH=1		       # math
CFLAGS += -DLUA_USE_TABLE=1		       # table


#
# LuaOS Lua modules to include
#
CFLAGS += -DLUA_USE_TMR=1		       # timer
CFLAGS += -DLUA_USE_PIO=1		       # gpio
CFLAGS += -DLUA_USE_LORA=1		       # lora
CFLAGS += -DLUA_USE_PACK=1		       # pack
CFLAGS += -DLUA_USE_THREAD=1		   # thread
