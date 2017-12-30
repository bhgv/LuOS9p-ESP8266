
BASEDIR ?= ../..
include $(BASEDIR)/config

EXTRADEFS += $(TEKLIB_DEFS)

###############################################################################

LIBS = \
	$(LIBDIR)/libtekdebug.a \
	$(LIBDIR)/libtekcmain.a \
	$(LIBDIR)/libtekc.a \
	$(LIBDIR)/libtek.a \
	$(LIBDIR)/libtekstring.a \

$(OBJDIR)/init.lo: init.c $(INCDIR)/tek/lib/init.h
	$(CC) $(LIBCFLAGS) -o $@ -c init.c
$(OBJDIR)/teklib.lo: teklib.c
	$(CC) $(LIBCFLAGS) -o $@ -c teklib.c
$(OBJDIR)/debug.lo: debug.c
	$(CC) $(LIBCFLAGS) -o $@ -c debug.c
$(OBJDIR)/main.lo: $(PLATFORM)/main.c $(INCDIR)/tek/lib/init.h
	$(CC) $(LIBCFLAGS) -o $@ -c $(PLATFORM)/main.c
$(OBJDIR)/host.lo: $(PLATFORM)/host.c $(INCDIR)/tek/lib/init.h
	$(CC) $(LIBCFLAGS) -o $@ -c $(PLATFORM)/host.c
$(OBJDIR)/string.lo: string.c
	$(CC) $(LIBCFLAGS) -o $@ -c string.c

$(LIBDIR)/libtekdebug.a: \
	$(OBJDIR)/debug.lo
	$(AR) $@ $?
$(LIBDIR)/libtekcmain.a: \
	$(OBJDIR)/init.lo $(OBJDIR)/teklib.lo $(OBJDIR)/host.lo $(OBJDIR)/main.lo
	$(AR) $@ $?
$(LIBDIR)/libtekc.a: \
	$(OBJDIR)/init.lo $(OBJDIR)/teklib.lo $(OBJDIR)/host.lo
	$(AR) $@ $?
$(LIBDIR)/libtek.a: \
	$(OBJDIR)/teklib.lo
	$(AR) $@ $?
$(LIBDIR)/libtekstring.a: \
	$(OBJDIR)/string.lo
	$(AR) $@ $?

###############################################################################

libs: $(LIBDIR) $(OBJDIR) $(LIBS)

tools:

modules:

clean: FORCE
	-$(RM) $(MODS) $(TOOLS) $(LIBS)
	-$(RMDIR) $(OBJDIR)
