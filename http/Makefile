S_EXE=s_go

LUA_T=../out/bin/lua
NODE=$(shell which node)

GO=$(shell which go)
SCREEN=$(shell which screen)
CURL=$(shell which curl)
WRK=$(shell which wrk)
NODE=$(shell which node)
#GOWRK=$(shell which go-wrk)

USERNAME=mickey
PASSWORD=goofey
HOST=127.0.0.1
PORT=10000
CONNS=400
SECONDS=30
THREADS=4
WRK_URL="http://$(HOST):$(PORT)/auth?username=$(USERNAME)&password=$(PASSWORD)"

$(S_EXE): s_go.go $(GO)
	$(GO) build -o $@ $<

clean:
	-rm $(S_EXE)

lsrv:
	LUA_PATH="$(CURDIR)/../out/share/lua/5.4/?.lua;;" \
	  LUA_CPATH="$(CURDIR)/../out/lib/lua/5.4/?.so;;" \
	  $(CURDIR)/../out/bin/lua s_t.lua $(PORT)

go: $(S_EXE)
	$(CURDIR)/${S_EXE} $(PORT) > /dev/null &
	$(MAKE) USERNAME=$(USERNAME) PORT=$(PORT) user
	$(MAKE) CONNECTIONHEADER=$(CONN_HEADER) wrkr
	killall $<

lua-t: $(LUA_T)
	LUA_PATH="$(CURDIR)/../out/share/lua/5.4/?.lua;;" \
	  LUA_CPATH="$(CURDIR)/../out/lib/lua/5.4/?.so;;" \
	  $(CURDIR)/../out/bin/lua s_t.lua $(PORT)  &
	sleep 1
	$(MAKE) USERNAME=$(USERNAME) PORT=$(PORT) user
	$(MAKE) wrkr
	killall lua

node: $(node)
	node s_node.js $(PORT)  &
	sleep 1
	#$(eval LASTPID := $(shell return $$!))
	$(MAKE) USERNAME=$(USERNAME) PORT=$(PORT) user
	$(MAKE) wrkr
	#kill $(LASTPID)
	killall node

user: $(CURL)
	$(CURL) -i "http://$(HOST):$(PORT)/newUser?username=$(USERNAME)&password=$(PASSWORD)"

wrk: $(WRK)
	$(WRK) -t $(THREADS) -c $(CONNS) -d $(SECONDS) --latency  $(WRK_URL)

wrkr: $(WRK)
	$(WRK) -t $(THREADS) -c $(CONNS) -s $(CURDIR)/setup.lua -d $(SECONDS) --latency $(WRK_URL)
