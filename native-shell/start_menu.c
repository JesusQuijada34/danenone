#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *pinned_grid;
    GtkWidget *recommended;
    GtkWidget *apps;
    GtkWidget *packages;
    GtkWidget *search;
    GList *all_applications;
} StartMenu;

static void filter_list(GtkWidget *list, const char *query) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(list));
    gchar *needle = g_utf8_casefold(query ? query : "", -1);
    for (GList *item = children; item; item = item->next) {
        GtkWidget *row = item->data;
        const char *key = g_object_get_data(G_OBJECT(row), "search-key");
        gchar *folded = g_utf8_casefold(key ? key : "", -1);
        gboolean visible = !needle || !*needle || (folded && strstr(folded, needle) != NULL);
        gtk_widget_set_visible(row, visible);
        g_free(folded);
    }
    g_free(needle);
    g_list_free(children);
}

static void filter_changed(GtkSearchEntry *entry, gpointer data) {
    StartMenu *menu = data;
    const char *query = gtk_entry_get_text(GTK_ENTRY(entry));
    filter_list(menu->pinned_grid, query);
    filter_list(menu->recommended, query);
    filter_list(menu->apps, query);
    filter_list(menu->packages, query);
}

static void record_recent(GAppInfo *app) {
    const char *id = g_app_info_get_id(app);
    if (!id || !*id) return;
    gchar *directory = g_build_filename(g_get_user_state_dir(), "influent-danenone", NULL);
    gchar *path = g_build_filename(directory, "recent-apps", NULL);
    if (g_mkdir_with_parents(directory, 0700) == 0) {
        gchar *old = NULL;
        g_file_get_contents(path, &old, NULL, NULL);
        gchar *updated = g_strdup_printf("%s\n%s\n", id, old ? old : "");
        g_file_set_contents(path, updated, -1, NULL);
        g_free(updated);
        g_free(old);
    }
    g_free(path);
    g_free(directory);
}

static void launch_app(GtkButton *button, gpointer data) {
    StartMenu *menu = data;
    GAppInfo *app = g_object_get_data(G_OBJECT(button), "app-info");
    if (!app) return;
    GError *error = NULL;
    record_recent(app);
    g_app_info_launch(app, NULL, NULL, &error);
    if (error) g_error_free(error);
    if (menu && menu->window) gtk_widget_destroy(menu->window);
}

static GtkWidget *app_button(StartMenu *menu, GAppInfo *app, gboolean compact) {
    const char *name = g_app_info_get_display_name(app);
    GtkWidget *button = gtk_button_new();
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, compact ? 3 : 7);
    GtkWidget *image = NULL;
    GIcon *icon = g_app_info_get_icon(app);
    if (icon) image = gtk_image_new_from_gicon(icon, GTK_ICON_SIZE_DIALOG);
    else image = gtk_image_new_from_icon_name("application-x-executable-symbolic", GTK_ICON_SIZE_DIALOG);
    gtk_image_set_pixel_size(GTK_IMAGE(image), compact ? 28 : 42);
    GtkWidget *label = gtk_label_new(name ? name : "Aplicación");
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_label_set_max_width_chars(GTK_LABEL(label), compact ? 22 : 14);
    gtk_widget_set_name(label, compact ? "recommend-label" : "pinned-label");
    gtk_box_pack_start(GTK_BOX(content), image, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), label, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(button), content);
    gtk_widget_set_name(button, compact ? "recommend-button" : "pinned-button");
    gtk_widget_set_tooltip_text(button, name ? name : "Aplicación");
    g_object_set_data(G_OBJECT(button), "app-info", app);
    g_object_set_data_full(G_OBJECT(button), "search-key", g_strdup(name ? name : ""), g_free);
    g_signal_connect(button, "clicked", G_CALLBACK(launch_app), menu);
    return button;
}

static GtkWidget *app_row(StartMenu *menu, GAppInfo *app) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *button = app_button(menu, app, TRUE);
    gtk_widget_set_halign(button, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(button, TRUE);
    gtk_container_add(GTK_CONTAINER(row), button);
    const char *name = g_app_info_get_display_name(app);
    g_object_set_data_full(G_OBJECT(row), "search-key", g_strdup(name ? name : ""), g_free);
    return row;
}

static void load_apps(StartMenu *menu) {
    GList *apps = g_app_info_get_all();
    for (GList *item = apps; item; item = item->next) {
        GAppInfo *app = item->data;
        if (!g_app_info_should_show(app)) {
            g_object_unref(app);
            continue;
        }
        menu->all_applications = g_list_append(menu->all_applications, app);
    }
    g_list_free(apps);
}

static GAppInfo *find_app(StartMenu *menu, const char *id) {
    for (GList *item = menu->all_applications; item; item = item->next) {
        GAppInfo *app = item->data;
        if (g_strcmp0(g_app_info_get_id(app), id) == 0) return app;
    }
    return NULL;
}

static void add_pinned(StartMenu *menu) {
    guint column = 0;
    guint row = 0;
    guint count = 0;
    for (GList *item = menu->all_applications; item && count < 8; item = item->next, count++) {
        GtkWidget *button = app_button(menu, item->data, FALSE);
        gtk_widget_set_size_request(button, 112, 92);
        gtk_grid_attach(GTK_GRID(menu->pinned_grid), button, column, row, 1, 1);
        if (++column == 4) {
            column = 0;
            row++;
        }
    }
    if (count == 0) {
        GtkWidget *empty = gtk_label_new("No hay aplicaciones ancladas todavía");
        gtk_widget_set_name(empty, "empty-label");
        gtk_grid_attach(GTK_GRID(menu->pinned_grid), empty, 0, 0, 4, 1);
    }
}

static void add_recommended(StartMenu *menu) {
    gchar *path = g_build_filename(g_get_user_state_dir(), "influent-danenone", "recent-apps", NULL);
    gchar *contents = NULL;
    gboolean added = FALSE;
    if (g_file_get_contents(path, &contents, NULL, NULL)) {
        gchar **lines = g_strsplit(contents, "\n", -1);
        guint count = 0;
        for (gchar **line = lines; *line && count < 5; line++) {
            if (!**line) continue;
            GAppInfo *app = find_app(menu, *line);
            if (!app) continue;
            GtkWidget *row = app_row(menu, app);
            gtk_list_box_insert(GTK_LIST_BOX(menu->recommended), row, -1);
            added = TRUE;
            count++;
        }
        g_strfreev(lines);
    }
    if (!added) {
        GtkWidget *empty = gtk_label_new("Las aplicaciones que abras aparecerán aquí");
        gtk_widget_set_name(empty, "empty-label");
        gtk_widget_set_margin_top(empty, 8);
        gtk_widget_set_margin_bottom(empty, 8);
        gtk_list_box_insert(GTK_LIST_BOX(menu->recommended), empty, -1);
    }
    g_free(contents);
    g_free(path);
}

static void add_all_apps(StartMenu *menu) {
    for (GList *item = menu->all_applications; item; item = item->next) {
        gtk_list_box_insert(GTK_LIST_BOX(menu->apps), app_row(menu, item->data), -1);
    }
    if (!menu->all_applications) {
        gtk_list_box_insert(GTK_LIST_BOX(menu->apps), gtk_label_new("No se encontraron aplicaciones"), -1);
    }
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

static void destroy_menu(GtkWidget *window, gpointer data) {
    (void)window;
    StartMenu *menu = data;
    g_list_free_full(menu->all_applications, g_object_unref);
    g_free(menu);
}

static void activate(GtkApplication *app, gpointer data) {
    (void)data;
    StartMenu *menu = g_new0(StartMenu, 1);
    menu->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(menu->window), "Inicio · Influent Danenone");
    gtk_window_set_default_size(GTK_WINDOW(menu->window), 820, 720);
    gtk_window_set_position(GTK_WINDOW(menu->window), GTK_WIN_POS_CENTER);
    g_signal_connect(menu->window, "key-press-event", G_CALLBACK(key_press), NULL);
    g_signal_connect(menu->window, "destroy", G_CALLBACK(destroy_menu), menu);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_set_name(root, "start-root");
    gtk_widget_set_margin_start(root, 28);
    gtk_widget_set_margin_end(root, 28);
    gtk_widget_set_margin_top(root, 24);
    gtk_widget_set_margin_bottom(root, 24);

    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *icon = gtk_image_new_from_file("/usr/share/influent/danenone-cube-logo.png");
    if (gtk_image_get_storage_type(GTK_IMAGE(icon)) == GTK_IMAGE_EMPTY) icon = gtk_image_new_from_icon_name("applications-system-symbolic", GTK_ICON_SIZE_DIALOG);
    gtk_widget_set_size_request(icon, 64, 64);
    gtk_box_pack_start(GTK_BOX(header), icon, FALSE, FALSE, 0);
    GtkWidget *heading_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *heading = gtk_label_new("Inicio");
    GtkWidget *subtitle = gtk_label_new("Aplicaciones, archivos y ajustes");
    gtk_widget_set_name(heading, "start-heading");
    gtk_widget_set_name(subtitle, "start-subtitle");
    gtk_label_set_xalign(GTK_LABEL(heading), 0.0);
    gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0);
    gtk_box_pack_start(GTK_BOX(heading_box), heading, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(heading_box), subtitle, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header), heading_box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(root), header, FALSE, FALSE, 0);

    menu->search = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(menu->search), "Buscar aplicaciones y archivos");
    gtk_box_pack_start(GTK_BOX(root), menu->search, FALSE, FALSE, 0);

    GtkWidget *pinned_title = gtk_label_new("Ancladas");
    gtk_widget_set_name(pinned_title, "section-title");
    gtk_label_set_xalign(GTK_LABEL(pinned_title), 0.0);
    gtk_box_pack_start(GTK_BOX(root), pinned_title, FALSE, FALSE, 0);
    menu->pinned_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(menu->pinned_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(menu->pinned_grid), 10);
    gtk_box_pack_start(GTK_BOX(root), menu->pinned_grid, FALSE, FALSE, 0);

    GtkWidget *recommended_title = gtk_label_new("Recomendadas");
    gtk_widget_set_name(recommended_title, "section-title");
    gtk_label_set_xalign(GTK_LABEL(recommended_title), 0.0);
    gtk_box_pack_start(GTK_BOX(root), recommended_title, FALSE, FALSE, 0);
    menu->recommended = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(menu->recommended), GTK_SELECTION_NONE);
    GtkWidget *recommended_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(recommended_scroll, -1, 100);
    gtk_container_add(GTK_CONTAINER(recommended_scroll), menu->recommended);
    gtk_box_pack_start(GTK_BOX(root), recommended_scroll, FALSE, FALSE, 0);

    GtkWidget *notebook = gtk_notebook_new();
    menu->apps = gtk_list_box_new();
    menu->packages = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(menu->apps), GTK_SELECTION_NONE);
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(menu->packages), GTK_SELECTION_NONE);
    GtkWidget *apps_scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *packages_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(apps_scroll), menu->apps);
    gtk_container_add(GTK_CONTAINER(packages_scroll), menu->packages);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), apps_scroll, gtk_label_new("Todas las aplicaciones"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packages_scroll, gtk_label_new("Paquetes"));
    gtk_box_pack_start(GTK_BOX(root), notebook, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(menu->window), root);

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "window { background: rgba(10,17,30,0.96); color: #f4f8ff; }"
        "#start-root { background: rgba(10,17,30,0.96); }"
        "#start-heading { color: #f7faff; font-size: 29px; font-weight: 700; }"
        "#start-subtitle, #empty-label { color: rgba(208,222,244,0.70); font-size: 13px; }"
        "#section-title { color: #eef5ff; font-size: 15px; font-weight: 700; margin-top: 4px; }"
        "entry { background: rgba(255,255,255,0.10); color: #ffffff; border: 1px solid rgba(219,235,255,0.22); border-radius: 14px; padding: 11px 14px; }"
        "#pinned-button, #recommend-button { background: rgba(55,78,111,0.48); color: #f5f8ff; border: 1px solid rgba(217,235,255,0.18); border-radius: 17px; padding: 9px; }"
        "#pinned-button:hover, #recommend-button:hover { background: rgba(87,133,191,0.72); border-color: rgba(238,247,255,0.40); }"
        "#pinned-label, #recommend-label { color: #f2f6ff; }"
        "notebook, notebook header, scrolledwindow, list { background: rgba(22,35,56,0.46); color: #eef5ff; border-radius: 16px; }"
        "notebook tab { color: #b9cae4; padding: 8px 14px; }"
        "notebook tab:checked { color: #ffffff; background: rgba(89,143,213,0.58); border-radius: 10px; }"
        "#apps-list, #packages-list { background: transparent; }",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    load_apps(menu);
    add_pinned(menu);
    add_recommended(menu);
    add_all_apps(menu);
    add_packages(menu);
    g_signal_connect(menu->search, "search-changed", G_CALLBACK(filter_changed), menu);
    gtk_widget_show_all(menu->window);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.influent.danenone.start", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
