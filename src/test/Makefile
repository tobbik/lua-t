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

T_OBJ=$(T_SRC:.c=.o)
T_EXE=$(T_SRC:.c=)

all: $(T_SRC) $(T_OBJ) t_unittest.o $(T_EXE)
	$(foreach exe,$(T_EXE),./$(exe);)

%.o: %.c
	cat ../$< $< $(T_MAIN) | $(CC) -x c $(INCS) $(CFLAGS) -c - -o $@

%: %.o
	$(LD) t_unittest.o $< -o $@ $(LDFLAGS) $(LIBS)

t_unittest.o: t_unittest.c
	$(CC) $(CFLAGS) -c t_unittest.c -o t_unittest.o


clean:
	-rm $(T_OBJ) $(T_EXE) t_unittest.o
