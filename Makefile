# vim: ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
#
# \file      Makefile
# \brief     Makefile for the lua-t library
# \author    tkieslich
# \copyright See Copyright notice at the end of t.h

PLAT=guess
PLATS= guess freebsd linux
MK_INSTALL=install.mk

LVER=5.4

CURDIR != pwd
SRCDIR=$(CURDIR)/src
LUADIR=$(CURDIR)/lua
PREFIX != /bin/sh guess_prefix.sh
RM != which rm

all: $(PLAT)

$(PLATS):
	$(MAKE) -C $(SRCDIR) \
	 LVER="$(LVER)" \
	 PREFIX="$(PREFIX)"

$(LUADIR)/$(MK_INSTALL):
	@cd $(LUADIR) && echo -e "\ninstall: \$$(T_INSTALL)\n" > $(MK_INSTALL)
	@cd $(LUADIR) && echo -e "\n\$$(INSTALL_LMOD):\n\t\$$(INSTALL_EXEC) -d \$$(INSTALL_LMOD)" >> $(MK_INSTALL)
	@cd $(LUADIR) && echo -e "\n\$$(INSTALL_LMOD)/t.lua: t.lua \$$(INSTALL_LMOD)\n\t\$$(INSTALL_DATA) t.lua \$$(INSTALL_LMOD)" >> $(MK_INSTALL)
	@cd $(LUADIR) && for DIR in `find t -type d`; do  \
	  echo -e "\n\n### -> DIRECTORY: $${DIR}" >> $(MK_INSTALL); \
	  echo -e "\$$(INSTALL_LMOD)/$${DIR}: $${DIR}\n\t\$$(INSTALL_EXEC) -d \$$(INSTALL_LMOD)/$${DIR}" >> $(MK_INSTALL); \
	  for FILE in `ls $$DIR/*.lua`; do \
	    echo -e "\n\$$(INSTALL_LMOD)/$${FILE}: $${FILE} \$$(INSTALL_LMOD)/$${DIR}\n\t\$$(INSTALL_DATA) $${FILE} \$$(INSTALL_LMOD)/$${DIR}" >> $(MK_INSTALL); \
	  done \
	done

install: $(LUADIR)/install.mk
	$(MAKE) -C $(SRCDIR) \
	  LVER=$(LVER) \
	  MYCFLAGS="$(MYCFLAGS)" \
	  LDFLAGS="$(LDFLAGS)" \
	  INCDIR="$(INCDIR)" \
	  PREFIX="$(PREFIX)" install
	$(MAKE) -C $(LUADIR) \
	  MK_INSTALL=$(MK_INSTALL) \
	  LVER=$(LVER) \
	  PREFIX="$(PREFIX)" install

test: $(SRCDIR)
	$(MAKE) -C $(SRCDIR) \
	 LVER=$(LVER) \
	 MYCFLAGS="$(MYCFLAGS)" \
	 LDFLAGS="$(LDFLAGS)" \
	 INCDIR=$(INCDIR) test

# echo config parameters
echo: $(LUADIR)/install.mk
	@echo "LVER= $(LVER)"
	@echo "PREFIX= $(PREFIX)"
	@echo "INCDIR= $(INCDIR)"
	@echo "LDFLAGS= $(LDFLAGS)"
	$(MAKE) -C $(SRCDIR) -s echo
	$(MAKE) -C $(LUADIR) -s echo

clean: $(LUADIR)/install.mk
	$(MAKE) -C $(SRCDIR) \
	 T_DBG_SRC=t_dbg.c \
	 T_NRY_LIB=nry.so \
	 clean
	-$(RM) -f $(LUADIR)/install.mk

uninstall: $(LUADIR)/install.mk
	$(MAKE) -C $(SRCDIR) uninstall
	$(MAKE) -C $(LUADIR) uninstall
	-$(RM) -f $(LUADIR)/install.mk

pristine:
	$(MAKE) -C $(SRCDIR) pristine

.PHONY: all $(PLATS) clean install test echo clean uninstall pristine

#include docker.mk
include dev.mk

