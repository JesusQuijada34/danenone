CC ?= gcc
CFLAGS += -O2 -Wall -Wextra -Wpedantic $(shell pkg-config --cflags gtk+-3.0 gtk-layer-shell-0)
LDLIBS += $(shell pkg-config --libs gtk+-3.0 gtk-layer-shell-0) -lm

all: influent-danenone-shell influent-danenone-tour influent-danenone-start influent-danenone-firstboot influent-danenone-splashboot

influent-danenone-shell: main.c window_visuals.c window_visuals.h
	$(CC) $(CFLAGS) -o $@ main.c window_visuals.c $(LDLIBS)

influent-danenone-tour: tour.c cube_guide.c cube_guide.h
	$(CC) $(CFLAGS) -o $@ tour.c cube_guide.c $(LDLIBS)

influent-danenone-start: start_menu.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

influent-danenone-firstboot: first_boot_setup.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

influent-danenone-splashboot: splashboot.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f influent-danenone-shell influent-danenone-tour influent-danenone-start influent-danenone-firstboot influent-danenone-splashboot
