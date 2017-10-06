# Component makefile for extras/ssd1306

# expected anyone using ssd1306 driver includes it as 'ssd1306/ssd1306.h'
platform_INC_DIR = $(platform_ROOT) $(platform_ROOT).. $(platform_ROOT)../i2c $(platform_ROOT)../../core/include $(platform_ROOT)../../lwip/lwip/espressif/include 

# args for passing into compile rule generation
platform_SRC_DIR = $(platform_ROOT)

platform_CFLAGS = -std=gnu11 -Wimplicit $(CFLAGS)


$(eval $(call component_compile_rules,platform))
