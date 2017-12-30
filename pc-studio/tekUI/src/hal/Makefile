
BASEDIR ?= ../..
include $(BASEDIR)/config

###############################################################################

LIBS = $(LIBDIR)/libhal.a

$(OBJDIR)/hal_mod.lo: hal_mod.c hal_mod.h $(INCDIR)/tek/mod/hal.h
	$(CC) $(LIBCFLAGS) -o $@ -c hal_mod.c
$(OBJDIR)/hal.lo: $(PLATFORM)/hal.c hal_mod.h $(INCDIR)/tek/mod/hal.h
	$(CC) $(LIBCFLAGS) -o $@ -c $(PLATFORM)/hal.c

$(LIBDIR)/libhal.a: $(OBJDIR)/hal_mod.lo $(OBJDIR)/hal.lo
	$(AR) $@ $?

###############################################################################

libs: $(LIBDIR) $(OBJDIR) $(LIBS)

modules: # $(OBJDIR) $(MODDIR) $(MODS)

tools:

clean: FORCE
	-$(RM) $(MODS) $(TOOLS) $(LIBS)
	-$(RMDIR) $(OBJDIR)
