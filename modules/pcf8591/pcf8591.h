/**
 * Copyright (C) 2017 bhgv
 *
 * Driver for  8-bit analog-to-digital conversion and
 * an 8-bit digital-to-analog conversion PCF8591
 */
#ifndef _EXTRAS_PCF8591_H_
#define _EXTRAS_PCF8591_H_

#include <drivers/i2c-platform.h>

#ifdef __cplusplus
extern "C"
{
#endif


#define PCF8591_DEFAULT_ADDRESS 0x48

//void pcf8591_init(void); //FIXME : library incomplete ?

uint8_t pcf8591_read(unsigned char addr, uint8_t analog_pin);

uint8_t pcf8591_write(unsigned char addr, uint8_t data);



#ifdef __cplusplus
}
#endif

#endif /* _EXTRAS_PCF8591_H_ */
