#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <string.h>

#define MANIFEST_PATH "/usr/share/influent/languages/manifest.tsv"

typedef enum { STAGE_WIFI, STAGE_LANGUAGE, STAGE_APPLY } SetupStage;

static gchar *setup_marker_path(void) {
    return g_build_filename(g_get_user_state_dir(), "influent-danenone", "firstboot-complete", NULL);
}

static gboolean setup_complete(void) {
    gchar *path = setup_marker_path();
    gboolean exists = g_file_test(path, G_FILE_TEST_EXISTS);
    g_free(path);
    return exists;
}

static void mark_setup_complete(void) {
    gchar *directory = g_build_filename(g_get_user_state_dir(), "influent-danenone", NULL);
    gchar *path = setup_marker_path();
    if (g_mkdir_with_parents(directory, 0700) == 0) g_file_set_contents(path, "done\n", -1, NULL);
    g_free(path);
    g_free(directory);
}

typedef struct {
    GtkApplication *app;
    GtkWidget *window;
    GtkWidget *title;
    GtkWidget *status;
    GtkWidget *language;
    GtkWidget *continue_button;
    GtkWidget *network_button;
    SetupStage stage;
} SetupState;

static gboolean wifi_connected(void) {
    gchar *out = NULL;
    gint status = 1;
    GError *error = NULL;
    gboolean ok = g_spawn_command_line_sync("nmcli -t -f STATE general", &out, NULL, &status, &error);
    gboolean connected = ok && status == 0 && out && g_strrstr(out, "connected") != NULL;
    if (error) g_error_free(error);
    g_free(out);
    return connected;
}

static void set_status(SetupState *state, const char *text) {
    gtk_label_set_text(GTK_LABEL(state->status), text);
}

static void open_network_editor(GtkButton *button, gpointer data) {
    (void)button; (void)data;
    GError *error = NULL;
    char *argv[] = {"nm-connection-editor", NULL};
    if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error) && error) {
        g_error_free(error);
    }
}

static void enter_language_stage(SetupState *state) {
    state->stage = STAGE_LANGUAGE;
    gtk_widget_set_visible(state->network_button, FALSE);
    gtk_widget_set_sensitive(state->language, TRUE);
    gtk_widget_set_visible(state->language, TRUE);
    gtk_label_set_text(GTK_LABEL(state->title), "Elige el idioma de Danenone");
    set_status(state, "Wi-Fi conectada. Ahora puedes elegir el idioma; English viene integrado.");
    gtk_button_set_label(GTK_BUTTON(state->continue_button), "Descargar y aplicar");
}

static void check_wifi(GtkButton *button, gpointer data) {
    (void)button;
    SetupState *state = data;
    if (!wifi_connected()) {
        set_status(state, "Conecta Wi-Fi primero. No se descargará ningún paquete sin conectividad.");
        gtk_widget_set_visible(state->network_button, TRUE);
        return;
    }
    enter_language_stage(state);
}

static gboolean valid_sha256(const char *value) {
    if (!value || strlen(value) != 64) return FALSE;
    for (size_t i = 0; i < 64; i++) {
        if (!g_ascii_isxdigit(value[i])) return FALSE;
    }
    return TRUE;
}

static gboolean secure_url(const char *url) {
    return url && g_str_has_prefix(url, "https://") && !strstr(url, "..") && !strchr(url, '\n');
}

static gboolean find_language(const char *code, gchar **url, gchar **sha256, gchar **package) {
    gchar *contents = NULL;
    if (!g_file_get_contents(MANIFEST_PATH, &contents, NULL, NULL)) return FALSE;
    gchar **lines = g_strsplit(contents, "\n", -1);
    gboolean found = FALSE;
    for (gchar **line = lines; *line; line++) {
        if ((*line)[0] == '#' || !**line) continue;
        gchar **parts = g_strsplit(*line, "|", 4);
        if (parts[0] && g_strcmp0(parts[0], code) == 0 && parts[1] && parts[2] && parts[3]) {
            *url = g_strdup(parts[2]);
            *sha256 = g_strdup(parts[3]);
            *package = g_strdup(parts[1]);
            found = TRUE;
            g_strfreev(parts);
            break;
        }
        g_strfreev(parts);
    }
    g_strfreev(lines);
    g_free(contents);
    return found;
}

static gboolean run_download_and_verify(const char *url, const char *expected, gchar **downloaded_path) {
    if (!secure_url(url) || !valid_sha256(expected)) return FALSE;
    gchar *tmp = g_strdup_printf("/tmp/influent-language-%" G_GINT64_FORMAT ".deb", g_get_real_time());
    gchar *command = g_strdup_printf("curl --fail --silent --show-error --location --proto '=https' --tlsv1.2 --output '%s' '%s'", tmp, url);
    gint status = 1;
    GError *error = NULL;
    gboolean ok = g_spawn_command_line_sync(command, NULL, NULL, &status, &error);
    if (error) g_error_free(error);
    g_free(command);
    if (!ok || status != 0) {
        g_remove(tmp);
        g_free(tmp);
        return FALSE;
    }
    gchar *sum_out = NULL;
    gchar *sum_command = g_strdup_printf("sha256sum '%s'", tmp);
    ok = g_spawn_command_line_sync(sum_command, &sum_out, NULL, &status, &error);
    if (error) g_error_free(error);
    g_free(sum_command);
    gboolean valid = ok && status == 0 && sum_out && g_str_has_prefix(sum_out, expected);
    g_free(sum_out);
    if (!valid) {
        g_remove(tmp);
        g_free(tmp);
        return FALSE;
    }
    *downloaded_path = tmp;
    return TRUE;
}

static void apply_language(SetupState *state) {
    const char *code = gtk_combo_box_get_active_id(GTK_COMBO_BOX(state->language));
    if (!code) code = "en";
    if (g_strcmp0(code, "en") == 0) {
        state->stage = STAGE_APPLY;
        mark_setup_complete();
        set_status(state, "English ya está integrado. La configuración inicial quedó aplicada.");
        gtk_button_set_label(GTK_BUTTON(state->continue_button), "Terminar");
        return;
    }
    gchar *url = NULL, *sha = NULL, *package = NULL;
    if (!find_language(code, &url, &sha, &package) || g_strcmp0(url, "builtin") == 0) {
        set_status(state, "Ese paquete de idioma todavía no tiene un asset verificado publicado.");
        g_free(url); g_free(sha); g_free(package);
        return;
    }
    set_status(state, "Descargando y verificando el paquete de idioma…");
    gchar *path = NULL;
    if (!run_download_and_verify(url, sha, &path)) {
        set_status(state, "No se pudo verificar el paquete. No se instalará ningún archivo.");
        g_free(url); g_free(sha); g_free(package);
        return;
    }
    gchar *install_command = g_strdup_printf("sh -c \"command -v dpkg >/dev/null 2>&1 && pkexec /usr/bin/dpkg -i '%s'\"", path);
    gint status = 1;
    GError *error = NULL;
    gboolean ok = g_spawn_command_line_sync(install_command, NULL, NULL, &status, &error);
    if (error) g_error_free(error);
    if (ok && status == 0) {
        state->stage = STAGE_APPLY;
        mark_setup_complete();
        set_status(state, "Paquete instalado y verificado. Reinicia o cierra este asistente para aplicar el idioma.");
        gtk_button_set_label(GTK_BUTTON(state->continue_button), "Terminar");
    } else {
        set_status(state, "El paquete fue verificado, pero este runtime no tiene dpkg o requiere autorización administrativa.");
    }
    g_remove(path);
    g_free(install_command); g_free(path); g_free(url); g_free(sha); g_free(package);
}

static void continue_clicked(GtkButton *button, gpointer data) {
    (void)button;
    SetupState *state = data;
    if (state->stage == STAGE_WIFI) check_wifi(button, state);
    else if (state->stage == STAGE_LANGUAGE) apply_language(state);
    else gtk_widget_destroy(state->window);
}

static void activate(GtkApplication *app, gpointer data) {
    (void)data;
    SetupState *state = g_new0(SetupState, 1);
    state->app = app;
    state->stage = STAGE_WIFI;
    state->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(state->window), "Primer arranque · Influent Danenone");
    gtk_window_set_default_size(GTK_WINDOW(state->window), 700, 470);
    gtk_window_set_position(GTK_WINDOW(state->window), GTK_WIN_POS_CENTER);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);
    gtk_widget_set_margin_start(root, 42); gtk_widget_set_margin_end(root, 42);
    gtk_widget_set_margin_top(root, 38); gtk_widget_set_margin_bottom(root, 30);
    GtkWidget *notch = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(notch, "setup-notch");
    gtk_widget_set_size_request(notch, 168, 28);
    gtk_box_pack_start(GTK_BOX(root), notch, FALSE, FALSE, 0);
    state->title = gtk_label_new("Conecta Danenone a Wi-Fi");
    gtk_widget_set_name(state->title, "setup-title");
    gtk_label_set_xalign(GTK_LABEL(state->title), 0.0);
    gtk_box_pack_start(GTK_BOX(root), state->title, FALSE, FALSE, 0);
    GtkWidget *description = gtk_label_new("Necesitamos conectividad antes de ofrecer idiomas descargables. English queda disponible sin descarga.");
    gtk_widget_set_name(description, "setup-description");
    gtk_label_set_xalign(GTK_LABEL(description), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(description), TRUE);
    gtk_box_pack_start(GTK_BOX(root), description, FALSE, FALSE, 0);
    state->language = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->language), "en", "English (integrado)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->language), "es", "Español (descarga verificada)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->language), "fr", "Français (descarga verificada)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->language), "de", "Deutsch (descarga verificada)");
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(state->language), "en");
    gtk_widget_set_visible(state->language, FALSE);
    gtk_box_pack_start(GTK_BOX(root), state->language, FALSE, FALSE, 0);
    state->status = gtk_label_new("Estado: comprobando cuando pulses Continuar.");
    gtk_widget_set_name(state->status, "setup-status");
    gtk_label_set_xalign(GTK_LABEL(state->status), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(state->status), TRUE);
    gtk_box_pack_start(GTK_BOX(root), state->status, TRUE, TRUE, 0);
    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    state->network_button = gtk_button_new_with_label("Abrir ajustes de red");
    state->continue_button = gtk_button_new_with_label("Continuar");
    gtk_widget_set_name(state->network_button, "setup-network");
    gtk_widget_set_name(state->continue_button, "setup-continue");
    gtk_widget_set_visible(state->network_button, FALSE);
    gtk_box_pack_start(GTK_BOX(actions), state->network_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(actions), state->continue_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(root), actions, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(state->window), root);

    g_signal_connect(state->continue_button, "clicked", G_CALLBACK(continue_clicked), state);
    g_signal_connect(state->network_button, "clicked", G_CALLBACK(open_network_editor), state);
    g_signal_connect_swapped(state->window, "destroy", G_CALLBACK(g_free), state);

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "window { background: #08111f; color: #f4f8ff; } #setup-notch { background: #02050a; border-radius: 0 0 18px 18px; } #setup-title { font-size: 30px; font-weight: 700; } #setup-description, #setup-status { color: #c2d4eb; font-size: 16px; } button, combobox { background: rgba(74,111,173,0.78); color: #fff; border-radius: 12px; padding: 10px 16px; } #setup-continue { background: rgba(109,180,148,0.9); }", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);
    gtk_widget_show_all(state->window);
    gtk_widget_set_visible(state->language, FALSE);
    gtk_widget_set_visible(state->network_button, FALSE);
}

int main(int argc, char **argv) {
    if (argc > 1 && g_strcmp0(argv[1], "--check-wifi") == 0) {
        g_print("wifi=%s\n", wifi_connected() ? "connected" : "disconnected");
        return 0;
    }
    gboolean replay = FALSE;
    for (int i = 1; i < argc; i++) {
        if (g_strcmp0(argv[i], "--replay") == 0) replay = TRUE;
    }
    if (!replay && setup_complete()) return 0;

    GtkApplication *app = gtk_application_new("com.influent.danenone.firstboot", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    char **gtk_argv = g_new0(char *, (gsize)argc + 1);
    int gtk_argc = 0;
    for (int i = 0; i < argc; i++) {
        if (g_strcmp0(argv[i], "--replay") == 0) continue;
        gtk_argv[gtk_argc++] = argv[i];
    }
    int status = g_application_run(G_APPLICATION(app), gtk_argc, gtk_argv);
    g_free(gtk_argv);
    g_object_unref(app);
    return status;
}
