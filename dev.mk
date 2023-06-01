# vim: ft=make ts=3 sw=3 sts=3 sta noet tw=80 list
RM != which rm

#DMYCFLAGS:= -pedantic -O3
TRYFLAGS != /bin/sh guess_platflags.sh
DMYCFLAGS= -D DEBUG=1 -g -O3 -Winline $(TRYFLAGS)

PLAT=guess
LVER=5.4
LREL=3
CURDIR != pwd

D_CC=gcc
D_LD=gcc

PREFIX=$(CURDIR)/local

local/bin/lua:
	$(MAKE) -C $(PREFIX) \
	 CC=$(D_CC) LD=$(D_LD) \
	 MYCFLAGS="$(DMYCFLAGS)" \
	 LVER="$(LVER)" \
	 PREFIX="$(PREFIX)"

dev-clean:
	$(MAKE) CC=$(D_CC) LD=$(D_LD) \
	 T_DBG_SRC=t_dbg.c \
	 T_NRY_LIB=nry.so \
	 clean
	$(MAKE) \
	 T_NRY_LIB=nry.so \
	 PREFIX="$(PREFIX)" uninstall

dev-rinse:
	$(MAKE) dev-clean
	$(MAKE) -C $(PREFIX) clean
	$(MAKE) -C $(PREFIX) uninstall

dev-nuke:
	$(MAKE) dev-clean
	$(MAKE) -C $(PREFIX) uninstall
	$(MAKE) -C $(PREFIX) nuke

dev-pristine:
	$(MAKE) dev-nuke
	$(MAKE) -C $(PREFIX) pristine

dev: local/bin/lua
	$(MAKE) -j4 CC=$(D_CC) LD=$(D_LD) \
	 T_DBG_SRC=t_dbg.c \
	 T_NRY_LIB=nry.so  \
	 L_SRC_NRY=t/Numarray.lua \
	 INCDIR="-I$(PREFIX)/include" \
	 MYCFLAGS="$(DMYCFLAGS)" \
	 MYLDFLAGS="$(MYLDFLAGS)" \
	 PREFIX="$(PREFIX)"
	$(MAKE) \
	 LVER=$(LVER) LREL=$(LREL) \
	 T_NRY_LIB=nry.so  \
	 L_SRC_NRY=t/Numarray.lua \
	 PREFIX="$(PREFIX)" install

dev-run:
	$(MAKE) dev
	time LUA_PATH="$(PREFIX)/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(PREFIX)/lib/lua/5.4/?.so;;" \
	 $(PREFIX)/bin/lua scratchpad.lua

dev-exec:
	$(MAKE) dev
	LUA_PATH="$(PREFIX)/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(PREFIX)/lib/lua/5.4/?.so;;" \
	 $(PREFIX)/bin/lua -i scratchpad.lua

dev-test:
	$(MAKE) dev
	LUA_PATH="$(PREFIX)/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(PREFIX)/lib/lua/5.4/?.so;;" \
	 $(PREFIX)/bin/lua -i test/runner.lua

dev-t1:
	$(MAKE) dev
	LUA_PATH="$(PREFIX)/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(PREFIX)/lib/lua/5.4/?.so;;" \
	 $(PREFIX)/bin/lua -i test/t1.lua

dev-gdb:
	$(MAKE) dev
	LUA_PATH="$(PREFIX)/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(PREFIX)/lib/lua/5.4/?.so;;" \
	 gdb --args $(PREFIX)/bin/lua -i test/runner.lua

dev-example:
	$(MAKE) dev
	LUA_PATH="$(PREFIX)/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(PREFIX)/lib/lua/5.4/?.so;;" \
	 $(PREFIX)/bin/lua -i example/t_net_ifc.lua


dev-echo:
	@echo "PLAT= $(PLAT)"
	@echo "LVER= $(LVER)"
	@echo "PREFIX= $(PREFIX)"
	@echo "CC= $(CC)"
	@echo "LD= $(LD)"
	$(MAKE) -C $(SRCDIR) -s \
	 PREFIX="$(PREFIX)" \
	 INCDIR="$(PREFIX)/include" \
	 MYCFLAGS="$(DMYCFLAGS)" \
	 MYLDFLAGS="$(MYLDFLAGS)" \
	 echo
	$(MAKE) -C $(LUADIR) -s \
	 PREFIX="$(PREFIX)" \
	 INCDIR="$(PREFIX)/include" \
	 MYCFLAGS="$(DMYCFLAGS)" \
	 MYLDFLAGS="$(MYLDFLAGS)" \
	 echo

run-dev:
	#$(MAKE) dev
	LUA_PATH="$(PREFIX)/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(PREFIX)/lib/lua/5.4/?.so;;" \
	 $(PREFIX)/bin/lua -i scp1.lua

srv-dev:
	LUA_PATH="$(PREFIX)/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(PREFIX)/lib/lua/5.4/?.so;;" \
	 $(PREFIX)/bin/lua srv.lua

cli-dev:
	LUA_PATH="$(PREFIX)/share/lua/5.4/?.lua;;" \
	 LUA_CPATH="$(PREFIX)/lib/lua/5.4/?.so;;" \
	 $(PREFIX)/bin/lua cli.lua
