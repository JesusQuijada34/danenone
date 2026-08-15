#include <gtk/gtk.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <time.h>

static GtkWidget *clock_label;

static gboolean update_clock(gpointer data) {
    GtkWidget *label = GTK_WIDGET(data);
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    char text[32];
    strftime(text, sizeof(text), "%H:%M", local);
    gtk_label_set_text(GTK_LABEL(label), text);
    return G_SOURCE_CONTINUE;
}

static GtkWidget *make_button(const char *label, const char *class_name) {
    GtkWidget *button = gtk_button_new_with_label(label);
    gtk_widget_set_name(button, class_name);
    gtk_widget_set_hexpand(button, FALSE);
    gtk_widget_set_vexpand(button, TRUE);
    return button;
}

static void build_taskbar(void) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_namespace(GTK_WINDOW(window), "influent-danenone-taskbar");
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_exclusive_zone(GTK_WINDOW(window), 84);
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 72);

    GtkWidget *bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_name(bar, "taskbar");
    gtk_widget_set_margin_start(bar, 22);
    gtk_widget_set_margin_end(bar, 22);
    gtk_widget_set_margin_top(bar, 10);
    gtk_widget_set_margin_bottom(bar, 10);

    GtkWidget *start = make_button("◈", "start-button");
    gtk_box_pack_start(GTK_BOX(bar), start, FALSE, FALSE, 0);
    GtkWidget *spacer_left = gtk_label_new(NULL);
    gtk_widget_set_hexpand(spacer_left, TRUE);
    gtk_box_pack_start(GTK_BOX(bar), spacer_left, TRUE, TRUE, 0);
    const char *items[] = {"◈", "▣", "◌"};
    for (size_t index = 0; index < G_N_ELEMENTS(items); ++index) {
        GtkWidget *button = make_button(items[index], "task-button");
        gtk_box_pack_start(GTK_BOX(bar), button, FALSE, FALSE, 0);
    }
    GtkWidget *spacer_right = gtk_label_new(NULL);
    gtk_widget_set_hexpand(spacer_right, TRUE);
    gtk_box_pack_start(GTK_BOX(bar), spacer_right, TRUE, TRUE, 0);
    clock_label = gtk_label_new(NULL);
    gtk_widget_set_name(clock_label, "clock");
    gtk_box_pack_start(GTK_BOX(bar), clock_label, FALSE, FALSE, 8);
    GtkWidget *control = make_button("☰", "task-button");
    gtk_box_pack_start(GTK_BOX(bar), control, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), bar);
    gtk_widget_show_all(window);
    update_clock(clock_label);
    g_timeout_add_seconds(1, update_clock, clock_label);
}

static void build_notch(void) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_namespace(GTK_WINDOW(window), "influent-danenone-notch");
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_window_set_default_size(GTK_WINDOW(window), 270, 46);
    GtkWidget *notch = gtk_label_new("Influent Danenone");
    gtk_widget_set_name(notch, "notch");
    gtk_container_add(GTK_CONTAINER(window), notch);
    gtk_widget_show_all(window);
}

static void activate(GtkApplication *app, gpointer data) {
    (void)app;
    (void)data;
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "#taskbar { background: rgba(12, 22, 38, 0.90); border: 1px solid rgba(220,235,255,0.25); border-radius: 26px; }"
        "#notch { background: #05070d; color: #eef5ff; border-radius: 22px; padding: 10px 30px; font-weight: 600; }"
        "button { color: #eef5ff; background: rgba(35,58,88,0.75); border: 1px solid rgba(220,235,255,0.2); border-radius: 14px; padding: 8px 14px; }"
        "button:hover { background: rgba(79,132,196,0.9); }"
        "#start-button { background: rgba(93,143,221,0.95); font-size: 20px; }"
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
