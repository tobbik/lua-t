
LUA_T=../out/bin/lua
NODE=$(shell which node)

SCREEN_RC=screen.rc
SCREEN=$(shell which screen)
CURL=$(shell which curl)
WRK=$(shell which wrk)
#GOWRK=$(shell which go-wrk)

USERNAME=mickey
PASSWORD=goofey
HOST=127.0.0.1
PORT=10000
CONNS=400
SECONDS=15


run: $(SCREEN)
	$(SCREEN) -S perf2 -c $(CURDIR)/screen.rc

lsrv:
	LUA_PATH="$(CURDIR)/../out/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(CURDIR)/../out/lib/lua/5.4/?.so;;" \
	 $(CURDIR)/../out/bin/lua s_t.lua $(PORT)

lua-t: $(LUA_T)
	LUA_PATH="$(CURDIR)/../out/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(CURDIR)/../out/lib/lua/5.4/?.so;;" \
	 $(CURDIR)/../out/bin/lua s_t.lua $(PORT) > /dev/null &
	#$(eval LASTPID := $(shell return $$!))
	$(MAKE) USERNAME=$(USERNAME) PORT=$(PORT) user
	$(MAKE) wrkk
	#kill $(LASTPID)
	killall lua

node: $(LUA_T)
	node s_node.js $(PORT) > /dev/null &
	sleep 1
	#$(eval LASTPID := $(shell return $$!))
	$(MAKE) USERNAME=$(USERNAME) PORT=$(PORT) user
	$(MAKE) wrkk
	#kill $(LASTPID)
	killall node

user: $(CURL)
	$(CURL) -i "http://$(HOST):$(PORT)/newUser?username=$(USERNAME)&password=$(PASSWORD)"

wrk: $(WRK)
	$(WRK) -t 12 -c $(CONNS) -d $(SECONDS) --latency -H "Connection: close"      "http://$(HOST):$(PORT)/auth?username=$(USERNAME)&password=$(PASSWORD)"

wrkk: $(WRK)
	$(WRK) -t 12 -c $(CONNS) -d $(SECONDS) --latency -H "Connection: keep-alive" "http://$(HOST):$(PORT)/auth?username=$(USERNAME)&password=$(PASSWORD)"

gowrk: $(GOWRK)
	$(GOWRK) -c $(CONNS) -d $(SECONDS) -H "Connection: close"      "http://$(HOST):$(PORT)/auth?username=$(USERNAME)&password=$(PASSWORD)" 2> /dev/null

gowrkk: $(GOWRK)
	$(GOWRK) -c $(CONNS) -d $(SECONDS) -H "Connection: keep-alive" "http://$(HOST):$(PORT)/auth?username=$(USERNAME)&password=$(PASSWORD)" 2> /dev/null

users:
	$(MAKE) USERNAME=$(USERNAME) PORT=8000 user
	$(MAKE) USERNAME=$(USERNAME) PORT=8001 user
	$(MAKE) USERNAME=$(USERNAME) PORT=8002 user
	$(MAKE) USERNAME=$(USERNAME) PORT=8003 user
