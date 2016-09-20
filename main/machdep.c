#include "FreeRTOS.h"
#include "whitecat.h"

#include "esp/uart.h"

extern void _clock_init();
extern void _syscalls_init();
extern void _pthread_init();
extern void _console_init();
extern void _lora_init();

void mach_init() {  
    //resource_init();
    _clock_init();
    _syscalls_init();
	_console_init();
    _pthread_init();
    //_signal_init();
    
    #if LUA_USE_LORA
    _lora_init();
    #endif
    
    //mach_dev();
}