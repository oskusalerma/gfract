CC=gcc
#CFLAGS=-g -Wall
CFLAGS=-O2 -Wall
LIBS=-lpng -lz

PROGNAME=gfract

OBJS=main.o color.o fractal.o palette.o globals.o misc.o attr_dlg.o \
my_png.o pal_rot_dlg.o timer.o
HEADERS=externs.h palette.h color.h misc.h fractal_types.h attr_dlg.h \
my_png.h pal_rot_dlg.h timer.h version.h

$(PROGNAME): $(OBJS)
	$(CC) `pkg-config --cflags gtk+-2.0` $(CFLAGS) $(OBJS) -o $(PROGNAME) \
	`pkg-config --libs gtk+-2.0` $(LIBS)

%.o : %.c $(HEADERS)
	$(CC) `pkg-config --cflags gtk+-2.0` $(CFLAGS) -c $<

main.o: main.c $(HEADERS) *.xpm

clean:
	rm -f $(PROGNAME) $(OBJS)
