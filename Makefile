VER=5.2
ifeq ($(VER), 5.3)
	LUASRC=lua-5.3.0-alpha.tar.gz
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

$(COMPDIR)/$(VER)/src: $(SRCDIR)/$(LUASRC)
	mkdir -p $(COMPDIR)/$(VER)
	tar -xvzf $(SRCDIR)/$(LUASRC) -C $(COMPDIR)/$(VER) --strip-components=1
#ifeq ($(VER),5.2)
#		patch -d $(COMPDIR)/$(VER)/src/ -i $(SRCDIR)/lua-5.2.3_upstream.patch
#endif
ifeq ($(relocate), true)
ifeq ($(VER), 5.2)
		patch -d $(COMPDIR)/$(VER)/src/ -i $(SRCDIR)/lua-5.2.3_relocate.patch
endif
ifeq ($(VER), 5.3)
		patch -d $(COMPDIR)/$(VER)/src/ -i $(SRCDIR)/lua-5.3_relocate.patch
endif
endif

$(COMPDIR)/$(VER)/src/lua: $(COMPDIR)/$(VER)/src
	cd $(COMPDIR)/$(VER) ; \
		$(MAKE) CC=$(CC) LD=$(LD) \
		MYCFLAGS=" -g -fPIC "\
		linux

$(XTDIR)/xt.so: $(COMPDIR)/$(VER)/src
	cd $(XTDIR) ; $(MAKE) CC=$(CC) LD=$(LD) \
		VER=$(VER) \
		MYCFLAGS=' -g' \
		INCS="$(COMPDIR)/$(VER)/src" xt.so

install: $(COMPDIR)/$(VER)/src/lua $(XTDIR)/xt.so
	cd $(COMPDIR)/$(VER) ; $(MAKE) CC=$(CC) LD=$(LD) \
		VER=$(VER) \
		INSTALL_TOP="$(OUTDIR)" \
		install
	cd $(XTDIR) ; $(MAKE) CC=$(CC) LD=$(LD) \
		VER=$(VER) \
		PREFIX="$(OUTDIR)" install

clean:
	cd $(XTDIR); make clean
	rm -rf $(COMPDIR) $(OUTDIR)

pristine:
	rm $(SRCDIR)/$(LUASRC)
