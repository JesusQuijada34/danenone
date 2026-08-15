CC ?= gcc
CFLAGS += -O2 -Wall -Wextra -Wpedantic $(shell pkg-config --cflags gtk+-3.0 gtk-layer-shell-0)
LDLIBS += $(shell pkg-config --libs gtk+-3.0 gtk-layer-shell-0)

all: influent-danenone-shell influent-danenone-tour

influent-danenone-shell: main.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

influent-danenone-tour: tour.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f influent-danenone-shell influent-danenone-tour
