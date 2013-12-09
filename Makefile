LUAVER=LUA52
ifeq ($(LUAVER), LUA53)
	LUASRC=lua-5.3.0-work1.tar.gz
	DLPATH=http://www.lua.org/work
else
	LUASRC=lua-5.2.3.tar.gz
	DLPATH=http://www.lua.org/ftp
endif
COMPDIR=$(CURDIR)/compile
OUTDIR=$(CURDIR)/out
SRCDIR=$(CURDIR)/sources
XTDIR=$(CURDIR)/xt
CC=clang
LD=clang
relocate=true

all: install

$(SRCDIR)/$(LUASRC):
	wget $(DLPATH)/$(LUASRC) -O $(SRCDIR)/$(LUASRC)

$(COMPDIR)/$(LUAVER)/src: $(SRCDIR)/$(LUASRC)
	mkdir -p $(COMPDIR)/$(LUAVER)
	tar -xvzf $(SRCDIR)/$(LUASRC) -C $(COMPDIR)/$(LUAVER) --strip-components=1
#ifeq ($(LUAVER),LUA52)
#		patch -d $(COMPDIR)/$(LUAVER)/src/ -i $(SRCDIR)/lua-5.2.3_upstream.patch
#endif
ifeq ($(LUAVER), LUA52LD)
		patch -d $(COMPDIR)/$(LUAVER)/src/ -i $(SRCDIR)/lua-5.2.2_ll.patch
endif
ifeq ($(relocate), true)
		patch -d $(COMPDIR)/$(LUAVER)/src/ -i $(SRCDIR)/lua-5.2.3_relocate.patch
endif

$(COMPDIR)/$(LUAVER)/src/lua: $(COMPDIR)/$(LUAVER)/src
	cd $(COMPDIR)/$(LUAVER) ; \
		$(MAKE) CC=$(CC) LD=$(LD) \
		MYCFLAGS=" -g -fPIC "\
		linux

$(XTDIR)/xt.so: $(COMPDIR)/$(LUAVER)/src
	cd $(XTDIR) ; $(MAKE) CC=$(CC) LD=$(LD) \
		MYCFLAGS=' -g' \
		INCS="$(COMPDIR)/$(LUAVER)/src" xt.so

install: $(COMPDIR)/$(LUAVER)/src/lua $(XTDIR)/xt.so
	cd $(COMPDIR)/$(LUAVER) ; $(MAKE) CC=$(CC) LD=$(LD) \
		INSTALL_TOP="$(OUTDIR)" \
		install
	cd $(XTDIR) ; $(MAKE) CC=$(CC) LD=$(LD) \
		PREFIX="$(OUTDIR)" install

clean:
	cd $(XTDIR); make clean
	rm -rf $(COMPDIR) $(OUTDIR)

pristine:
	rm $(SRCDIR)/$(LUASRC)
