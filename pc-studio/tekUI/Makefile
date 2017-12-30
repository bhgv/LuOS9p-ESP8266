
include config

.PHONY: all libs modules install clean help distclean docs release

all: libs modules # tools

libs:
	cd src && $(MAKE) $@

dll: modules
	cd tek/lib && $(MAKE) $@

modules: libs
	cd src && $(MAKE) $@
	cd tek/lib && $(MAKE) $@
	cd tek/ui && $(MAKE) $@

tools: modules dll
	cd src && $(MAKE) $@
	cd tek/lib && $(MAKE) $@

install:
	cd tek && $(MAKE) $@
	cd tek/lib && $(MAKE) $@
	cd tek/ui && $(MAKE) $@

clean:
	cd src && $(MAKE) $@
	cd tek/lib && $(MAKE) $@
	cd tek/ui && $(MAKE) $@
	-rm tek/lib/tekui_classlib.c

help: default-help
	@echo "Extra build targets for this Makefile:"
	@echo "-------------------------------------------------------------------------------"
	@echo "tools ................... build standalone tekui executable"
	@echo "distclean ............... remove all temporary files and directories"
	@echo "docs .................... (re-)generate documentation"
	@echo "kdiff ................... diffview of recent changes (using kdiff3)"
	@echo "==============================================================================="

distclean: clean
	-$(RMDIR) lib bin/mod
	-find src tek -type d -name build | xargs $(RMDIR)
	-find -name "*$(DLLEXT)" -type f | xargs $(RM)

docs:
	bin/gendoc.lua README -r manual.html --header VERSION -i 32 -n "tekUI" > doc/index.html
	bin/gendoc.lua COPYRIGHT -i 32 -n "tekUI Copyright" > doc/copyright.html
	bin/gendoc.lua TODO -i 32 -n "tekUI TODO" > doc/todo.html
	bin/gendoc.lua CHANGES -i 32 -r manual.html -n "tekUI Changelog" > doc/changes.html
	bin/gendoc.lua tek/ --header VERSION --adddate -n "tekUI Class Reference Manual" > doc/manual.html

text-docs:
	@bin/gendoc.lua README -p -i 32 > doc/readme.txt
	@bin/gendoc.lua COPYRIGHT -p -i 32 > doc/copyright.txt
	@bin/gendoc.lua TODO -p -i 32 > doc/todo.txt
	@bin/gendoc.lua CHANGES -p -i 32 > doc/changes.txt
	@bin/gendoc.lua tek/ -p -n "tekUI Reference manual" | tr "\t" " " > doc/manual.txt

kdiff:
	-(a=$$(mktemp -du) && hg clone $$PWD $$a && kdiff3 $$a $$PWD; rm -rf $$a)
