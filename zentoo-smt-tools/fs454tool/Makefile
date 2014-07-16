#  fs454tool: 
#
#  A tool to setup the fs454 in the SMT 7020S Set-Top-Box.
#

CFLAGS = -O2 -Wall
CC = gcc
INSTALL = install
TARGET = /usr/sbin

PREFIX = ""

OBJS=fs454tool.o
PROG=fs454tool

all: fs454tool

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

fs454tool: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(PROG)

install: $(PROG)
	$(INSTALL) $(PROG) $(PREFIX)$(TARGET)/$(PROG)

clean:
	rm -f $(OBJS) $(PROG)

	
