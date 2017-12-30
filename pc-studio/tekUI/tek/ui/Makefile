
BASEDIR ?= ../..
include $(BASEDIR)/config

EXTRADEFS += $(LUA_DEFS)

CLASSES = \
	class/application.lua \
	class/border.lua \
	class/button.lua \
	class/canvas.lua \
	class/checkmark.lua \
	class/dirlist.lua \
	class/display.lua \
	class/drawhook.lua \
	class/element.lua \
	class/family.lua \
	class/floattext.lua \
	class/widget.lua \
	class/gauge.lua \
	class/group.lua \
	class/handle.lua \
	class/image.lua \
	class/imagewidget.lua \
	class/input.lua \
	class/layout.lua \
	class/lister.lua \
	class/listview.lua \
	class/menuitem.lua \
	class/numeric.lua \
	class/pagegroup.lua \
	class/popitem.lua \
	class/poplist.lua \
	class/popupwindow.lua \
	class/radiobutton.lua \
	class/scrollbar.lua \
	class/scrollgroup.lua \
	class/sizeable.lua \
	class/slider.lua \
	class/spacer.lua \
	class/text.lua \
	class/textedit.lua \
	class/window.lua
BORDERS = border/default.lua
HOOKS = hook/ripple.lua
IMAGES = \
	image/arrowdown.lua \
	image/arrowleft.lua \
	image/arrowright.lua \
	image/arrowup.lua \
	image/checkmark.lua \
	image/file.lua \
	image/radiobutton.lua
STYLES = \
	style/desktop.css \
	style/default.css \
	style/klinik.css \
	style/stain.css \
	style/desktop.lua \
	style/gradient.css \
	style/industrial.css
FONTS = \
	font/Vera.ttf \
	font/VeraMono.ttf
CURSORS = cursor/cursor-green.png
CATALOGS1 = \
	locale/schulze-mueller.de/dirlist-class/en \
	locale/schulze-mueller.de/dirlist-class/de

###############################################################################

MODS_C = class/area$(DLLEXT) class/frame$(DLLEXT)
MODS_L = layout/default$(DLLEXT)

layout/default$(DLLEXT): $(OBJDIR)/default.lo
	$(CC) $(MODCFLAGS) -o $@ $(OBJDIR)/default.lo $(LUA_LIBS)
$(OBJDIR)/default.lo: layout/default.c
	$(CC) $(LIBCFLAGS) -o $@ -c layout/default.c

class/area$(DLLEXT): $(OBJDIR)/area.lo
	$(CC) $(MODCFLAGS) -o $@ $(OBJDIR)/area.lo $(LUA_LIBS)
$(OBJDIR)/area.lo: class/area.c
	$(CC) $(LIBCFLAGS) -o $@ -c class/area.c

class/frame$(DLLEXT): $(OBJDIR)/frame.lo
	$(CC) $(MODCFLAGS) -o $@ $(OBJDIR)/frame.lo $(LUA_LIBS)
$(OBJDIR)/frame.lo: class/frame.c
	$(CC) $(LIBCFLAGS) -o $@ -c class/frame.c

###############################################################################

modules: $(OBJDIR) $(MODS_C) $(MODS_L)

install:
	$(INSTALL_D) $(LUA_LIB)/tek/ui/layout
	$(INSTALL_D) $(LUA_LIB)/tek/ui/class
	$(INSTALL_D) $(LUA_SHARE)/tek/ui/layout
	$(INSTALL_D) $(LUA_SHARE)/tek/ui/border
	$(INSTALL_D) $(LUA_SHARE)/tek/ui/hook
	$(INSTALL_D) $(LUA_SHARE)/tek/ui/image
	$(INSTALL_D) $(LUA_SHARE)/tek/ui/class
	$(INSTALL_D) $(LUA_SHARE)/tek/ui/font
	$(INSTALL_D) $(LUA_SHARE)/tek/ui/cursor
	$(INSTALL_D) $(LUA_SHARE)/tek/ui/style
	$(INSTALL_D) $(LUA_SHARE)/tek/ui/locale/schulze-mueller.de/dirlist-class
	$(INSTALL_S) $(MODS_L) $(LUA_LIB)/tek/ui/layout
	$(INSTALL_S) $(MODS_C) $(LUA_LIB)/tek/ui/class
	$(INSTALL_F) layout/fixed.lua $(LUA_SHARE)/tek/ui/layout
	$(INSTALL_F) $(CLASSES) $(LUA_SHARE)/tek/ui/class
	$(INSTALL_F) $(BORDERS) $(LUA_SHARE)/tek/ui/border
	$(INSTALL_F) $(HOOKS) $(LUA_SHARE)/tek/ui/hook
	$(INSTALL_F) $(IMAGES) $(LUA_SHARE)/tek/ui/image
	$(INSTALL_F) $(FONTS) $(LUA_SHARE)/tek/ui/font
	$(INSTALL_F) $(CURSORS) $(LUA_SHARE)/tek/ui/cursor
	$(INSTALL_F) $(STYLES) $(LUA_SHARE)/tek/ui/style
	$(INSTALL_F) $(CATALOGS1) $(LUA_SHARE)/tek/ui/locale/schulze-mueller.de/dirlist-class

clean: FORCE
	-$(RM) $(MODS_C) $(MODS_L) $(LIBS)
	-$(RMDIR) $(OBJDIR)
