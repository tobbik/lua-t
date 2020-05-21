# vim: ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
#
# \file      Makefile
# \brief     Makefile for the lua-t library
# \author    tkieslich
# \copyright See Copyright notice at the end of t.h

LVER=5.3

INSTALL_CMOD=$(PREFIX)/lib/lua/$(LVER)/t
INSTALL_LMOD=$(PREFIX)/share/lua/$(LVER)/t

PREFIX=$(shell pkg-config --variable=prefix lua)
INCDIR=$(shell pkg-config --variable=includedir lua)
LDFLAGS=$(shell pkg-config --libs lua) -lcrypt
PLAT=linux
MYCFLAGS=" -O2"
SRCDIR=$(CURDIR)/src
LUADIR=$(CURDIR)/lua

CC=gcc
LD=gcc

SCREEN_RC=screen.rc
SCREEN=$(shell which screen)

include dev.mk

# Docker build specific
DOCKER=$(shell which docker)
DOCKERPS=$(DOCKER) ps --format "table {{.Names}}"
CONTAINER=luab
VERSION=5.3
IMAGE=lua$(shell echo $(VERSION) | sed "s/\.//")
TZDATAPATH=build/tz

all: $(SRCDIR)/*.so

$(SRCDIR)/*.so:
	$(MAKE) -C $(SRCDIR) CC=$(CC) LD=$(LD) \
	 LVER="$(LVER)" \
	 MYCFLAGS="$(MYCFLAGS)" \
	 LDFLAGS="$(LDFLAGS)" \
	 INCS="$(INCDIR)" \
	 PREFIX="$(PREFIX)"

install: $(SRCDIR)/$(LIBS)
	$(MAKE) -C $(SRCDIR) CC=$(CC) LD=$(LD) \
	 INSTALL_CMOD=$(INSTALL_CMOD) \
	 LVER=$(LVER) \
	 MYCFLAGS="$(MYCFLAGS)" \
	 LDFLAGS="$(LDFLAGS)" \
	 INCS=$(INCDIR) \
	 PREFIX="$(PREFIX)" install
	$(MAKE) -C $(LUADIR) \
	 INSTALL_LMOD=$(INSTALL_LMOD) \
	 LVER=$(LVER) \
	 PREFIX="$(PREFIX)" install

test: $(SRCDIR)
	$(MAKE) -C $(SRCDIR) CC=$(CC) LD=$(LD) \
	 LVER=$(LVER) \
	 MYCFLAGS="$(MYCFLAGS)" \
	 LDFLAGS="$(LDFLAGS)" \
	 INCDIR=$(INCDIR) test

# echo config parameters
echo:
	$(MAKE) -C $(SRCDIR) -s echo
	@echo "PLAT= $(PLAT)"
	@echo "LVER= $(LVER)"
	@echo "PREFIX= $(PREFIX)"
	@echo "INCDIR= $(INCDIR)"
	@echo "LDFLAGS= $(LDFLAGS)"
	@echo "MYCFLAGS= $(MYCFLAGS)"

clean:
	$(MAKE) -C $(SRCDIR) clean
	$(MAKE) -C $(LUADIR) clean

pristine:
	$(MAKE) -C $(SRCDIR) pristine

run: $(SCREEN_RC)
	$(SCREEN) -S lua -c "$(SCREEN_RC)"

perf:
	$(SCREEN) -S perf -c "screen-perf.rc"


# following targets are for docker builds
docker-build: $(DOCKER)
	$(DOCKER) build --tag $(IMAGE) .

docker-run:
	$(DOCKER) run -i -t --name $(IMAGE) $(IMAGE) /bin/bash

docker-start:
	$(DOCKER) start -i $(IMAGE)

docker-stop:
	$(DOCKER) stop $(IMAGE)

dclean:
	$(DOCKER) image rm $(IMAGE)


# Docker for development
#
$(TZDATAPATH):
	cat /etc/localtime > $(TZDATAPATH)

docker-dev: $(DOCKER) $(TZDATAPATH)
	$(DOCKER) build  --tag  $(IMAGE)dev -f Dockerfile.dev .
	$(DOCKER) run    -i -t \
	 --name   $(IMAGE)dev \
	 --mount  src="$(CURDIR)",target=/build,type=bind \
	 $(IMAGE)dev \
	 /bin/bash

docker-dev-start:
	$(DOCKER) start -i $(IMAGE)dev

docker-dev-stop:
	$(DOCKER) stop $(IMAGE)dev

docker-dev-clean:
	$(DOCKER) rm $(IMAGE)dev

docker-dev-pristine:
	$(DOCKER) image rm $(IMAGE)dev

docker-dev-rinse:
	$(MAKE) docker-dev-clean
	$(MAKE) docker-dev-pristine
	-rm $(TZDATAPATH)
