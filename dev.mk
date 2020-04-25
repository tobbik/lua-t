# vim: ft=make ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
CURL=$(shell which curl)
TAR=$(shell which tar)
UNZIP=$(shell which unzip)

LVER=5.4
LREL=0
LUASRC=lua-$(LVER).$(LREL)-rc1.tar.gz
LUAURL=https://www.lua.org/work
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

MYCFLAGS=-g -O2 -fbuiltin -march=armv8-a -pipe -fstack-protector-strong -fno-plt

CC=clang
LD=clang

dev-all: $(TINSTALL)

dev-54:
	$(MAKE) \
		LVER=5.4 LREL=0 LUASRC=lua-5.4.0-rc1.tar.gz LUAURL=http://www.lua.org/work \
		dev-all

dev-arm:
	$(MAKE) LVER=5.4 LREL=0 LUASRC=lua-5.4.0-work2.tar.gz LUAURL=http://www.lua.org/work \
	  MYCFLAGS=" -m32 -O2 -mthumb -march=armv8-a -mcpu=cortex-a72 -mtune=cortex-a72.cortex-a53 -mfloat-abi=softfp" \
	  CC=clang LD=clang \
	  LDFLAGS="$(LDFLAGS)" \
	  INCS="$(LUAINC)" \
	  PREFIX="$(PREFIX)" \
	  DEBUG=1 BUILD_EXAMPLE=1 install

# create a local Lua installation
$(DLDIR):
	mkdir -p $@

$(DLDIR)/$(LUASRC): $(DLDIR)
	$(CURL) -o $@   $(LUAURL)/$(LUASRC)

$(COMPDIR)/$(LVER): $(DLDIR)/$(LUASRC)
	mkdir -p $@
	$(TAR) -xzf $< -C $@ --strip-components=1
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
	$(TAR) -xzf $< -C $@ --strip-components=1

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

dev-rinse:
	$(MAKE) dev-clean
	-$(RM) -r $(COMPDIR)/*
	-$(RM) -r $(PREFIX)/*

dev-pristine:
	$(MAKE) dev-rinse
	-$(RM) -r $(DLDIR)

dev-run: $(TINSTALL)
	LUA_PATH="$(CURDIR)/out/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(CURDIR)/out/lib/lua/5.4/?.so;;" \
	 $(CURDIR)/out/bin/lua -i scratchpad.lua

dev-test: $(TINSTALL)
	LUA_PATH="$(CURDIR)/out/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(CURDIR)/out/lib/lua/5.4/?.so;;" \
	 $(CURDIR)/out/bin/lua -i $(CURDIR)/test/runner.lua buf
