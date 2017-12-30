
BASEDIR ?= ../..
include $(BASEDIR)/config

EXTRADEFS += $(WINDOWS_DEFS)

###############################################################################

LIBS = $(LIBDIR)/libdisplay_win.a

$(OBJDIR)/display_win_mod.lo: \
	display_win_mod.c display_win_mod.h
	$(CC) $(LIBCFLAGS) -o $@ -c display_win_mod.c
$(OBJDIR)/display_win_api.lo: \
	display_win_api.c display_win_mod.h
	$(CC) $(LIBCFLAGS) -o $@ -c display_win_api.c
$(OBJDIR)/display_win_font.lo: \
	display_win_font.c display_win_mod.h
	$(CC) $(LIBCFLAGS) -o $@ -c display_win_font.c

$(LIBDIR)/libdisplay_win.a: \
	$(OBJDIR)/display_win_mod.lo $(OBJDIR)/display_win_api.lo \
	$(OBJDIR)/display_win_font.lo
	$(AR) $@ $?

###############################################################################

libs: $(LIBDIR) $(OBJDIR) $(LIBS)

tools:

modules:

clean: FORCE
	-$(RM) $(MODS) $(TOOLS) $(LIBS)
	-$(RMDIR) $(OBJDIR)
