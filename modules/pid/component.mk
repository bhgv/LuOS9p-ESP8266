

INC_DIRS += $(pid_ROOT) 


# args for passing into compile rule generation
pid_INC_DIR =  # all in INC_DIRS, needed for normal operation
pid_SRC_DIR = $(pid_ROOT) 


$(eval $(call component_compile_rules,pid))
