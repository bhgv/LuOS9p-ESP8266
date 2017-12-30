
BASEDIR ?= ..
include $(BASEDIR)/config

all: libs tools modules
clean libs tools modules:
	cd teklib && $(MAKE) $@
	cd hal && $(MAKE) $@
	cd exec && $(MAKE) $@
	cd misc && $(MAKE) $@
	cd visual && $(MAKE) $@
# primary display driver:
	cd display_$(DISPLAY_DRIVER) && $(MAKE) $@
# comment in additional (sub-) display driver(s) here:
#	cd display_x11 && $(MAKE) $@
#	cd display_directfb && $(MAKE) $@
