/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Johan Kanflo (github.com/kanflo)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/**
 * I2C driver for ESP8266 written for use with esp-open-rtos
 * Based on https://en.wikipedia.org/wiki/I²C#Example_of_bit-banging_the_I.C2.B2C_Master_protocol
 */

#ifndef __I2C_DEF_H__
#define __I2C_DEF_H__

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Define i2c bus max number
 */
#ifndef MAX_I2C_BUS
    #define MAX_I2C_BUS 2
#endif

typedef enum
{
 	I2C_FREQ_80K = 0,//!< I2C_FREQ_80K
 	I2C_FREQ_100K,   //!< I2C_FREQ_100K
 	I2C_FREQ_400K,   //!< I2C_FREQ_400K
 	I2C_FREQ_500K,   //!< I2C_FREQ_500K
} i2c_freq_t;

/**
 * Device descriptor
 */
typedef struct i2c_dev
{
  uint8_t bus;
  uint8_t addr;
} i2c_dev_t;

#ifdef	__cplusplus
}
#endif

#endif /* __I2C_H__ */
