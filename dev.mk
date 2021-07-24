# vim: ft=make ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
CURL=$(shell which curl)
TAR=$(shell which tar)
UNZIP=$(shell which unzip)

MYCFLAGS=-g -O2 -Winline
UNAME_M := $(shell uname -m)
ifeq ($(UNAME_M),x86_64)
   MYCFLAGS += -D AMD64 -march=native
endif
ifneq ($(filter %86,$(UNAME_M)),)
   MYCFLAGS += -D IA32 -march=native
endif
ifneq ($(filter arm%,$(UNAME_M)),)
ifneq ($(filter armv8l%,$(UNAME_M)),)
   # RockChip RK3399 specific flags
   MYCFLAGS += -m32 -O2 -mthumb -march=armv8-a -mcpu=cortex-a72 -mtune=cortex-a72.cortex-a53 -mfloat-abi=softfp
else
   MYCFLAGS += -D ARM -fbuiltin -march=native -pipe -fstack-protector-strong -fno-plt -O0
endif
endif
ifneq ($(filter aarch%,$(UNAME_M)),)
   MYCFLAGS += -D ARM -fbuiltin -march=armv8-a -pipe -fstack-protector-strong -fno-plt -O0
endif

LVER=5.4
LREL=3
LUASRC=lua-$(LVER).$(LREL).tar.gz
LUAURL=https://www.lua.org/ftp

COMPDIR=$(CURDIR)/compile
PREFIX=$(CURDIR)/out
DLDIR=$(CURDIR)/download
INCDIR=$(PREFIX)/include

CC=clang
LD=clang


all: $(PREFIX)/bin/lua

# create a local Lua installation
$(DLDIR):
	mkdir -p $@

$(DLDIR)/$(LUASRC): $(DLDIR)
	$(CURL) -o $@   $(LUAURL)/$(LUASRC)

$(COMPDIR)/$(LVER): $(DLDIR)/$(LUASRC)
	mkdir -p $@
	$(TAR) -xzf $< -C $@ --strip-components=1
	sed -i "s/-O2 //" $@/src/Makefile

$(COMPDIR)/$(LVER)/src/lua: $(COMPDIR)/$(LVER)
	$(MAKE) -C $< -j 4 CC=$(CC) LD=$(LD) \
		MYCFLAGS="$(MYCFLAGS)" \
		linux-readline

$(PREFIX)/bin/lua: $(COMPDIR)/$(LVER)/src/lua
	$(MAKE) -C $(COMPDIR)/$(LVER) CC=$(CC) LD=$(LD) \
		LVER=$(LVER) \
		INSTALL_TOP=$(PREFIX) \
		install

dev-clean:
	$(MAKE) CC=$(CC) LD=$(LD) \
		LVER=$(LVER) \
		MYCFLAGS="$(MYCFLAGS)" \
		INCDIR="$(INCDIR)" \
		BUILD_EXAMPLE=1 \
		DEBUG=1 \
		PREFIX="$(PREFIX)" clean
	$(MAKE) -C http clean

dev-rinse:
	$(MAKE) dev-clean
	-$(RM) -r $(COMPDIR)/*
	-$(RM) -r $(PREFIX)

dev-pristine:
	$(MAKE) dev-rinse
	-$(RM) -r $(DLDIR)

dev: $(PREFIX)/bin/lua
	$(MAKE)  -j4 CC=$(CC) LD=$(LD) \
	 LVER=$(LVER) LREL=$(LREL) LUASRC=$(LUASRC) LUAURL=$(LUAURL) \
	 MYCFLAGS="$(MYCFLAGS)" \
	 INCDIR="$(INCDIR)" \
	 LDFLAGS="$(LDFLAGS)" \
	 BUILD_EXAMPLE=1 \
	 DEBUG=1 \
	 PREFIX="$(PREFIX)" install

dev-run:
	$(MAKE) dev
	time LUA_PATH="$(CURDIR)/out/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(CURDIR)/out/lib/lua/5.4/?.so;;" \
	 $(CURDIR)/out/bin/lua scp.lua

dev-exec:
	$(MAKE) dev
	LUA_PATH="$(CURDIR)/out/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(CURDIR)/out/lib/lua/5.4/?.so;;" \
	 $(CURDIR)/out/bin/lua -i tst.lua

dev-test:
	$(MAKE) dev
	LUA_PATH="$(CURDIR)/out/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(CURDIR)/out/lib/lua/5.4/?.so;;" \
	 $(CURDIR)/out/bin/lua -i $(CURDIR)/test/runner.lua\

dev-t1:
	$(MAKE) dev
	LUA_PATH="$(CURDIR)/out/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(CURDIR)/out/lib/lua/5.4/?.so;;" \
	 $(CURDIR)/out/bin/lua -i $(CURDIR)/test/t1.lua

dev-gdb:
	$(MAKE) dev
	LUA_PATH="$(CURDIR)/out/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(CURDIR)/out/lib/lua/5.4/?.so;;" \
	 gdb --args $(CURDIR)/out/bin/lua -i tn1.lua

dev-example:
	$(MAKE) dev
	LUA_PATH="$(CURDIR)/out/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(CURDIR)/out/lib/lua/5.4/?.so;;" \
	 $(CURDIR)/out/bin/lua -i example/t_net_ifc.lua


run-dev:
	LUA_PATH="$(CURDIR)/out/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(CURDIR)/out/lib/lua/5.4/?.so;;" \
	 $(CURDIR)/out/bin/lua -i scp.lua
