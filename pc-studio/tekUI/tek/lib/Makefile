
BASEDIR ?= ../..
include $(BASEDIR)/config

EXTRADEFS += $(LUA_DEFS) $(TEKUI_DEFS) $(TEKLIB_DEFS)
EXTRADEFS += -DDISPLAY_DRIVER=\"$(DISPLAY_DRIVER)\"

###############################################################################

# primary display driver:
DISPLAYS = display/$(DISPLAY_DRIVER)$(DLLEXT)

# comment in additional (sub-) display driver(s) here:
# DISPLAYS += display/x11$(DLLEXT)
# DISPLAYS += display/directfb$(DLLEXT)

###############################################################################

MODS = region$(DLLEXT) exec$(DLLEXT) visual$(DLLEXT) string$(DLLEXT) support$(DLLEXT)

EXECLIBS = $(LIBDIR)/libhal.a $(LIBDIR)/libexec.a $(LIBDIR)/libtekc.a $(LIBDIR)/libtekdebug.a
VISUALLIBS = $(LIBDIR)/libvisual.a $(LIBDIR)/libtek.a $(LIBDIR)/libtekdebug.a
DISPLAYX11LIBS = $(LIBDIR)/libdisplay_x11.a $(LIBDIR)/libtek.a $(LIBDIR)/libtekdebug.a $(LIBDIR)/libutf8.a $(LIBDIR)/libpixconv.a
DISPLAYDFBLIBS = $(LIBDIR)/libdisplay_directfb.a $(LIBDIR)/libtek.a $(LIBDIR)/libtekdebug.a $(LIBDIR)/libutf8.a
DISPLAYWINLIBS = $(LIBDIR)/libdisplay_win.a $(LIBDIR)/libtek.a $(LIBDIR)/libtekdebug.a $(LIBDIR)/libutf8.a
DISPLAYRAWFBLIBS = $(LIBDIR)/libdisplay_rawfb.a $(LIBDIR)/libtek.a $(LIBDIR)/libtekdebug.a $(LIBDIR)/libregion.a $(LIBDIR)/libutf8.a $(LIBDIR)/libpixconv.a

region$(DLLEXT): $(OBJDIR)/region.lo
	$(CC) $(MODCFLAGS) -o $@ $(OBJDIR)/region.lo $(PLATFORM_LIBS) $(LUA_LIBS) -lregion -ltekdebug

string$(DLLEXT): $(OBJDIR)/string.lo
	$(CC) $(MODCFLAGS) -o $@ $(OBJDIR)/string.lo $(PLATFORM_LIBS) $(LUA_LIBS) -ltek -ltekdebug -lutf8

support$(DLLEXT): $(OBJDIR)/support.lo
	$(CC) $(MODCFLAGS) -o $@ $(OBJDIR)/support.lo $(LUA_LIBS)

exec$(DLLEXT): $(OBJDIR)/exec_lua.lo $(EXECLIBS)
	$(CC) $(MODCFLAGS) -o $@ $(OBJDIR)/exec_lua.lo -L$(LIBDIR) -lhal -lexec -ltekc -ltekdebug $(PLATFORM_LIBS) $(LUA_LIBS)

visual$(DLLEXT): $(OBJDIR)/visual_lua.lo $(OBJDIR)/visual_api.lo $(OBJDIR)/visual_io.lo $(VISUALLIBS)
	$(CC) $(MODCFLAGS) -o $@ $(OBJDIR)/visual_lua.lo $(OBJDIR)/visual_api.lo $(OBJDIR)/visual_io.lo -L$(LIBDIR) -lvisual -lcachemanager -limgload -lpixconv -ltek -ltekdebug $(LUA_LIBS) $(TEKUI_LIBS)

display/x11$(DLLEXT): $(OBJDIR)/x11_lua.lo $(DISPLAYX11LIBS)
	$(CC) $(MODCFLAGS) -o $@ $(OBJDIR)/x11_lua.lo -L$(LIBDIR) -ldisplay_x11 $(X11_LIBS) -lutf8 -lpixconv -limgcache -ltek -ltekdebug $(LUA_LIBS)

display/directfb$(DLLEXT): $(OBJDIR)/directfb_lua.lo $(DISPLAYDFBLIBS)
	$(CC) $(MODCFLAGS) -o $@ $(OBJDIR)/directfb_lua.lo -L$(LIBDIR) -ldisplay_directfb $(DIRECTFB_LIBS) -lutf8 -ltek -ltekdebug $(LUA_LIBS)

display/windows$(DLLEXT): $(OBJDIR)/windows_lua.lo $(DISPLAYWINLIBS)
	$(CC) $(MODCFLAGS) -o $@ $(OBJDIR)/windows_lua.lo -L$(LIBDIR) -ldisplay_win $(WIN_LIBS) -lutf8 -ltek -ltekdebug $(LUA_LIBS)

display/rawfb$(DLLEXT): $(OBJDIR)/rawfb_lua.lo $(DISPLAYRAWFBLIBS)
	$(CC) $(MODCFLAGS) -o $@ $(OBJDIR)/rawfb_lua.lo -L$(LIBDIR) -ldisplay_rawfb -limgload $(RAWFB_LIBS) -lutf8 -lregion -limgcache -lpixconv -ltek -ltekdebug $(LUA_LIBS)

$(OBJDIR)/region.lo: region.c
	$(CC) $(LIBCFLAGS) -o $@ -c region.c

$(OBJDIR)/string.lo: string.c
	$(CC) $(LIBCFLAGS) -o $@ -c string.c

$(OBJDIR)/support.lo: support.c
	$(CC) $(LIBCFLAGS) -o $@ -c support.c

$(OBJDIR)/exec_lua.lo: exec_lua.c
	$(CC) $(LIBCFLAGS) -o $@ -c exec_lua.c

$(OBJDIR)/visual_lua.lo: visual_lua.c visual_lua.h
	$(CC) $(LIBCFLAGS) -o $@ -c visual_lua.c
$(OBJDIR)/visual_api.lo: visual_api.c visual_lua.h
	$(CC) $(LIBCFLAGS) -o $@ -c visual_api.c
$(OBJDIR)/visual_io.lo: visual_io.c visual_lua.h
	$(CC) $(LIBCFLAGS) -o $@ -c visual_io.c

$(OBJDIR)/x11_lua.lo: display/x11_lua.c
	$(CC) $(LIBCFLAGS) -o $@ -c display/x11_lua.c

$(OBJDIR)/directfb_lua.lo: display/directfb_lua.c
	$(CC) $(LIBCFLAGS) -o $@ -c display/directfb_lua.c

$(OBJDIR)/windows_lua.lo: display/windows_lua.c
	$(CC) $(LIBCFLAGS) -o $@ -c display/windows_lua.c

$(OBJDIR)/rawfb_lua.lo: display/rawfb_lua.c
	$(CC) $(LIBCFLAGS) $(RAWFB_DEFS) -o $@ -c display/rawfb_lua.c

###############################################################################
#
#	Lua/tekUI standalone executable:

LUAEXE_TEKUI = $(BASEDIR)/bin/tekui.exe
LUAEXE_BUILD = $(BASEDIR)/bin/lua_build.exe

$(LUAEXE_BUILD): linit_tekui.c
	$(CC) -I$(LUADISTDIR)/src $(CPPFLAGS) $(CFLAGS) -o $@ $(LUADISTDIR)/src/lua.c linit_tekui.c -lm

tekui_classlib.c: $(LUAEXE_BUILD)
	cd $(BASEDIR) && $(EXECUTE) ./bin/lua_build.exe bin/compiler.lua -o tek/lib/tekui_classlib.c -l $$(cat tek/lib/MODLIST) -i -c -s

$(OBJDIR)/linit_tekui_classlib.lo: linit_tekui.c tekui_classlib.c
	$(CC) $(LIBCFLAGS) $(LUAEXE_DEFS) -I $(LUADISTDIR)/src -I../../src/hal -I../../display_$(DISPLAY_DRIVER) -DLUA_TEKUI_INCLUDE_CLASS_LIBRARY -o $@ -c linit_tekui.c 

$(LUAEXE_TEKUI): $(OBJDIR)/linit_tekui_classlib.lo luamain.c
	$(CC) -I$(LUADISTDIR)/src -o $@ luamain.c $(OBJDIR)/linit_tekui_classlib.lo -L$(BASEDIR) -L$(LIBDIR) $(TEKUI_LIBS) $(LUAEXE_LIBS)
# 	$(STRIP) $@

###############################################################################

dll: $(OBJDIR)

tools: dll $(OBJDIR) $(LUAEXE_TEKUI)

modules: $(OBJDIR) $(MODS) $(DISPLAYS)

install:
	$(INSTALL_D) $(LUA_LIB)/tek/lib/display
	$(INSTALL_S) $(MODS) $(LUA_LIB)/tek/lib
	$(INSTALL_S) $(DISPLAYS) $(LUA_LIB)/tek/lib/display
	$(INSTALL_D) $(LUA_SHARE)/tek/lib
	$(INSTALL_F) debug.lua args.lua $(LUA_SHARE)/tek/lib

clean: FORCE
	-$(RM) $(MODS) $(DISPLAYS) $(LIBS) $(LUAEXE_TEKUI) $(LUAEXE_BUILD)
	-$(RMDIR) $(OBJDIR)
