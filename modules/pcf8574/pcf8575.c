
#include "pcf8575.h"


uint16_t old_val;

uint16_t pcf8575_port_read(unsigned char addr)
{
    uint16_t res = 0;
//    platform_i2c_send_start(0);
//	platform_i2c_send_address(0, addr, 0);
//	platform_i2c_send_byte(0, reg);
//	platform_i2c_send_stop(0);

    platform_i2c_send_start(0);
	platform_i2c_send_address(0, addr, 1);
	res = platform_i2c_recv_byte(0, 1);
	res |= platform_i2c_recv_byte(0, 1) << 8;
	platform_i2c_send_stop(0);
	udelay(2);
//    if (i2c_slave_read(dev->bus, dev->addr, NULL, &res, 1))
//            return 0;
    return res;
}

void pcf8575_port_write(const unsigned char addr, uint16_t val)
{
	old_val = val;
	
//	platform_i2c_send_start(0);
//	platform_i2c_send_address(0, addr, 0);
//	platform_i2c_send_byte(0, reg);
////	platform_i2c_send_stop(0);
	
	platform_i2c_send_start(0);
	platform_i2c_send_address(0, addr, 0);
	platform_i2c_send_byte(0, val);	
	platform_i2c_send_byte(0, val >> 8);
	platform_i2c_send_stop(0);
	udelay(2);
//    i2c_slave_write(dev->bus, dev->addr, NULL, &value, 1);
}

bool pcf8575_gpio_read(unsigned char addr, uint8_t num)
{
	if(num < 0 || num > 15) return 0;
    return (bool)((pcf8575_port_read(addr) >> num) & 1);
}

void pcf8575_gpio_write(unsigned char addr, uint8_t num, uint16_t value)
{
	if(num < 0 || num > 15) return ;
	value = value ? 1 : 0;
	
    uint16_t bit = (uint16_t)value << num;
    uint16_t mask = ~(1 << num);
	
//    pcf8575_port_write (addr, (pcf8575_port_read(addr) & mask) | bit);
    pcf8575_port_write (addr, (old_val & mask) | bit);
}

void pcf8575_init(unsigned char addr){
	pcf8575_port_write(addr, 0xffff);
}



