CC := clang
PKGS := xcb xcb-keysyms xcb-util xcb-ewmh cairo lua

CFLAGS  = $(shell pkg-config --cflags $(PKGS))
LDFLAGS = $(shell pkg-config --libs $(PKGS)) -lev
CFLAGS += -fno-strict-aliasing -std=c99 -g -O2
CFLAGS += -I/usr/include/libev 

TARGETS = src/menu
OBJECTS = src/menu.o
LUADATA = src/lua_dsl.h src/lua_util.h

all: $(TARGETS) $(LUADATA)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

lua_%.h: %.lua
	lua bin2c.lua $< $(<:src/%.lua=lua_%) > $@

menus: $(OBJECTS)
	$(CC) -o $@ $+ `pkg-config --libs cairo xcb xcb-util xcb-ewmh lua` -lev

clean:
	rm -f $(TARGETS) $(OBJECTS) $(LUADATA)
