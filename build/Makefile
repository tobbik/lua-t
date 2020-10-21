DOCKER=$(shell which docker)
DOCKERPS=$(DOCKER) ps --format "table {{.Names}}"
CONTAINER=luab
VERSION=5.4
IMAGE=lua$(shell echo $(VERSION) | sed "s/\.//")

start:
	$(DOCKERPS) | grep -qi "$(CONTAINER)" || docker start "$(CONTAINER)"

stop:
	$(DOCKERPS) | grep -qi "$(CONTAINER)" && docker stop "$(CONTAINER)"

remove:
	$(DOCKERPS) -a | grep -qi "$(CONTAINER)" && docker rm $(CONTAINER)

prepare:
	mkdir -p $(CURDIR)/$(VERSION)/lua-t
	cp -arp ../src      $(CURDIR)/$(VERSION)/lua-t/
	cp -arp ../lua      $(CURDIR)/$(VERSION)/lua-t/
	cp -arp ../test     $(CURDIR)/$(VERSION)/lua-t/
	cp -arp ../example  $(CURDIR)/$(VERSION)/lua-t/
	cp      ../Makefile $(CURDIR)/$(VERSION)/lua-t/

build: $(DOCKER)
	$(MAKE) prepare
	$(DOCKER) build --tag $(IMAGE) 5.4

exec:
	docker exec $(CONTAINER) /bin/bash

run:
	$(DOCKER) run --name $(CONTAINER) $(IMAGE)

runit:
	$(DOCKER) run -i -t $(IMAGE) /bin/bash


clean:
	#$(MAKE) stop
	#$(MAKE) remove
	$(DOCKER) image rm $(IMAGE)
	-rm -rf $(CURDIR)/$(VERSION)/lua-t
