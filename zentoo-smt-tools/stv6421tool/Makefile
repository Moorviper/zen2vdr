#  stv6421tool: 
#
#  A tool to setup the stv6421 in the SMT 7020S Set-Top-Box.
#

CFLAGS = -O2 -Wall
CC = gcc
INSTALL = install
TARGET = /usr/sbin

PREFIX = ""

OBJS=stv6421tool.o
PROG=stv6421tool

all: stv6421tool

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

fs454tool: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(PROG)

install: $(PROG)
	$(INSTALL) $(PROG) $(PREFIX)$(TARGET)/$(PROG)

clean:
	rm -f $(OBJS) $(PROG)

	
