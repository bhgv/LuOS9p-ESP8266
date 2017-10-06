# args for passing into compile rule generation
pthread_SRC_DIR = $(pthread_ROOT)

pthread_INC_DIR = $(pthread_ROOT) \
		$(ROOT)sys \
		$(ROOT)include \


$(eval $(call component_compile_rules,pthread))
