#include <gio/gio.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *apps;
    GtkWidget *packages;
    GtkWidget *search;
} StartMenu;

static void filter_list(GtkWidget *list, const char *query) {
    GList *rows = gtk_container_get_children(GTK_CONTAINER(list));
    gchar *needle = g_utf8_casefold(query ? query : "", -1);
    for (GList *item = rows; item; item = item->next) {
        GtkWidget *row = item->data;
        const char *key = g_object_get_data(G_OBJECT(row), "search-key");
        gchar *folded = g_utf8_casefold(key ? key : "", -1);
        gboolean visible = !needle || !*needle || (folded && strstr(folded, needle) != NULL);
        gtk_widget_set_visible(row, visible);
        g_free(folded);
    }
    g_free(needle);
    g_list_free(rows);
}

static void filter_changed(GtkSearchEntry *entry, gpointer data) {
    StartMenu *menu = data;
    const char *query = gtk_entry_get_text(GTK_ENTRY(entry));
    filter_list(menu->apps, query);
    filter_list(menu->packages, query);
}

static void release_app(gpointer data, GClosure *closure) {
    (void)closure;
    g_object_unref(data);
}

static void launch_app(GtkButton *button, gpointer data) {
    (void)button;
    GAppInfo *app = data;
    GError *error = NULL;
    g_app_info_launch(app, NULL, NULL, &error);
    if (error) g_error_free(error);
}

static GtkWidget *app_row(GAppInfo *app) {
    const char *name = g_app_info_get_display_name(app);
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *button = gtk_button_new_with_label(name ? name : "Aplicación");
    gtk_widget_set_halign(button, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(button, TRUE);
    gtk_widget_set_name(button, "start-app-button");
    gtk_container_add(GTK_CONTAINER(row), button);
    g_object_set_data_full(G_OBJECT(row), "search-key", g_strdup(name ? name : ""), g_free);
    g_signal_connect_data(button, "clicked", G_CALLBACK(launch_app), g_object_ref(app), release_app, 0);
    return row;
}

static void add_apps(StartMenu *menu) {
    GList *apps = g_app_info_get_all();
    for (GList *item = apps; item; item = item->next) {
        GAppInfo *app = item->data;
        if (!g_app_info_should_show(app)) continue;
        GtkWidget *row = app_row(app);
        gtk_list_box_insert(GTK_LIST_BOX(menu->apps), row, -1);
    }
    g_list_free_full(apps, g_object_unref);
}

static void add_package_line(StartMenu *menu, const char *line) {
    if (!line || !*line) return;
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(line);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(label, 12);
    gtk_widget_set_margin_end(label, 12);
    gtk_widget_set_margin_top(label, 5);
    gtk_widget_set_margin_bottom(label, 5);
    gtk_container_add(GTK_CONTAINER(row), label);
    g_object_set_data_full(G_OBJECT(row), "search-key", g_strdup(line), g_free);
    gtk_list_box_insert(GTK_LIST_BOX(menu->packages), row, -1);
}

static void add_packages(StartMenu *menu) {
    gchar *stdout_data = NULL;
    GError *error = NULL;
    gint status = 0;
    if (!g_spawn_command_line_sync("pacman -Qq", &stdout_data, NULL, &status, &error)) {
        if (error) g_error_free(error);
        add_package_line(menu, "Los paquetes no están disponibles en esta sesión");
        return;
    }
    gchar **lines = g_strsplit(stdout_data ? stdout_data : "", "\n", 0);
    guint count = 0;
    for (gchar **line = lines; *line && count < 400; line++, count++) add_package_line(menu, *line);
    if (count == 0) add_package_line(menu, "No se encontraron paquetes instalados");
    g_strfreev(lines);
    g_free(stdout_data);
}

static gboolean key_press(GtkWidget *window, GdkEventKey *event, gpointer data) {
    (void)data;
    if (event->keyval == GDK_KEY_Escape) {
        gtk_widget_destroy(window);
        return TRUE;
    }
    return FALSE;
}

static void activate(GtkApplication *app, gpointer data) {
    (void)data;
    StartMenu *menu = g_new0(StartMenu, 1);
    menu->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(menu->window), "Inicio · Influent Danenone");
    gtk_window_set_default_size(GTK_WINDOW(menu->window), 760, 620);
    gtk_window_set_position(GTK_WINDOW(menu->window), GTK_WIN_POS_CENTER);
    g_signal_connect(menu->window, "key-press-event", G_CALLBACK(key_press), NULL);
    g_signal_connect_swapped(menu->window, "destroy", G_CALLBACK(g_free), menu);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_set_margin_start(root, 24);
    gtk_widget_set_margin_end(root, 24);
    gtk_widget_set_margin_top(root, 22);
    gtk_widget_set_margin_bottom(root, 22);
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *icon = gtk_image_new_from_file("/usr/share/icons/influent/danenone-cube-normal.svg");
    if (gtk_image_get_storage_type(GTK_IMAGE(icon)) == GTK_IMAGE_EMPTY) {
        gtk_widget_destroy(icon);
        icon = gtk_label_new("Danenone");
    }
    gtk_widget_set_size_request(icon, 58, 58);
    gtk_box_pack_start(GTK_BOX(header), icon, FALSE, FALSE, 0);
    GtkWidget *heading = gtk_label_new("Inicio");
    gtk_widget_set_name(heading, "start-heading");
    gtk_label_set_xalign(GTK_LABEL(heading), 0.0);
    gtk_box_pack_start(GTK_BOX(header), heading, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(root), header, FALSE, FALSE, 0);

    menu->search = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(menu->search), "Buscar aplicaciones y paquetes");
    gtk_box_pack_start(GTK_BOX(root), menu->search, FALSE, FALSE, 0);

    GtkWidget *notebook = gtk_notebook_new();
    menu->apps = gtk_list_box_new();
    menu->packages = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(menu->apps), GTK_SELECTION_NONE);
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(menu->packages), GTK_SELECTION_NONE);
    GtkWidget *apps_scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *packages_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(apps_scroll), menu->apps);
    gtk_container_add(GTK_CONTAINER(packages_scroll), menu->packages);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), apps_scroll, gtk_label_new("Aplicaciones"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packages_scroll, gtk_label_new("Paquetes"));
    gtk_box_pack_start(GTK_BOX(root), notebook, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(menu->window), root);

    g_signal_connect(menu->search, "search-changed", G_CALLBACK(filter_changed), menu);
    add_apps(menu);
    add_packages(menu);
    gtk_widget_show_all(menu->window);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.influent.danenone.start", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
