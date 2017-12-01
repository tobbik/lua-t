EXE=s_go

GO=$(shell which go)
LUA_T=../../../lua-build/out/bin/lua
LUVIT=$(shell which luvit)
NODE=$(shell which node)

SCREEN_RC=screen.rc
SCREEN=$(shell which screen)
CURL=$(shell which curl)
AB=$(shell which ab)
WRK=$(shell which wrk)
GOWRK=$(shell which go-wrk)

USERNAME=username
PASSWORD=password
HOST=127.0.0.1
PORT=8000
CONNS=400
SECONDS=6

include $(CURDIR)/../instruct.make

$(EXE):  s_go.go $(GO)
	$(GO) build -o $@ $<

run: $(SCREEN)
	$(SCREEN) -S perf2 -c $(CURDIR)/screen.rc

lua-t: $(LUA_T)
	$(LUA_T) s_t.lua $(PORT) > /dev/null &
	#$(eval LASTPID := $(shell return $$!))
	$(MAKE) createuser
	$(MAKE) ab
	$(MAKE) wrk
	#kill $(LASTPID)
	killall lua

luvit: $(LUVIT)
	$(LUVIT) s_luvit.lua $(PORT) > /dev/null &
	#$(eval LASTPID := $(shell return $$!))
	$(MAKE) createuser
	$(MAKE) ab
	$(MAKE) wrk
	#kill $(LASTPID)
	killall luvit

user: $(CURL)
	$(CURL) -i "http://$(HOST):$(PORT)/newUser?username=$(USERNAME)&password=$(PASSWORD)"

ab: $(AB)
	$(AB) -c$(CONNS) -n500000 -t$(SECONDS) "http://$(HOST):$(PORT)/auth?username=$(USERNAME)&password=$(PASSWORD)"

abk: $(AB)
	$(AB) -k -c$(CONNS) -n500000 -t$(SECONDS) "http://$(HOST):$(PORT)/auth?username=$(USERNAME)&password=$(PASSWORD)"

wrk: $(WRK)
	$(WRK) -t 12 -c $(CONNS) -d $(SECONDS) -H "Connection: close"      "http://$(HOST):$(PORT)/auth?username=$(USERNAME)&password=$(PASSWORD)"

wrkk: $(WRK)
	$(WRK) -t 12 -c $(CONNS) -d $(SECONDS) -H "Connection: keep-alive" "http://$(HOST):$(PORT)/auth?username=$(USERNAME)&password=$(PASSWORD)"

gowrk: $(GOWRK)
	$(GOWRK) -c $(CONNS) -d $(SECONDS) -H "Connection: close"      "http://$(HOST):$(PORT)/auth?username=$(USERNAME)&password=$(PASSWORD)" 2> /dev/null

gowrkk: $(GOWRK)
	$(GOWRK) -c $(CONNS) -d $(SECONDS) -H "Connection: keep-alive" "http://$(HOST):$(PORT)/auth?username=$(USERNAME)&password=$(PASSWORD)" 2> /dev/null

users:
	$(MAKE) USERNAME=$(USERNAME) PORT=8000 user
	$(MAKE) USERNAME=$(USERNAME) PORT=8001 user
	$(MAKE) USERNAME=$(USERNAME) PORT=8002 user
	$(MAKE) USERNAME=$(USERNAME) PORT=8003 user
