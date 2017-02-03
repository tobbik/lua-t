# vim: ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
#
# \file      src/Makefile
# \brief     Makefile for the lua-t library source code
# \author    tkieslich
# \copyright See Copyright notice at the end of t.h

T_LIB_DYN=t.so
T_LIB_STA=t.a

T_SRC=t.c \
	 t_net.c \
	 t_net_tcp.c \
	 t_net_udp.c \
	 t_net_ip4.c \
	 t_ael.c \
	 t_ael_sel.c \
	 t_tim.c \
	 t_enc.c \
	 t_enc_rc4.c \
	 t_enc_crc.c \
	 t_enc_b64.c \
	 t_buf.c \
	 t_pck.c \
	 t_pck_cmb.c \
	 t_pck_fmt.c \
	 t_wsk.c \
	 t_oht.c \
	 t_set.c \
	 t_htp.c \
	 t_htp_srv.c \
	 t_htp_con.c \
	 t_htp_str.c \
	 t_net_ifc.c \
	 t_tst.c \
	 t_tst_cse.c

T_PRE:=

ifdef BUILD_EXAMPLE
T_PRE:=$(T_PRE) -D T_NRY=1
T_SRC:=$(T_SRC) t_nry.c
endif

#
LVER=5.3
PREFIX=$(shell pkg-config --variable=prefix lua)
INCDIR=$(shell pkg-config --variable=includedir lua)
#LDFLAGS=$(shell pkg-config --libs lua) -lcrypt
LDFLAGS:=$(LDFLAGS) -lcrypt
# clang can be substituted with gcc (command line args compatible)
CC=clang
LD=clang
#CFLAGS:= -Wall -Wextra -O0 -std=c99 -fpic $(MYCFLAGS) $(T_PRE)
CFLAGS:= -Wall -Wextra -O0 -fpic $(MYCFLAGS) $(T_PRE)
AR=ar rcu
RANLIB=ranlib

T_OBJ=$(T_SRC:.c=.o)

all: $(T_SRC) $(T_LIB_DYN) $(T_LIB_STA)

$(T_LIB_DYN): $(T_OBJ)
	$(LD) -O -shared $(T_OBJ) -o $@ $(LDFLAGS) $(LIBS)

$(T_LIB_STA): $(T_OBJ)
	$(AR) $@ $(T_OBJ)
	$(RANLIB) $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

install: $(T_SRC) $(T_LIB_DYN) $(T_LIB_STA)
	cp $(T_LIB_DYN) $(PREFIX)/lib/lua/$(LVER)/$(T_LIB_DYN)
	cp $(T_LIB_STA) $(PREFIX)/lib/lua/$(LVER)/$(T_LIB_STA)

test: $(SRCDIR)
	$(MAKE) -C $(CURDIR)/test CC=$(CC) LD=$(LD) \
		LVER=$(LVER) \
		MYCFLAGS="$(MYCFLAGS)" \
		LDFLAGS="$(LDFLAGS)" \
		INCDIR=$(INCDIR)

%: %.o
	$(CC) $< -o $@ $(LDFLAGS) $(LIBS)

%.so: %.o
	$(LD) -O -shared $< -o $@ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) -I$(INCS) $(CFLAGS) -c $< -o $@

echo:
	@echo "PLAT= $(PLAT)"
	@echo "LVER= $(LVER)"
	@echo "PREFIX= $(PREFIX)"
	@echo "CC= $(CC)"
	@echo "LD= $(LD)"
	@echo "INCDIR= $(INCDIR)"
	@echo "LDFLAGS= $(LDFLAGS)"
	@echo "CFLAGS= $(CFLAGS)"
	@echo "LIBS= $(LIBS)"
	@echo "AR= $(AR)"
	@echo "RANLIB= $(RANLIB)"
	@echo "RM= $(RM)"

clean:
	$(MAKE) -C $(CURDIR)/test clean
	-rm $(T_OBJ) $(T_LIB_DYN) $(T_LIB_STA)

.PHONY: all test clean
