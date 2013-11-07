LUASRC=lua-5.2.2.tar.gz
DLPATH=http://www.lua.org/ftp
COMPDIR=$(CURDIR)/compile
OUTDIR=$(CURDIR)/out
SRCDIR=$(CURDIR)/sources
XTDIR=$(CURDIR)/xt
CC=clang
LD=clang

lxt: $(COMPDIR)/xt/xt.so
	cp $(SRCDIR)/lxt.make $(COMPDIR)/Makefile
	cd $(COMPDIR) ; \
	$(MAKE) CC=$(CC) LD=$(LD) \
		COMPDIR=$(COMPDIR) \
		OUTDIR=$(OUTDIR)
	$(MAKE) install

lxt53: $(COMPDIR)/xt/xt.so
	cp $(SRCDIR)/lxt53.make $(COMPDIR)/Makefile
	cd $(COMPDIR) ; \
	$(MAKE) CC=$(CC) LD=$(LD) \
		LUASRC=lua-5.3.0-work1.tar.gz \
		DLPATH=http://www.lua.org/work \
		COMPDIR=$(COMPDIR) \
		SRCDIR=$(SRCDIR) \
		OUTDIR=$(OUTDIR) \
		XTDIR=$(XTDIR) lxtinstall

$(SRCDIR)/$(LUASRC):
	wget $(DLPATH)/$(LUASRC) -O $(SRCDIR)/$(LUASRC)

$(COMPDIR)/Lua/src: $(SRCDIR)/$(LUASRC)
	mkdir -p $(COMPDIR)/Lua
	tar -xvzf $(SRCDIR)/$(LUASRC) -C $(COMPDIR)/Lua --strip-components=1
	patch -d $(COMPDIR)/Lua/src/ -i $(SRCDIR)/lua-5.2.2_upstream.patch || exit
	patch -d $(COMPDIR)/Lua/src/ -i $(SRCDIR)/../xt/build_static.patch -p2 || exit

$(COMPDIR)/xt:
	mkdir -p $(COMPDIR)/xt
	cp -ar $(XTDIR)/* $(COMPDIR)/xt/

$(COMPDIR)/xt/xt.so: $(COMPDIR)/Lua/src $(COMPDIR)/xt
	cd $(COMPDIR)/xt ; $(MAKE) CC=$(CC) LD=$(LD) \
		MYCFLAGS=' -g' \
		INCS="$(COMPDIR)/Lua/src" all || exit

install: $(COMPDIR)/Lua/src/lua
	cd $(COMPDIR)/Lua ; $(MAKE) CC=$(CC) LD=$(LD) \
		INSTALL_TOP="$(OUTDIR)" \
		install

clean:
	rm -rf $(COMPDIR) $(OUTDIR)

pristine:
	rm $(SRCDIR)/$(LUASRC)
