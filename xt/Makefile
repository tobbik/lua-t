# vim: ts=4 sw=4 st=4 sta tw=80 list
#
# clang can be substituted with gcc (command line args compatible)
CC= gcc
LD= gcc
CFLAGS= -Wall -Wextra -g -O0 -std=c99 -fpic
#INCS=$(shell pkg-config --cflags lua5.1)
#LDFLAGS=$(shell pkg-config --libs lua5.1)
AR= ar rcu
RANLIB= ranlib

XT_SRC=l_xt.c \
	 l_xt_socket.c \
	 l_xt_ip.c \
	 l_xt_timer.c \
	 xt_buf.c \
	 xt_buf_fld.c \
	 l_xt_debug.c

XT_OBJ=$(XT_SRC:.c=.o)

XT_LIB=xt.so
XT_LIB_A=xt.a

all: $(XT_SRC) $(XT_LIB) $(XT_LIB_A)

$(XT_LIB): $(XT_OBJ)
	$(LD) -O -shared $(XT_OBJ) -o $@ $(LDFLAGS) $(LIBS)

$(XT_LIB_A): $(XT_OBJ)
	$(AR) $@ $(XT_OBJ)
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

install: $(XT_SRC) $(XT_LIB)
	cp $(XT_LIB)   $(PREFIX)/lib/lua/5.2/$(XT_LIB)

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
	-rm $(XT_OBJ) $(XT_LIB) $(XT_LIB_A)
