PROG := gfract

CC := gcc

DEFS := -Wall -ansi -pedantic -D_GNU_SOURCE
DEFS := $(shell pkg-config --cflags gtk+-2.0) $(DEFS)

CFLAGS := $(DEFS) -O2
#CFLAGS := $(DEFS) -Og

LDFLAGS := -lpng -lz
LDFLAGS := $(shell pkg-config --libs gtk+-2.0) $(LDFLAGS)

SRC := $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(SRC))
HEADERS := $(wildcard *.h *.xpm)

$(PROG): $(OBJS)
	@echo Linking $(PROG)
	@$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(PROG)

%.o: %.c $(HEADERS)
	@echo Compiling $<
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(PROG) $(OBJS)
