# Component makefile for extras/ssd1306

# expected anyone using ssd1306 driver includes it as 'ssd1306/ssd1306.h'
INC_DIRS += $(pnmio_ROOT)

# args for passing into compile rule generation
pnmio_SRC_DIR = $(pnmio_ROOT)

#pnmio_CFLAGS = -DSSD1306_I2C_SUPPORT=${SSD1306_I2C_SUPPORT} -DSSD1306_SPI4_SUPPORT=${SSD1306_SPI4_SUPPORT} $(CFLAGS)


$(eval $(call component_compile_rules,pnmio))
