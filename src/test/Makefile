# vim: ts=3 sw=3 st=3 sts=3 sta noet tw=80 list
#
# \file      src/test/Makefile
# \brief     Makefile for the lua-t library source code
# \author    tkieslich
# \copyright See Copyright notice at the end of t.h

T_SRC=t_tim.c

T_MAIN=t_unittest_main.c

#
LVER=5.3
PREFIX=$(shell pkg-config --variable=prefix lua)
INCDIR=$(shell pkg-config --variable=includedir lua)
INCS=-I$(INCDIR) -I../
LDFLAGS:=$(LDFLAGS) -lcrypt
# clang can be substituted with gcc (command line args compatible)
CC=clang
LD=clang
CFLAGS:=-Wall -Wextra -O0 -std=c99 -fpic $(MYCFLAGS) $(T_PRE)

T_EXE=$(T_SRC:.c=)

all: cat $(T_SRC) $(T_EXE)
	$(foreach exe,$(T_EXE),./$(exe);)

cat: t_unittest_cat.c
	$(CC) $(CFLAGS) t_unittest_cat.c -o $@ $(LIBS)

%: %.c
	./cat ../$< ./$< | $(CC) -x c $(INCS) $(CFLAGS) - -o $@ $(LDFLAGS) $(LIBS)

clean:
	-rm $(T_EXE) cat
