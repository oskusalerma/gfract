CC=gcc
#CFLAGS=-g -Wall
CFLAGS=-O2 -Wall -Winline
LIBS=-lpng -lz

OBJS=main.o fractal.o palette.o globals.o misc.o attr_dlg.o my_png.o \
pal_rot_dlg.o timer.o color.o
HEADERS=externs.h palette.h misc.h fractal_types.h attr_dlg.h my_png.h \
pal_rot_dlg.h timer.h version.h color.h

gfract: $(OBJS)
	$(CC) `gtk-config --cflags` $(CFLAGS) $(OBJS) -o gfract \
`gtk-config --libs` $(LIBS)

%.o : %.c $(HEADERS)
	$(CC) `gtk-config --cflags` $(CFLAGS) -c $<

main.o: main.c $(HEADERS) *.xpm

clean:
	rm -f gfract $(OBJS)
