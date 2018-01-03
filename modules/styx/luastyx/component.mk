INC_DIRS += $(luastyx_ROOT)

# args for passing into compile rule generation
luastyx_SRC_DIR = $(luastyx_ROOT) 

$(eval $(call component_compile_rules,luastyx))
