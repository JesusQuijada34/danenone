#ifndef DANENONE_WINDOW_VISUALS_H
#define DANENONE_WINDOW_VISUALS_H

#include <gtk/gtk.h>

typedef enum {
    DANENONE_THEME_LIGHT,
    DANENONE_THEME_DARK
} DanenoneTheme;

void danenone_window_visuals_install(DanenoneTheme theme);
void danenone_window_visuals_apply(GtkWidget *window, gboolean active, DanenoneTheme theme);
GtkWidget *danenone_window_titlebar(const char *title, GtkWidget *window, DanenoneTheme theme);

#endif
