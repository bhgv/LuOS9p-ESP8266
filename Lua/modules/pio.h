#ifndef PIO_H
#define	PIO_H

#include <types.h>
#include "drivers/gpio/gpio.h"
enum
{
  // Pin operations
  PLATFORM_IO_PIN_SET,
  PLATFORM_IO_PIN_CLEAR,
  PLATFORM_IO_PIN_GET,
  PLATFORM_IO_PIN_DIR_INPUT,
  PLATFORM_IO_PIN_DIR_OUTPUT,
  PLATFORM_IO_PIN_PULLUP,
  PLATFORM_IO_PIN_PULLDOWN,
  PLATFORM_IO_PIN_NOPULL,
  PLATFORM_IO_PIN_NUM,
  
  // Port operations
  PLATFORM_IO_PORT_SET_VALUE,
  PLATFORM_IO_PORT_GET_VALUE,
  PLATFORM_IO_PORT_DIR_INPUT,
  PLATFORM_IO_PORT_DIR_OUTPUT
};

typedef u32 pio_type;

// Max of 7 i/o port for PIC32MZ family (from A to G)
#define PLATFORM_IO_PORTS 7

// Each i/o port are 16 bits - wide

#define PLATFORM_IO_PORTS_BITS 16

// Each pin is encoded in 4 bits
#define PLATFORM_IO_PINS_BITS 4

// Each i/o pin is encoded as port (bits 7-4) and pin (bits 3-0)
#define PLATFORM_IO_ENCODE(port, pin) (((port) << PLATFORM_IO_PINS_BITS) | (pin))

#define PLATFORM_IO_ALL_PINS 0xffffffff

int platform_pio_has_port( unsigned port );
const char* platform_pio_get_prefix( unsigned port );
int platform_pio_has_pin( unsigned port, unsigned pin );
int platform_pio_get_num_pins( unsigned port );
pio_type platform_pio_op( unsigned port, pio_type pinmask, int op );


#endif	

