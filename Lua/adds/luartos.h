#ifndef LUA_RTOS_LUARTOS_H_
#define LUA_RTOS_LUARTOS_H_

#include "FreeRTOS.h"
//#include "esp_task.h"
//#include "sdkconfig.h"

/* Board type */
#define LUA_RTOS_BOARD "ESP8266"

/* Fat file system */
#define CONFIG_LUA_RTOS_USE_FAT CONFIG_LUA_RTOS_USE_SPI_SD

/*
 * Lua RTOS
 */
//#define LUA_USE_ROTABLE	   1

// Get the UART assigned to the console
//#if CONFIG_LUA_RTOS_CONSOLE_UART0
//#define CONSOLE_UART 0
//#endif

/*
#if CONFIG_LUA_RTOS_CONSOLE_UART1
#define CONSOLE_UART 1
#endif

#if CONFIG_LUA_RTOS_CONSOLE_UART2
#define CONSOLE_UART 2
#endif
*/

// Get the console baud rate
//#if CONFIG_LUA_RTOS_CONSOLE_BR_57600
//#define CONSOLE_BR 57600
//#endif

//#if CONFIG_LUA_RTOS_CONSOLE_BR_115200
//#define CONSOLE_BR 115200
//#endif

// Get the console buffer length
#ifndef CONSOLE_BUFFER_LEN
#ifdef CONFIG_LUA_RTOS_CONSOLE_BUFFER_LEN
#define CONSOLE_BUFFER_LEN CONFIG_LUA_RTOS_CONSOLE_BUFFER_LEN
#else
#define CONSOLE_BUFFER_LEN 100
#endif
#endif

#ifndef CONSOLE_UART
#define CONSOLE_UART 1
#endif

#ifndef CONSOLE_BR
#define CONSOLE_BR 115200
#endif

// LoRa WAN
#define US_PER_OSTICK   20
#define OSTICKS_PER_SEC 50000
#define LMIC_SPI_KHZ    1000

#if CONFIG_LUA_RTOS_LORA_NODE_RADIO_SX1276
	#define CFG_sx1276_radio 1
#else
	#if CONFIG_LUA_RTOS_LORA_NODE_RADIO_SX1272
		#define CFG_sx1272_radio 1
	#else
		#define CFG_sx1276_radio 1
	#endif
#endif

#if CONFIG_LUA_RTOS_LORA_NODE_BAND_EU868
	#define CFG_eu868 1
#else
	#if CONFIG_LUA_RTOS_LORA_NODE_BAND_US915
		#define CFG_us915 1
	#else
		#define CFG_eu868 1
	#endif
#endif


#define CONFIG_FREERTOS_THREAD_LOCAL_STORAGE_POINTERS 2

#if CONFIG_FREERTOS_THREAD_LOCAL_STORAGE_POINTERS <= 1
#error "Please, review the 'Number of thread local storage pointers' settings in kconfig. Must be >= 2."
#endif

#define THREAD_LOCAL_STORAGE_POINTER_ID (CONFIG_FREERTOS_THREAD_LOCAL_STORAGE_POINTERS - 1)

// External GPIO
#define EXTERNAL_GPIO 0
#define EXTERNAL_GPIO_PINS 0
#define EXTERNAL_GPIO_PORTS 0
#if CONFIG_GPIO_PCA9698 ||  CONFIG_GPIO_PCA9505
	#undef EXTERNAL_GPIO_PINS
	#define EXTERNAL_GPIO_PINS 40
	#undef EXTERNAL_GPIO_PORTS
	#define EXTERNAL_GPIO_PORTS 5
	#undef EXTERNAL_GPIO
	#define EXTERNAL_GPIO 1

	#if CONFIG_GPIO_PCA9698
	#define EXTERNAL_GPIO_NAME "PCA9698"
	#endif

#if CONFIG_GPIO_PCA9505
	#define EXTERNAL_GPIO_NAME "PCA9505"
	#endif
#endif

#endif
