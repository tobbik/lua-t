# vim: ft=make


all: lxtcompile

$(SRCDIR)/$(LUASRC):
	wget http://www.lua.org/ftp/$(LUASRC) -O $(SRCDIR)/$(LUASRC)

luasource: $(SRCDIR)/$(LUASRC)
	mkdir -p $(COMPDIR)/Lua
	tar -xvzf $(SRCDIR)/$(LUASRC) -C $(COMPDIR)/Lua --strip-components=1
	patch -d $(COMPDIR)/Lua/src/ -i $(SRCDIR)/lua-5.2.2_upstream.patch || exit
	patch -d $(COMPDIR)/Lua/src/ -i $(SRCDIR)/../xt/build_static.patch -p2 || exit

xtsource:
	mkdir -p $(COMPDIR)/xt
	cp -avr $(XTDIR)/* $(COMPDIR)/xt/

xtcompile: luasource xtsource
	#rm $(COMPDIR)/xt/*.o ; $(MAKE) clean
	cd $(COMPDIR)/xt ; $(MAKE) CC=$(CC) LD=$(LD) \
		MYCFLAGS=' -g' \
		INCS="$(COMPDIR)/Lua/src" all || exit

lxtcompile: xtcompile
	cd $(COMPDIR)/Lua ; $(MAKE) CC=$(CC) LD=$(LD) \
		MYCFLAGS=" -g -fPIC -DL_XT_ROOT=\"\\\"$(OUTDIR)/\\\"\" -I$(COMPDIR)/xt" \
		MYOBJS="$(COMPDIR)/xt/l_xt*.o" \
		linux

lxtinstall: lxtcompile
	cd $(COMPDIR)/Lua ; $(MAKE) CC=$(CC) LD=$(LD) \
		INSTALL_TOP="$(OUTDIR)" \
		install

clean:
	rm -rf Lua
