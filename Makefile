# vim: ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
#
# \file      Makefile
# \brief     Makefile for the lua-t library
# \author    tkieslich
# \copyright See Copyright notice at the end of t.h

LVER=5.3
PREFIX=$(shell pkg-config --variable=prefix lua)
INCDIR=$(shell pkg-config --variable=includedir lua)
LDFLAGS=$(shell pkg-config --libs lua)
PLAT=linux
#MYFLAGS=" -g "
SRCDIR=$(CURDIR)/src

CC=clang
LD=clang

all:
	cd $(SRCDIR) ; $(MAKE) CC=$(CC) LD=$(LD) \
		VER="$(VER)" \
		MYCFLAGS="$(MYCFLAGS)" \
		INCS="$(INCDIR)" \
		PREFIX="$(OUTDIR)"

install:
	cd $(SRCDIR) ; $(MAKE) CC=$(CC) LD=$(LD) \
		VER=$(VER) \
		MYCFLAGS=$(MYCFLAGS) \
		INCS=$(INCDIR) \
		PREFIX="$(PREFIX)" install

# echo config parameters
echo:
	@cd src && $(MAKE) -s echo
	@echo "PLAT= $(PLAT)"
	@echo "LVER= $(LVER)"
	@echo "PREFIX= $(PREFIX)"
	@echo "INCDIR= $(INCDIR)"
	@echo "LDFLAGS= $(LDFLAGS)"

clean:
	cd $(SRCDIR); make clean
