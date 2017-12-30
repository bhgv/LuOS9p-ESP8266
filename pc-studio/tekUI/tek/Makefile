
BASEDIR ?= ..
include $(BASEDIR)/config

###############################################################################

LUACLASSES = \
	class/object.lua class/list.lua

install:
	$(INSTALL_D) $(LUA_SHARE)/tek/class
	$(INSTALL_F) class.lua ui.lua $(LUA_SHARE)/tek
	$(INSTALL_F) $(LUACLASSES) $(LUA_SHARE)/tek/class
