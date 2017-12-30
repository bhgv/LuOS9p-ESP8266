
BASEDIR ?= ../..
include $(BASEDIR)/config

EXTRADEFS += $(DIRECTFB_DEFS) -DDEF_FONTDIR=\"$(LUA_SHARE)/tek/ui/font/\" \
	-DDEF_CURSORFILE=\"$(LUA_SHARE)/tek/ui/cursor/cursor-green.png\"

###############################################################################

LIBS = $(LIBDIR)/libdisplay_directfb.a

$(OBJDIR)/display_dfb_mod.lo: \
	display_dfb_mod.c display_dfb_mod.h
	$(CC) $(LIBCFLAGS) -o $@ -c display_dfb_mod.c
$(OBJDIR)/display_dfb_api.lo: \
	display_dfb_api.c display_dfb_mod.h
	$(CC) $(LIBCFLAGS) -o $@ -c display_dfb_api.c
$(OBJDIR)/display_dfb_font.lo: \
	display_dfb_font.c display_dfb_mod.h
	$(CC) $(LIBCFLAGS) -o $@ -c display_dfb_font.c

$(LIBDIR)/libdisplay_directfb.a: \
	$(OBJDIR)/display_dfb_mod.lo $(OBJDIR)/display_dfb_api.lo \
	$(OBJDIR)/display_dfb_font.lo
	$(AR) $@ $?

###############################################################################

libs: $(LIBDIR) $(OBJDIR) $(LIBS)

modules:

tools:

clean: FORCE
	-$(RM) $(MODS) $(TOOLS) $(LIBS)
	-$(RMDIR) $(OBJDIR)
