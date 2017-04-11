
# vim: ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
#
# \file      src/Makefile
# \brief     Makefile for the lua-t library source code
# \author    tkieslich
# \copyright See Copyright notice at the end of t.h
#
LVER=5.3
PREFIX=$(shell pkg-config --variable=prefix lua)
INSTALL_LMOD=$(PREFIX)/share/lua/$(LVER)/t

# How to install. If your install program does not support "-p", then
# you may have to run ranlib on the installed t.a
INSTALL= install -p
INSTALL_EXEC= $(INSTALL) -m 0755
INSTALL_DATA= $(INSTALL) -m 0644

T_SRC=Buffer.lua \
   Buffer/Segment.lua \
   Encode.lua \
   Encode/Base64.lua \
   Encode/Crc.lua \
   Encode/RC4.lua \
   Http.lua \
   Http/Connection.lua \
   Http/Server.lua \
   Http/Stream.lua \
   Http/WebSocket.lua \
   Loop.lua \
   Net.lua \
   Net/Interface.lua \
   Net/Address.lua \
   Net/Socket.lua \
   OrderedHashTable.lua \
   Pack.lua \
   Pack/Field.lua \
   Set.lua \
   Table.lua \
   Test.lua \
   Test/Case.lua \
   Time.lua

T_INSTALL=$(T_SRC:%.lua=$(INSTALL_LMOD)/%.lua)

ifdef BUILD_EXAMPLE
T_SRC:=$(T_SRC) Numarray.lua
endif

all: $(T_SRC)

install: $(T_INSTALL)
	$(INSTALL_DATA) -Dm644 t.lua $(INSTALL_LMOD)/../t.lua

$(INSTALL_LMOD)/%.lua: %.lua
	$(INSTALL_DATA) -Dm644 $< $@

test: $(SRCDIR)
	$(MAKE) -C $(CURDIR)/test CC=$(CC) LD=$(LD) \
		LVER=$(LVER) \
		MYCFLAGS="$(MYCFLAGS)" \
		LDFLAGS="$(LDFLAGS)" \
		INCDIR=$(INCDIR)

clean:

.PHONY: all test clean
