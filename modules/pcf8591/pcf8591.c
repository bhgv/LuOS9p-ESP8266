#include <stddef.h>
#include <stdint.h>

#include "pcf8591.h"


#define PCF8591_CTRL_REG_READ 0x03


uint8_t pcf8591_read(unsigned char addr, uint8_t ch)
{
	ch = PCF8591_CTRL_REG_READ & ch;
    uint8_t res[4]; 
    uint8_t reg = 0x40 | 4; 
	uint8_t x=1;
	
	uint8_t ot = i2c_master_set_delay_us(1);

	while (x>0 && x<4){
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
	    platform_i2c_send_stop(0);
	    udelay(2);
//	printf("adc ch(%d) = %d,  %d, %d, %d\n", ch, res[0], res[1], res[2], res[3]);

		if( !(res[0]==255 && res[1]==255 && res[2]==255 && res[3]==255) )
			x=0;
		else
			x++;
	}
	i2c_master_set_delay_us(ot);

    return res[ch];
}

uint8_t pcf8591_write(unsigned char addr, uint8_t data)
{
    uint8_t reg = 0x40;

    platform_i2c_send_start(0);
    platform_i2c_send_address(0, addr, 0);
    platform_i2c_send_byte(0, reg);

    platform_i2c_send_byte(0, data);
    platform_i2c_send_stop(0);
    udelay(2);
//    i2c_slave_read(dev->bus, dev->addr, &control_reg, &res, 1);

    return 0;
}
