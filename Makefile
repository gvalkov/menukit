CC := clang
PKGS := xcb xcb-keysyms xcb-util xcb-ewmh cairo lua

CFLAGS  = $(shell pkg-config --cflags $(PKGS))
LDFLAGS = $(shell pkg-config --libs $(PKGS)) -lev
CFLAGS += -fno-strict-aliasing -std=c99 -g -O2
CFLAGS += -I/usr/include/libev 

TARGETS=menus
OBJECTS=menus.o

all: $(TARGETS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

menus: $(OBJECTS)
	$(CC) -o $@ $+ `pkg-config --libs cairo xcb xcb-util xcb-ewmh lua` -lev

clean:
	rm -f $(TARGETS) $(OBJECTS)
