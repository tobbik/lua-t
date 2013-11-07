LUASRC=lua-5.2.2.tar.gz
DLPATH=http://www.lua.org/ftp
LUAVER=LUA52
COMPDIR=$(CURDIR)/compile
OUTDIR=$(CURDIR)/out
SRCDIR=$(CURDIR)/sources
XTDIR=$(CURDIR)/xt
CC=clang
LD=clang

lxt: $(XTDIR)/xt.a
	$(MAKE) install

lxt64:
	$(MAKE) CC=$(CC) LD=$(LD) \
		LUAVER=LUA52LD \
		$(XTDIR)/xt.a
	$(MAKE) LUAVER=LUA52LD install

lxt53:
	$(MAKE) CC=$(CC) LD=$(LD) \
		LUAVER=LUA53 \
		LUASRC=lua-5.3.0-work1.tar.gz \
		DLPATH=http://www.lua.org/work \
		$(XTDIR)/xt.a
	$(MAKE) LUAVER=LUA53 install

$(SRCDIR)/$(LUASRC):
	wget $(DLPATH)/$(LUASRC) -O $(SRCDIR)/$(LUASRC)

$(COMPDIR)/LUA52/src: $(SRCDIR)/$(LUASRC)
	mkdir -p $(COMPDIR)/$(LUAVER)
	tar -xvzf $(SRCDIR)/$(LUASRC) -C $(COMPDIR)/$(LUAVER) --strip-components=1
	patch -d $(COMPDIR)/$(LUAVER)/src/ -i $(SRCDIR)/lua-5.2.2_upstream.patch
	patch -d $(COMPDIR)/$(LUAVER)/src/ -i $(XTDIR)/build_static.patch -p2

$(COMPDIR)/LUA52LD/src: $(SRCDIR)/$(LUASRC)
	mkdir -p $(COMPDIR)/$(LUAVER)
	tar -xvzf $(SRCDIR)/$(LUASRC) -C $(COMPDIR)/$(LUAVER) --strip-components=1
	patch -d $(COMPDIR)/$(LUAVER)/src/ -i $(SRCDIR)/lua-5.2.2_ll.patch || exit
	patch -d $(COMPDIR)/$(LUAVER)/src/ -i $(SRCDIR)/lua-5.2.2_upstream.patch || exit
	patch -d $(COMPDIR)/$(LUAVER)/src/ -i $(XTDIR)/build_static.patch -p2 || exit

$(COMPDIR)/LUA53/src: $(SRCDIR)/$(LUASRC)
	mkdir -p $(COMPDIR)/$(LUAVER)
	tar -xvzf $(SRCDIR)/$(LUASRC) -C $(COMPDIR)/$(LUAVER) --strip-components=1
	patch -d $(COMPDIR)/$(LUAVER)/src/ -i $(XTDIR)/build_static.patch -p2 || exit

$(XTDIR)/xt.a: $(COMPDIR)/$(LUAVER)/src
	cd $(XTDIR) ; $(MAKE) CC=$(CC) LD=$(LD) \
		MYCFLAGS=' -g' \
		INCS="$(COMPDIR)/$(LUAVER)/src" xt.a

$(COMPDIR)/$(LUAVER)/src/lua: $(XTDIR)/xt.a
	cd $(COMPDIR)/$(LUAVER) ; \
		$(MAKE) CC=$(CC) LD=$(LD) \
		MYCFLAGS=" -g -fPIC -DL_XT_ROOT=\"\\\"$(OUTDIR)/\\\"\" -I$(XTDIR)" \
		MYOBJS="$(XTDIR)/l_xt*.o" \
		linux

install: $(COMPDIR)/$(LUAVER)/src/lua
	cd $(COMPDIR)/$(LUAVER) ; $(MAKE) CC=$(CC) LD=$(LD) \
		INSTALL_TOP="$(OUTDIR)" \
		install

clean:
	cd $(XTDIR); make clean
	rm -rf $(COMPDIR) $(OUTDIR)

pristine:
	rm $(SRCDIR)/$(LUASRC)
