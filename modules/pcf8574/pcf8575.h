/**
 * \file Driver for PCF8575 compartible remote 16-bit I/O expanders for I2C-bus
 * \author bhgv (based on ruslan uss pcf8574 driver)
 */
#ifndef PCF8575_H_
#define PCF8575_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <drivers/i2c-platform.h>


#define PCF8575_DEFAULT_ADDRESS 0x20


#ifdef __cplusplus
extern "C"
{
#endif

void pcf8575_init(unsigned char addr);

/**
 * \brief Read GPIO port value
 * \param addr I2C register address (0b0100<A2><A1><A0> for PCF8574)
 * \return 8-bit GPIO port value
 */
uint16_t pcf8575_port_read(unsigned char addr);

/**
 * \brief Continiously read GPIO port values to buffer
 * @param addr I2C register address (0b0100<A2><A1><A0> for PCF8574)
 * @param buf Target buffer
 * @param len Buffer length
 * @return Number of bytes read
 */
//size_t pcf8575_port_read_buf(unsigned char addr, void *buf, size_t len);

/**
 * \brief Write value to GPIO port
 * \param addr I2C register address (0b0100<A2><A1><A0> for PCF8574)
 * \param value GPIO port value
 */
void pcf8575_port_write(const unsigned char addr, uint16_t value);

/**
 * \brief Continiously write GPIO values to GPIO port
 * \param addr I2C register address (0b0100<A2><A1><A0> for PCF8574)
 * @param buf Buffer with values
 * @param len Buffer length
 * @return Number of bytes written
 */
//size_t pcf8575_port_write_buf(const unsigned char addr, void *buf, size_t len);

/**
 * \brief Read input value of a GPIO pin
 * \param addr I2C register address (0b0100<A2><A1><A0> for PCF8574)
 * \param num pin number (0..7)
 * \return GPIO pin value
 */
bool pcf8575_gpio_read(unsigned char addr, uint8_t num);

/**
 * \brief Set GPIO pin output
 * Note this is READ - MODIFY - WRITE operation! Please read PCF8574
 * datasheet first.
 * \param addr I2C register address (0b0100<A2><A1><A0> for PCF8574)
 * \param num pin number (0..7)
 * \param value true for high level
 */
void pcf8575_gpio_write(unsigned char addr, uint8_t num, uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* PCF8575_H_ */
