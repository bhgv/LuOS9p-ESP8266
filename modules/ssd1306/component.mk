# Component makefile for extras/ssd1306

# expected anyone using ssd1306 driver includes it as 'ssd1306/ssd1306.h'
ssd1306_INC_DIR = $(ssd1306_ROOT) $(ROOT) \
	    $(ROOT)platform/esp8266/core/include \
	    $(ROOT)platform/esp8266/lwip/lwip/espressif/include \
	    $(ROOT)include/platform/esp8266/espressif/esp8266 \


# I2C support is on by default
#SSD1306_I2C_SUPPORT ?= 1
# SPI4 support is on by default
#SSD1306_SPI4_SUPPORT ?= 0

# args for passing into compile rule generation
ssd1306_SRC_DIR = $(ssd1306_ROOT)

ssd1306_CFLAGS = -std=gnu11 -Wimplicit $(CFLAGS)


$(eval $(call component_compile_rules,ssd1306))
