#
# Gopherus Makefile for Linux (and BSD, and probably other POSIX systems)
# Copyright (C) 2013-2020 Mateusz Viste
#

CC ?= gcc
CFLAGS = -std=gnu89 -O3 -Wall -Wextra -pedantic

all: gopherus

gopherus: gopherus.o dnscache.o fs/fs-lin.o history.o net-bsd.o parseurl.o readflin.o startpg.o ui-curse.o wordwrap.o
	$(CC) gopherus.o dnscache.o fs/fs-lin.o history.o net-bsd.o parseurl.o readflin.o startpg.o ui-curse.o wordwrap.o -o gopherus -lncursesw $(CFLAGS)

gopherus-sdl: gopherus.o dnscache.o fs/fs-lin.o history.o net-bsd.o parseurl.o readflin.o startpg.o ui-sdl.o wordwrap.o
	$(CC) gopherus.o dnscache.o fs/fs-lin.o history.o net-bsd.o parseurl.o readflin.o startpg.o ui-sdl.o wordwrap.o -o gopherus-sdl -lSDL2 $(CFLAGS)

net-bsd.o: net/net-bsd.c
	$(CC) -c net/net-bsd.c -o net-bsd.o $(CFLAGS)

ui-curse.o: ui/ui-curse.c
	$(CC) -c ui/ui-curse.c -o ui-curse.o $(CFLAGS)

ui-sdl.o: ui/ui-sdl.c
	$(CC) -c ui/ui-sdl.c -o ui-sdl.o $(CFLAGS)

wraptest: wraptest.c wordwrap.o
	$(CC) wraptest.c wordwrap.o -o wraptest $(CFLAGS)

clean:
	rm -f *.o fs/*.o net/*.o idoc/*.o ui/*.o
	rm -f gopherus gopherus-sdl

