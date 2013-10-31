LUASRC=lua-5.2.2.tar.gz
COMPDIR=$(CURDIR)/compile
OUTDIR=$(CURDIR)/out
SRCDIR=$(CURDIR)/sources
XTDIR=$(CURDIR)/xt
CC=clang
LD=clang

lxt: compiledir
	cp $(SRCDIR)/lxt.make $(COMPDIR)/Makefile
	cd $(COMPDIR) ; \
	$(MAKE) CC=$(CC) LD=$(LD) \
		LUASRC=$(LUASRC) \
		COMPDIR=$(COMPDIR) \
		SRCDIR=$(SRCDIR) \
		OUTDIR=$(OUTDIR) \
		XTDIR=$(XTDIR) lxtinstall

compiledir:
	mkdir -p $(COMPDIR)

#luasource: $(SRCDIR)/$(LUASRC) compiledir
#	mkdir -p $(COMPDIR)/Lua
#	tar -xvzf $(SRCDIR)/$(LUASRC) -C $(COMPDIR)/Lua --strip-components=1
#
#$(SRCDIR)/$(LUASRC):
#	wget http://www.lua.org/ftp/$(LUASRC) -O $(SRCDIR)/$(LUASRC)
#
#luapatch: luasource
#	patch -d $(COMPDIR)/Lua/src/ -i $(CURDIR)/$(SRCDIR)/lua-5.2.2_upstream.patch || exit

clean:
	rm -rf $(COMPDIR) $(OUTDIR)
