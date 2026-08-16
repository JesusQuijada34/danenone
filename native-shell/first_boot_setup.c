#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MANIFEST_PATH "/usr/share/influent/languages/manifest.tsv"
#define CUBE_DIR "/usr/share/icons/influent"
#define WALLPAPER_PATH "/usr/share/backgrounds/influent/danenone-river-wallpaper.jpg"
#define OOBE_WALLPAPER_PATH "/usr/share/backgrounds/influent/oobe-river-blurred.jpg"
#define BRAND_LOGO_PATH "/usr/share/influent/danenone-cube-logo.png"
#define OOBE_ICON_DIR "/usr/share/icons/influent/oobe"

typedef enum {
    STAGE_WELCOME,
    STAGE_WIFI,
    STAGE_LANGUAGE,
    STAGE_REGION,
    STAGE_PRIVACY,
    STAGE_APPLY,
    STAGE_DONE
} SetupStage;

typedef struct {
    GtkApplication *app;
    GtkWidget *window;
    GtkWidget *stack;
    GtkWidget *title;
    GtkWidget *subtitle;
    GtkWidget *stage_label;
    GtkWidget *status;
    GtkWidget *progress;
    GtkWidget *cube;
    GtkWidget *cube_caption;
    GtkWidget *language;
    GtkWidget *keyboard;
    GtkWidget *timezone;
    GtkWidget *username;
    GtkWidget *password;
    GtkWidget *telemetry;
    GtkWidget *network_button;
    GtkWidget *back_button;
    GtkWidget *continue_button;
    GtkWidget *outer;
    GtkWidget *finish_overlay;
    GtkWidget *finish_spinner;
    SetupStage stage;
    guint cube_timer;
    guint apply_timer;
    guint intro_timer;
    guint finish_timer;
    guint cube_state;
    gdouble intro_opacity;
} SetupState;

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

static const char *cube_file_for_state(guint state) {
    static const char *names[] = {
        "danenone-cube-normal.svg",
        "danenone-cube-hover.svg",
        "danenone-cube-pressed.svg"
    };
    static char path[512];
    const char *name = names[state % 3];
    const char *candidates[] = {
        CUBE_DIR "/danenone-cube-normal.svg",
        "/home/ubuntu/danenone/native-shell/assets/danenone-cube/danenone-cube-normal.svg",
        "assets/danenone-cube/danenone-cube-normal.svg"
    };
    if (state % 3 == 0) {
        for (guint i = 0; i < G_N_ELEMENTS(candidates); i++) {
            if (g_file_test(candidates[i], G_FILE_TEST_EXISTS)) {
                g_strlcpy(path, candidates[i], sizeof(path));
                return path;
            }
        }
    }
    g_snprintf(path, sizeof(path), "%s/%s", CUBE_DIR, name);
    if (g_file_test(path, G_FILE_TEST_EXISTS)) return path;
    g_snprintf(path, sizeof(path), "/home/ubuntu/danenone/native-shell/assets/danenone-cube/%s", name);
    if (g_file_test(path, G_FILE_TEST_EXISTS)) return path;
    g_snprintf(path, sizeof(path), "assets/danenone-cube/%s", name);
    return path;
}

static gboolean animate_cube(gpointer data) {
    SetupState *state = data;
    state->cube_state = (state->cube_state + 1) % 3;
    if (g_file_test(BRAND_LOGO_PATH, G_FILE_TEST_EXISTS)) {
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(BRAND_LOGO_PATH, 220, 220, TRUE, NULL);
        if (pixbuf) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(state->cube), pixbuf);
            g_object_unref(pixbuf);
        }
    }
    gtk_widget_set_size_request(state->cube, 220, 220);
    gtk_image_set_pixel_size(GTK_IMAGE(state->cube), 220);
    const char *captions[] = {"Observando tu espacio", "Preparando una bienvenida", "Todo listo para acompañarte"};
    gtk_label_set_text(GTK_LABEL(state->cube_caption), captions[state->cube_state]);
    return G_SOURCE_CONTINUE;
}

static gboolean intro_step(gpointer data) {
    SetupState *state = data;
    state->intro_opacity += 0.08;
    if (state->intro_opacity >= 1.0) {
        state->intro_opacity = 1.0;
        gtk_widget_set_opacity(state->outer, state->intro_opacity);
        state->intro_timer = 0;
        return G_SOURCE_REMOVE;
    }
    gtk_widget_set_opacity(state->outer, state->intro_opacity);
    return G_SOURCE_CONTINUE;
}

static gboolean finish_timeout(gpointer data) {
    SetupState *state = data;
    state->finish_timer = 0;
    if (state->window) gtk_widget_destroy(state->window);
    return G_SOURCE_REMOVE;
}

static void show_finish_screen(SetupState *state) {
    gtk_widget_hide(state->outer);
    gtk_widget_show(state->finish_overlay);
    gtk_spinner_start(GTK_SPINNER(state->finish_spinner));
    if (!state->finish_timer) state->finish_timer = g_timeout_add(1800, finish_timeout, state);
}

static void cleanup_state(GtkWidget *window, gpointer data) {
    (void)window;
    SetupState *state = data;
    if (state->cube_timer) g_source_remove(state->cube_timer);
    if (state->apply_timer) g_source_remove(state->apply_timer);
    if (state->intro_timer) g_source_remove(state->intro_timer);
    if (state->finish_timer) g_source_remove(state->finish_timer);
    g_free(state);
}

static void open_network_editor(GtkButton *button, gpointer data) {
    (void)button;
    (void)data;
    GError *error = NULL;
    char *argv[] = {"nm-connection-editor", NULL};
    if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error) && error) {
        g_error_free(error);
    }
}

static void populate_languages(SetupState *state) {
    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(state->language));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->language), "en", "English — integrado");
    gchar *contents = NULL;
    if (g_file_get_contents(MANIFEST_PATH, &contents, NULL, NULL)) {
        gchar **lines = g_strsplit(contents, "\n", -1);
        for (gchar **line = lines; *line; line++) {
            if ((*line)[0] == '#' || !**line) continue;
            gchar **parts = g_strsplit(*line, "|", 5);
            if (parts[0] && parts[1] && parts[2] && parts[4] && g_strcmp0(parts[0], "en") != 0) {
                const char *availability = g_strcmp0(parts[2], "builtin") == 0 ? "integrado" : "descarga verificada";
                gchar *label = g_strdup_printf("%s — %s", parts[4], availability);
                gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->language), parts[0], label);
                g_free(label);
            }
            g_strfreev(parts);
        }
        g_strfreev(lines);
    }
    g_free(contents);
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(state->language), "en");
}

static gboolean valid_sha256(const char *value) {
    if (!value || strlen(value) != 64) return FALSE;
    for (size_t i = 0; i < 64; i++) if (!g_ascii_isxdigit(value[i])) return FALSE;
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
        gchar **parts = g_strsplit(*line, "|", 5);
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

static gboolean apply_language_choice(SetupState *state) {
    const char *code = gtk_combo_box_get_active_id(GTK_COMBO_BOX(state->language));
    if (!code || g_strcmp0(code, "en") == 0) {
        set_status(state, "English está integrado. No se necesita descargar nada.");
        return TRUE;
    }
    gchar *url = NULL, *sha = NULL, *package = NULL;
    if (!find_language(code, &url, &sha, &package) || g_strcmp0(url, "builtin") == 0) {
        set_status(state, "Ese paquete todavía no tiene un asset HTTPS verificado publicado.");
        g_free(url); g_free(sha); g_free(package);
        return FALSE;
    }
    set_status(state, "Descargando y verificando el paquete de idioma…");
    gchar *path = NULL;
    gboolean valid = run_download_and_verify(url, sha, &path);
    if (!valid) {
        set_status(state, "No se pudo verificar el paquete. No se instalará ningún archivo.");
        g_free(url); g_free(sha); g_free(package);
        return FALSE;
    }
    gchar *install_command = g_strdup_printf("sh -c \"command -v dpkg >/dev/null 2>&1 && pkexec /usr/bin/dpkg -i '%s'\"", path);
    gint status = 1;
    GError *error = NULL;
    gboolean ok = g_spawn_command_line_sync(install_command, NULL, NULL, &status, &error);
    if (error) g_error_free(error);
    g_remove(path);
    g_free(install_command); g_free(path); g_free(url); g_free(sha); g_free(package);
    if (!ok || status != 0) {
        set_status(state, "El paquete fue verificado, pero requiere dpkg y autorización administrativa.");
        return FALSE;
    }
    set_status(state, "Idioma instalado y verificado correctamente.");
    return TRUE;
}

static gboolean valid_username(const char *value) {
    if (!value || !*value || strlen(value) > 32) return FALSE;
    if (!(g_ascii_isalpha(value[0]) || value[0] == '_')) return FALSE;
    for (const char *p = value + 1; *p; p++) {
        if (!(g_ascii_isalnum(*p) || *p == '_' || *p == '-')) return FALSE;
    }
    return TRUE;
}

static gboolean create_local_user(SetupState *state) {
    const char *username = gtk_entry_get_text(GTK_ENTRY(state->username));
    const char *password = gtk_entry_get_text(GTK_ENTRY(state->password));
    if (!valid_username(username)) {
        set_status(state, "Usa un nombre local de 1 a 32 caracteres: letras, números, guion o guion bajo.");
        return FALSE;
    }
    if (!password || strlen(password) < 4) {
        set_status(state, "La contraseña debe tener al menos 4 caracteres para crear el usuario.");
        return FALSE;
    }
    gchar *quser = g_shell_quote(username);
    gchar *qpass = g_shell_quote(password);
    const char *prefix = getuid() == 0 ? "" : "sudo -n ";
    gchar *user_cmd = g_strdup_printf("%s/usr/bin/id -u %s >/dev/null 2>&1 || %s/usr/bin/useradd -m -U -s /bin/bash %s", prefix, quser, prefix, quser);
    gint status = 1;
    GError *error = NULL;
    gboolean ok = g_spawn_command_line_sync(user_cmd, NULL, NULL, &status, &error);
    if (error) g_error_free(error);
    g_free(user_cmd);
    if (!ok || status != 0) {
        set_status(state, "No se pudo crear el usuario local. No se continuará con el login.");
        g_free(quser); g_free(qpass);
        return FALSE;
    }
    gchar *password_cmd = g_strdup_printf("printf '%%s:%%s\\n' %s %s | %s/usr/bin/chpasswd", quser, qpass, prefix);
    error = NULL;
    ok = g_spawn_command_line_sync(password_cmd, NULL, NULL, &status, &error);
    if (error) g_error_free(error);
    g_free(password_cmd);
    if (!ok || status != 0) {
        set_status(state, "El usuario fue creado, pero no se pudo establecer la contraseña.");
        g_free(quser); g_free(qpass);
        return FALSE;
    }
    gchar *login_cmd = g_strdup_printf("sh -c \\\"%s/usr/bin/sed -i -e 's|^user = .*|user = \\\\\"%s\\\\\\\"|' -e 's|/home/danenone|/home/%s|g' /etc/greetd/config.toml\\\"", prefix, username, username);
    error = NULL;
    g_spawn_command_line_sync(login_cmd, NULL, NULL, &status, &error);
    if (error) g_error_free(error);
    g_free(login_cmd);
    g_free(quser); g_free(qpass);
    set_status(state, "Usuario local creado. Su sesión se usará en el próximo inicio.");
    return TRUE;
}

static void set_stage(SetupState *state, SetupStage stage) {
    state->stage = stage;
    const char *names[] = {"Bienvenida", "Conectividad", "Idioma", "Región y teclado", "Privacidad y usuario", "Aplicando cambios", "Listo"};
    gchar *stage_text = g_strdup_printf("Paso %d de 6 · %s", stage == STAGE_DONE ? 6 : stage + 1, names[stage]);
    gtk_label_set_text(GTK_LABEL(state->stage_label), stage_text);
    g_free(stage_text);
    gtk_stack_set_visible_child_name(GTK_STACK(state->stack), names[stage]);
    gtk_widget_set_visible(state->back_button, stage > STAGE_WELCOME && stage < STAGE_APPLY);
    gtk_widget_set_visible(state->network_button, stage == STAGE_WIFI);
    gtk_widget_set_sensitive(state->continue_button, stage != STAGE_APPLY);
    if (stage == STAGE_WELCOME) {
        gtk_label_set_text(GTK_LABEL(state->title), "Configura tu espacio Danenone");
        gtk_label_set_text(GTK_LABEL(state->subtitle), "Unos pasos más y tendrás un escritorio listo para ti.");
        gtk_button_set_label(GTK_BUTTON(state->continue_button), "Comenzar");
        gtk_label_set_text(GTK_LABEL(state->cube_caption), "Te acompañaré durante la configuración");
    } else if (stage == STAGE_WIFI) {
        gtk_label_set_text(GTK_LABEL(state->title), "Conecta el sistema");
        gtk_label_set_text(GTK_LABEL(state->subtitle), "Necesitamos Wi-Fi antes de ofrecer idiomas descargables y actualizaciones.");
        gtk_button_set_label(GTK_BUTTON(state->continue_button), "Comprobar conexión");
        set_status(state, "La conectividad se comprueba usando el estado real de NetworkManager.");
    } else if (stage == STAGE_LANGUAGE) {
        gtk_label_set_text(GTK_LABEL(state->title), "Elige el idioma");
        gtk_label_set_text(GTK_LABEL(state->subtitle), "English está integrado. Los demás paquetes se descargan por HTTPS y se verifican con SHA-256.");
        gtk_button_set_label(GTK_BUTTON(state->continue_button), "Continuar");
        set_status(state, "Selecciona el idioma que quieres usar.");
    } else if (stage == STAGE_REGION) {
        gtk_label_set_text(GTK_LABEL(state->title), "Ajusta tu región");
        gtk_label_set_text(GTK_LABEL(state->subtitle), "Define teclado y zona horaria para que Danenone se adapte a tu ubicación.");
        gtk_button_set_label(GTK_BUTTON(state->continue_button), "Continuar");
        set_status(state, "Estos controles preparan la sesión del usuario.");
    } else if (stage == STAGE_PRIVACY) {
        gtk_label_set_text(GTK_LABEL(state->title), "Crea tu espacio privado");
        gtk_label_set_text(GTK_LABEL(state->subtitle), "Elige el nombre del usuario y decide si quieres compartir diagnósticos opcionales.");
        gtk_button_set_label(GTK_BUTTON(state->continue_button), "Aplicar configuración");
        set_status(state, "Tus preferencias se mantienen locales por defecto.");
    } else if (stage == STAGE_APPLY) {
        gtk_label_set_text(GTK_LABEL(state->title), "Preparando Danenone");
        gtk_label_set_text(GTK_LABEL(state->subtitle), "Aplicando idioma, región, preferencias y componentes del escritorio.");
        gtk_button_set_label(GTK_BUTTON(state->continue_button), "Continuar");
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(state->progress), 0.0);
        gtk_widget_set_visible(state->progress, TRUE);
        if (!state->apply_timer) state->apply_timer = g_timeout_add(180, (GSourceFunc)animate_cube, state);
    } else {
        gtk_label_set_text(GTK_LABEL(state->title), "Todo está listo");
        gtk_label_set_text(GTK_LABEL(state->subtitle), "Tu espacio Danenone está preparado. Ahora comienza el tour del cubito.");
        gtk_button_set_label(GTK_BUTTON(state->continue_button), "Entrar al escritorio");
        set_status(state, "Configuración completada correctamente.");
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(state->progress), 1.0);
    }
    gdouble fraction = stage == STAGE_DONE ? 1.0 : ((gdouble)stage / 6.0);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(state->progress), fraction);
}

static void back_clicked(GtkButton *button, gpointer data) {
    (void)button;
    SetupState *state = data;
    if (state->stage > STAGE_WELCOME && state->stage < STAGE_APPLY) set_stage(state, (SetupStage)(state->stage - 1));
}

static void continue_clicked(GtkButton *button, gpointer data) {
    (void)button;
    SetupState *state = data;
    if (state->stage == STAGE_WELCOME) set_stage(state, STAGE_WIFI);
    else if (state->stage == STAGE_WIFI) {
        if (wifi_connected()) set_stage(state, STAGE_LANGUAGE);
        else {
            set_status(state, "Conecta Wi-Fi primero. No se descargará ningún paquete sin conectividad.");
            gtk_widget_set_visible(state->network_button, TRUE);
        }
    } else if (state->stage == STAGE_LANGUAGE) {
        if (apply_language_choice(state)) set_stage(state, STAGE_REGION);
    } else if (state->stage == STAGE_REGION) set_stage(state, STAGE_PRIVACY);
    else if (state->stage == STAGE_PRIVACY) {
        if (create_local_user(state)) set_stage(state, STAGE_APPLY);
    }
    else if (state->stage == STAGE_APPLY) {
        mark_setup_complete();
        set_stage(state, STAGE_DONE);
    } else show_finish_screen(state);
}

static void close_clicked(GtkButton *button, gpointer data) {
    (void)button;
    SetupState *state = data;
    gtk_window_close(GTK_WINDOW(state->window));
}

static GtkWidget *make_card_label(const char *text) {
    GtkWidget *label = gtk_label_new(text);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_widget_set_name(label, "oobe-card-label");
    return label;
}

static GtkWidget *make_action_button(const char *label, const char *icon_path, const char *css_name, GtkPositionType icon_position) {
    GtkWidget *button = gtk_button_new_with_label(label);
    GtkWidget *icon = gtk_image_new_from_file(icon_path);
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 18);
    gtk_button_set_image(GTK_BUTTON(button), icon);
    gtk_button_set_always_show_image(GTK_BUTTON(button), TRUE);
    gtk_button_set_image_position(GTK_BUTTON(button), icon_position);
    gtk_widget_set_name(button, css_name);
    return button;
}

static void add_combo_row(GtkWidget *box, const char *label_text, GtkWidget *combo) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(row), make_card_label(label_text), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), combo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 12);
}

static GtkWidget *make_page(SetupState *state, const char *name, const char *body) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_set_name(box, "oobe-page");
    gtk_container_set_border_width(GTK_CONTAINER(box), 26);
    gtk_box_pack_start(GTK_BOX(box), make_card_label(body), FALSE, FALSE, 0);
    gtk_stack_add_named(GTK_STACK(state->stack), box, name);
    return box;
}

static void activate(GtkApplication *app, gpointer data) {
    (void)data;
    SetupState *state = g_new0(SetupState, 1);
    state->app = app;
    state->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(state->window), "Configurar Influent Danenone");
    gtk_window_set_default_size(GTK_WINDOW(state->window), 1280, 800);
    gtk_window_set_position(GTK_WINDOW(state->window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(state->window), FALSE);

    GtkWidget *background = gtk_overlay_new();
    GtkWidget *background_base = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(background_base, TRUE);
    gtk_widget_set_vexpand(background_base, TRUE);
    gtk_container_add(GTK_CONTAINER(background), background_base);
    GtkWidget *wallpaper = NULL;
    const char *oobe_wallpaper_path = g_file_test(OOBE_WALLPAPER_PATH, G_FILE_TEST_EXISTS) ? OOBE_WALLPAPER_PATH : "/home/ubuntu/danenone/native-shell/assets/oobe-river-blurred.jpg";
    if (g_file_test(oobe_wallpaper_path, G_FILE_TEST_EXISTS)) {
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(oobe_wallpaper_path, 1600, 1000, TRUE, NULL);
        if (pixbuf) {
            wallpaper = gtk_image_new_from_pixbuf(pixbuf);
            gtk_widget_set_halign(wallpaper, GTK_ALIGN_CENTER);
            gtk_widget_set_valign(wallpaper, GTK_ALIGN_CENTER);
            gtk_widget_set_hexpand(wallpaper, TRUE);
            gtk_widget_set_vexpand(wallpaper, TRUE);
            gtk_widget_set_opacity(wallpaper, 1.0);
            g_object_unref(pixbuf);
        }
    }
    if (wallpaper) gtk_overlay_add_overlay(GTK_OVERLAY(background), wallpaper);

    GtkWidget *wash = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(wash, "oobe-wash");
    gtk_overlay_add_overlay(GTK_OVERLAY(background), wash);

    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(outer, "oobe-outer");
    gtk_overlay_add_overlay(GTK_OVERLAY(background), outer);
    gtk_widget_set_halign(outer, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(outer, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(outer, FALSE);
    gtk_widget_set_vexpand(outer, FALSE);
    gtk_widget_set_size_request(outer, 1090, 660);
    gtk_widget_set_margin_top(outer, 50);
    gtk_widget_set_margin_bottom(outer, 50);
    gtk_widget_set_margin_start(outer, 70);
    gtk_widget_set_margin_end(outer, 70);
    state->outer = outer;

    state->finish_overlay = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(state->finish_overlay, "oobe-finish");
    gtk_widget_set_hexpand(state->finish_overlay, TRUE);
    gtk_widget_set_vexpand(state->finish_overlay, TRUE);
    gtk_widget_set_halign(state->finish_overlay, GTK_ALIGN_FILL);
    gtk_widget_set_valign(state->finish_overlay, GTK_ALIGN_FILL);
    gtk_widget_set_no_show_all(state->finish_overlay, TRUE);
    GtkWidget *finish_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_name(finish_panel, "oobe-finish-panel");
    gtk_widget_set_size_request(finish_panel, 330, 260);
    gtk_widget_set_halign(finish_panel, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(finish_panel, GTK_ALIGN_CENTER);
    const char *finish_logo_path = g_file_test(BRAND_LOGO_PATH, G_FILE_TEST_EXISTS) ? BRAND_LOGO_PATH : "/home/ubuntu/danenone/native-shell/assets/danenone-cube/danenone-cube-logo.png";
    GtkWidget *finish_logo = gtk_image_new();
    GdkPixbuf *finish_pixbuf = gdk_pixbuf_new_from_file_at_scale(finish_logo_path, 150, 150, TRUE, NULL);
    if (finish_pixbuf) {
        gtk_image_set_from_pixbuf(GTK_IMAGE(finish_logo), finish_pixbuf);
        g_object_unref(finish_pixbuf);
    }
    gtk_widget_set_size_request(finish_logo, 150, 150);
    gtk_image_set_pixel_size(GTK_IMAGE(finish_logo), 150);
    gtk_widget_set_halign(finish_logo, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(finish_panel), finish_logo, FALSE, FALSE, 0);
    state->finish_spinner = gtk_spinner_new();
    gtk_widget_set_halign(state->finish_spinner, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(finish_panel), state->finish_spinner, FALSE, FALSE, 0);
    GtkWidget *finish_title = gtk_label_new("Preparando tu escritorio");
    gtk_widget_set_name(finish_title, "oobe-finish-title");
    gtk_box_pack_start(GTK_BOX(finish_panel), finish_title, FALSE, FALSE, 0);
    GtkWidget *finish_caption = gtk_label_new("Un momento; el cubo está dejando todo listo.");
    gtk_widget_set_name(finish_caption, "oobe-finish-caption");
    gtk_box_pack_start(GTK_BOX(finish_panel), finish_caption, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(state->finish_overlay), finish_panel, TRUE, TRUE, 0);
    gtk_overlay_add_overlay(GTK_OVERLAY(background), state->finish_overlay);

    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_name(header, "oobe-header");
    GtkWidget *brand = gtk_label_new("Influent Danenone");
    gtk_widget_set_name(brand, "oobe-brand");
    gtk_box_pack_start(GTK_BOX(header), brand, FALSE, FALSE, 0);
    GtkWidget *header_right = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 14);
    state->stage_label = gtk_label_new("Paso 1 de 6 · Bienvenida");
    gtk_widget_set_name(state->stage_label, "oobe-stage");
    gtk_box_pack_start(GTK_BOX(header_right), state->stage_label, FALSE, FALSE, 0);
    GtkWidget *close_button = make_action_button("", OOBE_ICON_DIR "/close.svg", "oobe-close", GTK_POS_RIGHT);
    gtk_widget_set_tooltip_text(close_button, "Cerrar");
    gtk_widget_set_size_request(close_button, 34, 34);
    gtk_box_pack_start(GTK_BOX(header_right), close_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(header), header_right, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(outer), header, FALSE, FALSE, 0);
    g_signal_connect(close_button, "clicked", G_CALLBACK(close_clicked), state);

    GtkWidget *body = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(body, "oobe-body");
    gtk_box_pack_start(GTK_BOX(outer), body, TRUE, TRUE, 0);

    GtkWidget *left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);
    gtk_widget_set_name(left, "oobe-left");
    gtk_widget_set_size_request(left, 370, -1);
    gtk_box_pack_start(GTK_BOX(body), left, FALSE, TRUE, 0);
    const char *brand_logo = g_file_test(BRAND_LOGO_PATH, G_FILE_TEST_EXISTS) ? BRAND_LOGO_PATH : "/home/ubuntu/danenone/native-shell/assets/danenone-cube/danenone-cube-logo.png";
    state->cube = gtk_image_new();
    GdkPixbuf *brand_pixbuf = gdk_pixbuf_new_from_file_at_scale(brand_logo, 220, 220, TRUE, NULL);
    if (brand_pixbuf) {
        gtk_image_set_from_pixbuf(GTK_IMAGE(state->cube), brand_pixbuf);
        g_object_unref(brand_pixbuf);
    }
    gtk_widget_set_size_request(state->cube, 220, 220);
    gtk_image_set_pixel_size(GTK_IMAGE(state->cube), 220);
    gtk_widget_set_halign(state->cube, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(left), state->cube, FALSE, FALSE, 22);
    GtkWidget *left_title = gtk_label_new("Tu guía de configuración");
    gtk_widget_set_name(left_title, "oobe-left-title");
    gtk_box_pack_start(GTK_BOX(left), left_title, FALSE, FALSE, 0);
    state->cube_caption = gtk_label_new("Te acompañaré durante la configuración");
    gtk_widget_set_name(state->cube_caption, "oobe-cube-caption");
    gtk_label_set_line_wrap(GTK_LABEL(state->cube_caption), TRUE);
    gtk_label_set_justify(GTK_LABEL(state->cube_caption), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(left), state->cube_caption, FALSE, FALSE, 0);
    GtkWidget *left_note = gtk_label_new("Una experiencia clara, privada y hecha para tu espacio.");
    gtk_widget_set_name(left_note, "oobe-left-note");
    gtk_label_set_line_wrap(GTK_LABEL(left_note), TRUE);
    gtk_label_set_justify(GTK_LABEL(left_note), GTK_JUSTIFY_CENTER);
    gtk_box_pack_end(GTK_BOX(left), left_note, FALSE, FALSE, 25);

    GtkWidget *right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(right, "oobe-right");
    gtk_box_pack_start(GTK_BOX(body), right, TRUE, TRUE, 0);
    state->title = gtk_label_new(NULL);
    gtk_widget_set_name(state->title, "oobe-title");
    gtk_label_set_xalign(GTK_LABEL(state->title), 0.0);
    gtk_box_pack_start(GTK_BOX(right), state->title, FALSE, FALSE, 26);
    state->subtitle = gtk_label_new(NULL);
    gtk_widget_set_name(state->subtitle, "oobe-subtitle");
    gtk_label_set_xalign(GTK_LABEL(state->subtitle), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(state->subtitle), TRUE);
    gtk_box_pack_start(GTK_BOX(right), state->subtitle, FALSE, FALSE, 0);

    state->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(state->stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(state->stack), 260);
    gtk_box_pack_start(GTK_BOX(right), state->stack, TRUE, TRUE, 0);

    GtkWidget *welcome = make_page(state, "Bienvenida", "Puedes revisar cada decisión antes de aplicarla. Danenone no descargará idiomas ni modificará preferencias sin pasar por tus controles.");
    gtk_box_pack_end(GTK_BOX(welcome), make_card_label("El cubo te acompañará en cada paso."), FALSE, FALSE, 0);
    GtkWidget *wifi = make_page(state, "Conectividad", "La conexión se comprueba con NetworkManager. Primero conecta Wi-Fi y después podrás descargar paquetes de idioma verificados.");
    state->network_button = make_action_button("Abrir ajustes de red", OOBE_ICON_DIR "/wifi.svg", "oobe-secondary", GTK_POS_LEFT);
    gtk_box_pack_end(GTK_BOX(wifi), state->network_button, FALSE, FALSE, 0);
    GtkWidget *lang = make_page(state, "Idioma", "English viene integrado por respeto a quienes necesitan arrancar sin conexión. Las otras opciones muestran su disponibilidad real.");
    state->language = gtk_combo_box_text_new();
    populate_languages(state);
    add_combo_row(lang, "Idioma de la interfaz", state->language);
    GtkWidget *region = make_page(state, "Región y teclado", "Selecciona los controles que se usarán en tu sesión. Estos valores se aplicarán al finalizar.");
    state->keyboard = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->keyboard), "us", "English — US");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->keyboard), "es", "Español");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->keyboard), "latam", "Español — Latinoamérica");
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(state->keyboard), "us");
    add_combo_row(region, "Distribución del teclado", state->keyboard);
    state->timezone = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->timezone), "local", "Usar zona horaria del sistema");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->timezone), "utc", "UTC");
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(state->timezone), "local");
    add_combo_row(region, "Zona horaria", state->timezone);
    GtkWidget *privacy = make_page(state, "Privacidad y usuario", "Elige un nombre para tu espacio y decide si quieres activar diagnósticos opcionales. No se activa ninguna cuenta remota.");
    state->username = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(state->username), "danenone");
    gtk_entry_set_placeholder_text(GTK_ENTRY(state->username), "Nombre del usuario");
    add_combo_row(privacy, "Nombre local", state->username);
    state->password = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(state->password), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(state->password), "Contraseña local");
    add_combo_row(privacy, "Contraseña", state->password);
    state->telemetry = gtk_check_button_new_with_label("Permitir diagnósticos opcionales y anónimos");
    gtk_box_pack_start(GTK_BOX(privacy), state->telemetry, FALSE, FALSE, 8);
    GtkWidget *apply = make_page(state, "Aplicando cambios", "Estamos preparando el escritorio, la barra, el notch y el tour del cubito.");
    state->progress = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(state->progress), TRUE);
    gtk_box_pack_end(GTK_BOX(apply), state->progress, FALSE, FALSE, 15);
    GtkWidget *done = make_page(state, "Listo", "Tu espacio está preparado. Al entrar comenzará el tour guiado de Danenone.");
    gtk_box_pack_end(GTK_BOX(done), make_card_label("Puedes volver a ejecutar este asistente con --replay."), FALSE, FALSE, 0);

    state->status = gtk_label_new(NULL);
    gtk_widget_set_name(state->status, "oobe-status");
    gtk_label_set_xalign(GTK_LABEL(state->status), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(state->status), TRUE);
    gtk_box_pack_start(GTK_BOX(right), state->status, FALSE, FALSE, 12);

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_name(actions, "oobe-actions");
    state->back_button = make_action_button("Atrás", OOBE_ICON_DIR "/arrow-left.svg", "oobe-secondary", GTK_POS_LEFT);
    state->continue_button = make_action_button("Comenzar", OOBE_ICON_DIR "/arrow-right.svg", "oobe-primary", GTK_POS_RIGHT);
    gtk_box_pack_start(GTK_BOX(actions), state->back_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(actions), state->continue_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(right), actions, FALSE, FALSE, 22);

    g_signal_connect(state->continue_button, "clicked", G_CALLBACK(continue_clicked), state);
    g_signal_connect(state->back_button, "clicked", G_CALLBACK(back_clicked), state);
    g_signal_connect(state->network_button, "clicked", G_CALLBACK(open_network_editor), state);
    g_signal_connect(state->window, "destroy", G_CALLBACK(cleanup_state), state);

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "window { background: #071421; color: #f7f9ff; }"
        "#oobe-wash { background: rgba(5,12,28,0.42); }"
        "#oobe-outer { background: rgba(8,17,36,0.70); border: 1px solid rgba(255,255,255,0.22); border-radius: 30px; padding: 0; }"
        "#oobe-header { background: rgba(8,17,36,0.38); padding: 18px 28px; border-bottom: 1px solid rgba(255,255,255,0.16); }"
        "#oobe-brand { font-size: 17px; font-weight: 700; color: #f6f8ff; }"
        "#oobe-stage { color: rgba(235,242,255,0.68); font-size: 13px; }"
        "#oobe-left { background: rgba(22,34,66,0.50); border-radius: 0 0 0 30px; padding: 20px; }"
        "#oobe-right { background: rgba(7,15,31,0.26); padding: 0 26px; }"
        "#oobe-title { font-size: 29px; font-weight: 700; color: #ffffff; }"
        "#oobe-subtitle, #oobe-status { font-size: 15px; color: rgba(235,242,255,0.72); }"
        "#oobe-left-title { font-size: 18px; font-weight: 700; color: #f5f8ff; }"
        "#oobe-cube-caption, #oobe-left-note { color: rgba(231,240,255,0.68); font-size: 14px; }"
        "#oobe-page { background: rgba(3,10,24,0.38); border: 1px solid rgba(255,255,255,0.13); border-radius: 20px; margin-top: 20px; }"
        "#oobe-card-label { color: rgba(235,242,255,0.76); font-size: 15px; }"
        "entry, combobox { background: rgba(3,9,20,0.54); color: #f7f9ff; border: 1px solid rgba(207,224,255,0.24); border-radius: 15px; padding: 11px 15px; }"
        "entry:focus, combobox:focus { border: 2px solid rgba(153,108,235,0.90); }"
        "button { border-radius: 19px; padding: 10px 17px; min-height: 22px; font-weight: 600; }"
        "button:hover { background: rgba(255,255,255,0.16); }"
        "button:active { background: rgba(151,111,232,0.72); }"
        "#oobe-primary { background: rgba(106,42,174,0.94); color: white; border: 1px solid rgba(232,216,255,0.46); font-weight: 700; }"
        "#oobe-primary:hover { background: rgba(132,64,211,0.98); }"
        "#oobe-secondary { background: rgba(255,255,255,0.08); color: #f3f6ff; border: 1px solid rgba(232,240,255,0.22); }"
        "#oobe-close { background: rgba(255,255,255,0.08); color: #f3f6ff; border: 1px solid rgba(232,240,255,0.18); padding: 4px; }"
        "#oobe-close:hover { background: rgba(255,255,255,0.20); }"
        "#oobe-finish { background: rgba(3,9,21,0.72); }"
        "#oobe-finish-panel { background: rgba(20,25,58,0.78); border: 1px solid rgba(255,255,255,0.24); border-radius: 30px; padding: 30px; }"
        "#oobe-finish-title { color: #f8fbff; font-size: 24px; font-weight: 700; }"
        "#oobe-finish-caption { color: rgba(232,242,255,0.76); font-size: 14px; }"
        "spinner { color: #9cc9f3; }"
        "checkbutton { color: rgba(235,242,255,0.76); padding: 8px 2px; }"
        "progressbar trough { background: rgba(255,255,255,0.16); border-radius: 9px; min-height: 12px; }"
        "progressbar progress { background: #9d68e6; border-radius: 9px; min-height: 12px; }",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    gtk_container_add(GTK_CONTAINER(state->window), background);
    state->intro_opacity = 0.0;
    gtk_widget_set_opacity(state->outer, state->intro_opacity);
    state->cube_timer = g_timeout_add(900, animate_cube, state);
    gtk_widget_show_all(state->window);
    gtk_widget_hide(state->finish_overlay);
    state->intro_timer = g_timeout_add(30, intro_step, state);
    set_stage(state, STAGE_WELCOME);
}

int main(int argc, char **argv) {
    if (argc > 1 && g_strcmp0(argv[1], "--check-wifi") == 0) {
        g_print("wifi=%s\n", wifi_connected() ? "connected" : "disconnected");
        return 0;
    }
    gboolean replay = FALSE;
    for (int i = 1; i < argc; i++) if (g_strcmp0(argv[i], "--replay") == 0) replay = TRUE;
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
