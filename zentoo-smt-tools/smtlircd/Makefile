#  smtlircd: 
#
#  A lirc daemon for the Samsung SMT 7020S Set-Top-Box.
#
# To get Debug output enable "-DDEBUG" in CFLAGS

#CFLAGS = -O2 -Wall -DDEBUG
CFLAGS = -O2 -Wall
CC = gcc
INSTALL = install
TARGET = /usr/sbin

PREFIX = ""

OBJS=lirc.o remote.o display.o
PROG=smtlircd

all: smtlircd 

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

smtlircd: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(PROG)

install: $(PROG)
	$(INSTALL) $(PROG) $(PREFIX)$(TARGET)/$(PROG)

clean:
	rm -f $(OBJS) $(PROG)

	
