INC_DIRS += $(libstyx_ROOT)

# args for passing into compile rule generation
libstyx_SRC_DIR = $(libstyx_ROOT) 

$(eval $(call component_compile_rules,libstyx))
