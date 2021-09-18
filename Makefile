# vim: ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
#
# \file      Makefile
# \brief     Makefile for the lua-t library
# \author    tkieslich
# \copyright See Copyright notice at the end of t.h

LVER=5.4

CURDIR != pwd
SRCDIR=$(CURDIR)/src
LUADIR=$(CURDIR)/lua
PREFIX != /bin/sh ../guess_prefix.sh

all: $(SRCDIR)/*.so

$(SRCDIR)/*.so:
	$(MAKE) -C $(SRCDIR) \
	 LVER="$(LVER)" \
	 PREFIX="$(PREFIX)"

install:
	$(MAKE) -C $(SRCDIR) \
	  LVER=$(LVER) \
	  MYCFLAGS="$(MYCFLAGS)" \
	  LDFLAGS="$(LDFLAGS)" \
	  INCDIR="$(INCDIR)" \
	  PREFIX="$(PREFIX)" install
	$(MAKE) -C $(LUADIR) \
	  LVER=$(LVER) \
	  PREFIX="$(PREFIX)" install

test: $(SRCDIR)
	$(MAKE) -C $(SRCDIR) \
	 LVER=$(LVER) \
	 MYCFLAGS="$(MYCFLAGS)" \
	 LDFLAGS="$(LDFLAGS)" \
	 INCDIR=$(INCDIR) test

# echo config parameters
echo:
	@echo "LVER= $(LVER)"
	@echo "PREFIX= $(PREFIX)"
	@echo "INCDIR= $(INCDIR)"
	@echo "LDFLAGS= $(LDFLAGS)"
	$(MAKE) -C $(SRCDIR) -s echo
	$(MAKE) -C $(LUADIR) -s echo

clean:
	$(MAKE) -C $(SRCDIR) \
	 T_DBG_SRC=t_dbg.c \
	 T_NRY_LIB=nry.so \
	 clean

uninstall:
	$(MAKE) -C $(SRCDIR) uninstall
	$(MAKE) -C $(LUADIR) uninstall

pristine:
	$(MAKE) -C $(SRCDIR) pristine

include docker.mk
include dev.mk

