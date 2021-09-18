# vim: ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
#
# \file      src/Makefile
# \brief     Makefile for the lua-t library source code
# \author    tkieslich
# \copyright See Copyright notice at the end of t.h
#
LVER=5.4
PREFIX=/usr/local
INSTALL_LMOD=$(PREFIX)/share/lua/$(LVER)

# How to install. If your install program does not support "-p", then
# you may have to run ranlib on the installed t.a
INSTALLER != which install
INSTALL_DATA=$(INSTALLER) -p -Dm 0644
RM != which rm

T_SRC=\
   t/Buffer.lua \
   t/Csv.lua \
   t/Encode.lua \
   t/Encode/Base64.lua \
   t/Encode/Crc.lua \
   t/Encode/Rc4.lua \
   t/Http.lua \
   t/Http/Server.lua \
   t/Http/Stream.lua \
   t/Http/Request.lua \
   t/Http/Response.lua \
   t/Http/WebSocket.lua \
   t/Http/Status.lua \
   t/Http/Method.lua \
   t/Http/Version.lua \
   t/Loop.lua \
   t/Net.lua \
   t/Net/Interface.lua \
   t/Net/Address.lua \
   t/Net/Socket.lua \
   t/Net/Socket/Protocol.lua \
   t/Net/Socket/Type.lua \
   t/Net/Family.lua \
   t/OrderedHashTable.lua \
   t/Pack.lua \
   t/Pack/Index.lua \
   t/Set.lua \
   t/Table.lua \
   t/Test.lua \
   t/Test/Suite.lua \
   t.lua \
   $(L_SRC_NRY)

T_INSTALL=$(T_SRC:%.lua=$(INSTALL_LMOD)/%.lua)

all: $(T_SRC)

install: $(T_INSTALL)

$(INSTALL_LMOD)/%.lua: %.lua
	$(INSTALL_DATA) $^ $@

uninstall:
	-$(RM) -f  $(T_INSTALL)
	-$(RM) -fr $(INSTALL_LMOD)/t

echo:
	@echo "LVER= $(LVER)"
	@echo "PREFIX= $(PREFIX)"
	@echo "INSTALL_LMOD= $(INSTALL_LMOD)"

.PHONY: all clean
