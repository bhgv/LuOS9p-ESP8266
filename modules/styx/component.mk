

styx_COMPONENTS = \
    $(styx_ROOT)/9infr \
    $(styx_ROOT)/libstyx \
    $(styx_ROOT)/luastyx 



INC_DIRS += $(styx_ROOT) \
    $(styx_ROOT)/9infr \
    $(styx_ROOT)/libstyx \
    $(styx_ROOT)/luastyx 


# args for passing into compile rule generation
styx_INC_DIR =  # all in INC_DIRS, needed for normal operation
styx_SRC_DIR = $(styx_ROOT) \
    $(styx_ROOT)/9infr \
    $(styx_ROOT)/libstyx \
    $(styx_ROOT)/luastyx 


# LWIP 1.4.1 generates a single warning so we need to disable -Werror when building it
#lwip_CFLAGS = $(CFLAGS) -Wno-address

$(eval $(call component_compile_rules,styx))
