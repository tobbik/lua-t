# vim: ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
#
# \file      Makefile
# \brief     Makefile for the lua-t library
# \author    tkieslich
# \copyright See Copyright notice at the end of t.h

#
# clang can be substituted with gcc (command line args compatible)
CC= clang
LD= clang
CFLAGS= -Wall -Wextra -g -O0 -std=c99 -fpic
#INCS=$(shell pkg-config --cflags lua5.1)
#LDFLAGS=$(shell pkg-config --libs lua5.1)
AR= ar rcu
RANLIB= ranlib
VER= 5.2

T_SRC=t.c \
	 t_sck.c \
	 t_ipx.c \
	 t_elp.c \
	 t_elp_sel.c \
	 t_tim.c \
	 t_enc.c \
	 t_enc_arc4.c \
	 t_enc_crc.c \
	 t_enc_b64.c \
	 t_buf.c \
	 t_pck.c \
	 t_pckc.c \
	 t_tst.c


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

default:
	$(MAKE) intel

system:
	$(MAKE) \
	"CC=$(CC)" \
	"LD=$(LD)" \
	"INCS=$(shell pkg-config --cflags lua)" \
	"LDFLAGS=$(shell pkg-config --libs lua)" \
	all

local:
	$(MAKE) \
	"CC=$(CC)" \
	"LD=$(LD)" \
	"INCS=$(INCS)" \
	"LDFLAGS=$(LIBS)" \
	all

arm:
	$(MAKE) \
	"CC=$(HOME)/BLD/arm/host/usr/bin/arm-linux-gcc" \
	"LD=$(HOME)/BLD/arm/host/usr/bin/arm-linux-gcc" \
	"LDFLAGS=-llua -lm -ldl -fPIC" \
	"CFLAGS=$(CFLAGS) -fPIC" \
	"LDFLAGS=-L$(HOME)/BLD/arm/staging/usr/lib" \
	"INCS=-I$(HOME)/BLD/arm/staging/usr/include" \
	all

install: $(T_SRC) $(T_LIB)
	cp $(T_LIB)   $(PREFIX)/lib/lua/$(VER)/$(T_LIB)

#%: %.c
#	$(CC) -I$(INCS) $(CFLAGS) -c $< -o $@.o
#	$(CC) $@.o -o $@ $(LDFLAGS) $(LIBS)
#	rm $@.o

%: %.o
	$(CC) $< -o $@ $(LDFLAGS) $(LIBS)

%.so: %.o
	$(LD) -O -shared $< -o $@ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) -I$(INCS) $(CFLAGS) -c $< -o $@

clean:
	-rm $(T_OBJ) $(T_LIB) $(T_LIB_A)
