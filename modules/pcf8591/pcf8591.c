#include <stddef.h>
#include <stdint.h>

#include "pcf8591.h"

/**
 * CAUTION: PLEASE SET LOW FREQUENCY
 */

#define PCF8591_CTRL_REG_READ 0x03

/*
uint8_t pcf8591_read(unsigned char addr, uint8_t *adc0, uint8_t *adc1, uint8_t *adc2, uint8_t *adc3)
{
    uint8_t res = 0;
    uint8_t reg = 4; // + (PCF8591_CTRL_REG_READ & analog_pin);

    platform_i2c_send_start(0);
    platform_i2c_send_address(0, addr, 0);
    platform_i2c_send_byte(0, reg);
    platform_i2c_send_stop(0);

    platform_i2c_send_start(0);
    platform_i2c_send_address(0, addr, 1);
    res = platform_i2c_recv_byte(0, 1);
    *adc0 = platform_i2c_recv_byte(0, 1);
    *adc1 = platform_i2c_recv_byte(0, 1);
    *adc2 = platform_i2c_recv_byte(0, 1);
    *adc3 = platform_i2c_recv_byte(0, 1);
    platform_i2c_send_stop(0);
    udelay(20);
//    i2c_slave_read(dev->bus, dev->addr, &control_reg, &res, 1);

    return res;
}
*/

uint8_t pcf8591_read(unsigned char addr, uint8_t ch)
{
    uint8_t res[4]; // = 0;
    uint8_t reg = 4; //(PCF8591_CTRL_REG_READ & ch);

    platform_i2c_send_start(0);
    platform_i2c_send_address(0, addr, 0);
    platform_i2c_send_byte(0, reg);
    platform_i2c_send_stop(0);

    platform_i2c_send_start(0);
    platform_i2c_send_address(0, addr, 1);
	
    platform_i2c_recv_byte(0, 1);
    res[0] = platform_i2c_recv_byte(0, 1);
	res[1] = platform_i2c_recv_byte(0, 1);
    res[2] = platform_i2c_recv_byte(0, 1);
	res[3] = platform_i2c_recv_byte(0, 1);
//    *adc1 = platform_i2c_recv_byte(0, 1);
//    *adc2 = platform_i2c_recv_byte(0, 1);
//    *adc3 = platform_i2c_recv_byte(0, 1);
    platform_i2c_send_stop(0);
    udelay(2);
//    i2c_slave_read(dev->bus, dev->addr, &control_reg, &res, 1);

    return res[PCF8591_CTRL_REG_READ & ch];
}

uint8_t pcf8591_write(unsigned char addr, uint8_t data)
{
//    uint8_t res = 0;
    uint8_t reg = 0x40;

    platform_i2c_send_start(0);
    platform_i2c_send_address(0, addr, 0);
    platform_i2c_send_byte(0, reg);
//    platform_i2c_send_stop(0);

//    platform_i2c_send_start(0);
//    platform_i2c_send_address(0, dev->addr, 1);
    platform_i2c_send_byte(0, data);
    platform_i2c_send_stop(0);
    udelay(20);
//    i2c_slave_read(dev->bus, dev->addr, &control_reg, &res, 1);

    return 0;
}
