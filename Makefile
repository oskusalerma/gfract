# main GTK frontend
GTK_FRONTEND := gfract
GTK_FRONTEND_OBJS := attr_dlg.o color.o color_dlg.o fractal.o globals.o \
main.o misc.o my_png.o pal_rot_dlg.o palette.o timer.o palettes.o \
image_info.o CondVar.o Mutex.o RenderThread.o Thread.o WorkQueue.o \
Config.o Tool.o

# toolbar icons
ICON_FILES := mandel_icon.pix

CXX := g++

DEFS := -Wall -ansi -pedantic -D_REENTRANT -D_GNU_SOURCE
DEFS := $(shell pkg-config --cflags gtk+-2.0 libpng) $(DEFS)

CXXFLAGS := $(DEFS) -O2 -fomit-frame-pointer -ffast-math
#CXXFLAGS := $(DEFS) -g

LDFLAGS := -lz -lpthread -lboost_regex -lboost_filesystem -lboost_system
LDFLAGS := $(shell pkg-config --libs gtk+-2.0 libpng) $(LDFLAGS)

HEADERS := $(wildcard *.h)
PALETTES := $(wildcard palettes/*.map)

$(GTK_FRONTEND): $(GTK_FRONTEND_OBJS)
	@echo Linking $(GTK_FRONTEND)
	@$(CXX) $(CXXFLAGS) $(GTK_FRONTEND_OBJS) $(LDFLAGS) -o $(GTK_FRONTEND)

%.o: %.cpp $(HEADERS) Makefile
	@echo Compiling $<
	@$(CXX) $(CXXFLAGS) -c $< -o $@

palettes.cpp: make_palettes.py $(PALETTES)
	./make_palettes.py

main.o: $(ICON_FILES)

%.pix: %.png Makefile
	@echo Generating inline image for $<
	@gdk-pixbuf-csource --name $(*)_pix --raw $< > $@

clean:
	rm -f $(GTK_FRONTEND) $(GTK_FRONTEND_OBJS) palettes.cpp
