VER=5.3
ifeq ($(VER), 5.3)
	LUASRC=lua-5.3.0-rc3.tar.gz
	DLPATH=http://www.lua.org/work
else
	LUASRC=lua-5.2.3.tar.gz
	DLPATH=http://www.lua.org/ftp
endif
COMPDIR=$(CURDIR)/compile
OUTDIR=$(CURDIR)/out
SRCDIR=$(CURDIR)/sources
TDIR=$(CURDIR)/t
CC=clang
LD=clang
relocate=true

all: install

$(SRCDIR)/$(LUASRC):
	curl -o $(SRCDIR)/$(LUASRC)   $(DLPATH)/$(LUASRC) 

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
		$(MAKE) -j 4 CC=$(CC) LD=$(LD) \
		MYCFLAGS=" -g -fPIC "\
		linux

$(TDIR)/t.so: $(COMPDIR)/$(VER)/src
	cd $(TDIR) ; $(MAKE) -j 4 CC=$(CC) LD=$(LD) \
		VER=$(VER) \
		MYCFLAGS=' -g' \
		INCS="$(COMPDIR)/$(VER)/src" t.so

install: $(COMPDIR)/$(VER)/src/lua $(TDIR)/t.so
	cd $(COMPDIR)/$(VER) ; $(MAKE) CC=$(CC) LD=$(LD) \
		VER=$(VER) \
		INSTALL_TOP="$(OUTDIR)" \
		install
	cd $(TDIR) ; $(MAKE) CC=$(CC) LD=$(LD) \
		VER=$(VER) \
		MYCFLAGS=' -g' \
		INCS="$(COMPDIR)/$(VER)/src" \
		PREFIX="$(OUTDIR)" install

clean:
	cd $(TDIR); make clean
	rm -rf $(COMPDIR)

remove: clean
	rm -rf $(OUTDIR)

pristine:
	rm $(SRCDIR)/$(LUASRC)
