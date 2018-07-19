# vim: ft=make ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
CURL=$(shell which curl)
UNZIP=$(shell which unzip)

LVER=5.3
LREL=4
LUASRC=lua-$(LVER).$(LREL).tar.gz
LUAURL=http://www.lua.org/ftp
PLURL=https://codeload.github.com/stevedonovan/Penlight/tar.gz/master


COMPDIR=$(CURDIR)/compile
PREFIX=$(CURDIR)/out
DLDIR=$(CURDIR)/download
LUAINC=$(PREFIX)/include
PLNAME=Penlight-master
PLSRC=$(PLNAME).tar.gz
PLZIP=$(DLDIR)/$(PLNAME).tar.gz
PLINSTALL=$(PREFIX)/share/lua/$(LVER)/pl
TINSTALL=$(PREFIX)/lib/lua/$(LVER)/t

MYCFLAGS=" -O0 -g"

all: $(PREFIX)/bin/lua


MYCFLAGS=-g -O2 -fbuiltin

CC=clang
LD=clang

dev-all: $(TINSTALL)

dev-54:
	$(MAKE) \
		LVER=5.4 LREL=0 LUASRC=lua-5.4.0-work2.tar.gz LUAURL=http://www.lua.org/work \
		dev-all

# create a local Lua installation
$(DLDIR):
	mkdir -p $@

$(DLDIR)/$(LUASRC): $(DLDIR)
	$(CURL) -o $<   $(LUAURL)/$(LUASRC)

$(COMPDIR)/$(LVER): $(DLDIR)/$(LUASRC)
	mkdir -p $@
	tar -xvzf $< -C $@ --strip-components=1
	sed -i "s/-O2 //" $@/src/Makefile

$(COMPDIR)/5.3/src/lua: $(COMPDIR)/5.3
	$(MAKE) -C $< -j 4 CC=$(CC) LD=$(LD) \
		MYCFLAGS="$(MYCFLAGS)" \
		linux

$(COMPDIR)/5.4/src/lua: $(COMPDIR)/5.4
	$(MAKE) -C $< -j 4 CC=$(CC) LD=$(LD) \
		MYCFLAGS="$(MYCFLAGS)" \
		linux-readline

$(PREFIX)/bin/lua: $(COMPDIR)/$(LVER)/src/lua
	$(MAKE) -C $(COMPDIR)/$(LVER) CC=$(CC) LD=$(LD) \
		LVER=$(LVER) \
		INSTALL_TOP=$(PREFIX) \
		install


# get a local PenLight installation
$(DLDIR)/$(PLSRC): $(DLDIR)
	$(CURL) -o $@ $(PLURL)

$(COMPDIR)/$(PLNAME): $(DLDIR)/$(PLSRC) $(COMPDIR)/$(LVER)
	mkdir -p $@
	tar -xvzf $< -C $@ --strip-components=1

$(PLINSTALL): $(COMPDIR)/$(PLNAME) $(PREFIX)/bin/lua
	mkdir -p $@
	cp -apr $</lua/pl $(PREFIX)/share/lua/$(LVER)

# compile and install lua-t in local Lua
dev-build:
	$(MAKE) CC=$(CC) LD=$(LD) \
		LVER=$(LVER) \
		MYCFLAGS="$(MYCFLAGS)" \
		INCDIR="$(LUAINC)" \
		BUILD_EXAMPLE=1 \
		DEBUG=1 \
		PREFIX="$(PREFIX)" all

$(TINSTALL): $(PLINSTALL)
	$(MAKE)  CC=$(CC) LD=$(LD) \
		LVER=$(LVER) \
		MYCFLAGS="$(MYCFLAGS)" \
		INCDIR="$(LUAINC)" \
		BUILD_EXAMPLE=1 \
		DEBUG=1 \
		PREFIX="$(PREFIX)" install

dev-clean:
	$(MAKE) CC=$(CC) LD=$(LD) \
		LVER=$(LVER) \
		MYCFLAGS="$(MYCFLAGS)" \
		INCDIR="$(LUAINC)" \
		BUILD_EXAMPLE=1 \
		DEBUG=1 \
		PREFIX="$(PREFIX)" clean

dev-rmout:
	-rm -rf $(PREFIX)

dev-rinse:
	$(MAKE) dev-clean
	$(MAKE) dev-rmout

dev-rm:
	$(MAKE) dev-rinse
	-rm -rf $(COMPDIR)

dev-pristine:
	$(MAKE) dev-rinse
	$(MAKE) dev-rmcomp
	-rm -rf $(DLDIR)

dev-test:
	$(MAKE) CC=$(CC) LD=$(LD) \
		LVER=$(LVER) \
		MYCFLAGS="$(MYCFLAGS)" \
		INCDIR="$(LUAINC)" test

dev-run: $(TINSTALL)
	LUA_PATH="$(CURDIR)/out/share/lua/5.3/?.lua;;" \
	  LUA_CPATH="$(CURDIR)/out/lib/lua/5.3/?.so;;" \
	  $(CURDIR)/out/bin/lua
