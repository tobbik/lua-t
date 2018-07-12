# vim: ft=make ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
CURL=$(shell which curl)
UNZIP=$(shell which unzip)

LVER=5.3
LREL=4
LRL=$(LVER).$(LREL)
LUASRC=lua-$(LRL).tar.gz
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

# create a local Lua installation
$(DLDIR):
	mkdir -p $(DLDIR)

$(DLDIR)/$(LUASRC): $(DLDIR)
	$(CURL) -o $(DLDIR)/$(LUASRC)   $(LUAURL)/$(LUASRC)

$(COMPDIR)/$(LVER)/src: $(DLDIR)/$(LUASRC)
	mkdir -p $(COMPDIR)/$(LVER)
	tar -xvzf $(DLDIR)/$(LUASRC) -C $(COMPDIR)/$(LVER) --strip-components=1
	sed -i "s/-O2 //" $(COMPDIR)/$(LVER)/src/Makefile

$(COMPDIR)/$(LVER)/src/lua: $(COMPDIR)/$(LVER)/src
	$(MAKE) -C $(COMPDIR)/$(LVER) -j 4 CC=$(CC) LD=$(LD) \
		MYCFLAGS="$(MYCFLAGS)" \
		linux

$(PREFIX)/bin/lua: $(COMPDIR)/$(LVER)/src/lua
	$(MAKE) -C $(COMPDIR)/$(LVER) CC=$(CC) LD=$(LD) \
		LVER=$(LVER) \
		INSTALL_TOP=$(PREFIX) \
		install


# get a local PenLight installation
$(DLDIR)/$(PLSRC): $(DLDIR)
	$(CURL) -o $(DLDIR)/$(PLSRC) $(PLURL)

$(COMPDIR)/$(PLNAME): $(DLDIR)/$(PLSRC) $(COMPDIR)/$(LVER)/src
	mkdir -p $(COMPDIR)/$(PLNAME)
	tar -xvzf $(DLDIR)/$(PLSRC) -C $(COMPDIR)/$(PLNAME) --strip-components=1

$(PLINSTALL): $(COMPDIR)/$(PLNAME) $(PREFIX)/bin/lua
	mkdir -p $(PLINSTALL)
	cp -apr  $(COMPDIR)/$(PLNAME)/lua/pl $(PREFIX)/share/lua/$(LVER)

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

dev-rm:
	-rm -rf $(PREFIX)

dev-rinse:
	$(MAKE) dev-clean
	$(MAKE) dev-rm

dev-pristine:
	$(MAKE) dev-rinse
	-rm -rf $(COMPDIR) $(DLDIR)

dev-test:
	$(MAKE) CC=$(CC) LD=$(LD) \
		LVER=$(LVER) \
		MYCFLAGS="$(MYCFLAGS)" \
		INCDIR="$(LUAINC)" test

dev-run: $(TINSTALL)
	LUA_PATH="$(CURDIR)/out/share/lua/5.3/?.lua;;" \
	  LUA_CPATH="$(CURDIR)/out/lib/lua/5.3/?.so;;" \
	  $(CURDIR)/out/bin/lua
