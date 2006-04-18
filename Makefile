PROG := gfract

CXX := g++

DEFS := -Wall -ansi -pedantic -D_GNU_SOURCE -DGDK_DISABLE_DEPRECATED \
	-DGTK_DISABLE_DEPRECATED

DEFS := $(shell pkg-config --cflags gtk+-2.0 libpng) $(DEFS)

CXXFLAGS := $(DEFS) -O2 -fomit-frame-pointer -ffast-math
#CXXFLAGS := $(DEFS) -g

LDFLAGS := -lz
LDFLAGS := $(shell pkg-config --libs gtk+-2.0 libpng) $(LDFLAGS)

SRC := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp, %.o, $(SRC))
HEADERS := $(wildcard *.h *.xpm)

$(PROG): $(OBJS)
	@echo Linking $(PROG)
	@$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $(PROG)

%.o: %.cpp $(HEADERS)
	@echo Compiling $<
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(PROG) $(OBJS)
