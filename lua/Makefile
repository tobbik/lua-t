# vim: ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
#
# \file      src/Makefile
# \brief     Makefile for the lua-t library source code
# \author    tkieslich
# \copyright See Copyright notice at the end of t.h
#
LVER=5.4
PREFIX=$(shell pkg-config --variable=prefix lua)
INSTALL_LMOD=$(PREFIX)/share/lua/$(LVER)/t

# How to install. If your install program does not support "-p", then
# you may have to run ranlib on the installed t.a
INSTALL= install -p
INSTALL_DATA= $(INSTALL) -Dm0644

T_SRC=\
   Buffer.lua \
   Csv.lua \
   Encode.lua \
   Encode/Base64.lua \
   Encode/Crc.lua \
   Encode/Rc4.lua \
   Http.lua \
   Http/Server.lua \
   Http/Stream.lua \
   Http/Request.lua \
   Http/Response.lua \
   Http/WebSocket.lua \
   Http/Status.lua \
   Http/Method.lua \
   Http/Version.lua \
   Loop.lua \
   Net.lua \
   Net/Interface.lua \
   Net/Address.lua \
   Net/Socket.lua \
   Net/Socket/Protocol.lua \
   Net/Socket/Shutdown.lua \
   Net/Family.lua \
   OrderedHashTable.lua \
   Pack.lua \
   Pack/Index.lua \
   Set.lua \
   Table.lua \
   Test.lua \
   Test/Case.lua \
   Test/Context.lua \
   Time.lua

ifdef BUILD_EXAMPLE
T_SRC:=$(T_SRC) Numarray.lua
endif

T_INSTALL=$(T_SRC:%.lua=$(INSTALL_LMOD)/%.lua)

all: $(T_SRC)

install: $(T_INSTALL)
	$(INSTALL_DATA) t.lua $(INSTALL_LMOD)/../t.lua

$(INSTALL_LMOD)/%.lua: %.lua
	$(INSTALL_DATA) $< $@

clean:

.PHONY: all clean
