# main GTK frontend
GTK_FRONTEND := gfract
GTK_FRONTEND_OBJS := attr_dlg.o color.o color_dlg.o fractal.o globals.o \
main.o misc.o my_png.o pal_rot_dlg.o palette.o timer.o

CXX := g++

DEFS := -Wall -ansi -pedantic -D_GNU_SOURCE -DGDK_DISABLE_DEPRECATED \
	-DGTK_DISABLE_DEPRECATED

DEFS := $(shell pkg-config --cflags gtk+-2.0 libpng) $(DEFS)

CXXFLAGS := $(DEFS) -O2 -fomit-frame-pointer -ffast-math
#CXXFLAGS := $(DEFS) -g

LDFLAGS := -lz
LDFLAGS := $(shell pkg-config --libs gtk+-2.0 libpng) $(LDFLAGS)

HEADERS := $(wildcard *.h *.xpm)

$(GTK_FRONTEND): $(GTK_FRONTEND_OBJS)
	@echo Linking $(GTK_FRONTEND)
	@$(CXX) $(CXXFLAGS) $(GTK_FRONTEND_OBJS) $(LDFLAGS) -o $(GTK_FRONTEND)

%.o: %.cpp $(HEADERS) Makefile
	@echo Compiling $<
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(GTK_FRONTEND) $(GTK_FRONTEND_OBJS)
