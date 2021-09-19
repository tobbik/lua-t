# vim: ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
#
# \file      src/Makefile
# \brief     Makefile for the lua-t library source code
# \author    tkieslich
# \copyright See Copyright notice at the end of t.h
#
LVER=5.4
CURDIR != pwd
INCDIR != /bin/sh ../guess_incdir.sh
LDFLAGS != /bin/sh ../guess_libs.sh

# clang can be substituted with gcc (command line args compatible)
CC=gcc
LD=gcc
CFLAGS+= $(INCDIR) -Wall -Wextra -Werror -std=c99 $(SYSCFLAGS) $(MYCFLAGS)
INSTALL_CMOD=$(PREFIX)/lib/lua/$(LVER)/t
PLAT= guess
ALL= all

# How to install. If your install program does not support "-p", then
# you may have to run ranlib on the installed t.a
INSTALLER != which install
INSTALL_DATA=$(INSTALLER) -p -Dm 0644
RM != which rm
LS != which ls
UNAME != which uname

# #######################################################################
# ALL SOURCES
T_SRC=t.c $(T_DBG_SRC)
AEL_FLS != $(LS) t_ael*.c
BUF_FLS != $(LS) t_buf*.c
CSV_FLS != $(LS) t_csv*.c
ENC_FLS != $(LS) t_enc*.c
HTP_FLS != $(LS) t_htp*.c
NET_FLS != $(LS) t_net*.c
NRY_FLS != $(LS) t_nry*.c
PCK_FLS != $(LS) t_pck*.c

T_AEL_SRC=$(T_SRC) $(AEL_FLS) t_net.c $(T_AEL_PLT_SRC)
T_BUF_SRC=$(T_SRC) $(BUF_FLS)
T_CSV_SRC=$(T_SRC) $(CSV_FLS)
T_ENC_SRC=$(T_SRC) $(ENC_FLS) t_buf.c
T_HTP_SRC=$(T_SRC) $(HTP_FLS) t_buf.c
T_NET_SRC=$(T_SRC) $(NET_FLS) t_buf.c $(T_NET_PLT_SRC)
T_NRY_SRC=$(T_SRC) $(NRY_FLS)
T_PCK_SRC=$(T_SRC) $(PCK_FLS) t_buf.c

# ALL OBJECTS
T_AEL_OBJ=$(T_AEL_SRC:%.c=%.o)
T_BUF_OBJ=$(T_BUF_SRC:%.c=%.o)
T_CSV_OBJ=$(T_CSV_SRC:%.c=%.o)
T_ENC_OBJ=$(T_ENC_SRC:%.c=%.o)
T_HTP_OBJ=$(T_HTP_SRC:%.c=%.o)
T_NET_OBJ=$(T_NET_SRC:%.c=%.o)
T_NRY_OBJ=$(T_NRY_SRC:%.c=%.o)
T_PCK_OBJ=$(T_PCK_SRC:%.c=%.o)

OBJS=$(T_AEL_OBJ) $(T_BUF_OBJ) $(T_CSV_OBJ) $(T_ENC_OBJ) $(T_HTP_OBJ) $(T_NET_OBJ) $(T_PCK_OBJ) $(T_NRY_OBJ)

# ALL DYNAMIC OBJECTS
T_AEL_LIB=ael.so
T_BUF_LIB=buf.so
T_CSV_LIB=csv.so
T_ENC_LIB=enc.so
T_HTP_LIB=htp.so
T_NET_LIB=net.so
#T_NRY_LIB=nry.so   Add `T_NRY_LIB=nry.so` to `make` and `make install` if you wish to install it
T_PCK_LIB=pck.so

LIBS=$(T_AEL_LIB) $(T_BUF_LIB) $(T_CSV_LIB) $(T_ENC_LIB) $(T_HTP_LIB) $(T_NET_LIB) $(T_PCK_LIB) $(T_NRY_LIB)

T_INSTALL=$(LIBS:%.so=$(INSTALL_CMOD)/%.so)

# ##############################################
# TARGETS
default: $(PLAT)

all: $(LIBS)

$(T_AEL_LIB): $(T_AEL_OBJ)
	$(LD) -shared $^ -o $@ $(LDFLAGS)
$(T_BUF_LIB): $(T_BUF_OBJ)
	$(LD) -shared $^ -o $@ $(LDFLAGS)
$(T_CSV_LIB): $(T_CSV_OBJ)
	$(LD) -shared $^ -o $@ $(LDFLAGS)
$(T_ENC_LIB): $(T_ENC_OBJ)
	$(LD) -shared $^ -o $@ $(LDFLAGS)
$(T_HTP_LIB): $(T_HTP_OBJ)
	$(LD) -shared $^ -o $@ $(LDFLAGS)
$(T_NET_LIB): $(T_NET_OBJ)
	$(LD) -shared $^ -o $@ $(LDFLAGS)
$(T_PCK_LIB): $(T_PCK_OBJ)
	$(LD) -shared $^ -o $@ $(LDFLAGS)
$(T_NRY_LIB): $(T_NRY_OBJ)
	$(LD) -shared $^ -o $@ $(LDFLAGS)

guess:
	@echo Guessing `$(UNAME)`
	@$(MAKE) `$(UNAME)`

Linux linux:
	$(MAKE) \
	T_AEL_PLT_SRC="p_ael_epl.c" \
	T_NET_PLT_SRC="p_net_sck_unx.c p_net_ifc_lnx.c" \
	SYSCFLAGS="-D LUAT_USE_LINUX -fpic" \
	$(ALL)

FreeBSD NetBSD OpenBSD freebsd:
	$(MAKE) \
	T_AEL_PLT_SRC= \
	T_NET_PLT_SRC= \
	SYSCFLAGS="-D LUAT_USE_POSIX -fpic" \
	CC=clang \
	LD=clang \
	$(ALL)

install: $(LIBS) $(T_INSTALL)

test: $(SRCDIR)
	$(MAKE) -C $(CURDIR)/test CC=$(CC) LD=$(LD) \
	 LVER=$(LVER) \
	 MYCFLAGS="$(MYCFLAGS)" \
	 LDFLAGS="$(LDFLAGS)" \
	 INCDIR=$(INCDIR)

$(INSTALL_CMOD)/%.so: %.so
	$(INSTALL_DATA) $^ $@

echo:
	@echo "PLAT= $(PLAT)"
	@echo "LVER= $(LVER)"
	@echo "PREFIX= $(PREFIX)"
	@echo "CC= $(CC)"
	@echo "LD= $(LD)"
	@echo "INCDIR= $(INCDIR)"
	@echo "LDFLAGS= $(LDFLAGS)"
	@echo "CFLAGS= $(CFLAGS)"
	@echo "RM= $(RM)"
	@echo "INSTALL_CMOD= $(INSTALL_CMOD)"

clean:
	$(MAKE) -C $(CURDIR)/test clean
	$(MAKE) -C $(CURDIR) \
	 T_AEL_PLT_SRC="p_ael_epl.c p_ael_sel.c" \
	 T_NET_PLT_SRC="p_net_sck_unx.c p_net_ifc_lnx.c" \
	 delobjects

delobjects:
	-$(RM) -f $(LIBS)
	-$(RM) -f $(OBJS)

uninstall:
	-$(RM) -rf $(INSTALL_CMOD)

pristine:
	-$(RM) -f *.so
	-$(RM) -f *.o

.PHONY: all test clean

