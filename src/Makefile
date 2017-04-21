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
#LDFLAGS=$(shell pkg-config --libs lua) -lcrypt
LDFLAGS:=$(LDFLAGS) -lcrypt
# clang can be substituted with gcc (command line args compatible)
CC=clang
LD=clang
#CFLAGS:= -Wall -Wextra -pedantic -O0 -std=c99 -fpic $(MYCFLAGS)
CFLAGS:= -Wall -Wextra -O0 -std=c99 -fpic $(MYCFLAGS)
LDLIBS:= -llua -lm
INSTALL_CMOD=$(PREFIX)/lib/lua/$(LVER)/t

# How to install. If your install program does not support "-p", then
# you may have to run ranlib on the installed t.a
INSTALL= install -p
INSTALL_EXEC= $(INSTALL) -m 0755
INSTALL_DATA= $(INSTALL) -m 0644


# #######################################################################
# ALL SOURCES
T_AEL_SRC=$(wildcard t_ael*.c) t_net_cmn.c t_tim_cmn.c t_dbg.c t.c
T_BAS_SRC=$(wildcard t_bas*.c)
T_BUF_SRC=$(wildcard t_buf*.c)
T_ENC_SRC=$(wildcard t_enc*.c)
T_HTP_SRC=$(wildcard t_htp*.c) t_net_cmn.c
T_NET_SRC=$(wildcard t_net*.c) t_buf_cmn.c t_dbg.c t.c
T_PCK_SRC=$(wildcard t_pck*.c) t_buf_cmn.c
T_TIM_SRC=$(wildcard t_tim*.c)

# #######################################################################
# CONDITIONAL INCLUSION OF SOURCES AND OBJETCS
ifdef BUILD_EXAMPLE
   CFLAGS += -D T_NRY=1
   T_NRY_SRC= t_nry.c
   T_NRY_OBJ= t_nry.o
   T_LIB:=$(T_LIB) t_nry.so
endif

ifeq ($(OS),Windows_NT)
   CFLAGS += -D WIN32
   T_AEL_SRC:=$(T_AEL_SRC) p_ael_sel.c
   ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
      CFLAGS += -D AMD64
   else
      ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
         CFLAGS += -D AMD64
      endif
      ifeq ($(PROCESSOR_ARCHITECTURE),x86)
         CFLAGS += -D IA32
      endif
   endif
else
   UNAME_S := $(shell uname -s)
   ifeq ($(UNAME_S),Linux)
      CFLAGS += -D LINUX
      T_AEL_SRC:=$(T_AEL_SRC) p_ael_epl.c
      T_NET_SRC:=$(T_NET_SRC) p_net_sck_unx.c
   endif
   ifeq ($(UNAME_S),Darwin)
      CFLAGS += -D OSX
   endif
   UNAME_I := $(shell uname -i)
   ifeq ($(UNAME_I),x86_64)
      CFLAGS += -D AMD64
   endif
   ifneq ($(filter %86,$(UNAME_I)),)
      CFLAGS += -D IA32
   endif
   ifneq ($(filter arm%,$(UNAME_I)),)
      CFLAGS += -D ARM
   endif
endif

# ALL OBJECTS
T_AEL_OBJ=$(T_AEL_SRC:%.c=%.o)
T_BAS_OBJ=$(T_BAS_SRC:%.c=%.o)
T_BUF_OBJ=$(T_BUF_SRC:%.c=%.o)
T_ENC_OBJ=$(T_ENC_SRC:%.c=%.o)
T_HTP_OBJ=$(T_HTP_SRC:%.c=%.o)
T_NET_OBJ=$(T_NET_SRC:%.c=%.o)
T_PCK_OBJ=$(T_PCK_SRC:%.c=%.o)
T_TIM_OBJ=$(T_TIM_SRC:%.c=%.o)

OBJS=$(T_AEL_OBJ) $(T_BAS_OBJ) $(T_BUF_OBJ) $(T_ENC_OBJ) $(T_HTP_OBJ) $(T_NET_OBJ) $(T_PCK_OBJ) $(T_TIM_OBJ)

# ALL DYNAMIC OBJECTS
T_AEL_LIB=ael.so
T_BAS_LIB=bas.so
T_BUF_LIB=buf.so
T_ENC_LIB=enc.so
T_HTP_LIB=htp.so
T_NET_LIB=net.so
T_PCK_LIB=pck.so
T_TIM_LIB=tim.so

LIBS=$(T_AEL_LIB) $(T_BAS_LIB) $(T_BUF_LIB) $(T_ENC_LIB) $(T_HTP_LIB) $(T_NET_LIB) $(T_PCK_LIB) $(T_TIM_LIB)

T_INSTALL=$(LIBS:%.so=$(INSTALL_CMOD)/%.so)

# ############################################################
# TARGETS
all: $(OBJS) $(LIBS)

$(T_AEL_LIB): $(T_AEL_OBJ)
	$(LD) -O -shared $? -o $@ $(LDFLAGS)
$(T_BAS_LIB): $(T_BAS_OBJ)
	$(LD) -O -shared $? -o $@ $(LDFLAGS)
$(T_BUF_LIB): $(T_BUF_OBJ)
	$(LD) -O -shared $? -o $@ $(LDFLAGS)
$(T_ENC_LIB): $(T_ENC_OBJ)
	$(LD) -O -shared $? -o $@ $(LDFLAGS)
$(T_HTP_LIB): $(T_HTP_OBJ)
	$(LD) -O -shared $? -o $@ $(LDFLAGS)
$(T_NET_LIB): $(T_NET_OBJ)
	$(LD) -O -shared $? -o $@ $(LDFLAGS)
$(T_PCK_LIB): $(T_PCK_OBJ)
	$(LD) -O -shared $? -o $@ $(LDFLAGS)
$(T_TIM_LIB): $(T_TIM_OBJ)
	$(LD) -O -shared $? -o $@ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) $< -o $@

install: $(LIBS) $(T_INSTALL)

test: $(SRCDIR)
	$(MAKE) -C $(CURDIR)/test CC=$(CC) LD=$(LD) \
		LVER=$(LVER) \
		MYCFLAGS="$(MYCFLAGS)" \
		LDFLAGS="$(LDFLAGS)" \
		INCDIR=$(INCDIR)

%.so: %.o
	$(LD) -O -shared $? -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) -I$(INCS) $(CFLAGS) -c $< -o $@

$(INSTALL_CMOD)/%.so: %.so
	$(INSTALL_DATA) -Dm644 $< $@

echo:
	@echo "PLAT= $(PLAT)"
	@echo "LVER= $(LVER)"
	@echo "PREFIX= $(PREFIX)"
	@echo "CC= $(CC)"
	@echo "LD= $(LD)"
	@echo "INCDIR= $(INCDIR)"
	@echo "LDFLAGS= $(LDFLAGS)"
	@echo "CFLAGS= $(CFLAGS)"
	@echo "LDLIBS= $(LDLIBS)"
	@echo "AR= $(AR)"
	@echo "RANLIB= $(RANLIB)"
	@echo "RM= $(RM)"

clean:
	$(MAKE) -C $(CURDIR)/test clean
	-rm $(OBJS) $(LIBS)

.PHONY: all test clean


SO: $(T_OBJ)
	clang -O -shared t_net_cmn.o t_tim_cmn.o t_ael*o -o ael.so -llua -lm
	clang -O -shared             t_buf*o -o buf.so -llua -lm
	clang -O -shared t_enc*o             -o enc.so -llua -lm
	clang -O -shared t_net_cmn.o t_htp*o -o htp.so -llua -lm
	clang -O -shared t_buf_cmn.o t_net*o -o net.so -llua -lm
	clang -O -shared t_buf_cmn.o t_pck*o -o pck.so -llua -lm
	clang -O -shared             t_tim*o -o tim.so -llua -lm
