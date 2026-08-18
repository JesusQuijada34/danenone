#include <gtk/gtk.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <gtk4-layer-shell.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>

#define WALLPAPER_INSTALLED "/usr/share/backgrounds/influent/oobe-river-blurred.jpg"
#define WALLPAPER_LOCAL "/home/ubuntu/danenone/native-shell/assets/oobe-river-blurred.jpg"
#define TRASH_ICON_INSTALLED "/usr/share/icons/danenone/places/scalable/danenone-trash.svg"
#define TRASH_ICON_LOCAL "/home/ubuntu/danenone/native-shell/assets/icons/danenone-trash.svg"
#define FILES_ICON_INSTALLED "/usr/share/icons/danenone/apps/scalable/danenone-files.svg"
#define FILES_ICON_LOCAL "/home/ubuntu/danenone/native-shell/assets/icons/danenone-files.svg"
#define START_ICON_INSTALLED "/usr/share/icons/danenone/apps/scalable/danenone-start.svg"
#define START_ICON_LOCAL "/home/ubuntu/danenone/native-shell/assets/icons/danenone-start.svg"
#define LAYOUT_PATH ".config/influent-danenone/desktop-layout.tsv"

typedef struct {
    gchar *id;
    gchar *name;
    GIcon *icon;
    GAppInfo *app;
    gchar *uri;
    gboolean trash;
    gint saved_row;
    gint saved_col;
} DesktopItem;

typedef struct {
    GtkApplication *app;
    GtkWidget *window;
    GtkWidget *taskbar_window;
    GtkWidget *notch_window;
    GtkWidget *root;
    GtkWidget *desktop_grid;
    GtkWidget *taskbar;
    GtkWidget *clock;
    GPtrArray *items;
    GHashTable *layout;
    gboolean rebuilding;
} DesktopShell;

static const char *asset_path(const char *installed, const char *local) {
    return g_file_test(installed, G_FILE_TEST_EXISTS) ? installed : local;
}

static gboolean layer_shell_available(void) {
    GdkDisplay *display = gdk_display_get_default();
    const char *name = display ? gdk_display_get_name(display) : NULL;
    return name && g_str_has_prefix(name, "wayland-") && gtk_layer_is_supported();
}

static GtkWidget *picture_icon(const char *installed, const char *local, int size) {
    GtkWidget *picture = gtk_picture_new_for_filename(asset_path(installed, local));
    gtk_picture_set_content_fit(GTK_PICTURE(picture), GTK_CONTENT_FIT_CONTAIN);
    gtk_widget_set_size_request(picture, size, size);
    return picture;
}

static void desktop_item_free(gpointer data) {
    DesktopItem *item = data;
    if (!item) return;
    g_free(item->id);
    g_free(item->name);
    g_clear_object(&item->icon);
    g_clear_object(&item->app);
    g_free(item->uri);
    g_free(item);
}

static gchar *layout_file_path(void) {
    const char *config = g_get_user_config_dir();
    return g_build_filename(config, "influent-danenone", "desktop-layout.tsv", NULL);
}

static void load_layout(DesktopShell *shell) {
    gchar *path = layout_file_path();
    gchar *contents = NULL;
    if (g_file_get_contents(path, &contents, NULL, NULL)) {
        gchar **lines = g_strsplit(contents, "\n", -1);
        for (guint i = 0; lines[i]; i++) {
            gchar **parts = g_strsplit(lines[i], "\t", 3);
            if (parts[0] && parts[0][0] && parts[1] && parts[2]) {
                gchar *position = g_strdup_printf("%s\t%s", parts[1], parts[2]);
                g_hash_table_replace(shell->layout, g_strdup(parts[0]), position);
            }
            g_strfreev(parts);
        }
        g_strfreev(lines);
    }
    g_free(contents);
    g_free(path);
}

static void save_layout(DesktopShell *shell, guint rows) {
    if (!shell || !shell->items || rows == 0) return;
    GString *output = g_string_new("");
    for (guint i = 0; i < shell->items->len; i++) {
        DesktopItem *item = g_ptr_array_index(shell->items, i);
        guint column = i / rows;
        guint row = i % rows;
        g_string_append_printf(output, "%s\t%u\t%u\n", item->id, row, column);
    }
    gchar *path = layout_file_path();
    gchar *dir = g_path_get_dirname(path);
    g_mkdir_with_parents(dir, 0700);
    gchar *tmp = g_strdup_printf("%s.tmp", path);
    if (g_file_set_contents(tmp, output->str, -1, NULL)) {
        g_chmod(tmp, 0600);
        g_rename(tmp, path);
    } else {
        g_remove(tmp);
    }
    g_free(tmp);
    g_free(dir);
    g_free(path);
    g_string_free(output, TRUE);
}

static gint item_compare(gconstpointer a, gconstpointer b, gpointer data) {
    DesktopShell *shell = data;
    const DesktopItem *left = *(DesktopItem * const *)a;
    const DesktopItem *right = *(DesktopItem * const *)b;
    const char *lp = g_hash_table_lookup(shell->layout, left->id);
    const char *rp = g_hash_table_lookup(shell->layout, right->id);
    if (lp && rp) return g_strcmp0(lp, rp);
    if (lp) return -1;
    if (rp) return 1;
    if (left->trash != right->trash) return left->trash ? -1 : 1;
    return g_utf8_collate(left->name, right->name);
}

static DesktopItem *new_item(const char *id, const char *name, GIcon *icon, GAppInfo *app, const char *uri, gboolean trash) {
    DesktopItem *item = g_new0(DesktopItem, 1);
    item->id = g_strdup(id);
    item->name = g_strdup(name ? name : id);
    item->icon = icon ? G_ICON(g_object_ref(icon)) : NULL;
    item->app = app ? G_APP_INFO(g_object_ref(app)) : NULL;
    item->uri = g_strdup(uri);
    item->trash = trash;
    return item;
}

static void add_app_item(DesktopShell *shell, GHashTable *seen, GAppInfo *app) {
    if (!app || !g_app_info_should_show(app) || !G_IS_DESKTOP_APP_INFO(app)) return;
    const char *id = g_app_info_get_id(app);
    const char *name = g_app_info_get_name(app);
    GIcon *icon = g_app_info_get_icon(app);
    if (!id || !id[0] || !name || !name[0] || !icon || g_hash_table_contains(seen, id)) return;
    g_hash_table_add(seen, g_strdup(id));
    g_ptr_array_add(shell->items, new_item(id, name, icon, app, NULL, FALSE));
}

static void collect_items(DesktopShell *shell) {
    shell->items = g_ptr_array_new_with_free_func(desktop_item_free);
    GHashTable *seen = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    GFile *trash_file = g_file_new_for_path(asset_path(TRASH_ICON_INSTALLED, TRASH_ICON_LOCAL));
    GIcon *trash_icon = g_file_icon_new(trash_file);
    g_ptr_array_add(shell->items, new_item("special:trash", "Papelera", trash_icon, NULL, "trash:///", TRUE));
    g_object_unref(trash_icon);
    g_object_unref(trash_file);

    const char *pinned_ids[] = {"firefox.desktop", "org.kde.dolphin.desktop", "vlc.desktop", "foundstore.desktop", NULL};
    GList *apps = g_app_info_get_all();
    for (guint p = 0; pinned_ids[p]; p++) {
        for (GList *node = apps; node; node = node->next) {
            GAppInfo *app = G_APP_INFO(node->data);
            const char *id = g_app_info_get_id(app);
            if (id && g_strcmp0(id, pinned_ids[p]) == 0) add_app_item(shell, seen, app);
        }
    }
    g_list_free_full(apps, g_object_unref);

    const char *desktop_dir = g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP);
    if (!desktop_dir || !desktop_dir[0]) desktop_dir = g_build_filename(g_get_home_dir(), "Desktop", NULL);
    GDir *dir = g_dir_open(desktop_dir, 0, NULL);
    if (dir) {
        const char *filename = NULL;
        while ((filename = g_dir_read_name(dir))) {
            if (!g_str_has_suffix(filename, ".desktop")) continue;
            gchar *path = g_build_filename(desktop_dir, filename, NULL);
            GDesktopAppInfo *app = g_desktop_app_info_new_from_filename(path);
            if (app) { add_app_item(shell, seen, G_APP_INFO(app)); g_object_unref(app); }
            g_free(path);
        }
        g_dir_close(dir);
    }

    GVolumeMonitor *monitor = g_volume_monitor_get();
    GList *mounts = g_volume_monitor_get_mounts(monitor);
    for (GList *node = mounts; node; node = node->next) {
        GMount *mount = G_MOUNT(node->data);
        GFile *root = g_mount_get_root(mount);
        gchar *uri = g_file_get_uri(root);
        gchar *id = g_strdup_printf("mount:%s", uri ? uri : "unknown");
        GIcon *icon = g_mount_get_icon(mount);
        g_ptr_array_add(shell->items, new_item(id, g_mount_get_name(mount), icon, NULL, uri, FALSE));
        g_free(id);
        g_free(uri);
        g_object_unref(root);
        g_object_unref(mount);
    }
    g_list_free(mounts);
    g_object_unref(monitor);
    g_hash_table_destroy(seen);
    g_ptr_array_sort_with_data(shell->items, item_compare, shell);
}

static void launch_item(GtkButton *button, gpointer data) {
    (void)button;
    DesktopItem *item = data;
    if (!item) return;
    if (item->trash || item->uri) {
        g_app_info_launch_default_for_uri(item->uri ? item->uri : "trash:///", NULL, NULL);
    } else if (item->app) {
        g_app_info_launch(item->app, NULL, NULL, NULL);
    }
}

static GtkWidget *item_button(DesktopItem *item) {
    GtkWidget *button = gtk_button_new();
    gtk_widget_add_css_class(button, "desktop-item");
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    GtkWidget *icon = NULL;
    if (item->icon) {
        icon = gtk_image_new_from_gicon(item->icon);
        gtk_image_set_pixel_size(GTK_IMAGE(icon), 64);
    } else {
        icon = picture_icon(FILES_ICON_INSTALLED, FILES_ICON_LOCAL, 64);
    }
    gtk_box_append(GTK_BOX(box), icon);
    GtkWidget *label = gtk_label_new(item->name);
    gtk_widget_add_css_class(label, "desktop-item-label");
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_max_width_chars(GTK_LABEL(label), 15);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(box), label);
    gtk_button_set_child(GTK_BUTTON(button), box);
    gtk_widget_set_tooltip_text(button, item->name);
    g_signal_connect(button, "clicked", G_CALLBACK(launch_item), item);
    return button;
}

static guint desktop_rows(DesktopShell *shell) {
    int height = gtk_widget_get_height(shell->desktop_grid);
    if (height <= 0) height = 640;
    guint rows = (guint)MAX(1, (height - 24) / 118);
    return MIN(rows, 8);
}

static void rebuild_grid(DesktopShell *shell) {
    if (!shell || shell->rebuilding || !shell->desktop_grid) return;
    shell->rebuilding = TRUE;
    while (gtk_widget_get_first_child(shell->desktop_grid)) gtk_grid_remove(GTK_GRID(shell->desktop_grid), gtk_widget_get_first_child(shell->desktop_grid));
    guint rows = desktop_rows(shell);
    for (guint i = 0; i < shell->items->len; i++) {
        DesktopItem *item = g_ptr_array_index(shell->items, i);
        guint column = i / rows;
        guint row = i % rows;
        gtk_grid_attach(GTK_GRID(shell->desktop_grid), item_button(item), (int)column, (int)row, 1, 1);
    }
    save_layout(shell, rows);
    shell->rebuilding = FALSE;
}

static void grid_size_changed(GObject *object, GParamSpec *pspec, gpointer data) {
    (void)object; (void)pspec;
    rebuild_grid(data);
}

static gboolean update_clock(gpointer data) {
    GtkLabel *clock = GTK_LABEL(data);
    GDateTime *now = g_date_time_new_now_local();
    gchar *text = g_date_time_format(now, "%H:%M");
    gtk_label_set_text(clock, text);
    g_free(text);
    g_date_time_unref(now);
    return G_SOURCE_CONTINUE;
}

static void spawn_program(const char *command) {
    if (command) g_spawn_command_line_async(command, NULL);
}

static GtkWidget *task_button(const char *label, const char *icon_name, const char *command) {
    GtkWidget *button = gtk_button_new();
    gtk_widget_add_css_class(button, "task-button");
    GtkWidget *image = gtk_image_new_from_icon_name(icon_name);
    gtk_image_set_pixel_size(GTK_IMAGE(image), 24);
    gtk_button_set_child(GTK_BUTTON(button), image);
    gtk_widget_set_tooltip_text(button, label);
    if (command) g_signal_connect_swapped(button, "clicked", G_CALLBACK(spawn_program), (gpointer)command);
    return button;
}

static void build_taskbar(DesktopShell *shell) {
    shell->taskbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_add_css_class(shell->taskbar, "taskbar");
    gtk_widget_set_halign(shell->taskbar, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(shell->taskbar, GTK_ALIGN_END);
    gtk_widget_set_margin_start(shell->taskbar, 22);
    gtk_widget_set_margin_end(shell->taskbar, 22);
    gtk_widget_set_margin_bottom(shell->taskbar, 18);
    GtkWidget *start = gtk_button_new();
    gtk_widget_add_css_class(start, "task-button");
    gtk_button_set_child(GTK_BUTTON(start), picture_icon(START_ICON_INSTALLED, START_ICON_LOCAL, 28));
    gtk_widget_set_tooltip_text(start, "Inicio");
    g_signal_connect_swapped(start, "clicked", G_CALLBACK(spawn_program), (gpointer)"/usr/local/bin/influent-danenone-start");
    gtk_box_append(GTK_BOX(shell->taskbar), start);
    gtk_box_append(GTK_BOX(shell->taskbar), task_button("Archivos", "folder-symbolic", "dolphin"));
    gtk_box_append(GTK_BOX(shell->taskbar), task_button("Navegador", "web-browser-symbolic", "firefox"));
    gtk_box_append(GTK_BOX(shell->taskbar), task_button("Foundstore", "applications-system-symbolic", "foundstore"));
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(shell->taskbar), spacer);
    shell->clock = gtk_label_new("");
    gtk_widget_add_css_class(shell->clock, "task-clock");
    gtk_box_append(GTK_BOX(shell->taskbar), shell->clock);
    if (layer_shell_available()) {
        shell->taskbar_window = gtk_application_window_new(shell->app);
        gtk_layer_init_for_window(GTK_WINDOW(shell->taskbar_window));
        gtk_layer_set_namespace(GTK_WINDOW(shell->taskbar_window), "influent-danenone-taskbar-gtk4");
        gtk_layer_set_layer(GTK_WINDOW(shell->taskbar_window), GTK_LAYER_SHELL_LAYER_TOP);
        gtk_layer_set_anchor(GTK_WINDOW(shell->taskbar_window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(shell->taskbar_window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(shell->taskbar_window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_exclusive_zone(GTK_WINDOW(shell->taskbar_window), 84);
        gtk_layer_set_margin(GTK_WINDOW(shell->taskbar_window), GTK_LAYER_SHELL_EDGE_BOTTOM, 8);
        gtk_window_set_child(GTK_WINDOW(shell->taskbar_window), shell->taskbar);
        gtk_window_present(GTK_WINDOW(shell->taskbar_window));
    } else {
        gtk_overlay_add_overlay(GTK_OVERLAY(shell->root), shell->taskbar);
    }
}

static void build_notch(DesktopShell *shell) {
    if (!layer_shell_available()) return;
    shell->notch_window = gtk_application_window_new(shell->app);
    gtk_layer_init_for_window(GTK_WINDOW(shell->notch_window));
    gtk_layer_set_namespace(GTK_WINDOW(shell->notch_window), "influent-danenone-notch-gtk4");
    gtk_layer_set_layer(GTK_WINDOW(shell->notch_window), GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_anchor(GTK_WINDOW(shell->notch_window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_exclusive_zone(GTK_WINDOW(shell->notch_window), 28);
    gtk_window_set_default_size(GTK_WINDOW(shell->notch_window), 360, 28);
    GtkWidget *notch = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(notch, "notch");
    gtk_window_set_child(GTK_WINDOW(shell->notch_window), notch);
    gtk_window_present(GTK_WINDOW(shell->notch_window));
}

static void activate(GtkApplication *app, gpointer data) {
    (void)data;
    DesktopShell *shell = g_new0(DesktopShell, 1);
    shell->app = app;
    shell->layout = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    load_layout(shell);
    collect_items(shell);
    shell->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(shell->window), "Influent Danenone");
    if (layer_shell_available()) {
        gtk_layer_init_for_window(GTK_WINDOW(shell->window));
        gtk_layer_set_namespace(GTK_WINDOW(shell->window), "influent-danenone-desktop-gtk4");
        gtk_layer_set_layer(GTK_WINDOW(shell->window), GTK_LAYER_SHELL_LAYER_BACKGROUND);
        gtk_layer_set_anchor(GTK_WINDOW(shell->window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(shell->window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(shell->window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(shell->window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_exclusive_zone(GTK_WINDOW(shell->window), 0);
    } else {
        gtk_window_fullscreen(GTK_WINDOW(shell->window));
    }
    shell->root = gtk_overlay_new();
    gtk_window_set_child(GTK_WINDOW(shell->window), shell->root);
    GtkWidget *wallpaper = gtk_picture_new_for_filename(asset_path(WALLPAPER_INSTALLED, WALLPAPER_LOCAL));
    gtk_picture_set_content_fit(GTK_PICTURE(wallpaper), GTK_CONTENT_FIT_COVER);
    gtk_widget_set_hexpand(wallpaper, TRUE);
    gtk_widget_set_vexpand(wallpaper, TRUE);
    gtk_overlay_set_child(GTK_OVERLAY(shell->root), wallpaper);
    shell->desktop_grid = gtk_grid_new();
    gtk_widget_add_css_class(shell->desktop_grid, "desktop-grid");
    gtk_widget_set_halign(shell->desktop_grid, GTK_ALIGN_FILL);
    gtk_widget_set_valign(shell->desktop_grid, GTK_ALIGN_FILL);
    gtk_widget_set_margin_start(shell->desktop_grid, 26);
    gtk_widget_set_margin_end(shell->desktop_grid, 26);
    gtk_widget_set_margin_top(shell->desktop_grid, 58);
    gtk_widget_set_margin_bottom(shell->desktop_grid, 102);
    gtk_grid_set_row_spacing(GTK_GRID(shell->desktop_grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(shell->desktop_grid), 10);
    gtk_overlay_add_overlay(GTK_OVERLAY(shell->root), shell->desktop_grid);
    g_signal_connect(shell->desktop_grid, "notify::width", G_CALLBACK(grid_size_changed), shell);
    g_signal_connect(shell->desktop_grid, "notify::height", G_CALLBACK(grid_size_changed), shell);
    build_taskbar(shell);
    if (!gtk_layer_is_supported()) {
        GtkWidget *notch = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_add_css_class(notch, "notch");
        gtk_widget_set_halign(notch, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(notch, GTK_ALIGN_START);
        gtk_widget_set_size_request(notch, 360, 28);
        gtk_overlay_add_overlay(GTK_OVERLAY(shell->root), notch);
    } else {
        build_notch(shell);
    }
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css,
        "window { background: #172a25; }"
        ".desktop-grid { background: transparent; }"
        ".desktop-item { min-width: 104px; min-height: 104px; padding: 8px; background: transparent; border: 1px solid transparent; border-radius: 8px; color: #ffffff; }"
        ".desktop-item:hover { background: rgba(255,255,255,.12); border-color: rgba(255,255,255,.26); }"
        ".desktop-item-label { color: #ffffff; font-size: 13px; text-shadow: 0 1px 3px rgba(0,0,0,.72); }"
        ".taskbar { min-height: 58px; min-width: 560px; padding: 8px 12px; background: rgba(245,250,248,.78); border: 1px solid rgba(255,255,255,.82); border-radius: 12px; box-shadow: 0 10px 28px rgba(0,0,0,.22); }"
        ".task-button { min-width: 42px; min-height: 42px; padding: 6px; background: transparent; border: 1px solid transparent; border-radius: 8px; }"
        ".task-button:hover { background: rgba(0,185,130,.16); border-color: rgba(0,130,94,.32); }"
        ".task-clock { color: #17342d; font-size: 13px; padding: 0 8px; }"
        ".notch { min-height: 28px; min-width: 360px; background: #111a19; border-radius: 0 0 14px 14px; }"
        ".dark .taskbar { background: rgba(20,29,28,.88); border-color: rgba(255,255,255,.24); }"
        ".dark .task-clock { color: #ecf8f3; }");
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);
    gtk_window_present(GTK_WINDOW(shell->window));
    rebuild_grid(shell);
    update_clock(shell->clock);
    g_timeout_add_seconds(1, update_clock, shell->clock);
}

int main(int argc, char **argv) {
    setlocale(LC_ALL, "");
    GtkApplication *app = gtk_application_new("com.influent.danenone.desktop.gtk4", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
