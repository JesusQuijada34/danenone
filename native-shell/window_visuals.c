#include "window_visuals.h"

static GtkCssProvider *provider = NULL;

static const char *css_for_theme(DanenoneTheme theme) {
    if (theme == DANENONE_THEME_LIGHT) {
        return ".danenone-window { background: rgba(248,250,255,0.92); color: #172033; border: 1px solid rgba(255,255,255,0.74); border-radius: 18px; }"
               ".danenone-inactive { opacity: 0.76; background: rgba(238,242,249,0.70); }"
               ".danenone-titlebar { background: rgba(255,255,255,0.66); border-bottom: 1px solid rgba(120,140,170,0.22); padding: 10px 14px; }"
               ".danenone-title { color: #19233a; font-weight: 700; }"
               ".danenone-close, .danenone-minimize, .danenone-maximize { border-radius: 99px; min-width: 12px; min-height: 12px; padding: 0; margin-right: 6px; }"
               ".danenone-close { background: #ff625d; } .danenone-minimize { background: #f4bd4f; } .danenone-maximize { background: #39c85a; }";
    }
    return ".danenone-window { background: rgba(17,25,40,0.94); color: #eff5ff; border: 1px solid rgba(180,215,255,0.20); border-radius: 18px; }"
           ".danenone-inactive { opacity: 0.70; background: rgba(10,17,29,0.72); }"
           ".danenone-titlebar { background: rgba(25,37,57,0.74); border-bottom: 1px solid rgba(180,215,255,0.16); padding: 10px 14px; }"
           ".danenone-title { color: #f2f7ff; font-weight: 700; }"
           ".danenone-close, .danenone-minimize, .danenone-maximize { border-radius: 99px; min-width: 12px; min-height: 12px; padding: 0; margin-right: 6px; }"
           ".danenone-close { background: #ff625d; } .danenone-minimize { background: #f4bd4f; } .danenone-maximize { background: #39c85a; }";
}

void danenone_window_visuals_install(DanenoneTheme theme) {
    if (provider) g_object_unref(provider);
    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css_for_theme(theme), -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

void danenone_window_visuals_apply(GtkWidget *window, gboolean active, DanenoneTheme theme) {
    if (!window) return;
    danenone_window_visuals_install(theme);
    GtkStyleContext *context = gtk_widget_get_style_context(window);
    gtk_style_context_add_class(context, "danenone-window");
    if (active) gtk_style_context_remove_class(context, "danenone-inactive");
    else gtk_style_context_add_class(context, "danenone-inactive");
}

static void close_clicked(GtkButton *button, gpointer data) {
    (void)button;
    gtk_widget_destroy(GTK_WIDGET(data));
}

static void minimize_clicked(GtkButton *button, gpointer data) {
    (void)button;
    gtk_window_iconify(GTK_WINDOW(data));
}

static void maximize_clicked(GtkButton *button, gpointer data) {
    (void)button;
    GtkWindow *window = GTK_WINDOW(data);
    if (gtk_window_is_maximized(window)) gtk_window_unmaximize(window);
    else gtk_window_maximize(window);
}

GtkWidget *danenone_window_titlebar(const char *title, GtkWidget *window, DanenoneTheme theme) {
    GtkWidget *bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *close = gtk_button_new();
    GtkWidget *minimize = gtk_button_new();
    GtkWidget *maximize = gtk_button_new();
    GtkWidget *label = gtk_label_new(title ? title : "Danenone");
    gtk_widget_set_name(close, "danenone-close");
    gtk_widget_set_name(minimize, "danenone-minimize");
    gtk_widget_set_name(maximize, "danenone-maximize");
    gtk_widget_set_name(label, "danenone-title");
    gtk_box_pack_start(GTK_BOX(bar), close, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bar), minimize, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bar), maximize, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bar), label, TRUE, TRUE, 12);
    gtk_style_context_add_class(gtk_widget_get_style_context(bar), "danenone-titlebar");
    gtk_widget_set_tooltip_text(close, "Cerrar");
    gtk_widget_set_tooltip_text(minimize, "Minimizar");
    gtk_widget_set_tooltip_text(maximize, "Maximizar");
    g_signal_connect(close, "clicked", G_CALLBACK(close_clicked), window);
    g_signal_connect(minimize, "clicked", G_CALLBACK(minimize_clicked), window);
    g_signal_connect(maximize, "clicked", G_CALLBACK(maximize_clicked), window);
    danenone_window_visuals_apply(window, TRUE, theme);
    return bar;
}
