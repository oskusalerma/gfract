PROG := gfract

CC := g++

DEFS := -Wall -ansi -pedantic -D_GNU_SOURCE -DGDK_DISABLE_DEPRECATED
DEFS := $(shell pkg-config --cflags gtk+-2.0) $(DEFS)

CXXFLAGS := $(DEFS) -O2
#CXXFLAGS := $(DEFS) -g

LDFLAGS := -lpng -lz
LDFLAGS := $(shell pkg-config --libs gtk+-2.0) $(LDFLAGS)

SRC := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp, %.o, $(SRC))
HEADERS := $(wildcard *.h *.xpm)

$(PROG): $(OBJS)
	@echo Linking $(PROG)
	@$(CC) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $(PROG)

%.o: %.cpp $(HEADERS)
	@echo Compiling $<
	@$(CC) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(PROG) $(OBJS)
