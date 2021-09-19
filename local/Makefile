# vim: ft=make ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
CURL!= which curl
TAR != which tar
RM != which rm

LVER=5.4
LREL=3
LUASRC=lua-$(LVER).$(LREL).tar.gz
LUAURL=https://www.lua.org/ftp

CURDIR != pwd

all: $(CURDIR)/bin/lua

# create a local Lua installation
$(LUASRC):
	$(CURL) -o $@   $(LUAURL)/$(LUASRC)

$(LVER): $(LUASRC)
	mkdir -p $@
	$(TAR) -xzf $(LUASRC) -C $@ --strip-components=1
	sed "s/-O2 //" < $@/src/Makefile > $@/src/Makefile.bak \
	  && mv $@/src/Makefile.bak $@/src/Makefile

$(LVER)/src/lua: $(LVER)
	$(MAKE) -C $(LVER) -j 4 CC=$(CC) LD=$(LD) \
		guess
	#	linux-readline

$(CURDIR)/bin/lua: $(LVER)/src/lua
	$(MAKE) -C $(LVER) \
		INSTALL_TOP=$(CURDIR) \
		install

clean:
	$(MAKE) -C $(LVER) clean

uninstall:
	-$(RM) -rf bin include lib man share

nuke:
	-$(RM) -rf $(LVER)

pristine:
	-$(RM) -f $(LUASRC)

echo:
	@echo "PLAT= $(PLAT)"
	@echo "LVER= $(LVER)"
	@echo "PREFIX= $(PREFIX)"
	@echo "CC= $(CC)"
	@echo "LD= $(LD)"
	@echo "INCDIR= $(INCDIR)"
	@echo "LDFLAGS= $(LDFLAGS)"
	@echo "CFLAGS= $(CFLAGS)"
	@echo "LDLIBS= $(LDLIBS)"
	@echo "MYCFLAGS= $(MYCFLAGS)"
	@echo "DLDIR= $(DLDIR)"
	@echo "LUASRC= $(LUASRC)"


