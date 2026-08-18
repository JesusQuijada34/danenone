#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>
#include <glib/gstdio.h>
#include <string.h>

#define BRAND_LOGO_PATH "/usr/share/influent/danenone-cube-logo.png"
#define OOBE_WALLPAPER_PATH "/usr/share/backgrounds/influent/oobe-river-blurred.jpg"
#define LOCAL_LOGO_PATH "/home/ubuntu/danenone/native-shell/assets/danenone-cube/danenone-cube-logo.png"
#define LOCAL_WALLPAPER_PATH "/home/ubuntu/danenone/native-shell/assets/oobe-river-blurred.jpg"
#define PAGE_COUNT 12

static const char *LANGUAGE_LABELS[] = {
    "English — United States",
    "Español — Latinoamérica",
    "Español — España",
    "Português — Brasil",
    NULL
};
static const char *LANGUAGE_CODES[] = {"en_US", "es_419", "es_ES", "pt_BR"};
static const char *EDITION_LABELS[] = {
    "Home — uso personal",
    "Enterprise — administración y desarrollo",
    "Developer — toolchain completo",
    "Minimal — instalación reducida",
    "Frozen Lab — entorno sin actualizaciones",
    NULL
};
static const char *EDITION_CODES[] = {"home", "enterprise", "developer", "minimal", "frozen-lab"};

/* El flujo mantiene doce pasos agrupando idioma y conectividad en una sola pantalla. */
typedef struct {
    GtkApplication *app;
    GtkWidget *window, *overlay, *root, *content, *modal, *stack;
    GtkWidget *splash, *splash_progress, *final_splash, *final_progress;
    GtkWidget *identity, *cube, *brand, *stage, *status;
    GtkWidget *repair, *back, *next, *notch;
    GtkWidget *dark_toggle, *full_notch_toggle, *dynamic_notch_toggle;
    GtkWidget *language_dropdown, *edition_dropdown, *network_status;
    GtkWidget *license_accept, *username, *password, *summary;
    guint splash_timer, marquee_timer, intro_timer, final_timer;
    double intro_opacity, brand_reveal;
    int page;
} Oobe;

static const char *asset(const char *installed, const char *local) {
    return g_file_test(installed, G_FILE_TEST_EXISTS) ? installed : local;
}

static guint dropdown_index(GtkWidget *dropdown) {
    return dropdown ? gtk_drop_down_get_selected(GTK_DROP_DOWN(dropdown)) : 0;
}

static GtkWidget *make_dropdown(const char *const *items) {
    GtkStringList *model = gtk_string_list_new(items);
    GtkWidget *dropdown = gtk_drop_down_new(G_LIST_MODEL(model), NULL);
    gtk_widget_add_css_class(dropdown, "dropdown");
    g_object_unref(model);
    return dropdown;
}

static gboolean network_available(void) {
    GNetworkMonitor *monitor = g_network_monitor_get_default();
    return monitor && g_network_monitor_get_network_available(monitor);
}

static void write_atomic(const char *path, const char *contents) {
    gchar *tmp = g_strdup_printf("%s.tmp", path);
    if (g_file_set_contents(tmp, contents, -1, NULL) == TRUE) {
        g_chmod(tmp, 0600);
        g_rename(tmp, path);
    } else {
        g_remove(tmp);
    }
    g_free(tmp);
}

static void save_preferences(Oobe *o) {
    gchar *directory = g_build_filename(g_get_user_state_dir(), "influent-danenone", NULL);
    gchar *path = g_build_filename(directory, "visual-preferences.conf", NULL);
    gchar *selection_path = g_build_filename(directory, "oobe-selection.conf", NULL);
    const char *theme = (o && o->dark_toggle && gtk_check_button_get_active(GTK_CHECK_BUTTON(o->dark_toggle))) ? "dark" : "light";
    const char *notch = (o && o->dynamic_notch_toggle && gtk_check_button_get_active(GTK_CHECK_BUTTON(o->dynamic_notch_toggle))) ? "dynamic" : "full";
    const int notch_width = g_strcmp0(notch, "dynamic") == 0 ? 220 : 360;
    const int notch_height = 28;
    const int notch_radius = g_strcmp0(notch, "dynamic") == 0 ? 18 : 14;
    const char *language = (o && o->language_dropdown) ? LANGUAGE_CODES[dropdown_index(o->language_dropdown)] : "en_US";
    const char *edition = (o && o->edition_dropdown) ? EDITION_CODES[dropdown_index(o->edition_dropdown)] : "home";
    gchar *contents = g_strdup_printf(
        "theme=%s\nnotch=%s\nnotch_width=%d\nnotch_height=%d\nnotch_radius=%d\nnotch_border=none\nlanguage=%s\nedition=%s\nnetwork_available=%s\n",
        theme, notch, notch_width, notch_height, notch_radius, language, edition,
        network_available() ? "true" : "false");
    if (g_mkdir_with_parents(directory, 0700) == 0) {
        write_atomic(path, contents);
        gchar *selection = g_strdup_printf("LANGUAGE=%s\nEDITION=%s\nNETWORK_AVAILABLE=%s\n", language, edition, network_available() ? "true" : "false");
        write_atomic(selection_path, selection);
        g_free(selection);
    }
    g_free(contents); g_free(selection_path); g_free(path); g_free(directory);
}

static void refresh_oem_id(void) {
    GError *error = NULL;
    gint status = 1;
    if (g_file_test("/usr/local/bin/influent-oem-id", G_FILE_TEST_IS_EXECUTABLE)) g_spawn_command_line_sync("/usr/local/bin/influent-oem-id --refresh", NULL, NULL, &status, &error);
    if (error) g_error_free(error);
}

static void apply_install_selection(Oobe *o) {
    GError *error = NULL;
    gint status = 1;
    if (g_file_test("/usr/local/bin/influent-oobe-apply-selection", G_FILE_TEST_IS_EXECUTABLE)) g_spawn_command_line_sync("/usr/local/bin/influent-oobe-apply-selection", NULL, NULL, &status, &error);
    if (error) g_error_free(error);
    if (status != 0) gtk_label_set_text(GTK_LABEL(o->status), "La selección quedó guardada para el instalador; no se pudo escribir el sistema mientras se ejecuta en modo live.");
}

static gboolean detect_other_os(void) {
    const char *roots[] = {"/mnt", "/media", "/run/media", NULL};
    const char *markers[] = {"EFI/Microsoft/Boot/bootmgfw.efi", "EFI/ubuntu/grubx64.efi", "EFI/debian/grubx64.efi", NULL};
    for (int r = 0; roots[r]; r++) {
        GDir *dir = g_dir_open(roots[r], 0, NULL);
        if (!dir) continue;
        const char *entry;
        while ((entry = g_dir_read_name(dir))) {
            char *base = g_build_filename(roots[r], entry, NULL);
            for (int m = 0; markers[m]; m++) {
                char *path = g_build_filename(base, markers[m], NULL);
                gboolean found = g_file_test(path, G_FILE_TEST_EXISTS);
                g_free(path);
                if (found) { g_free(base); g_dir_close(dir); return TRUE; }
            }
            g_free(base);
        }
        g_dir_close(dir);
    }
    return FALSE;
}

static gboolean repair_available(void) {
    const char *roots[] = {"/mnt", "/media", "/run/media", NULL};
    const char *markers[] = {"etc/influent-danenone-release", "etc/influent-danenone/installed", ".influent-danenone-installed", NULL};
    for (int r = 0; roots[r]; r++) {
        GDir *dir = g_dir_open(roots[r], 0, NULL);
        if (!dir) continue;
        const char *entry;
        while ((entry = g_dir_read_name(dir))) {
            char *base = g_build_filename(roots[r], entry, NULL);
            for (int m = 0; markers[m]; m++) {
                char *path = g_build_filename(base, markers[m], NULL);
                gboolean found = g_file_test(path, G_FILE_TEST_EXISTS);
                g_free(path);
                if (found) { g_free(base); g_dir_close(dir); return TRUE; }
            }
            g_free(base);
        }
        g_dir_close(dir);
    }
    return FALSE;
}

static GtkWidget *scaled_picture(const char *path, int size) {
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(path, size, size, TRUE, &error);
    if (!pixbuf) { if (error) g_error_free(error); return gtk_picture_new(); }
    GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf);
    GtkWidget *picture = gtk_picture_new_for_paintable(GDK_PAINTABLE(texture));
    gtk_picture_set_content_fit(GTK_PICTURE(picture), GTK_CONTENT_FIT_CONTAIN);
    gtk_widget_set_size_request(picture, size, size);
    g_object_unref(texture); g_object_unref(pixbuf);
    return picture;
}

static GtkWidget *text_label(const char *text, const char *klass) {
    GtkWidget *w = gtk_label_new(text);
    if (klass) gtk_widget_add_css_class(w, klass);
    gtk_label_set_wrap(GTK_LABEL(w), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(w), 66);
    gtk_label_set_xalign(GTK_LABEL(w), 0.0f);
    return w;
}

static GtkWidget *action_button(const char *text, const char *klass) {
    GtkWidget *w = gtk_button_new_with_label(text);
    gtk_widget_add_css_class(w, klass);
    return w;
}

static GtkWidget *base_page(const char *title, const char *body) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(box, "oobe-page");
    gtk_widget_set_margin_start(box, 36); gtk_widget_set_margin_end(box, 36);
    gtk_widget_set_margin_top(box, 14); gtk_widget_set_margin_bottom(box, 14);
    gtk_box_append(GTK_BOX(box), text_label(title, "page-title"));
    gtk_box_append(GTK_BOX(box), text_label(body, "body-text"));
    return box;
}

static GtkWidget *welcome_page(void) {
    return base_page("Configura tu espacio Danenone", "Puedes revisar cada decisión antes de aplicarla. El asistente te acompañará por idioma, conectividad, edición, disco, usuario, privacidad y apariencia.");
}

static GtkWidget *language_network_page(Oobe *o) {
    GtkWidget *box = base_page("Idioma y conectividad", "El idioma se elige al principio. English permanece disponible sin conexión; las opciones adicionales se comprobarán antes de descargar sus paquetes.");
    o->language_dropdown = make_dropdown(LANGUAGE_LABELS);
    gtk_box_append(GTK_BOX(box), text_label("Idioma de la interfaz", "field-label"));
    gtk_box_append(GTK_BOX(box), o->language_dropdown);
    o->network_status = text_label("Comprobando NetworkManager…", "network-status");
    gtk_box_append(GTK_BOX(box), o->network_status);
    gtk_box_append(GTK_BOX(box), text_label("La edición English puede continuar sin red. Para idiomas descargables, conecta el equipo antes de avanzar.", "hint-text"));
    return box;
}

static GtkWidget *edition_page(Oobe *o) {
    GtkWidget *box = base_page("Elige la edición de Danenone", "Esta elección define el conjunto de paquetes y la política de actualizaciones de la instalación. Puedes cambiarla antes de aplicar los cambios.");
    o->edition_dropdown = make_dropdown(EDITION_LABELS);
    gtk_box_append(GTK_BOX(box), text_label("Edición de destino", "field-label"));
    gtk_box_append(GTK_BOX(box), o->edition_dropdown);
    GtkWidget *details = text_label("Home: personal y automático. Enterprise: administración gestionada. Developer: toolchain completo. Minimal: instalación reducida. Frozen Lab: actualizaciones desactivadas y operación offline.", "edition-details");
    gtk_box_append(GTK_BOX(box), details);
    return box;
}

static GtkWidget *license_page(Oobe *o) {
    GtkWidget *box = base_page("Términos y licencia", "Lee las condiciones de uso de Influent Danenone y de los componentes de código abierto incluidos en la imagen.");
    o->license_accept = gtk_check_button_new_with_label("Acepto los términos de licencia y la política de privacidad.");
    gtk_widget_add_css_class(o->license_accept, "setting-row");
    gtk_box_append(GTK_BOX(box), o->license_accept); return box;
}

static GtkWidget *disk_page(void) {
    gboolean other = detect_other_os();
    const char *intro = other ? "Se detectó otro sistema operativo en un volumen montado. Elige entre conservarlo en dual boot o usar el disco completo." : "No se detectó otro sistema operativo en los volúmenes montados. La instalación completa quedará disponible tras confirmar el disco.";
    GtkWidget *box = base_page("Disco y modo de instalación", intro);
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8); gtk_widget_add_css_class(card, "choice-card");
    GtkWidget *disk = gtk_check_button_new_with_label("Dispositivo detectado por el instalador · selección pendiente de confirmar");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(disk), TRUE);
    gtk_box_append(GTK_BOX(card), disk);
    GtkWidget *full = gtk_check_button_new_with_label("Instalación completa: borrar el disco seleccionado");
    GtkWidget *dual = gtk_check_button_new_with_label("Dual boot: conservar el sistema detectado y añadir Danenone");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(dual), GTK_CHECK_BUTTON(full));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(full), !other);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(dual), other);
    gtk_box_append(GTK_BOX(card), full); gtk_box_append(GTK_BOX(card), dual);
    gtk_box_append(GTK_BOX(box), card); return box;
}

static GtkWidget *user_page(Oobe *o) {
    GtkWidget *box = base_page("Crea tu usuario", "Este usuario será local y se utilizará para iniciar sesión en Danenone. Puedes cambiar estos datos más adelante desde Configuración.");
    o->username = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(o->username), "Nombre de usuario"); gtk_widget_add_css_class(o->username, "input");
    o->password = gtk_password_entry_new(); gtk_widget_set_tooltip_text(o->password, "Contraseña local"); gtk_widget_add_css_class(o->password, "input");
    gtk_box_append(GTK_BOX(box), o->username); gtk_box_append(GTK_BOX(box), o->password); return box;
}

static GtkWidget *tools_page(void) {
    GtkWidget *box = base_page("Herramientas adicionales", "Selecciona componentes opcionales que se instalarán después de preparar el sistema base.");
    const char *tools[] = {"Navegador web y códecs multimedia", "Herramientas de desarrollo C/C++", "Compatibilidad con paquetes Fluthin", "Utilidades de virtualización QEMU"};
    for (guint i = 0; i < G_N_ELEMENTS(tools); i++) gtk_box_append(GTK_BOX(box), gtk_check_button_new_with_label(tools[i]));
    return box;
}

static GtkWidget *appearance_page(Oobe *o) {
    GtkWidget *box = base_page("Estilo de interfaz", "Prueba la apariencia antes de continuar. El cambio se aplica automáticamente a esta interfaz GTK4.");
    o->dark_toggle = gtk_check_button_new_with_label("Usar modo oscuro");
    GtkWidget *light = gtk_check_button_new_with_label("Modo claro");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(light), GTK_CHECK_BUTTON(o->dark_toggle));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(light), TRUE);
    gtk_box_append(GTK_BOX(box), light); gtk_box_append(GTK_BOX(box), o->dark_toggle);
    GtkWidget *colors = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8); gtk_widget_add_css_class(colors, "color-row");
    const char *color_names[] = {"Verde Danenone", "Azul", "Ámbar"};
    for (guint i = 0; i < G_N_ELEMENTS(color_names); i++) gtk_box_append(GTK_BOX(colors), action_button(color_names[i], i == 0 ? "color-green" : "color-option"));
    gtk_box_append(GTK_BOX(box), colors); return box;
}

static GtkWidget *notch_page(Oobe *o) {
    GtkWidget *box = base_page("Configura el notch", "El notch completo está visible durante todo el OOBE. Puedes probar una isla dinámica o un recorte completo; el contenido se reajusta automáticamente.");
    o->full_notch_toggle = gtk_check_button_new_with_label("Recorte completo");
    o->dynamic_notch_toggle = gtk_check_button_new_with_label("Isla dinámica");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(o->dynamic_notch_toggle), GTK_CHECK_BUTTON(o->full_notch_toggle));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(o->full_notch_toggle), TRUE);
    gtk_box_append(GTK_BOX(box), o->full_notch_toggle); gtk_box_append(GTK_BOX(box), o->dynamic_notch_toggle); return box;
}

static GtkWidget *privacy_page(void) {
    GtkWidget *box = base_page("Privacidad y anuncios", "Elige qué comunicaciones opcionales pueden mostrar las aplicaciones. Esta decisión no activa cuentas remotas.");
    gtk_box_append(GTK_BOX(box), gtk_check_button_new_with_label("Permitir diagnósticos anónimos opcionales"));
    gtk_box_append(GTK_BOX(box), gtk_check_button_new_with_label("Permitir anuncios o publicidad dentro de aplicaciones"));
    return box;
}

static GtkWidget *summary_page(Oobe *o) {
    GtkWidget *box = base_page("Listo para instalar", "Revisa el resumen antes de continuar. La instalación no se inicia hasta que pulses el botón final.");
    o->summary = text_label("", "summary-card");
    gtk_box_append(GTK_BOX(box), o->summary); return box;
}

static GtkWidget *install_page(void) {
    GtkWidget *box = base_page("Instalando Influent Danenone", "Mantén el equipo conectado. El progreso real será informado por el instalador.");
    GtkWidget *progress = gtk_progress_bar_new(); gtk_widget_add_css_class(progress, "install-progress");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.0); gtk_widget_set_size_request(progress, 0, 8);
    gtk_box_append(GTK_BOX(box), progress); return box;
}

static void update_network_status(Oobe *o) {
    if (!o->network_status) return;
    gtk_label_set_text(GTK_LABEL(o->network_status), network_available() ? "Red disponible según NetworkManager." : "Red no disponible. English puede continuar sin conexión; los idiomas descargables requieren red.");
}

static void update_summary(Oobe *o) {
    if (!o->summary) return;
    const char *language = LANGUAGE_CODES[dropdown_index(o->language_dropdown)];
    const char *edition = EDITION_CODES[dropdown_index(o->edition_dropdown)];
    gchar *text = g_strdup_printf("Sistema: Influent Danenone\nEdición: %s\nIdioma: %s\nDisco: se confirmará por el instalador\nModo: instalación completa o dual boot según tu elección\nIdentidad OEM: se generará localmente y no muestra el número de serie", edition, language);
    gtk_label_set_text(GTK_LABEL(o->summary), text);
    g_free(text);
}

static void set_page(Oobe *o, int page_no) {
    o->page = CLAMP(page_no, 0, PAGE_COUNT - 1);
    const char *names[] = {"welcome", "language-network", "edition", "license", "disk", "user", "tools", "appearance", "notch", "privacy", "summary", "install"};
    const char *stages[] = {"Paso 1 de 12 · Bienvenida", "Paso 2 de 12 · Idioma y conectividad", "Paso 3 de 12 · Edición", "Paso 4 de 12 · Licencia", "Paso 5 de 12 · Disco", "Paso 6 de 12 · Usuario", "Paso 7 de 12 · Herramientas", "Paso 8 de 12 · Apariencia", "Paso 9 de 12 · Notch", "Paso 10 de 12 · Privacidad", "Paso 11 de 12 · Resumen", "Paso 12 de 12 · Instalación"};
    gtk_stack_set_visible_child_name(GTK_STACK(o->stack), names[o->page]);
    gtk_label_set_text(GTK_LABEL(o->stage), stages[o->page]);
    gtk_widget_set_visible(o->back, o->page > 0 && o->page < PAGE_COUNT - 1);
    gtk_widget_set_visible(o->repair, o->page == 0 && repair_available());
    gtk_button_set_label(GTK_BUTTON(o->next), o->page == PAGE_COUNT - 1 ? "Finalizar" : "Continuar");
    if (o->page == 1) update_network_status(o);
    if (o->page == 10) update_summary(o);
    const char *status[] = {"Puedes revisar cada decisión antes de aplicarla. Todo queda bajo tu control.", "La red y el idioma se comprueban antes de descargar paquetes.", "La edición define paquetes y política de actualizaciones.", "Debes aceptar los términos antes de continuar.", "La instalación real confirmará el disco y el modo elegido.", "El usuario local se crea sin una cuenta remota.", "Las herramientas opcionales se instalarán después del sistema base.", "El modo claro/oscuro se aplica automáticamente como prueba.", "El contenido se reajusta al editar el estilo del notch.", "Tus preferencias permanecen locales por defecto.", "Revisa el destino, la edición, el usuario, la privacidad y el aspecto.", "Aplicando el entorno, la barra, el notch y el tour."};
    gtk_label_set_text(GTK_LABEL(o->status), status[o->page]);
}

static gboolean validate_page(Oobe *o) {
    if (o->page == 1 && dropdown_index(o->language_dropdown) != 0 && !network_available()) {
        gtk_label_set_text(GTK_LABEL(o->status), "Conecta la red para descargar el paquete del idioma seleccionado, o elige English para continuar sin conexión.");
        return FALSE;
    }
    if (o->page == 3 && !gtk_check_button_get_active(GTK_CHECK_BUTTON(o->license_accept))) {
        gtk_label_set_text(GTK_LABEL(o->status), "Debes aceptar los términos de licencia y la política de privacidad.");
        return FALSE;
    }
    if (o->page == 5 && (!o->username || strlen(gtk_editable_get_text(GTK_EDITABLE(o->username))) < 1)) {
        gtk_label_set_text(GTK_LABEL(o->status), "Escribe un nombre de usuario local antes de continuar.");
        return FALSE;
    }
    return TRUE;
}

static gboolean final_step(gpointer data) {
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(data));
    return G_SOURCE_CONTINUE;
}

static void next_clicked(GtkButton *button, gpointer data) {
    (void)button; Oobe *o = data;
    if (o->page < PAGE_COUNT - 1) {
        if (!validate_page(o)) return;
        if (o->page == PAGE_COUNT - 2) save_preferences(o);
        set_page(o, o->page + 1);
    } else {
        save_preferences(o); apply_install_selection(o); refresh_oem_id();
        gtk_widget_set_visible(o->content, FALSE); gtk_widget_set_visible(o->notch, FALSE); gtk_widget_set_visible(o->final_splash, TRUE);
        o->final_timer = g_timeout_add(40, final_step, o->final_progress);
    }
}

static void back_clicked(GtkButton *button, gpointer data) { (void)button; Oobe *o = data; if (o->page > 0) set_page(o, o->page - 1); }
static void repair_clicked(GtkButton *button, gpointer data) { (void)button; Oobe *o = data; gtk_label_set_text(GTK_LABEL(o->status), "Se detectó una instalación Danenone; la reparación guiada está disponible."); }

static void dark_toggled(GtkCheckButton *button, gpointer data) {
    Oobe *o = data; if (gtk_check_button_get_active(button)) gtk_widget_add_css_class(o->root, "dark"); else gtk_widget_remove_css_class(o->root, "dark"); save_preferences(o);
}

static void full_notch_toggled(GtkCheckButton *button, gpointer data) {
    Oobe *o = data; if (!gtk_check_button_get_active(button)) return;
    gtk_widget_set_size_request(o->notch, 360, 28); gtk_widget_remove_css_class(o->notch, "dynamic-notch"); gtk_widget_set_margin_top(o->content, 30); save_preferences(o);
}

static void dynamic_notch_toggled(GtkCheckButton *button, gpointer data) {
    Oobe *o = data; if (!gtk_check_button_get_active(button)) return;
    gtk_widget_set_size_request(o->notch, 220, 28); gtk_widget_add_css_class(o->notch, "dynamic-notch"); gtk_widget_set_margin_top(o->content, 18); save_preferences(o);
}

static gboolean marquee_step(gpointer data) { Oobe *o = data; if (!o->splash_progress) return G_SOURCE_REMOVE; gtk_progress_bar_pulse(GTK_PROGRESS_BAR(o->splash_progress)); return G_SOURCE_CONTINUE; }
static gboolean intro_step(gpointer data) {
    Oobe *o = data;
    if (o->intro_opacity < 1.0) { o->intro_opacity = MIN(1.0, o->intro_opacity + 0.08); gtk_widget_set_opacity(o->content, o->intro_opacity); return G_SOURCE_CONTINUE; }
    o->brand_reveal = MIN(1.0, o->brand_reveal + 0.08); gtk_widget_set_opacity(o->brand, o->brand_reveal); gtk_widget_set_size_request(o->brand, (int)(o->brand_reveal * 180.0), -1);
    if (o->brand_reveal >= 1.0) { o->intro_timer = 0; return G_SOURCE_REMOVE; } return G_SOURCE_CONTINUE;
}
static gboolean finish_splash(gpointer data) { Oobe *o = data; o->splash_timer = 0; if (o->marquee_timer) { g_source_remove(o->marquee_timer); o->marquee_timer = 0; } gtk_widget_set_visible(o->splash, FALSE); o->intro_timer = g_timeout_add(30, intro_step, o); return G_SOURCE_REMOVE; }

static void load_css(void) {
    const char *css =
        "window { background: #dfe9e6; color: #14231f; }"
        ".oobe-wash { background: rgba(255,255,255,0.18); } .oobe-splash, .final-splash { background: transparent; }"
        ".splash-panel { background: transparent; border: 0; padding: 0; } .splash-progress trough { min-width: 220px; min-height: 3px; border: 0; border-radius: 0; background: rgba(255,255,255,0.38); } .splash-progress progress { min-height: 3px; border: 0; border-radius: 0; background: #00b982; }"
        ".notch { background: #111a19; border: 0; border-radius: 0 0 14px 14px; min-height: 28px; } .dynamic-notch { border-radius: 0 0 18px 18px; }"
        ".oobe-modal { background: rgba(255,255,255,0.80); border: 1px solid rgba(255,255,255,0.92); border-radius: 4px; padding: 26px; box-shadow: 0 18px 50px rgba(13,45,42,0.18); }"
        ".identity { min-height: 64px; } .brand { color: #17342d; font-family: Roboto, 'Noto Sans', sans-serif; font-size: 26px; font-weight: 500; } .stage { color: rgba(23,52,45,0.62); font-size: 13px; }"
        ".page { background: rgba(255,255,255,0.22); border: 1px solid rgba(255,255,255,0.58); padding: 16px; } .page-title { color: #17342d; font-size: 18px; font-weight: 700; } .body-text { color: rgba(20,45,39,0.78); font-size: 15px; } .status, .network-status, .hint-text, .edition-details, .field-label { color: rgba(20,45,39,0.68); font-size: 14px; }"
        ".dropdown, .input { padding: 10px; background: rgba(255,255,255,0.40); border: 1px solid rgba(255,255,255,0.70); } .summary-card, .choice-card { background: rgba(255,255,255,0.34); border: 1px solid rgba(255,255,255,0.64); padding: 16px; }"
        "button { padding: 10px 17px; border-radius: 3px; border: 1px solid rgba(255,255,255,0.82); box-shadow: 0 4px 14px rgba(31,75,66,0.12), inset 0 1px 0 rgba(255,255,255,0.88); } .primary { background: rgba(0,184,132,0.84); color: #07352a; border-bottom: 2px solid #008a5a; font-weight: 700; } .secondary { background: rgba(255,255,255,0.34); color: #17342d; border-bottom: 2px solid rgba(0,112,86,0.42); }"
        ".install-progress trough { min-height: 6px; border-radius: 0; background: rgba(20,65,55,0.16); } .install-progress progress { min-height: 6px; border-radius: 0; background: #00b982; } .final-title { color: #17342d; font-size: 20px; font-weight: 700; }"
        ".dark .oobe-modal { background: rgba(20,29,28,0.88); border-color: rgba(255,255,255,0.22); } .dark .page, .dark .summary-card { background: rgba(13,23,22,0.42); border-color: rgba(255,255,255,0.18); } .dark .page-title, .dark .body-text, .dark .status, .dark .network-status, .dark .hint-text, .dark .edition-details, .dark .field-label, .dark .brand, .dark .summary-card { color: #ecf8f3; }";
    GtkCssProvider *provider = gtk_css_provider_new(); gtk_css_provider_load_from_string(provider, css); gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION); g_object_unref(provider);
}

static void activate(GtkApplication *app, gpointer data) {
    (void)data; Oobe *o = g_new0(Oobe, 1); o->app = app; refresh_oem_id();
    o->window = gtk_application_window_new(app); gtk_window_set_title(GTK_WINDOW(o->window), "Influent Danenone"); gtk_window_set_default_size(GTK_WINDOW(o->window), 1280, 800); gtk_window_fullscreen(GTK_WINDOW(o->window)); load_css();
    o->overlay = GTK_WIDGET(gtk_overlay_new()); GtkWidget *wallpaper = gtk_picture_new_for_filename(asset(OOBE_WALLPAPER_PATH, LOCAL_WALLPAPER_PATH)); gtk_picture_set_content_fit(GTK_PICTURE(wallpaper), GTK_CONTENT_FIT_COVER); gtk_widget_set_hexpand(wallpaper, TRUE); gtk_widget_set_vexpand(wallpaper, TRUE); gtk_overlay_set_child(GTK_OVERLAY(o->overlay), wallpaper);
    GtkWidget *wash = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); gtk_widget_add_css_class(wash, "oobe-wash"); gtk_widget_set_hexpand(wash, TRUE); gtk_widget_set_vexpand(wash, TRUE); gtk_overlay_add_overlay(GTK_OVERLAY(o->overlay), wash);
    o->root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); gtk_widget_set_hexpand(o->root, TRUE); gtk_widget_set_vexpand(o->root, TRUE); gtk_overlay_add_overlay(GTK_OVERLAY(o->overlay), o->root);
    o->notch = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); gtk_widget_add_css_class(o->notch, "notch"); gtk_widget_set_halign(o->notch, GTK_ALIGN_CENTER); gtk_widget_set_valign(o->notch, GTK_ALIGN_START); gtk_widget_set_size_request(o->notch, 360, 28); gtk_overlay_add_overlay(GTK_OVERLAY(o->overlay), o->notch);
    o->splash = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); gtk_widget_add_css_class(o->splash, "oobe-splash"); gtk_widget_set_hexpand(o->splash, TRUE); gtk_widget_set_vexpand(o->splash, TRUE); GtkWidget *sp = gtk_box_new(GTK_ORIENTATION_VERTICAL, 22); gtk_widget_add_css_class(sp, "splash-panel"); gtk_widget_set_halign(sp, GTK_ALIGN_CENTER); gtk_widget_set_valign(sp, GTK_ALIGN_CENTER); gtk_box_append(GTK_BOX(sp), scaled_picture(asset(BRAND_LOGO_PATH, LOCAL_LOGO_PATH), 128)); o->splash_progress = gtk_progress_bar_new(); gtk_widget_add_css_class(o->splash_progress, "splash-progress"); gtk_widget_set_size_request(o->splash_progress, 220, 6); gtk_widget_set_margin_top(o->splash_progress, 8); gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(o->splash_progress), 0.10); gtk_box_append(GTK_BOX(sp), o->splash_progress); gtk_box_append(GTK_BOX(o->splash), sp); gtk_overlay_add_overlay(GTK_OVERLAY(o->overlay), o->splash);
    o->content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8); gtk_widget_set_halign(o->content, GTK_ALIGN_CENTER); gtk_widget_set_valign(o->content, GTK_ALIGN_CENTER); gtk_widget_set_opacity(o->content, 0.0); gtk_widget_set_margin_top(o->content, 30);
    o->identity = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10); gtk_widget_add_css_class(o->identity, "identity"); gtk_widget_set_halign(o->identity, GTK_ALIGN_CENTER); o->cube = scaled_picture(asset(BRAND_LOGO_PATH, LOCAL_LOGO_PATH), 64); o->brand = gtk_label_new("Danenone"); gtk_widget_add_css_class(o->brand, "brand"); gtk_widget_set_opacity(o->brand, 0.0); gtk_widget_set_size_request(o->brand, 0, -1); gtk_box_append(GTK_BOX(o->identity), o->cube); gtk_box_append(GTK_BOX(o->identity), o->brand); gtk_box_append(GTK_BOX(o->content), o->identity);
    o->modal = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10); gtk_widget_add_css_class(o->modal, "oobe-modal"); gtk_widget_set_size_request(o->modal, 660, 430); o->stack = gtk_stack_new(); gtk_stack_set_transition_type(GTK_STACK(o->stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT); gtk_stack_set_transition_duration(GTK_STACK(o->stack), 260); gtk_widget_set_vexpand(o->stack, TRUE); gtk_widget_set_hexpand(o->stack, TRUE);
    gtk_stack_add_named(GTK_STACK(o->stack), welcome_page(), "welcome"); gtk_stack_add_named(GTK_STACK(o->stack), language_network_page(o), "language-network"); gtk_stack_add_named(GTK_STACK(o->stack), edition_page(o), "edition"); gtk_stack_add_named(GTK_STACK(o->stack), license_page(o), "license"); gtk_stack_add_named(GTK_STACK(o->stack), disk_page(), "disk"); gtk_stack_add_named(GTK_STACK(o->stack), user_page(o), "user"); gtk_stack_add_named(GTK_STACK(o->stack), tools_page(), "tools"); gtk_stack_add_named(GTK_STACK(o->stack), appearance_page(o), "appearance"); gtk_stack_add_named(GTK_STACK(o->stack), notch_page(o), "notch"); gtk_stack_add_named(GTK_STACK(o->stack), privacy_page(), "privacy"); gtk_stack_add_named(GTK_STACK(o->stack), summary_page(o), "summary"); gtk_stack_add_named(GTK_STACK(o->stack), install_page(), "install"); gtk_box_append(GTK_BOX(o->modal), o->stack);
    o->status = text_label("", "status"); gtk_box_append(GTK_BOX(o->modal), o->status); GtkWidget *footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10); o->repair = action_button("Repara mi DaneDesk", "secondary"); o->back = action_button("Atrás", "secondary"); o->next = action_button("Continuar", "primary"); gtk_box_append(GTK_BOX(footer), o->repair); gtk_box_append(GTK_BOX(footer), o->back); GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); gtk_widget_set_hexpand(spacer, TRUE); gtk_box_append(GTK_BOX(footer), spacer); gtk_box_append(GTK_BOX(footer), o->next); gtk_box_append(GTK_BOX(o->modal), footer); gtk_box_append(GTK_BOX(o->content), o->modal); o->stage = text_label("", "stage"); gtk_widget_set_halign(o->stage, GTK_ALIGN_CENTER); gtk_box_append(GTK_BOX(o->content), o->stage); gtk_box_append(GTK_BOX(o->root), o->content);
    o->final_splash = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18); gtk_widget_add_css_class(o->final_splash, "final-splash"); gtk_widget_set_hexpand(o->final_splash, TRUE); gtk_widget_set_vexpand(o->final_splash, TRUE); gtk_widget_set_halign(o->final_splash, GTK_ALIGN_CENTER); gtk_widget_set_valign(o->final_splash, GTK_ALIGN_CENTER); gtk_box_append(GTK_BOX(o->final_splash), scaled_picture(asset(BRAND_LOGO_PATH, LOCAL_LOGO_PATH), 96)); gtk_box_append(GTK_BOX(o->final_splash), text_label("Danenone está listo", "final-title")); o->final_progress = gtk_progress_bar_new(); gtk_widget_add_css_class(o->final_progress, "splash-progress"); gtk_widget_set_size_request(o->final_progress, 220, 6); gtk_box_append(GTK_BOX(o->final_splash), o->final_progress); gtk_widget_set_visible(o->final_splash, FALSE); gtk_overlay_add_overlay(GTK_OVERLAY(o->overlay), o->final_splash);
    if (g_getenv("DANENONE_FINAL_SPLASH")) { gtk_widget_set_visible(o->content, FALSE); gtk_widget_set_visible(o->notch, FALSE); gtk_widget_set_visible(o->final_splash, TRUE); }
    gtk_widget_set_visible(o->repair, repair_available()); gtk_widget_set_visible(o->back, FALSE); g_signal_connect(o->next, "clicked", G_CALLBACK(next_clicked), o); g_signal_connect(o->back, "clicked", G_CALLBACK(back_clicked), o); g_signal_connect(o->repair, "clicked", G_CALLBACK(repair_clicked), o); g_signal_connect(o->dark_toggle, "toggled", G_CALLBACK(dark_toggled), o); g_signal_connect(o->full_notch_toggle, "toggled", G_CALLBACK(full_notch_toggled), o); g_signal_connect(o->dynamic_notch_toggle, "toggled", G_CALLBACK(dynamic_notch_toggled), o);
    gtk_window_set_child(GTK_WINDOW(o->window), o->overlay); gtk_window_present(GTK_WINDOW(o->window)); o->splash_timer = g_timeout_add(4500, finish_splash, o); o->marquee_timer = g_timeout_add(34, marquee_step, o); const char *start = g_getenv("DANENONE_OOBE_PAGE"); int initial = start ? CLAMP((int)g_ascii_strtoll(start, NULL, 10), 0, PAGE_COUNT - 1) : 0; set_page(o, initial);
}

int main(int argc, char **argv) { GtkApplication *app = gtk_application_new("com.influent.danenone.firstboot.gtk4", G_APPLICATION_DEFAULT_FLAGS); g_signal_connect(app, "activate", G_CALLBACK(activate), NULL); int status = g_application_run(G_APPLICATION(app), argc, argv); g_object_unref(app); return status; }
