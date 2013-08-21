CFLAGS += -W -Wall -g -O2
CFLAGS += `pkg-config --cflags cairo xcb`

TARGETS=main

all: $(TARGETS)

main: main.o
	$(CC) -o $@ $+ `pkg-config --libs cairo xcb`

clean:
	$(RM) $(TARGETS)
	$(RM) *.o
