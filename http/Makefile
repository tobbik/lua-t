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
HOST=pihole
PORT=10000
CONNS=100
SECONDS=20
THREADS=4
WRK_URL="http://$(HOST):$(PORT)/auth?username=$(USERNAME)&password=$(PASSWORD)"
WRK_MULTI="http://$(HOST):$(PORT)/multi?multiplier=1200"

$(S_EXE): s_go.go $(GO)
	$(GO) build -o $@ $<

clean:
	-rm $(S_EXE)

gsrv: $(S_EXE)
	$(CURDIR)/${S_EXE} $(PORT)

lsrv:
	LUA_PATH="$(CURDIR)/../out/share/lua/5.4/?.lua;;" \
	  LUA_CPATH="$(CURDIR)/../out/lib/lua/5.4/?.so;;" \
	  $(CURDIR)/../out/bin/lua s_t.lua $(PORT)

nsrv: $(node)
	node s_node.js $(PORT)

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
	#killall node

user: $(CURL)
	$(CURL) -i "http://$(HOST):$(PORT)/newUser?username=$(USERNAME)&password=$(PASSWORD)"

wrk: $(WRK)
	$(WRK) -t $(THREADS) -c $(CONNS) -d $(SECONDS) --latency  $(WRK_URL)

wrkr: $(WRK)
	$(WRK) -t $(THREADS) -c $(CONNS) -s $(CURDIR)/report.lua -d $(SECONDS) --latency $(WRK_URL)

wrkm: $(WRK)
	$(WRK) -t $(THREADS) -c $(CONNS) -s $(CURDIR)/report.lua -d $(SECONDS) --latency $(WRK_MULTI)

lm: $(LUA_T)
	LUA_PATH="$(CURDIR)/../out/share/lua/5.4/?.lua;;" \
	  LUA_CPATH="$(CURDIR)/../out/lib/lua/5.4/?.so;;" \
	  $(CURDIR)/../out/bin/lua s_t.lua $(PORT)  &
	sleep 1
	$(MAKE) wrkm
	killall lua

nm: $(NODE)
	node s_node.js $(PORT)  &
	sleep 1
	$(MAKE) wrkm
	killall node

gm: $(S_EXE)
	$(CURDIR)/${S_EXE} $(PORT) > /dev/null &
	sleep 1
	$(MAKE) wrkm
	killall $<

