# vim: ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
#
# \file      docker.mk
# \brief     Makefile containing docker related targets
# \author    tkieslich
# \copyright See Copyright notice at the end of t.h

LVER=5.4
LVR != echo $(LVER) | sed "s/\.//"
CURDIR != pwd

PREFIX != pkgconf --variable=prefix lua

SRCDIR=$(CURDIR)/src
LUADIR=$(CURDIR)/lua

# Docker build specific
DOCKER != which docker
DOCKERPS=$(DOCKER) ps --format "table {{.Names}}"
CONTAINER=luab
IMAGE:= lua$(LVR)
TZDATAPATH=build/tz

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

# Docker to development inside container
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

.PHONY:
