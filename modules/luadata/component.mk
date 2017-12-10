
INC_DIRS += $(luadata_ROOT) \

# args for passing into compile rule generation
luadata_INC_DIR =  # all in INC_DIRS, needed for normal operation
luadata_SRC_DIR = $(luadata_ROOT)

# LWIP 1.4.1 generates a single warning so we need to disable -Werror when building it
#lwip_CFLAGS = $(CFLAGS) -Wno-address

$(eval $(call component_compile_rules,luadata))
