# vim: ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
#
# \file      src/Makefile
# \brief     Makefile for the lua-t library source code
# \author    tkieslich
# \copyright See Copyright notice at the end of t.h

#
LVER=5.3
PREFIX=$(shell pkg-config --variable=prefix lua)
INCDIR=$(shell pkg-config --variable=includedir lua)
LDFLAGS=$(shell pkg-config --libs lua)
# clang can be substituted with gcc (command line args compatible)
CC=clang
LD=clang
CFLAGS= -Wall -Wextra -g -O0 -std=c99 -fpic
AR=ar rcu
RANLIB=ranlib

T_SRC=t.c \
	 t_sck.c \
	 t_ipx.c \
	 t_ael.c \
	 t_ael_sel.c \
	 t_tim.c \
	 t_enc.c \
	 t_enc_arc4.c \
	 t_enc_crc.c \
	 t_enc_b64.c \
	 t_buf.c \
	 t_pck.c \
	 t_wsk.c \
	 t_tst.c \
	 t_htp.c \
	 t_htp_srv.c \
	 t_htp_con.c \
	 t_htp_str.c

T_OBJ=$(T_SRC:.c=.o)

T_LIB=t.so
T_LIB_A=t.a

all: $(T_SRC) $(T_LIB) $(T_LIB_A)

$(T_LIB): $(T_OBJ)
	$(LD) -O -shared $(T_OBJ) -o $@ $(LDFLAGS) $(LIBS) -lcrypt

$(T_LIB_A): $(T_OBJ)
	$(AR) $@ $(T_OBJ)
	$(RANLIB) $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

install: $(T_SRC) $(T_LIB) $(T_LIB_A)
	cp $(T_LIB) $(PREFIX)/lib/lua/$(LVER)/$(T_LIB)
	cp $(T_LIB_A) $(PREFIX)/lib/lua/$(LVER)/$(T_LIB_A)


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
	@echo "INCDIR= $(INCDIR)"
	@echo "LDFLAGS= $(LDFLAGS)"
	@echo "PLAT= $(PLAT)"
	@echo "CC= $(CC)"
	@echo "CFLAGS= $(CFLAGS)"
	@echo "LDFLAGS= $(SYSLDFLAGS)"
	@echo "LIBS= $(LIBS)"
	@echo "AR= $(AR)"
	@echo "RANLIB= $(RANLIB)"
	@echo "RM= $(RM)"

clean:
	-rm $(T_OBJ) $(T_LIB) $(T_LIB_A)
