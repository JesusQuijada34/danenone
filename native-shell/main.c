#include <gtk/gtk.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include "window_visuals.h"
#include <time.h>

static GtkWidget *clock_label;

static gboolean layer_shell_available(void) {
    GdkDisplay *display = gdk_display_get_default();
    const char *name = display ? gdk_display_get_name(display) : NULL;
    return name && g_str_has_prefix(name, "wayland-") && gtk_layer_is_supported();
}

static gboolean update_clock(gpointer data) {
    GtkWidget *label = GTK_WIDGET(data);
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    char text[32];
    strftime(text, sizeof(text), "%H:%M", local);
    gtk_label_set_text(GTK_LABEL(label), text);
    return G_SOURCE_CONTINUE;
}

static GtkWidget *make_icon_button(const char *icon_name, const char *tooltip) {
    GtkWidget *button = gtk_button_new();
    GtkWidget *image = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
    gtk_image_set_pixel_size(GTK_IMAGE(image), 22);
    gtk_container_add(GTK_CONTAINER(button), image);
    gtk_widget_set_name(button, "task-icon-button");
    gtk_widget_set_tooltip_text(button, tooltip);
    gtk_widget_set_hexpand(button, FALSE);
    gtk_widget_set_vexpand(button, TRUE);
    return button;
}

static const char *danenone_logo_path(void) {
    static char path[512];
    g_snprintf(path, sizeof(path), "/usr/share/influent/danenone-cube-logo.png");
    if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
        g_snprintf(path, sizeof(path), "assets/danenone-cube/danenone-cube-logo.png");
    }
    return path;
}

static void start_icon_set(GtkWidget *button, const char *state) {
    (void)state;
    GtkWidget *image = g_object_get_data(G_OBJECT(button), "start-image");
    if (image) gtk_image_set_from_file(GTK_IMAGE(image), danenone_logo_path());
}

static gboolean start_enter(GtkWidget *widget, GdkEventCrossing *event, gpointer data) {
    (void)event; (void)data;
    start_icon_set(widget, "hover");
    return FALSE;
}

static gboolean start_leave(GtkWidget *widget, GdkEventCrossing *event, gpointer data) {
    (void)event; (void)data;
    start_icon_set(widget, "normal");
    return FALSE;
}

static gboolean start_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    (void)data;
    if (event->button == 1) start_icon_set(widget, "pressed");
    return FALSE;
}

static gboolean start_release(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    (void)data;
    if (event->button == 1) start_icon_set(widget, "hover");
    return FALSE;
}

static void start_clicked(GtkButton *button, gpointer data) {
    (void)button; (void)data;
    GError *error = NULL;
    char *argv[] = {"/usr/local/bin/influent-danenone-start", NULL};
    if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error)) {
        if (error) g_error_free(error);
        char *local_argv[] = {"./influent-danenone-start", NULL};
        g_spawn_async(NULL, local_argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);
    }
}

static GtkWidget *make_start_button(void) {
    GtkWidget *button = gtk_button_new();
    GtkWidget *image = gtk_image_new_from_file(danenone_logo_path());
    gtk_widget_set_size_request(image, 38, 38);
    g_object_set_data(G_OBJECT(button), "start-image", image);
    gtk_container_add(GTK_CONTAINER(button), image);
    gtk_widget_set_name(button, "start-button");
    gtk_widget_set_tooltip_text(button, "Inicio");
    g_signal_connect(button, "clicked", G_CALLBACK(start_clicked), NULL);
    g_signal_connect(button, "enter-notify-event", G_CALLBACK(start_enter), NULL);
    g_signal_connect(button, "leave-notify-event", G_CALLBACK(start_leave), NULL);
    g_signal_connect(button, "button-press-event", G_CALLBACK(start_press), NULL);
    g_signal_connect(button, "button-release-event", G_CALLBACK(start_release), NULL);
    return button;
}

static void build_taskbar(void) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gboolean use_layer_shell = layer_shell_available();
    if (use_layer_shell) {
        gtk_layer_init_for_window(GTK_WINDOW(window));
        gtk_layer_set_namespace(GTK_WINDOW(window), "influent-danenone-taskbar");
        gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_TOP);
        gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_exclusive_zone(GTK_WINDOW(window), 84);
    } else {
        gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
        gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    }
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 72);

    GtkWidget *bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_name(bar, "taskbar");
    gtk_widget_set_margin_start(bar, 22);
    gtk_widget_set_margin_end(bar, 22);
    gtk_widget_set_margin_top(bar, 10);
    gtk_widget_set_margin_bottom(bar, 10);

    GtkWidget *start = make_start_button();
    gtk_box_pack_start(GTK_BOX(bar), start, FALSE, FALSE, 0);
    GtkWidget *spacer_left = gtk_label_new(NULL);
    gtk_widget_set_hexpand(spacer_left, TRUE);
    gtk_box_pack_start(GTK_BOX(bar), spacer_left, TRUE, TRUE, 0);
    const char *icons[] = {"view-grid-symbolic", "folder-symbolic", "notifications-symbolic"};
    const char *tooltips[] = {"Aplicaciones", "Archivos", "Notificaciones"};
    for (size_t index = 0; index < G_N_ELEMENTS(icons); ++index) {
        GtkWidget *button = make_icon_button(icons[index], tooltips[index]);
        gtk_box_pack_start(GTK_BOX(bar), button, FALSE, FALSE, 0);
    }
    GtkWidget *spacer_right = gtk_label_new(NULL);
    gtk_widget_set_hexpand(spacer_right, TRUE);
    gtk_box_pack_start(GTK_BOX(bar), spacer_right, TRUE, TRUE, 0);
    clock_label = gtk_label_new(NULL);
    gtk_widget_set_name(clock_label, "clock");
    gtk_box_pack_start(GTK_BOX(bar), clock_label, FALSE, FALSE, 8);
    GtkWidget *control = make_icon_button("open-menu-symbolic", "Centro de Control");
    gtk_box_pack_start(GTK_BOX(bar), control, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), bar);
    gtk_widget_show_all(window);
    update_clock(clock_label);
    g_timeout_add_seconds(1, update_clock, clock_label);
}

static void build_notch(void) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gboolean use_layer_shell = layer_shell_available();
    if (use_layer_shell) {
        gtk_layer_init_for_window(GTK_WINDOW(window));
        gtk_layer_set_namespace(GTK_WINDOW(window), "influent-danenone-notch");
        gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_TOP);
        gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    } else {
        gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
        gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    }
    gtk_window_set_default_size(GTK_WINDOW(window), 270, 46);
    GtkWidget *notch = gtk_label_new("Influent Danenone");
    gtk_widget_set_name(notch, "notch");
    gtk_container_add(GTK_CONTAINER(window), notch);
    gtk_widget_show_all(window);
}

static void activate(GtkApplication *app, gpointer data) {
    (void)app;
    (void)data;
    danenone_window_visuals_install(DANENONE_THEME_DARK);
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "#taskbar { background: rgba(12, 22, 38, 0.90); border: 1px solid rgba(220,235,255,0.25); border-radius: 26px; }"
        "#notch { background: #05070d; color: #eef5ff; border-radius: 22px; padding: 10px 30px; font-weight: 600; }"
        "button { color: #eef5ff; background: rgba(35,58,88,0.75); border: 1px solid rgba(220,235,255,0.2); border-radius: 14px; padding: 8px 14px; }"
        "button:hover { background: rgba(79,132,196,0.9); }"
        "#start-button { background: rgba(93,143,221,0.95); font-size: 20px; }"
        "#task-icon-button { min-width: 42px; padding: 6px; }"
        "#task-icon-button image { color: #dcecff; }"
        "#task-icon-button:hover image { color: #ffffff; }"
        "#clock { font-weight: 600; padding: 0 8px; }", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    build_taskbar();
    build_notch();
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.influent.danenone.shell", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
