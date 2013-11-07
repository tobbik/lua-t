LUASRC=lua-5.2.2.tar.gz
DLPATH=http://www.lua.org/ftp
COMPDIR=$(CURDIR)/compile
OUTDIR=$(CURDIR)/out
SRCDIR=$(CURDIR)/sources
XTDIR=$(CURDIR)/xt
CC=clang
LD=clang

lxt: $(COMPDIR)/xt/xt.so
	patch -d $(COMPDIR)/Lua/src/ -i $(SRCDIR)/lua-5.2.2_upstream.patch || exit
	patch -d $(COMPDIR)/Lua/src/ -i $(COMPDIR)/xt/build_static.patch -p2 || exit
	$(MAKE) install

lxt64: $(COMPDIR)/xt/xt.so
	patch -d $(COMPDIR)/Lua/src/ -i $(SRCDIR)/lua-5.2.2_upstream.patch || exit
	patch -d $(COMPDIR)/Lua/src/ -i $(SRCDIR)/lua-5.2.2_ll.patch || exit
	patch -d $(COMPDIR)/Lua/src/ -i $(COMPDIR)/xt/build_static.patch -p2 || exit
	$(MAKE) install

lxt53:
	$(MAKE) CC=$(CC) LD=$(LD) \
		LUASRC=lua-5.3.0-work1.tar.gz \
		DLPATH=http://www.lua.org/work \
		$(COMPDIR)/xt/xt.so
	patch -d $(COMPDIR)/Lua/src/ -i $(COMPDIR)/xt/build_static.patch -p2 || exit
	$(MAKE) install

$(SRCDIR)/$(LUASRC):
	wget $(DLPATH)/$(LUASRC) -O $(SRCDIR)/$(LUASRC)

$(COMPDIR)/Lua/src: $(SRCDIR)/$(LUASRC)
	mkdir -p $(COMPDIR)/Lua
	tar -xvzf $(SRCDIR)/$(LUASRC) -C $(COMPDIR)/Lua --strip-components=1

$(COMPDIR)/xt:
	mkdir -p $(COMPDIR)/xt
	cp -ar $(XTDIR)/* $(COMPDIR)/xt/

$(COMPDIR)/xt/xt.so: $(COMPDIR)/Lua/src $(COMPDIR)/xt
	cd $(COMPDIR)/xt ; $(MAKE) CC=$(CC) LD=$(LD) \
		MYCFLAGS=' -g' \
		INCS="$(COMPDIR)/Lua/src" all || exit

$(COMPDIR)/Lua/src/lua: $(COMPDIR)/xt/xt.so
	cd $(COMPDIR)/Lua ; \
		$(MAKE) CC=$(CC) LD=$(LD) \
		MYCFLAGS=" -g -fPIC -DL_XT_ROOT=\"\\\"$(OUTDIR)/\\\"\" -I$(COMPDIR)/xt" \
		MYOBJS="$(COMPDIR)/xt/l_xt*.o" \
		linux

install: $(COMPDIR)/Lua/src/lua
	cd $(COMPDIR)/Lua ; $(MAKE) CC=$(CC) LD=$(LD) \
		INSTALL_TOP="$(OUTDIR)" \
		install

clean:
	rm -rf $(COMPDIR) $(OUTDIR)

pristine:
	rm $(SRCDIR)/$(LUASRC)
