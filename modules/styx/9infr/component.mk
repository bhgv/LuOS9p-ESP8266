INC_DIRS += $(9infr_ROOT)

# args for passing into compile rule generation
9infr_SRC_DIR = $(9infr_ROOT) 

$(eval $(call component_compile_rules,9infr))
