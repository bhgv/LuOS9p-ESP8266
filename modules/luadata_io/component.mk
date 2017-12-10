
INC_DIRS += $(lua9p_ROOT) \

# args for passing into compile rule generation
lua9p_INC_DIR =  # all in INC_DIRS, needed for normal operation
lua9p_SRC_DIR = $(lua9p_ROOT)

# LWIP 1.4.1 generates a single warning so we need to disable -Werror when building it
#lwip_CFLAGS = $(CFLAGS) -Wno-address

$(eval $(call component_compile_rules,lua9p))
