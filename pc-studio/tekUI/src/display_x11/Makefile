
BASEDIR ?= ../..
include $(BASEDIR)/config

EXTRADEFS += $(X11_DEFS) $(TEKLIB_DEFS)

###############################################################################

LIBS = $(LIBDIR)/libdisplay_x11.a

# $(OBJDIR)/display_x11_mod.lo: \
# 	display_x11_mod.c display_x11_mod.h
# 	$(CC) $(LIBCFLAGS) -o $@ -c display_x11_mod.c
# $(OBJDIR)/display_x11_api.lo: \
# 	display_x11_api.c display_x11_mod.h
# 	$(CC) $(LIBCFLAGS) -o $@ -c display_x11_api.c
# $(OBJDIR)/display_x11_font.lo: \
# 	display_x11_font.c display_x11_mod.h
# 	$(CC) $(LIBCFLAGS) -o $@ -c display_x11_font.c
# $(OBJDIR)/display_x11_inst.lo: \
# 	display_x11_inst.c display_x11_mod.h
# 	$(CC) $(LIBCFLAGS) -o $@ -c display_x11_inst.c
# 
# $(LIBDIR)/libdisplay_x11.a: \
# 	$(OBJDIR)/display_x11_mod.lo $(OBJDIR)/display_x11_api.lo \
# 	$(OBJDIR)/display_x11_inst.lo $(OBJDIR)/display_x11_font.lo
# 	$(AR) $@ $?

$(OBJDIR)/display_x11_all.lo: \
	display_x11_all.c display_x11_mod.h display_x11_mod.c display_x11_api.c \
	display_x11_inst.c display_x11_font.c ../../config
	$(CC) $(LIBCFLAGS) -o $@ -c display_x11_all.c

$(LIBDIR)/libdisplay_x11.a: \
	$(OBJDIR)/display_x11_all.lo
	$(AR) $@ $?

###############################################################################

libs: $(LIBDIR) $(OBJDIR) $(LIBS)

tools:

modules:

clean: FORCE
	-$(RM) $(MODS) $(TOOLS) $(LIBS)
	-$(RMDIR) $(OBJDIR)
