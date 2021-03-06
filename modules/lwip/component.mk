# Component makefile for LWIP

LWIP_DIR = $(lwip_ROOT)lwip/src/
INC_DIRS += $(LWIP_DIR)include \
	$(ROOT)lwip/include \
	$(lwip_ROOT)include \
	$(LWIP_DIR)include/posix \
	$(LWIP_DIR)include/ipv4 \
	$(LWIP_DIR)include/ipv4/lwip \
	$(LWIP_DIR)include/lwip \
	$(lwip_ROOT)lwip/espressif/include 

# args for passing into compile rule generation
lwip_INC_DIR =  # all in INC_DIRS, needed for normal operation
lwip_SRC_DIR = $(lwip_ROOT) $(LWIP_DIR)api $(LWIP_DIR)core $(LWIP_DIR)core/ipv4 $(LWIP_DIR)netif \
	$(lwip_ROOT)lwip/espressif 

# LWIP 1.4.1 generates a single warning so we need to disable -Werror when building it
lwip_CFLAGS = $(CFLAGS) -Wno-address


#%.o: %.c
#	$(CC) -c $(CFLAGS) -o $@ $<
#	$(OBJCOPY) --rename-section .text=.irom0.text --rename-section .literal=.irom0.literal $@


$(eval $(call component_compile_rules,lwip))

# Helpful error if git submodule not initialised
$(lwip_SRC_DIR):
	$(error "LWIP git submodule not installed. Please run 'git submodule init' then 'git submodule update'")

