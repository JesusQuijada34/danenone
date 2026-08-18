CC ?= cc
CFLAGS += -O2 -Wall -Wextra -Wpedantic $(shell pkg-config --cflags gtk+-3.0 gtk-layer-shell-0)
LDLIBS += $(shell pkg-config --libs gtk+-3.0 gtk-layer-shell-0) -lm
GTK4_CFLAGS := $(shell pkg-config --cflags gtk4 gdk-pixbuf-2.0 gio-unix-2.0)
GTK4_LIBS := $(shell pkg-config --libs gtk4 gdk-pixbuf-2.0 gio-2.0 gio-unix-2.0)
GTK4_LAYER_PKG_PATH := $(CURDIR)/../build/gtk4-layer-shell-prefix/lib/x86_64-linux-gnu/pkgconfig
GTK4_LAYER_CFLAGS := $(shell PKG_CONFIG_PATH=$(GTK4_LAYER_PKG_PATH) pkg-config --cflags gtk4-layer-shell-0 2>/dev/null)
GTK4_LAYER_LIBS := $(shell PKG_CONFIG_PATH=$(GTK4_LAYER_PKG_PATH) pkg-config --libs gtk4-layer-shell-0 2>/dev/null)

all: influent-danenone-shell influent-danenone-tour influent-danenone-start influent-danenone-firstboot influent-danenone-splashboot influent-danenone-desktop-gtk4

influent-danenone-shell: main.c window_visuals.c window_visuals.h
	$(CC) $(CFLAGS) -o $@ main.c window_visuals.c $(LDLIBS)

influent-danenone-tour: tour.c cube_guide.c cube_guide.h
	$(CC) $(CFLAGS) -o $@ tour.c cube_guide.c $(LDLIBS)

influent-danenone-start: start_menu.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

# GTK4 es el OOBE activo; first_boot_setup.c se conserva solo como referencia histórica.
influent-danenone-firstboot: first_boot_setup_gtk4.c
	$(CC) -O2 -Wall -Wextra -Wpedantic $(GTK4_CFLAGS) -o $@ $< $(GTK4_LIBS)

influent-danenone-desktop-gtk4: desktop_gtk4.c
	$(CC) -O2 -Wall -Wextra -Wpedantic $(GTK4_CFLAGS) $(GTK4_LAYER_CFLAGS) -o $@ $< $(GTK4_LIBS) $(GTK4_LAYER_LIBS)

influent-danenone-splashboot: splashboot.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

# Shell experimental Qt6; se mantiene separado hasta validar Hyprland real.
qt6-shell:
	cmake -S qt_shell -B qt_shell/build -DCMAKE_BUILD_TYPE=Release
	cmake --build qt_shell/build -j$${JOBS:-2}

clean:
	rm -f influent-danenone-shell influent-danenone-tour influent-danenone-start influent-danenone-firstboot influent-danenone-splashboot influent-danenone-desktop-gtk4
