CFLAGS += -DRBOOT_GPIO_ENABLED=0

CFLAGS += -DCPU_HZ=80000000L        # CPU frequency in hertz
CFLAGS += -DCORE_TIMER_HZ=CPU_HZ    # CPU core timer frequency
CFLAGS += -D_CLOCKS_PER_SEC_=100    # Number of interrupt ticks for reach 1 second

CFLAGS += -DUSE_CFI=1

CFLAGS += -DUSE_CONSOLE=1		    # Enable console
CFLAGS += -DCONSOLE_BR=115200	    # Console baud rate
CFLAGS += -DCONSOLE_UART=1		    # Console UART unit
CFLAGS += -DCONSOLE_SWAP_UART=2	    # Console alternative UART unit (0 = don't use alternative UART)
CFLAGS += -DCONSOLE_BUFFER_LEN=64   # Console buffer length in bytes

CFLAGS += -DLORA_UART=3
CFLAGS += -DLORA_UART_BR=57600
CFLAGS += -DLORA_UART_BUFF_SIZE=1024
CFLAGS += -DLORA_RST_PIN=14

CFLAGS += -DSPIFFS_ERASE_SIZE=4096
CFLAGS += -DSPIFFS_LOG_PAGE_SIZE=256
CFLAGS += -DSPIFFS_LOG_BLOCK_SIZE=8192
CFLAGS += -DSPIFFS_BASE_ADDR=0x100000
CFLAGS += -DSPIFFS_SIZE=0x80000

CFLAGS += -DDEBUG_FREE_MEM=1        #Enable LUA free mem debug utility