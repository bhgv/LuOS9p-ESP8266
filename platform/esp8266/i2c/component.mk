# Component makefile for extras/i2c

# expected anyone using i2c driver includes it as 'i2c/i2c.h'
#INC_DIRS += $(i2c_ROOT).. $(i2c_ROOT)../../include/espressif $(i2c_ROOT)../../include $(i2c_ROOT)../../core/include 

# args for passing into compile rule generation
i2c_INC_DIR = $(i2c_ROOT) $(i2c_ROOT).. \
	    $(ROOT)/include/platform/esp8266 \
	    $(ROOT)/sys/ \
	    $(i2c_ROOT)../core/include \
	    $(i2c_ROOT)../core/include/esp \
	    $(ROOT)/include/platform/esp8266/espressif/esp8266 \
	    $(i2c_ROOT)../lwip/lwip/espressif/include \

#	    $(ROOT)include/platform/esp8266/espressif \

i2c_SRC_DIR = $(i2c_ROOT)

$(eval $(call component_compile_rules,i2c))
