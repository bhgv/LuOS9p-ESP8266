
BASEDIR ?= ../..
include $(BASEDIR)/config

EXTRADEFS += $(RAWFB_DEFS) $(TEKUI_DEFS)

###############################################################################

LIBS = $(LIBDIR)/libdisplay_rawfb.a

# $(OBJDIR)/display_rfb_mod.lo: \
# 	display_rfb_mod.c display_rfb_mod.h
# 	$(CC) $(LIBCFLAGS) -o $@ -c display_rfb_mod.c
# $(OBJDIR)/display_rfb_api.lo: \
# 	display_rfb_api.c display_rfb_mod.h
# 	$(CC) $(LIBCFLAGS) -o $@ -c display_rfb_api.c
# $(OBJDIR)/display_rfb_font.lo: \
# 	display_rfb_font.c display_rfb_mod.h
# 	$(CC) $(LIBCFLAGS) -o $@ -c display_rfb_font.c
# $(OBJDIR)/display_rfb_draw.lo: \
# 	display_rfb_draw.c display_rfb_mod.h
# 	$(CC) $(LIBCFLAGS) -o $@ -c display_rfb_draw.c
# $(OBJDIR)/display_rfb_linux.lo: \
# 	display_rfb_linux.c display_rfb_mod.h
# 	$(CC) $(LIBCFLAGS) -o $@ -c display_rfb_linux.c
# 
# $(LIBDIR)/libdisplay_rawfb.a: \
# 	$(OBJDIR)/display_rfb_mod.lo $(OBJDIR)/display_rfb_api.lo \
# 	$(OBJDIR)/display_rfb_draw.lo $(OBJDIR)/display_rfb_font.lo \
# 	$(OBJDIR)/display_rfb_linux.lo
# 	$(AR) $@ $?
	
$(OBJDIR)/display_rfb_all.lo: \
	display_rfb_all.c display_rfb_mod.h display_rfb_mod.c \
	display_rfb_api.c display_rfb_font.c display_rfb_draw.c \
	display_rfb_linux.c vnc/display_rfb_vnc.c ../../config
	$(CC) $(LIBCFLAGS) -o $@ -c display_rfb_all.c

$(LIBDIR)/libdisplay_rawfb.a: \
	$(OBJDIR)/display_rfb_all.lo
	$(AR) $@ $?

###############################################################################

libs: $(LIBDIR) $(OBJDIR) $(LIBS)

tools:

modules:

clean: FORCE
	-$(RM) $(MODS) $(TOOLS) $(LIBS)
	-$(RMDIR) $(OBJDIR)
