#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>
#include <glib/gstdio.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define BRAND_LOGO_PATH "/usr/share/influent/danenone-cube-logo.png"
#define OOBE_WALLPAPER_PATH "/usr/share/backgrounds/influent/oobe-river-blurred.jpg"
#define LOCAL_LOGO_PATH "/home/ubuntu/danenone/native-shell/assets/danenone-cube/danenone-cube-logo.png"
#define LOCAL_WALLPAPER_PATH "/home/ubuntu/danenone/native-shell/assets/oobe-river-blurred.jpg"
#define LANGUAGE_MANIFEST_PATH "/usr/share/influent/languages/manifest.tsv"
#define LOCAL_LANGUAGE_MANIFEST_PATH "/home/ubuntu/danenone/native-shell/languages/manifest.tsv"
#define EMOJI_PROFILE_PATH "/usr/share/influent-danenone/emojis/profiles.tsv"
#define LOCAL_EMOJI_PROFILE_PATH "/home/ubuntu/danenone/native-shell/emojis/profiles.tsv"
#define LOCALE_DIR_PATH "/usr/share/influent-danenone/locales"
#define LOCAL_LOCALE_DIR_PATH "/home/ubuntu/danenone/archiso-profile/airootfs/usr/share/influent-danenone/locales"
#define STORAGE_FILES_ICON "/usr/share/influent/icons/danenone-files.svg"
#define STORAGE_CONTROL_ICON "/usr/share/influent/icons/danenone-control.svg"
#define STORAGE_SETTINGS_ICON "/usr/share/influent/icons/danenone-settings.svg"
#define STORAGE_SEARCH_ICON "/usr/share/influent/icons/danenone-search.svg"
#define LOCAL_STORAGE_FILES_ICON "/home/ubuntu/danenone/native-shell/assets/icons/danenone-files.svg"
#define LOCAL_STORAGE_CONTROL_ICON "/home/ubuntu/danenone/native-shell/assets/icons/danenone-control.svg"
#define LOCAL_STORAGE_SETTINGS_ICON "/home/ubuntu/danenone/native-shell/assets/icons/danenone-settings.svg"
#define LOCAL_STORAGE_SEARCH_ICON "/home/ubuntu/danenone/native-shell/assets/icons/danenone-search.svg"
#define PAGE_COUNT 15

static const char *EDITION_CODES[] = {"home", "enterprise", "developer", "minimal", "frozen-lab"};
static const char *EDITION_KEYS[] = {"edition_home", "edition_enterprise", "edition_developer", "edition_minimal", "edition_frozen", NULL};
static const char *PARTITION_KEYS[] = {"storage_auto", "storage_full", "storage_resize", "storage_dual", NULL};
static const char *FORMAT_KEYS[] = {"format_btrfs", "format_ext4", "format_none", NULL};
static const char *ACCENT_NAMES[] = {
    "Verde Danenone", "Verde bosque", "Menta", "Turquesa", "Cian", "Azul cielo", "Azul Danenone",
    "Azul profundo", "Índigo", "Violeta", "Púrpura", "Magenta", "Rosa", "Rojo coral", "Rojo",
    "Naranja", "Ámbar", "Dorado", "Lima", "Oliva", NULL
};
static const char *ACCENT_HEX[] = {
    "#00b982", "#16805b", "#32b38a", "#00a6a6", "#00a9d6", "#3a9bd9", "#2878c8", "#2450a6",
    "#4e55bd", "#744fc6", "#9a4ec2", "#c248a3", "#d84e83", "#d85b5b", "#c83b48", "#d8733b",
    "#d29a2e", "#d0b22e", "#86ae36", "#718c35"
};

typedef struct {
    GtkWidget *widget;
    char *key;
} Binding;

typedef struct {
    GtkWidget *widget;
    char *code;
} EmojiChoice;

typedef struct {
    GtkApplication *app;
    GtkWidget *window, *overlay, *root, *content, *modal, *stack;
    GtkWidget *splash, *splash_progress, *final_splash, *final_progress;
    GtkWidget *identity, *cube, *brand, *stage, *status;
    GtkWidget *repair, *back, *next, *lock_icon, *notch;
    GtkWidget *language_dropdown, *edition_dropdown, *network_status;
    GtkWidget *license_text, *license_accept;
    GtkWidget *disk_dropdown, *partition_target, *partition_plan, *resize_size, *format_plan, *format_confirm, *disk_status;
    GtkWidget *network_list, *network_password, *network_auth, *network_connect_hint;
    GtkWidget *theme_dark, *theme_light, *color_grid;
    GtkWidget *full_notch_toggle, *dynamic_notch_toggle;
    GtkWidget *summary, *emoji_dropdown;
    GtkWidget *username, *password;
    GtkWidget *accent_preview;
    GtkStringList *language_model, *edition_model, *partition_model, *format_model;
    GPtrArray *language_codes, *bindings, *emoji_choices, *fluthin_checks, *debian_checks;
    GtkCssProvider *accent_provider;
    GHashTable *runtime_strings;
    char *language, *network_ssid, *network_security;
    int accent_index;
    guint splash_timer, marquee_timer, intro_timer, final_timer, lock_timer;
    double intro_opacity, brand_reveal;
    int page;
} Oobe;

static const char *const tr_en[][2] = {
    {"window", "Influent Danenone"}, {"welcome_title", "Configure your Danenone space"},
    {"welcome_body", "Review each decision before applying it. The assistant will guide you through language, license, edition, storage, network, packages, user, privacy and appearance."},
    {"language_title", "Language and connectivity"}, {"language_body", "Choose the interface language first. English is built in; other languages are verified before their packages are downloaded."},
    {"language_label", "Interface language"}, {"network_available", "Network available according to NetworkManager."},
    {"network_missing", "No network is connected. Choose a network below or continue with English offline."},
    {"license_title", "License and agreements"}, {"license_body", "Read the license, privacy policy and optional application communication agreements in the selected language."},
    {"license_accept", "I accept the license, privacy policy and agreements."}, {"edition_home", "Home — personal use"}, {"edition_enterprise", "Enterprise — administration and development"}, {"edition_developer", "Developer — complete toolchain"}, {"edition_minimal", "Minimal — reduced installation"}, {"edition_frozen", "Frozen Lab — no updates"}, {"fluthin_packagemaker", "PackageMaker"}, {"fluthin_foundstore", "Foundstore and flut"}, {"fluthin_certificates", "Fluthin repository list and certificates"}, {"fluthin_settings", "Danenone Settings"}, {"debian_python", "Python"}, {"debian_vscode", "Visual Studio Code"}, {"debian_firefox", "Firefox"}, {"debian_dolphin", "Dolphin"}, {"debian_vlc", "VLC"}, {"debian_extra", "Additional programs for the selected edition"}, {"storage_auto", "Automatic safe plan"}, {"storage_full", "Use the entire disk"}, {"storage_resize", "Resize an existing partition"}, {"storage_dual", "Dual boot without erasing"}, {"format_btrfs", "Btrfs · ZSTD"}, {"format_ext4", "Ext4"}, {"format_none", "Do not format"}, {"format_confirm", "Prepare formatting for the installer final confirmation"}, {"edition_title", "Choose a Danenone edition"},
    {"edition_body", "The edition defines package sets and the update policy of the installation."}, {"edition_label", "Target edition"},
    {"storage_title", "Installation storage"}, {"storage_body", "Select the device and a safe installation plan. Formatting is not executed by the OOBE; the installer must confirm the final action."}, {"storage_tool_device", "Devices"}, {"storage_tool_partition", "Partition"}, {"storage_tool_resize", "Extend / resize"}, {"storage_tool_format", "Format"},
    {"device_label", "Target device"}, {"partition_label", "Partition plan"}, {"format_label", "Filesystem and formatting"},
    {"storage_safe", "No disk operation has been executed. The final installer confirmation is required."},
    {"network_title", "Connect to the internet"}, {"network_body", "If connectivity is unavailable, select a visible network. Secure networks reveal authentication below the list."},
    {"password_label", "Network password"}, {"scan_networks", "Refresh network list"}, {"network_connect", "The selected network will be tested before continuing."},
    {"fluthin_title", "Install Fluthin packages"}, {"fluthin_body", "Choose the package capabilities to preload. Fluthin packages remain registered through flut."},
    {"debian_title", "Install Debian-compatible tools"}, {"debian_body", "Select optional tools and edition extras. The selection is staged for the installer and is not executed in the live OOBE."},
    {"user_title", "Create your user"}, {"user_body", "Create a local user for the first login. Both fields are required before continuing."},
    {"username_label", "Local username"}, {"password_local_label", "Local password"}, {"password_hint", "Use at least eight characters."},
    {"appearance_title", "Theme and accent"}, {"appearance_body", "Choose light or dark mode and an accent. The surface updates immediately."},
    {"light", "Light mode"}, {"dark", "Dark mode"}, {"accent_label", "Accent color"},
    {"notch_title", "Configure the notch"}, {"notch_body", "The notch remains empty. Choose full coverage or Dynamic Island behavior; changes apply immediately."},
    {"full_notch", "Full coverage"}, {"dynamic_notch", "Dynamic Island only"},
    {"privacy_title", "Privacy and optional communications"}, {"privacy_body", "Choose whether applications may show optional diagnostics or advertising. The choice remains local."},
    {"diagnostics", "Allow optional anonymous diagnostics"}, {"advertising", "Allow advertising inside applications"},
    {"emoji_title", "Choose the system emoji style"}, {"emoji_body", "Select one of ten compatible emoji profiles. The default is Fluent-compatible; proprietary Windows fonts are not redistributed."},
    {"emoji_label", "Emoji profile"}, {"summary_title", "Ready to install"}, {"summary_body", "Review the complete selection. No installation begins until the final action is confirmed."},
    {"install_title", "Installing Influent Danenone"}, {"install_body", "Keep the computer connected. Real progress is reported by the installer."},
    {"back", "Back"}, {"next", "Continue"}, {"finish", "Finish"}, {"repair", "Repair my Danenone"},
    {"locked", "Required information is missing"}, {"step", "Step"}, {"oobe_status", "Review each decision before applying it."}, {NULL, NULL}
};
static gboolean network_available(void);
static void update_network_status(Oobe *o);
static void apply_language(Oobe *o);
static void refresh_next_sensitivity(Oobe *o);
static gboolean final_step(gpointer data);

static const char *const tr_es[][2] = {
    {"window", "Influent Danenone"}, {"welcome_title", "Configura tu espacio Danenone"},
    {"welcome_body", "Revisa cada decisión antes de aplicarla. El asistente te guiará por idioma, licencia, edición, almacenamiento, red, paquetes, usuario, privacidad y apariencia."},
    {"language_title", "Idioma y conectividad"}, {"language_body", "Elige primero el idioma de la interfaz. English está integrado; los demás idiomas se verifican antes de descargar sus paquetes."},
    {"language_label", "Idioma de la interfaz"}, {"network_available", "Red disponible según NetworkManager."}, {"network_missing", "No hay una red conectada. Elige una red o continúa con English sin conexión."},
    {"license_title", "Licencia y acuerdos"}, {"license_body", "Lee la licencia, la política de privacidad y los acuerdos de comunicación opcional en el idioma elegido."},
        {"license_accept", "Acepto la licencia, la política de privacidad y los acuerdos."}, {"edition_home", "Home — uso personal"}, {"edition_enterprise", "Enterprise — administración y desarrollo"}, {"edition_developer", "Developer — toolchain completo"}, {"edition_minimal", "Minimal — instalación reducida"}, {"edition_frozen", "Frozen Lab — sin actualizaciones"}, {"fluthin_packagemaker", "PackageMaker"}, {"fluthin_foundstore", "Foundstore y flut"}, {"fluthin_certificates", "Lista de repositorios y certificados Fluthin"}, {"fluthin_settings", "Configuración Danenone"}, {"debian_python", "Python"}, {"debian_vscode", "Visual Studio Code"}, {"debian_firefox", "Firefox"}, {"debian_dolphin", "Dolphin"}, {"debian_vlc", "VLC"}, {"debian_extra", "Programas adicionales de la edición seleccionada"}, {"storage_auto", "Plan automático seguro"}, {"storage_full", "Usar todo el disco"}, {"storage_resize", "Redimensionar partición existente"}, {"storage_dual", "Dual boot sin borrar"}, {"format_btrfs", "Btrfs · ZSTD"}, {"format_ext4", "Ext4"}, {"format_none", "No formatear"}, {"format_confirm", "Preparar el formato para confirmación final del instalador"}, {"edition_title", "Elige la edición de Danenone"},
 {"edition_body", "La edición define los paquetes y la política de actualizaciones de la instalación."}, {"edition_label", "Edición de destino"},
        {"storage_title", "Almacenamiento de instalación"}, {"storage_body", "Selecciona el dispositivo y un plan seguro. El OOBE no ejecuta formateos; el instalador debe confirmar la acción final."}, {"storage_tool_device", "Dispositivos"}, {"storage_tool_partition", "Particionar"}, {"storage_tool_resize", "Extender / redimensionar"}, {"storage_tool_format", "Formatear"}, {"device_label", "Dispositivo de destino"},
 {"partition_label", "Plan de partición"}, {"format_label", "Sistema de archivos y formato"}, {"storage_safe", "No se ha ejecutado ninguna operación de disco. Se requiere la confirmación final del instalador."},
    {"network_title", "Conecta a internet"}, {"network_body", "Si no hay conectividad, selecciona una red visible. Las redes seguras muestran la autenticación debajo de la lista."}, {"password_label", "Contraseña de red"}, {"scan_networks", "Actualizar redes"}, {"network_connect", "La red seleccionada se probará antes de continuar."},
    {"fluthin_title", "Instala paquetes Fluthin"}, {"fluthin_body", "Elige las capacidades que se precargarán. Los paquetes Fluthin quedarán registrados mediante flut."}, {"debian_title", "Instala herramientas compatibles con Debian"}, {"debian_body", "Selecciona herramientas opcionales y extras de la edición. Se preparan para el instalador y no se ejecutan en el OOBE live."},
    {"user_title", "Crea tu usuario"}, {"user_body", "Crea un usuario local para el primer inicio. Ambos campos son obligatorios."}, {"username_label", "Nombre de usuario local"}, {"password_local_label", "Contraseña local"}, {"password_hint", "Usa al menos ocho caracteres."},
    {"appearance_title", "Tema y acento"}, {"appearance_body", "Elige modo claro u oscuro y un acento. La superficie se actualiza inmediatamente."}, {"light", "Modo claro"}, {"dark", "Modo oscuro"}, {"accent_label", "Color de acento"},
    {"notch_title", "Configura el notch"}, {"notch_body", "El notch permanece vacío. Elige cubierta total o solo isla dinámica; los cambios se aplican inmediatamente."}, {"full_notch", "Cubierta total"}, {"dynamic_notch", "Solo isla dinámica"},
    {"privacy_title", "Privacidad y comunicaciones opcionales"}, {"privacy_body", "Elige si las aplicaciones pueden mostrar diagnósticos opcionales o publicidad. La decisión queda local."}, {"diagnostics", "Permitir diagnósticos anónimos opcionales"}, {"advertising", "Permitir publicidad dentro de aplicaciones"},
    {"emoji_title", "Elige el estilo de emojis del sistema"}, {"emoji_body", "Selecciona uno de diez perfiles compatibles. El predeterminado es compatible con Fluent; no se redistribuyen fuentes propietarias de Windows."}, {"emoji_label", "Perfil de emojis"}, {"summary_title", "Listo para instalar"}, {"summary_body", "Revisa la selección completa. La instalación no comienza hasta confirmar la acción final."}, {"install_title", "Instalando Influent Danenone"}, {"install_body", "Mantén el equipo conectado. El progreso real será informado por el instalador."},
    {"back", "Atrás"}, {"next", "Continuar"}, {"finish", "Finalizar"}, {"repair", "Repara mi Danenone"}, {"locked", "Falta información obligatoria"}, {"step", "Paso"}, {"oobe_status", "Revisa cada decisión antes de aplicarla."}, {NULL, NULL}
};

static const char *tr(Oobe *o, const char *key) {
    if (o && o->runtime_strings) {
        const char *runtime = g_hash_table_lookup(o->runtime_strings, key);
        if (runtime && runtime[0]) return runtime;
    }
    const char *const (*catalog)[2] = tr_en;
    if (o && o->language && (g_str_has_prefix(o->language, "es") || g_strcmp0(o->language, "spa") == 0)) catalog = tr_es;
    for (guint i = 0; catalog[i][0]; i++) if (g_strcmp0(catalog[i][0], key) == 0) return catalog[i][1];
    return key;
}

static const char *asset(const char *installed, const char *local) { return g_file_test(installed, G_FILE_TEST_EXISTS) ? installed : local; }
static guint selected_index(GtkWidget *w) { return w ? gtk_drop_down_get_selected(GTK_DROP_DOWN(w)) : 0; }
static const char *selected_language(Oobe *o) { return (o->language_codes && selected_index(o->language_dropdown) < o->language_codes->len) ? g_ptr_array_index(o->language_codes, selected_index(o->language_dropdown)) : "en"; }
static guint language_index(Oobe *o, const char *code) { if (!o || !o->language_codes || !code) return GTK_INVALID_LIST_POSITION; for (guint i = 0; i < o->language_codes->len; i++) if (g_strcmp0(g_ptr_array_index(o->language_codes, i), code) == 0) return i; return GTK_INVALID_LIST_POSITION; }

static void load_runtime_locale(Oobe *o) {
    if (!o) return;
    if (!o->runtime_strings) o->runtime_strings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    g_hash_table_remove_all(o->runtime_strings);
    const char *base = g_file_test(LOCALE_DIR_PATH, G_FILE_TEST_IS_DIR) ? LOCALE_DIR_PATH : LOCAL_LOCALE_DIR_PATH;
    gchar *path = g_strdup_printf("%s/%s.tsv", base, o->language && o->language[0] ? o->language : "en");
    gchar *contents = NULL;
    if (g_file_get_contents(path, &contents, NULL, NULL)) {
        gchar **lines = g_strsplit(contents, "\\n", -1);
        for (guint i = 0; lines[i]; i++) {
            if (lines[i][0] == '#' || !strchr(lines[i], '=')) continue;
            gchar **pair = g_strsplit(lines[i], "=", 2);
            if (pair[0][0] && pair[1][0]) g_hash_table_replace(o->runtime_strings, g_strdup(pair[0]), g_strdup(pair[1]));
            g_strfreev(pair);
        }
        g_strfreev(lines);
    }
    g_free(contents); g_free(path);
}

static void bind_widget(Oobe *o, GtkWidget *widget, const char *key) {
    Binding *b = g_new0(Binding, 1); b->widget = widget; b->key = g_strdup(key); g_ptr_array_add(o->bindings, b);
}

static GtkWidget *label_for(Oobe *o, const char *key, const char *klass) {
    GtkWidget *w = gtk_label_new(tr(o, key)); if (klass) gtk_widget_add_css_class(w, klass); gtk_label_set_wrap(GTK_LABEL(w), TRUE); gtk_label_set_max_width_chars(GTK_LABEL(w), 74); gtk_label_set_xalign(GTK_LABEL(w), 0.0f); bind_widget(o, w, key); return w;
}

static GtkWidget *button_for(Oobe *o, const char *key, const char *klass) {
    GtkWidget *w = gtk_button_new_with_label(tr(o, key)); if (klass) gtk_widget_add_css_class(w, klass); bind_widget(o, w, key); return w;
}

static void set_bound_text(Binding *b, const char *value) { if (GTK_IS_LABEL(b->widget)) gtk_label_set_text(GTK_LABEL(b->widget), value); else if (GTK_IS_BUTTON(b->widget)) gtk_button_set_label(GTK_BUTTON(b->widget), value); else if (GTK_IS_CHECK_BUTTON(b->widget)) gtk_check_button_set_label(GTK_CHECK_BUTTON(b->widget), value); }

static void update_translated_model(GtkStringList *model, Oobe *o, const char *const *keys) { if (!model || !o || !keys) return; guint count = 0; while (keys[count] && count < 7) count++; const char *values[8] = {0}; for (guint i = 0; i < count; i++) values[i] = tr(o, keys[i]); gtk_string_list_splice(model, 0, g_list_model_get_n_items(G_LIST_MODEL(model)), values); }

static void apply_language(Oobe *o) {
    for (guint i = 0; i < o->bindings->len; i++) { Binding *b = g_ptr_array_index(o->bindings, i); set_bound_text(b, tr(o, b->key)); }
    if (o->window) gtk_window_set_title(GTK_WINDOW(o->window), tr(o, "window"));
    if (o->stage) gtk_label_set_text(GTK_LABEL(o->stage), g_strdup_printf("%s %d de %d", tr(o, "step"), o->page + 1, PAGE_COUNT));
    if (o->license_text) gtk_label_set_text(GTK_LABEL(o->license_text), tr(o, "license_body"));
    if (o->accent_preview) gtk_label_set_text(GTK_LABEL(o->accent_preview), ACCENT_NAMES[o->accent_index]);
    update_translated_model(o->edition_model, o, EDITION_KEYS);
    update_translated_model(o->partition_model, o, PARTITION_KEYS);
    update_translated_model(o->format_model, o, FORMAT_KEYS);
    if (o->network_status) gtk_label_set_text(GTK_LABEL(o->network_status), network_available() ? tr(o, "network_available") : tr(o, "network_missing"));
    if (o->next) gtk_button_set_label(GTK_BUTTON(o->next), o->page == PAGE_COUNT - 1 ? tr(o, "finish") : tr(o, "next"));
    if (o->back) gtk_button_set_label(GTK_BUTTON(o->back), tr(o, "back"));
}

static void write_atomic(const char *path, const char *contents, mode_t mode) {
    gchar *tmp = g_strdup_printf("%s.tmp", path); if (g_file_set_contents(tmp, contents, -1, NULL)) { g_chmod(tmp, mode); g_rename(tmp, path); } else g_remove(tmp); g_free(tmp);
}

static char *csv_checks(GPtrArray *checks) {
    GString *s = g_string_new(""); for (guint i = 0; i < checks->len; i++) { GtkWidget *check = g_ptr_array_index(checks, i); if (gtk_check_button_get_active(GTK_CHECK_BUTTON(check))) { if (s->len) g_string_append_c(s, ','); g_string_append(s, gtk_widget_get_name(check)); } } return g_string_free(s, FALSE);
}

static void save_preferences(Oobe *o) {
    gchar *dir = g_build_filename(g_get_user_state_dir(), "influent-danenone", NULL); gchar *path = g_build_filename(dir, "visual-preferences.conf", NULL); gchar *selection = g_build_filename(dir, "oobe-selection.conf", NULL); gchar *packages = csv_checks(o->fluthin_checks); gchar *debian = csv_checks(o->debian_checks);
    const char *theme = gtk_check_button_get_active(GTK_CHECK_BUTTON(o->theme_dark)) ? "dark" : "light"; const char *notch = gtk_check_button_get_active(GTK_CHECK_BUTTON(o->dynamic_notch_toggle)) ? "dynamic" : "full"; const char *edition = EDITION_CODES[selected_index(o->edition_dropdown)]; const char *language = selected_language(o); const char *emoji = (o->emoji_choices && selected_index(o->emoji_dropdown) < o->emoji_choices->len) ? ((EmojiChoice *)g_ptr_array_index(o->emoji_choices, selected_index(o->emoji_dropdown)))->code : "fluent-compatible";
    const char *partition_target = o->partition_target && gtk_drop_down_get_selected_item(GTK_DROP_DOWN(o->partition_target)) ? gtk_string_object_get_string(GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(GTK_DROP_DOWN(o->partition_target)))) : "pendiente";
    const char *partition_plan = o->partition_plan && gtk_drop_down_get_selected_item(GTK_DROP_DOWN(o->partition_plan)) ? gtk_string_object_get_string(GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(GTK_DROP_DOWN(o->partition_plan)))) : "automático";
    const char *format_plan = o->format_plan && gtk_drop_down_get_selected_item(GTK_DROP_DOWN(o->format_plan)) ? gtk_string_object_get_string(GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(GTK_DROP_DOWN(o->format_plan)))) : "no formatear";
    double resize_gib = o->resize_size ? gtk_spin_button_get_value(GTK_SPIN_BUTTON(o->resize_size)) : 0;
    gboolean format_confirm = o->format_confirm && gtk_check_button_get_active(GTK_CHECK_BUTTON(o->format_confirm));
    gchar *visual = g_strdup_printf("theme=%s\naccent=%s\naccent_index=%d\nnotch=%s\nnotch_width=%d\nnotch_height=28\nnotch_radius=%d\nlanguage=%s\nedition=%s\nemoji=%s\nfluthin=%s\ndebian=%s\npartition_target=%s\npartition_plan=%s\nresize_gib=%.0f\nformat_plan=%s\nformat_confirm=%s\n", theme, ACCENT_HEX[o->accent_index], o->accent_index, notch, g_strcmp0(notch, "dynamic") == 0 ? 220 : 360, g_strcmp0(notch, "dynamic") == 0 ? 18 : 14, language, edition, emoji, packages, debian, partition_target, partition_plan, resize_gib, format_plan, format_confirm ? "true" : "false");
    gchar *sel = g_strdup_printf("LANGUAGE=%s\nEDITION=%s\nNETWORK_AVAILABLE=%s\nEMOJI=%s\n", language, edition, network_available() ? "true" : "false", emoji);
    if (g_mkdir_with_parents(dir, 0700) == 0) { write_atomic(path, visual, 0600); write_atomic(selection, sel, 0600); }
    const char *locale = g_strcmp0(language, "es") == 0 ? "es_ES.UTF-8" : g_strcmp0(language, "fr") == 0 ? "fr_FR.UTF-8" : g_strcmp0(language, "de") == 0 ? "de_DE.UTF-8" : g_strcmp0(language, "pt") == 0 ? "pt_BR.UTF-8" : "en_US.UTF-8";
    g_setenv("LANG", locale, TRUE); gchar *locale_path = g_build_filename(dir, "locale.conf", NULL); gchar *locale_data = g_strdup_printf("LANG=%s\nLANGUAGE_CODE=%s\n", locale, language); write_atomic(locale_path, locale_data, 0600);
    g_free(locale_data); g_free(locale_path); g_free(visual); g_free(sel); g_free(packages); g_free(debian); g_free(path); g_free(selection); g_free(dir);
}

static gboolean network_available(void) {
    GNetworkMonitor *monitor = g_network_monitor_get_default();
    return monitor && g_network_monitor_get_network_available(monitor);
}

static void update_network_status(Oobe *o) {
    if (o && o->network_status) gtk_label_set_text(GTK_LABEL(o->network_status), network_available() ? tr(o, "network_available") : tr(o, "network_missing"));
}

static GtkWidget *scaled_picture(const char *path, int size) {
    GError *error = NULL; GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(path, size, size, TRUE, &error); if (!pixbuf) { if (error) g_error_free(error); return gtk_picture_new(); } GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf); GtkWidget *picture = gtk_picture_new_for_paintable(GDK_PAINTABLE(texture)); gtk_picture_set_content_fit(GTK_PICTURE(picture), GTK_CONTENT_FIT_CONTAIN); gtk_widget_set_size_request(picture, size, size); g_object_unref(texture); g_object_unref(pixbuf); return picture;
}

static GtkWidget *base_page(Oobe *o, const char *title_key, const char *body_key) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12); gtk_widget_add_css_class(box, "oobe-page"); gtk_widget_set_margin_start(box, 36); gtk_widget_set_margin_end(box, 36); gtk_widget_set_margin_top(box, 14); gtk_widget_set_margin_bottom(box, 14); gtk_box_append(GTK_BOX(box), label_for(o, title_key, "page-title")); gtk_box_append(GTK_BOX(box), label_for(o, body_key, "body-text")); return box;
}

static GtkWidget *make_language_dropdown(Oobe *o) {
    o->language_model = gtk_string_list_new(NULL); o->language_codes = g_ptr_array_new_with_free_func(g_free); g_ptr_array_add(o->language_codes, g_strdup("en")); gtk_string_list_append(o->language_model, "English — United States (integrado)");
    const char *manifest = g_file_test(LANGUAGE_MANIFEST_PATH, G_FILE_TEST_EXISTS) ? LANGUAGE_MANIFEST_PATH : LOCAL_LANGUAGE_MANIFEST_PATH; gchar *contents = NULL;
    if (g_file_get_contents(manifest, &contents, NULL, NULL)) { gchar **lines = g_strsplit(contents, "\n", -1); for (guint i = 0; lines[i]; i++) { if (lines[i][0] == '#') continue; gchar **parts = g_strsplit(lines[i], "|", 5); if (g_strv_length(parts) >= 5 && parts[0][0] && g_strcmp0(parts[0], "en") != 0) { g_ptr_array_add(o->language_codes, g_strdup(parts[0])); gchar *label = g_strdup_printf("%s — %s", parts[4], parts[0]); gtk_string_list_append(o->language_model, label); g_free(label); } g_strfreev(parts); } g_strfreev(lines); }
    g_free(contents);     GtkWidget *dropdown = gtk_drop_down_new(G_LIST_MODEL(o->language_model), NULL); gtk_widget_add_css_class(dropdown, "dropdown"); return dropdown;

}

static GtkWidget *make_dropdown(const char *const *items) { GtkStringList *model = gtk_string_list_new(items); GtkWidget *dropdown = gtk_drop_down_new(G_LIST_MODEL(model), NULL); gtk_widget_add_css_class(dropdown, "dropdown"); g_object_unref(model); return dropdown; }
static GtkWidget *make_translated_dropdown(Oobe *o, const char *const *keys, GtkStringList **model_out) { GtkStringList *model = gtk_string_list_new(NULL); for (guint i = 0; keys[i]; i++) gtk_string_list_append(model, tr(o, keys[i])); if (model_out) *model_out = model; GtkWidget *dropdown = gtk_drop_down_new(G_LIST_MODEL(model), NULL); gtk_widget_add_css_class(dropdown, "dropdown"); return dropdown; }

static GtkWidget *language_page(Oobe *o) {
    GtkWidget *box = base_page(o, "language_title", "language_body"); gtk_box_append(GTK_BOX(box), label_for(o, "language_label", "field-label")); o->language_dropdown = make_language_dropdown(o); gtk_box_append(GTK_BOX(box), o->language_dropdown); o->network_status = label_for(o, "network_missing", "network-status"); gtk_box_append(GTK_BOX(box), o->network_status); return box;
}

static GtkWidget *license_page(Oobe *o) {
    GtkWidget *box = base_page(o, "license_title", "license_body"); o->license_text = label_for(o, "license_body", "license-text"); gtk_box_append(GTK_BOX(box), o->license_text); o->license_accept = gtk_check_button_new_with_label(tr(o, "license_accept")); gtk_widget_add_css_class(o->license_accept, "setting-row"); bind_widget(o, o->license_accept, "license_accept"); gtk_box_append(GTK_BOX(box), o->license_accept); return box;
}

static GtkWidget *edition_page(Oobe *o) { GtkWidget *box = base_page(o, "edition_title", "edition_body"); gtk_box_append(GTK_BOX(box), label_for(o, "edition_label", "field-label")); o->edition_dropdown = make_translated_dropdown(o, EDITION_KEYS, &o->edition_model); gtk_box_append(GTK_BOX(box), o->edition_dropdown); gtk_box_append(GTK_BOX(box), label_for(o, "edition_body", "edition-details")); return box; }

static gchar *list_disks(void) {
    gchar *out = NULL;
    gint status = 1;
    GError *error = NULL;
    g_spawn_command_line_sync("lsblk -rno NAME,SIZE,TYPE,FSTYPE", &out, NULL, &status, &error);
    if (error) g_error_free(error);
    GString *result = g_string_new("");
    if (status == 0 && out) {
        gchar **lines = g_strsplit(out, "\n", -1);
        for (guint i = 0; lines[i]; i++) {
            char name[128] = {0}, size[128] = {0}, type[128] = {0}, fstype[128] = {0};
            int fields = sscanf(lines[i], "%127s %127s %127s %127s", name, size, type, fstype);
            if (fields >= 3 && (g_strcmp0(type, "disk") == 0 || g_strcmp0(type, "part") == 0)) {
                if (result->len) g_string_append_c(result, '\n');
                if (fields == 4 && fstype[0]) g_string_append_printf(result, "/dev/%s · %s · %s · %s", name, size, type, fstype);
                else g_string_append_printf(result, "/dev/%s · %s · %s", name, size, type);
            }
        }
        g_strfreev(lines);
    }
    g_free(out);
    if (!result->len) g_string_append(result, "Dispositivo no disponible · confirmar en el instalador");
    return g_string_free(result, FALSE);
}

static void storage_focus_cb(GtkButton *button, gpointer data) { (void)button; if (data) gtk_widget_grab_focus(GTK_WIDGET(data)); }

static void storage_row_selected(GtkListBox *list, GtkListBoxRow *row, gpointer data) { (void)list; Oobe *o = data; if (!o || !row || !o->disk_dropdown) return; guint index = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(row), "storage-index")); guint count = g_list_model_get_n_items(G_LIST_MODEL(gtk_drop_down_get_model(GTK_DROP_DOWN(o->disk_dropdown)))); if (index < count) gtk_drop_down_set_selected(GTK_DROP_DOWN(o->disk_dropdown), index); }

static GtkWidget *storage_list_row(Oobe *o, const char *entry, guint index) { (void)o; gchar **parts = g_strsplit(entry, " · ", 4); const char *path = parts[0] ? parts[0] : entry; const char *size = parts[1] ? parts[1] : ""; const char *type = parts[2] ? parts[2] : "part"; const char *fs = parts[3] ? parts[3] : ""; gboolean disk = g_strcmp0(type, "disk") == 0; GtkWidget *row = gtk_list_box_row_new(); gtk_widget_add_css_class(row, "storage-row"); if (!disk) gtk_widget_add_css_class(row, "storage-partition-row"); GtkWidget *line = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10); gtk_widget_set_margin_start(line, disk ? 10 : 30); gtk_widget_set_margin_end(line, 10); gtk_widget_set_margin_top(line, 7); gtk_widget_set_margin_bottom(line, 7); GtkWidget *icon = scaled_picture(asset(disk ? STORAGE_FILES_ICON : STORAGE_CONTROL_ICON, disk ? LOCAL_STORAGE_FILES_ICON : LOCAL_STORAGE_CONTROL_ICON), 30); gtk_box_append(GTK_BOX(line), icon); GtkWidget *text = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2); GtkWidget *name = gtk_label_new(path); gtk_widget_add_css_class(name, disk ? "storage-device-name" : "storage-partition-name"); gtk_label_set_xalign(GTK_LABEL(name), 0.0f); gtk_box_append(GTK_BOX(text), name); gchar *meta = g_strdup_printf("%s%s%s", size, fs[0] ? " · " : "", fs); GtkWidget *details = gtk_label_new(meta); g_free(meta); gtk_widget_add_css_class(details, "storage-device-details"); gtk_label_set_xalign(GTK_LABEL(details), 0.0f); gtk_box_append(GTK_BOX(text), details); gtk_widget_set_hexpand(text, TRUE); gtk_box_append(GTK_BOX(line), text); gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), line); g_object_set_data(G_OBJECT(row), "storage-index", GUINT_TO_POINTER(index)); g_strfreev(parts); return row; }

static GtkWidget *storage_tool_button(Oobe *o, const char *label_key, const char *icon_installed, const char *icon_local, GtkWidget *target) { GtkWidget *button = gtk_button_new(); gtk_widget_add_css_class(button, "storage-tool"); GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3); gtk_widget_set_halign(content, GTK_ALIGN_CENTER); gtk_widget_set_margin_top(content, 1); gtk_widget_set_margin_bottom(content, 1); GtkWidget *icon = scaled_picture(asset(icon_installed, icon_local), 22); gtk_box_append(GTK_BOX(content), icon); gtk_box_append(GTK_BOX(content), label_for(o, label_key, "storage-tool-label")); gtk_button_set_child(GTK_BUTTON(button), content); g_signal_connect(button, "clicked", G_CALLBACK(storage_focus_cb), target); return button; }

static GtkWidget *storage_page(Oobe *o) { GtkWidget *box = base_page(o, "storage_title", "storage_body"); gtk_widget_add_css_class(box, "storage-page"); gchar *disk_text = list_disks(); gchar **disks = g_strsplit(disk_text, "\n", -1); o->disk_dropdown = make_dropdown((const char *const *)disks); o->partition_target = o->disk_dropdown; GtkWidget *device_list = gtk_list_box_new(); gtk_widget_add_css_class(device_list, "storage-device-list"); gtk_list_box_set_selection_mode(GTK_LIST_BOX(device_list), GTK_SELECTION_SINGLE); gtk_widget_set_vexpand(device_list, FALSE); gtk_widget_set_size_request(device_list, -1, 150); gtk_box_append(GTK_BOX(box), label_for(o, "device_label", "field-label")); guint device_index = 0; GtkWidget *first_row = NULL; for (guint i = 0; disks[i]; i++) { if (!disks[i][0]) continue; GtkWidget *row = storage_list_row(o, disks[i], device_index); gtk_list_box_append(GTK_LIST_BOX(device_list), row); if (!first_row) first_row = row; device_index++; } if (!device_index) { first_row = storage_list_row(o, "Dispositivo no disponible · confirmar en el instalador", 0); gtk_list_box_append(GTK_LIST_BOX(device_list), first_row); } g_signal_connect(device_list, "row-selected", G_CALLBACK(storage_row_selected), o); gtk_list_box_select_row(GTK_LIST_BOX(device_list), GTK_LIST_BOX_ROW(first_row)); gtk_box_append(GTK_BOX(box), device_list); gtk_widget_set_visible(o->disk_dropdown, FALSE); g_strfreev(disks); g_free(disk_text); o->partition_plan = make_translated_dropdown(o, PARTITION_KEYS, &o->partition_model); o->resize_size = gtk_spin_button_new_with_range(16, 4096, 1); gtk_spin_button_set_value(GTK_SPIN_BUTTON(o->resize_size), 64); gtk_widget_set_tooltip_text(o->resize_size, "Tamaño de redimensionado en GiB"); o->format_plan = make_translated_dropdown(o, FORMAT_KEYS, &o->format_model); o->format_confirm = gtk_check_button_new_with_label(tr(o, "format_confirm")); bind_widget(o, o->format_confirm, "format_confirm"); GtkWidget *tools = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8); gtk_widget_add_css_class(tools, "storage-tools"); gtk_widget_set_halign(tools, GTK_ALIGN_CENTER); gtk_widget_set_margin_top(tools, 8); gtk_widget_set_margin_bottom(tools, 6); gtk_box_append(GTK_BOX(tools), storage_tool_button(o, "storage_tool_partition", STORAGE_CONTROL_ICON, LOCAL_STORAGE_CONTROL_ICON, o->partition_plan)); gtk_box_append(GTK_BOX(tools), storage_tool_button(o, "storage_tool_resize", STORAGE_SETTINGS_ICON, LOCAL_STORAGE_SETTINGS_ICON, o->resize_size)); gtk_box_append(GTK_BOX(tools), storage_tool_button(o, "storage_tool_format", STORAGE_CONTROL_ICON, LOCAL_STORAGE_CONTROL_ICON, o->format_plan)); gtk_box_append(GTK_BOX(box), tools); GtkWidget *options = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5); gtk_widget_add_css_class(options, "storage-options"); gtk_box_append(GTK_BOX(options), label_for(o, "partition_label", "field-label")); gtk_box_append(GTK_BOX(options), o->partition_plan); gtk_box_append(GTK_BOX(options), label_for(o, "format_label", "field-label")); gtk_box_append(GTK_BOX(options), o->format_plan); gtk_box_append(GTK_BOX(options), o->resize_size); gtk_box_append(GTK_BOX(options), o->format_confirm); gtk_box_append(GTK_BOX(box), options); o->disk_status = label_for(o, "storage_safe", "status-card"); gtk_box_append(GTK_BOX(box), o->disk_status); return box; }

static void network_row_selected(GtkListBox *list, GtkListBoxRow *row, gpointer data) { (void)list; Oobe *o = data; if (!row) return; const char *ssid = g_object_get_data(G_OBJECT(row), "ssid"); const char *security = g_object_get_data(G_OBJECT(row), "security"); g_free(o->network_ssid); g_free(o->network_security); o->network_ssid = g_strdup(ssid ? ssid : ""); o->network_security = g_strdup(security ? security : ""); gboolean secure = o->network_security && o->network_security[0]; gtk_widget_set_visible(o->network_auth, secure); gtk_widget_set_visible(o->network_password, secure); gtk_widget_set_visible(o->network_connect_hint, TRUE); }

static void scan_networks(Oobe *o) {
    while (gtk_widget_get_first_child(o->network_list)) gtk_list_box_remove(GTK_LIST_BOX(o->network_list), gtk_widget_get_first_child(o->network_list));
    gchar *out = NULL; gint status = 1; GError *error = NULL; g_spawn_command_line_sync("nmcli -t -f SSID,SECURITY,SIGNAL dev wifi list", &out, NULL, &status, &error); if (error) g_error_free(error); guint count = 0;
    if (status == 0 && out) { gchar **lines = g_strsplit(out, "\n", -1); for (guint i = 0; lines[i]; i++) { gchar **parts = g_strsplit(lines[i], ":", 3); if (g_strv_length(parts) >= 3 && parts[0][0]) { GtkWidget *row = gtk_list_box_row_new(); GtkWidget *label = gtk_label_new(g_strdup_printf("%s    %s    %s%%", parts[0], parts[1][0] ? parts[1] : "open", parts[2])); gtk_label_set_xalign(GTK_LABEL(label), 0.0f); gtk_widget_add_css_class(row, "network-row"); gtk_widget_set_margin_start(label, 18); gtk_widget_set_margin_end(label, 14); gtk_widget_set_margin_top(label, 10); gtk_widget_set_margin_bottom(label, 10); gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label); g_object_set_data_full(G_OBJECT(row), "ssid", g_strdup(parts[0]), g_free); g_object_set_data_full(G_OBJECT(row), "security", g_strdup(parts[1]), g_free); gtk_list_box_append(GTK_LIST_BOX(o->network_list), row); count++; } g_strfreev(parts); } g_strfreev(lines); } g_free(out);
    if (!count) { GtkWidget *row = gtk_list_box_row_new(); GtkWidget *label = gtk_label_new(tr(o, "network_missing")); gtk_label_set_xalign(GTK_LABEL(label), 0.0f); gtk_widget_set_margin_start(label, 18); gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label); gtk_list_box_append(GTK_LIST_BOX(o->network_list), row); }
    update_network_status(o);
}

static GtkWidget *network_page(Oobe *o) { GtkWidget *box = base_page(o, "network_title", "network_body"); o->network_list = gtk_list_box_new(); gtk_list_box_set_selection_mode(GTK_LIST_BOX(o->network_list), GTK_SELECTION_SINGLE); gtk_widget_add_css_class(o->network_list, "network-list"); gtk_widget_set_size_request(o->network_list, -1, 150); gtk_box_append(GTK_BOX(box), o->network_list); o->network_auth = label_for(o, "password_label", "field-label"); o->network_password = gtk_entry_new(); gtk_entry_set_visibility(GTK_ENTRY(o->network_password), FALSE); gtk_widget_add_css_class(o->network_password, "input"); o->network_connect_hint = label_for(o, "network_connect", "hint-text"); gtk_box_append(GTK_BOX(box), o->network_auth); gtk_box_append(GTK_BOX(box), o->network_password); gtk_box_append(GTK_BOX(box), o->network_connect_hint); GtkWidget *scan = button_for(o, "scan_networks", "secondary"); gtk_box_append(GTK_BOX(box), scan); g_signal_connect_swapped(scan, "clicked", G_CALLBACK(scan_networks), o); g_signal_connect(o->network_list, "row-selected", G_CALLBACK(network_row_selected), o); gtk_widget_set_visible(o->network_auth, FALSE); gtk_widget_set_visible(o->network_password, FALSE); scan_networks(o); return box; }

static void add_check(Oobe *o, GtkWidget *box, GPtrArray *array, const char *id, const char *key, gboolean active) { GtkWidget *check = gtk_check_button_new_with_label(tr(o, key)); gtk_widget_set_name(check, id); gtk_check_button_set_active(GTK_CHECK_BUTTON(check), active); bind_widget(o, check, key); gtk_box_append(GTK_BOX(box), check); g_ptr_array_add(array, check); }
static GtkWidget *fluthin_page(Oobe *o) { GtkWidget *box = base_page(o, "fluthin_title", "fluthin_body"); o->fluthin_checks = g_ptr_array_new(); add_check(o, box, o->fluthin_checks, "packagemaker", "fluthin_packagemaker", TRUE); add_check(o, box, o->fluthin_checks, "foundstore", "fluthin_foundstore", TRUE); add_check(o, box, o->fluthin_checks, "fluthin-certificates", "fluthin_certificates", TRUE); add_check(o, box, o->fluthin_checks, "settingspanel", "fluthin_settings", FALSE); return box; }
static GtkWidget *debian_page(Oobe *o) { GtkWidget *box = base_page(o, "debian_title", "debian_body"); o->debian_checks = g_ptr_array_new(); add_check(o, box, o->debian_checks, "python", "debian_python", TRUE); add_check(o, box, o->debian_checks, "vscode", "debian_vscode", FALSE); add_check(o, box, o->debian_checks, "firefox", "debian_firefox", TRUE); add_check(o, box, o->debian_checks, "dolphin", "debian_dolphin", TRUE); add_check(o, box, o->debian_checks, "vlc", "debian_vlc", TRUE); add_check(o, box, o->debian_checks, "edition-extra", "debian_extra", TRUE); return box; }

static GtkWidget *user_page(Oobe *o) { GtkWidget *box = base_page(o, "user_title", "user_body"); gtk_box_append(GTK_BOX(box), label_for(o, "username_label", "field-label")); o->username = gtk_entry_new(); gtk_widget_add_css_class(o->username, "input"); gtk_box_append(GTK_BOX(box), o->username); gtk_box_append(GTK_BOX(box), label_for(o, "password_local_label", "field-label")); o->password = gtk_entry_new(); gtk_entry_set_visibility(GTK_ENTRY(o->password), FALSE); gtk_entry_set_input_purpose(GTK_ENTRY(o->password), GTK_INPUT_PURPOSE_PASSWORD); gtk_widget_add_css_class(o->password, "input"); gtk_box_append(GTK_BOX(box), o->password); gtk_box_append(GTK_BOX(box), label_for(o, "password_hint", "hint-text")); return box; }

static void update_accent_css(Oobe *o) {
    if (o->accent_provider) {
        gtk_style_context_remove_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(o->accent_provider));
        g_object_unref(o->accent_provider);
    }
    o->accent_provider = gtk_css_provider_new();
    GdkRGBA c; gdk_rgba_parse(&c, ACCENT_HEX[o->accent_index]);
    double factor = (o->theme_dark && gtk_check_button_get_active(GTK_CHECK_BUTTON(o->theme_dark))) ? 0.82 : 1.0;
    gchar *css = g_strdup_printf("#danenone-root button.primary { background: rgba(%d,%d,%d,0.90); border-bottom-color: rgba(%d,%d,%d,1.0); } #danenone-root .accent-chip { background: rgba(%d,%d,%d,0.92); }", (int)(c.red * 255 * factor), (int)(c.green * 255 * factor), (int)(c.blue * 255 * factor), (int)(c.red * 210), (int)(c.green * 210), (int)(c.blue * 210), (int)(c.red * 255), (int)(c.green * 255), (int)(c.blue * 255));
    gtk_css_provider_load_from_string(o->accent_provider, css);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(o->accent_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);
    g_free(css);
}
static void accent_clicked(GtkButton *button, gpointer data) { Oobe *o = data; o->accent_index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "accent-index")); update_accent_css(o); save_preferences(o); }
static GtkWidget *appearance_page(Oobe *o) { GtkWidget *box = base_page(o, "appearance_title", "appearance_body"); o->theme_light = gtk_check_button_new_with_label(tr(o, "light")); o->theme_dark = gtk_check_button_new_with_label(tr(o, "dark")); bind_widget(o, o->theme_light, "light"); bind_widget(o, o->theme_dark, "dark"); gtk_check_button_set_group(GTK_CHECK_BUTTON(o->theme_dark), GTK_CHECK_BUTTON(o->theme_light)); gtk_check_button_set_active(GTK_CHECK_BUTTON(o->theme_light), TRUE); gtk_box_append(GTK_BOX(box), o->theme_light); gtk_box_append(GTK_BOX(box), o->theme_dark); gtk_box_append(GTK_BOX(box), label_for(o, "accent_label", "field-label")); o->color_grid = gtk_grid_new(); gtk_grid_set_row_spacing(GTK_GRID(o->color_grid), 7); gtk_grid_set_column_spacing(GTK_GRID(o->color_grid), 7); for (int i = 0; i < 20; i++) { GtkWidget *b = gtk_button_new_with_label(ACCENT_NAMES[i]); gtk_widget_add_css_class(b, "accent-chip"); g_object_set_data(G_OBJECT(b), "accent-index", GINT_TO_POINTER(i)); gtk_grid_attach(GTK_GRID(o->color_grid), b, i % 4, i / 4, 1, 1); g_signal_connect(b, "clicked", G_CALLBACK(accent_clicked), o); } gtk_box_append(GTK_BOX(box), o->color_grid); o->accent_preview = gtk_label_new(ACCENT_NAMES[o->accent_index]); gtk_box_append(GTK_BOX(box), o->accent_preview); return box; }

static GtkWidget *notch_page(Oobe *o) { GtkWidget *box = base_page(o, "notch_title", "notch_body"); o->full_notch_toggle = gtk_check_button_new_with_label(tr(o, "full_notch")); o->dynamic_notch_toggle = gtk_check_button_new_with_label(tr(o, "dynamic_notch")); bind_widget(o, o->full_notch_toggle, "full_notch"); bind_widget(o, o->dynamic_notch_toggle, "dynamic_notch"); gtk_check_button_set_group(GTK_CHECK_BUTTON(o->dynamic_notch_toggle), GTK_CHECK_BUTTON(o->full_notch_toggle)); gtk_check_button_set_active(GTK_CHECK_BUTTON(o->full_notch_toggle), TRUE); gtk_box_append(GTK_BOX(box), o->full_notch_toggle); gtk_box_append(GTK_BOX(box), o->dynamic_notch_toggle); return box; }
static GtkWidget *privacy_page(Oobe *o) { GtkWidget *box = base_page(o, "privacy_title", "privacy_body"); gtk_box_append(GTK_BOX(box), gtk_check_button_new_with_label(tr(o, "diagnostics"))); gtk_box_append(GTK_BOX(box), gtk_check_button_new_with_label(tr(o, "advertising"))); return box; }

static GtkWidget *emoji_page(Oobe *o) { GtkWidget *box = base_page(o, "emoji_title", "emoji_body"); gtk_box_append(GTK_BOX(box), label_for(o, "emoji_label", "field-label")); GtkStringList *model = gtk_string_list_new(NULL); o->emoji_choices = g_ptr_array_new_with_free_func(g_free); const char *manifest = g_file_test(EMOJI_PROFILE_PATH, G_FILE_TEST_EXISTS) ? EMOJI_PROFILE_PATH : LOCAL_EMOJI_PROFILE_PATH; gchar *contents = NULL; if (g_file_get_contents(manifest, &contents, NULL, NULL)) { gchar **lines = g_strsplit(contents, "\n", -1); for (guint i = 0; lines[i]; i++) { gchar **p = g_strsplit(lines[i], "|", 4); if (g_strv_length(p) >= 4 && p[0][0] != '#') { EmojiChoice *choice = g_new0(EmojiChoice, 1); choice->code = g_strdup(p[0]); g_ptr_array_add(o->emoji_choices, choice); gchar *preview = g_strdup_printf("%s  [%s]", p[1], p[3]); gtk_string_list_append(model, preview); g_free(preview); } g_strfreev(p); } g_strfreev(lines); } g_free(contents); if (!o->emoji_choices->len) { EmojiChoice *choice = g_new0(EmojiChoice, 1); choice->code = g_strdup("fluent-compatible"); g_ptr_array_add(o->emoji_choices, choice); gtk_string_list_append(model, "Fluent-compatible [U+1F600 U+1F680 U+1F44D U+1F389]"); } o->emoji_dropdown = gtk_drop_down_new(G_LIST_MODEL(model), NULL); gtk_widget_add_css_class(o->emoji_dropdown, "dropdown"); g_object_unref(model); gtk_box_append(GTK_BOX(box), o->emoji_dropdown); return box; }

static GtkWidget *summary_page(Oobe *o) { GtkWidget *box = base_page(o, "summary_title", "summary_body"); o->summary = gtk_label_new(""); gtk_label_set_xalign(GTK_LABEL(o->summary), 0.0f); gtk_label_set_wrap(GTK_LABEL(o->summary), TRUE); gtk_widget_add_css_class(o->summary, "summary-card"); gtk_box_append(GTK_BOX(box), o->summary); return box; }
static GtkWidget *install_page(Oobe *o) { GtkWidget *box = base_page(o, "install_title", "install_body"); GtkWidget *progress = gtk_progress_bar_new(); gtk_widget_add_css_class(progress, "install-progress"); gtk_widget_set_size_request(progress, 420, 8); gtk_box_append(GTK_BOX(box), progress); return box; }

static gboolean network_connected(Oobe *o) { return network_available() || (o->network_ssid && o->network_ssid[0]); }
static gboolean valid_user(Oobe *o) { const char *u = gtk_editable_get_text(GTK_EDITABLE(o->username)); const char *p = gtk_editable_get_text(GTK_EDITABLE(o->password)); return u && strlen(u) >= 2 && p && strlen(p) >= 8; }
static gboolean danedesk_recovery_required(void) {
    const char *server = g_getenv("DANEDESK_SERVER");
    if (!server || !*server || !g_file_test("/usr/local/bin/danedesk-client", G_FILE_TEST_IS_EXECUTABLE)) return FALSE;
    gchar *stdout_text = NULL;
    gchar *stderr_text = NULL;
    gint status = 1;
    GError *error = NULL;
    gchar *argv[] = {"/usr/local/bin/danedesk-client", "check-status", "--server", (gchar *)server, NULL};
    gboolean ran = g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &stdout_text, &stderr_text, &status, &error);
    if (error) g_error_free(error);
    gboolean required = ran && status == 0 && stdout_text && strstr(stdout_text, "\"recoveryRequired\": true");
    g_free(stdout_text);
    g_free(stderr_text);
    return required;
}
static void refresh_next_cb(GtkWidget *widget, gpointer data) { (void)widget; refresh_next_sensitivity(data); }
static void refresh_next_sensitivity(Oobe *o) {
 gboolean ok = TRUE; if (o->page == 2) ok = gtk_check_button_get_active(GTK_CHECK_BUTTON(o->license_accept)); if (o->page == 4) ok = o->disk_dropdown != NULL; if (o->page == 5) ok = g_strcmp0(selected_language(o), "en") == 0 || network_connected(o); if (o->page == 8) ok = valid_user(o); gtk_widget_set_sensitive(o->next, ok); gtk_widget_set_visible(o->lock_icon, !ok); }

static gboolean validate_page(Oobe *o) { refresh_next_sensitivity(o); if (!gtk_widget_get_sensitive(o->next)) { gtk_label_set_text(GTK_LABEL(o->status), tr(o, "locked")); return FALSE; } if (o->page == 5 && o->network_security && o->network_security[0] && strlen(gtk_editable_get_text(GTK_EDITABLE(o->network_password))) < 1) { gtk_label_set_text(GTK_LABEL(o->status), tr(o, "locked")); return FALSE; } return TRUE; }
static void update_summary(Oobe *o) { const char *edition = EDITION_CODES[selected_index(o->edition_dropdown)]; const char *language = selected_language(o); const char *emoji = o->emoji_choices && selected_index(o->emoji_dropdown) < o->emoji_choices->len ? ((EmojiChoice *)g_ptr_array_index(o->emoji_choices, selected_index(o->emoji_dropdown)))->code : "fluent-compatible";     const char *partition_target = o->partition_target && gtk_drop_down_get_selected_item(GTK_DROP_DOWN(o->partition_target)) ? gtk_string_object_get_string(GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(GTK_DROP_DOWN(o->partition_target)))) : "pendiente";
    const char *partition_plan = o->partition_plan && gtk_drop_down_get_selected_item(GTK_DROP_DOWN(o->partition_plan)) ? gtk_string_object_get_string(GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(GTK_DROP_DOWN(o->partition_plan)))) : "automático";
    const char *format_plan = o->format_plan && gtk_drop_down_get_selected_item(GTK_DROP_DOWN(o->format_plan)) ? gtk_string_object_get_string(GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(GTK_DROP_DOWN(o->format_plan)))) : "no formatear";
    gchar *text = g_strdup_printf("Sistema: Influent Danenone\nIdioma: %s\nEdición: %s\nDispositivo: %s\nPartición: %s\nPlan: %s\nTamaño de redimensionado: %.0f GiB\nFormato: %s\nConfirmación de formato: %s\nFluthin: %s\nDebian: %s\nEmojis: %s\nOEM: identidad local mediante hash, sin serial en claro", language, edition, gtk_drop_down_get_selected_item(GTK_DROP_DOWN(o->disk_dropdown)) ? gtk_string_object_get_string(GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(GTK_DROP_DOWN(o->disk_dropdown)))) : "pendiente", partition_target, partition_plan, o->resize_size ? gtk_spin_button_get_value(GTK_SPIN_BUTTON(o->resize_size)) : 0, format_plan, o->format_confirm && gtk_check_button_get_active(GTK_CHECK_BUTTON(o->format_confirm)) ? "sí" : "no", csv_checks(o->fluthin_checks), csv_checks(o->debian_checks), emoji);
 gtk_label_set_text(GTK_LABEL(o->summary), text); g_free(text); }

static void set_page(Oobe *o, int page) { o->page = CLAMP(page, 0, PAGE_COUNT - 1); const char *names[] = {"welcome", "language", "license", "edition", "storage", "network", "fluthin", "debian", "user", "appearance", "notch", "privacy", "emoji", "summary", "install"}; const char *status[] = {"oobe_status", "network_connect", "edition_body", "edition_body", "storage_safe", "network_connect", "fluthin_body", "debian_body", "user_body", "appearance_body", "notch_body", "privacy_body", "emoji_body", "summary_body", "install_body"}; gtk_stack_set_visible_child_name(GTK_STACK(o->stack), names[o->page]); gtk_label_set_text(GTK_LABEL(o->stage), g_strdup_printf("%s %d de %d", tr(o, "step"), o->page + 1, PAGE_COUNT)); gtk_label_set_text(GTK_LABEL(o->status), tr(o, status[o->page])); gtk_widget_set_visible(o->back, o->page > 0 && o->page < PAGE_COUNT - 1); gtk_button_set_label(GTK_BUTTON(o->next), o->page == PAGE_COUNT - 1 ? tr(o, "finish") : tr(o, "next")); if (o->page == 1) scan_networks(o); if (o->page == 13) update_summary(o); refresh_next_sensitivity(o); }

static void connect_selected_network(Oobe *o) { if (!o->network_ssid || !o->network_ssid[0]) return; const char *password = gtk_editable_get_text(GTK_EDITABLE(o->network_password)); gchar *argv[] = {"nmcli", "dev", "wifi", "connect", o->network_ssid, "password", (char *)password, NULL}; GError *error = NULL; GSubprocess *process = g_subprocess_newv((const gchar *const *)argv, G_SUBPROCESS_FLAGS_NONE, &error); if (process) { g_subprocess_wait_check(process, NULL, NULL); g_object_unref(process); } if (error) g_error_free(error); }
static void next_clicked(GtkButton *button, gpointer data) { (void)button; Oobe *o = data; if (o->page < PAGE_COUNT - 1) { if (!validate_page(o)) return; if (o->page == 5) connect_selected_network(o); if (o->page == 13) { if (network_connected(o) && danedesk_recovery_required()) { gtk_label_set_text(GTK_LABEL(o->status), "Este DaneDesk requiere recuperación autorizada por su propietario antes de instalar."); return; } save_preferences(o); } set_page(o, o->page + 1); } else { save_preferences(o); GError *error = NULL; gint status = 1; if (g_file_test("/usr/local/bin/influent-oobe-apply-selection", G_FILE_TEST_IS_EXECUTABLE)) g_spawn_command_line_sync("/usr/local/bin/influent-oobe-apply-selection", NULL, NULL, &status, &error); if (error) g_error_free(error); if (g_file_test("/usr/local/bin/influent-oem-id", G_FILE_TEST_IS_EXECUTABLE)) g_spawn_command_line_sync("/usr/local/bin/influent-oem-id --refresh", NULL, NULL, &status, &error); if (error) g_error_free(error); gtk_widget_set_visible(o->content, FALSE); gtk_widget_set_visible(o->notch, FALSE); gtk_widget_set_visible(o->final_splash, TRUE); o->final_timer = g_timeout_add(40, final_step, o->final_progress); } }
static void back_clicked(GtkButton *button, gpointer data) { (void)button; Oobe *o = data; if (o->page > 0) set_page(o, o->page - 1); }
static void language_changed(GtkDropDown *dropdown, GParamSpec *pspec, gpointer data) { (void)dropdown; (void)pspec; Oobe *o = data; g_free(o->language); o->language = g_strdup(selected_language(o)); load_runtime_locale(o); apply_language(o); save_preferences(o); refresh_next_sensitivity(o); }
static void theme_changed(GtkCheckButton *button, gpointer data) { Oobe *o = data; if (gtk_check_button_get_active(button)) { if (button == GTK_CHECK_BUTTON(o->theme_dark)) gtk_widget_add_css_class(o->root, "dark"); else gtk_widget_remove_css_class(o->root, "dark"); update_accent_css(o); save_preferences(o); } }
static gboolean lock_pulse(gpointer data) { Oobe *o = data; if (!gtk_widget_get_visible(o->lock_icon)) return G_SOURCE_CONTINUE; gtk_widget_set_opacity(o->lock_icon, gtk_widget_get_opacity(o->lock_icon) > 0.65 ? 0.35 : 1.0); return G_SOURCE_CONTINUE; }
static void notch_changed(GtkCheckButton *button, gpointer data) { Oobe *o = data; if (!gtk_check_button_get_active(button)) return; gboolean dynamic = button == GTK_CHECK_BUTTON(o->dynamic_notch_toggle); gtk_widget_set_size_request(o->notch, dynamic ? 220 : 360, 28); if (dynamic) gtk_widget_add_css_class(o->notch, "dynamic-notch"); else gtk_widget_remove_css_class(o->notch, "dynamic-notch"); save_preferences(o); }

static gboolean final_step(gpointer data) { gtk_progress_bar_pulse(GTK_PROGRESS_BAR(data)); return G_SOURCE_CONTINUE; }
static gboolean marquee_step(gpointer data) { Oobe *o = data; gtk_progress_bar_pulse(GTK_PROGRESS_BAR(o->splash_progress)); return G_SOURCE_CONTINUE; }
static gboolean intro_step(gpointer data) { Oobe *o = data; if (o->intro_opacity < 1.0) { o->intro_opacity = MIN(1.0, o->intro_opacity + 0.08); gtk_widget_set_opacity(o->content, o->intro_opacity); return G_SOURCE_CONTINUE; } o->brand_reveal = MIN(1.0, o->brand_reveal + 0.08); gtk_widget_set_opacity(o->brand, o->brand_reveal); if (o->brand_reveal >= 1.0) return G_SOURCE_REMOVE; return G_SOURCE_CONTINUE; }
static gboolean finish_splash(gpointer data) { Oobe *o = data; if (o->marquee_timer) { g_source_remove(o->marquee_timer); o->marquee_timer = 0; } gtk_widget_set_visible(o->splash, FALSE); o->intro_timer = g_timeout_add(30, intro_step, o); return G_SOURCE_REMOVE; }

static void load_css(Oobe *o) {
    (void)o;
    const char *css =
        "window { background: #dfe9e6; color: #14231f; }"
        ".oobe-wash { background: rgba(255,255,255,.18); }"
        ".oobe-splash, .final-splash { background: transparent; }"
        ".splash-panel { background: transparent; border: 0; padding: 0; }"
        ".splash-progress trough { min-height: 3px; border: 0; border-radius: 0; background: rgba(255,255,255,.38); }"
        ".notch { background: #111a19; border: 0; border-radius: 0 0 14px 14px; min-height: 28px; }"
        ".dynamic-notch { border-radius: 0 0 18px 18px; }"
        ".oobe-modal { background: rgba(255,255,255,.80); border: 1px solid rgba(255,255,255,.92); border-radius: 4px; padding: 20px 24px 18px 24px; box-shadow: 0 18px 50px rgba(13,45,42,.18); }"
        ".identity { min-height: 64px; }"
        ".brand { color: #17342d; font-family: Roboto, 'Noto Sans', sans-serif; font-size: 26px; font-weight: 500; }"
        ".stage { color: rgba(23,52,45,.62); font-size: 13px; }"
        ".oobe-page { background: rgba(255,255,255,.22); border: 1px solid rgba(255,255,255,.58); padding: 12px; } .oobe-footer { min-height: 42px; } scrolledwindow undershoot { background: transparent; } .storage-page { padding-top: 8px; padding-bottom: 8px; } .storage-page .body-text { font-size: 14px; } .storage-page .dropdown { padding: 5px 8px; } .storage-device-list, .storage-options { background: rgba(255,255,255,.22); border: 1px solid rgba(255,255,255,.58); padding: 4px; } .storage-row { background: rgba(255,255,255,.34); border-bottom: 1px solid rgba(255,255,255,.54); } .storage-row:last-child { border-bottom: 0; } .storage-row:selected { background: rgba(0,185,130,.18); border-left: 4px solid #00b982; } .storage-device-name, .storage-partition-name { color: #17342d; font-size: 14px; font-weight: 700; } .storage-partition-name { font-size: 13px; font-weight: 600; } .storage-device-details { color: rgba(20,45,39,.64); font-size: 12px; } .storage-tools { background: rgba(255,255,255,.24); border: 1px solid rgba(255,255,255,.54); padding: 5px; } .storage-tool { min-width: 96px; padding: 3px 7px; background: rgba(255,255,255,.42); } .storage-tool-label { color: #17342d; font-size: 11px; font-weight: 600; }"
        ".page-title { color: #17342d; font-size: 18px; font-weight: 700; }"
        ".body-text { color: rgba(20,45,39,.78); font-size: 15px; }"
        ".status, .network-status, .hint-text, .field-label { color: rgba(20,45,39,.68); font-size: 14px; }"
        ".dropdown, .input { padding: 10px; background: rgba(255,255,255,.40); border: 1px solid rgba(255,255,255,.70); }"
        ".summary-card, .status-card, .network-list { background: rgba(255,255,255,.34); border: 1px solid rgba(255,255,255,.64); padding: 16px; }"
        ".network-row { border-left: 4px solid transparent; }"
        ".network-row:selected { border-left-color: #00b982; background: rgba(0,185,130,.16); }"
        "button { padding: 10px 17px; border-radius: 3px; border: 1px solid rgba(255,255,255,.82); box-shadow: 0 4px 14px rgba(31,75,66,.12), inset 0 1px 0 rgba(255,255,255,.88); }"
        "button.primary { background: rgba(0,184,132,.84); color: #07352a; border-bottom: 2px solid #008a5a; font-weight: 700; }"
        "button.secondary { background: rgba(255,255,255,.34); color: #17342d; border-bottom: 2px solid rgba(0,112,86,.42); }"
        ".install-progress trough { min-height: 6px; border-radius: 0; background: rgba(20,65,55,.16); }"
        ".final-title { color: #17342d; font-size: 20px; font-weight: 700; }"
        ".dark .oobe-modal { background: rgba(20,29,28,.88); border-color: rgba(255,255,255,.22); }"
        ".dark .oobe-page, .dark .summary-card, .dark .status-card, .dark .network-list { background: rgba(13,23,22,.42); border-color: rgba(255,255,255,.18); } .dark .storage-device-list, .dark .storage-options, .dark .storage-tools { background: rgba(13,23,22,.32); border-color: rgba(255,255,255,.18); } .dark .storage-row { background: rgba(13,23,22,.42); border-color: rgba(255,255,255,.12); } .dark .storage-device-name, .dark .storage-partition-name { color: #ecf8f3; } .dark .storage-device-details { color: rgba(236,248,243,.66); }"
        ".dark .page-title, .dark .body-text, .dark .status, .dark .network-status, .dark .hint-text, .dark .field-label, .dark .brand, .dark .summary-card { color: #ecf8f3; }";
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, css);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

static void activate(GtkApplication *app, gpointer data) {
    (void)data; Oobe *o = g_new0(Oobe, 1); o->app = app; o->accent_index = 0; o->language = g_strdup("en"); o->bindings = g_ptr_array_new(); load_runtime_locale(o);
    o->window = gtk_application_window_new(app); gtk_window_set_default_size(GTK_WINDOW(o->window), 1280, 800); gtk_window_fullscreen(GTK_WINDOW(o->window)); o->overlay = gtk_overlay_new(); GtkWidget *wallpaper = gtk_picture_new_for_filename(asset(OOBE_WALLPAPER_PATH, LOCAL_WALLPAPER_PATH)); gtk_picture_set_content_fit(GTK_PICTURE(wallpaper), GTK_CONTENT_FIT_COVER); gtk_widget_set_hexpand(wallpaper, TRUE); gtk_widget_set_vexpand(wallpaper, TRUE); gtk_overlay_set_child(GTK_OVERLAY(o->overlay), wallpaper); load_css(o);
    GtkWidget *wash = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); gtk_widget_add_css_class(wash, "oobe-wash"); gtk_widget_set_hexpand(wash, TRUE); gtk_widget_set_vexpand(wash, TRUE); gtk_overlay_add_overlay(GTK_OVERLAY(o->overlay), wash); o->root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); gtk_widget_set_name(o->root, "danenone-root"); gtk_widget_set_hexpand(o->root, TRUE); gtk_widget_set_vexpand(o->root, TRUE); gtk_overlay_add_overlay(GTK_OVERLAY(o->overlay), o->root); o->notch = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); gtk_widget_add_css_class(o->notch, "notch"); gtk_widget_set_halign(o->notch, GTK_ALIGN_CENTER); gtk_widget_set_valign(o->notch, GTK_ALIGN_START); gtk_widget_set_size_request(o->notch, 360, 28); gtk_overlay_add_overlay(GTK_OVERLAY(o->overlay), o->notch);
    o->splash = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); gtk_widget_add_css_class(o->splash, "oobe-splash"); gtk_widget_set_hexpand(o->splash, TRUE); gtk_widget_set_vexpand(o->splash, TRUE); GtkWidget *sp = gtk_box_new(GTK_ORIENTATION_VERTICAL, 22); gtk_widget_add_css_class(sp, "splash-panel"); gtk_widget_set_halign(sp, GTK_ALIGN_CENTER); gtk_widget_set_valign(sp, GTK_ALIGN_CENTER); gtk_box_append(GTK_BOX(sp), scaled_picture(asset(BRAND_LOGO_PATH, LOCAL_LOGO_PATH), 128)); o->splash_progress = gtk_progress_bar_new(); gtk_widget_add_css_class(o->splash_progress, "splash-progress"); gtk_widget_set_size_request(o->splash_progress, 220, 6); gtk_box_append(GTK_BOX(sp), o->splash_progress); gtk_box_append(GTK_BOX(o->splash), sp); gtk_overlay_add_overlay(GTK_OVERLAY(o->overlay), o->splash);
    o->content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6); gtk_widget_set_halign(o->content, GTK_ALIGN_CENTER); gtk_widget_set_valign(o->content, GTK_ALIGN_START); gtk_widget_set_opacity(o->content, 0.0); gtk_widget_set_margin_top(o->content, 34); gtk_widget_set_margin_bottom(o->content, 8); o->identity = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10); gtk_widget_add_css_class(o->identity, "identity"); gtk_widget_set_halign(o->identity, GTK_ALIGN_CENTER); gtk_widget_set_margin_top(o->identity, 42); o->cube = scaled_picture(asset(BRAND_LOGO_PATH, LOCAL_LOGO_PATH), 64); o->brand = gtk_label_new("Danenone"); gtk_widget_add_css_class(o->brand, "brand"); gtk_widget_set_opacity(o->brand, 0.0); gtk_box_append(GTK_BOX(o->identity), o->cube); gtk_box_append(GTK_BOX(o->identity), o->brand); gtk_box_append(GTK_BOX(o->content), o->identity);
    o->modal = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8); gtk_widget_add_css_class(o->modal, "oobe-modal"); gtk_widget_set_size_request(o->modal, 780, 600); gtk_widget_set_vexpand(o->modal, FALSE); o->stack = gtk_stack_new(); gtk_stack_set_transition_type(GTK_STACK(o->stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT); gtk_stack_set_transition_duration(GTK_STACK(o->stack), 260); gtk_widget_set_vexpand(o->stack, TRUE); gtk_widget_set_hexpand(o->stack, TRUE);
    gtk_stack_add_named(GTK_STACK(o->stack), base_page(o, "welcome_title", "welcome_body"), "welcome"); gtk_stack_add_named(GTK_STACK(o->stack), language_page(o), "language"); gtk_stack_add_named(GTK_STACK(o->stack), license_page(o), "license"); gtk_stack_add_named(GTK_STACK(o->stack), edition_page(o), "edition"); gtk_stack_add_named(GTK_STACK(o->stack), storage_page(o), "storage"); gtk_stack_add_named(GTK_STACK(o->stack), network_page(o), "network"); gtk_stack_add_named(GTK_STACK(o->stack), fluthin_page(o), "fluthin"); gtk_stack_add_named(GTK_STACK(o->stack), debian_page(o), "debian"); gtk_stack_add_named(GTK_STACK(o->stack), user_page(o), "user"); gtk_stack_add_named(GTK_STACK(o->stack), appearance_page(o), "appearance"); gtk_stack_add_named(GTK_STACK(o->stack), notch_page(o), "notch"); gtk_stack_add_named(GTK_STACK(o->stack), privacy_page(o), "privacy"); gtk_stack_add_named(GTK_STACK(o->stack), emoji_page(o), "emoji"); gtk_stack_add_named(GTK_STACK(o->stack), summary_page(o), "summary"); gtk_stack_add_named(GTK_STACK(o->stack), install_page(o), "install"); GtkWidget *stack_scroll = gtk_scrolled_window_new(); gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(stack_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC); gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(stack_scroll), FALSE); gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(stack_scroll), 410); gtk_scrolled_window_set_max_content_height(GTK_SCROLLED_WINDOW(stack_scroll), 440); gtk_widget_set_vexpand(stack_scroll, TRUE); gtk_widget_set_hexpand(stack_scroll, TRUE); gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(stack_scroll), o->stack); gtk_box_append(GTK_BOX(o->modal), stack_scroll);
    o->status = label_for(o, "oobe_status", "status"); gtk_widget_set_margin_top(o->status, 2); gtk_widget_set_margin_bottom(o->status, 2); gtk_box_append(GTK_BOX(o->modal), o->status); GtkWidget *footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8); gtk_widget_add_css_class(footer, "oobe-footer"); o->repair = button_for(o, "repair", "secondary"); o->back = button_for(o, "back", "secondary"); o->next = button_for(o, "next", "primary"); o->lock_icon = gtk_image_new_from_icon_name("object-locked-symbolic"); gtk_widget_add_css_class(o->lock_icon, "lock-icon"); GtkWidget *next_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6); gtk_box_append(GTK_BOX(next_box), o->lock_icon); gtk_box_append(GTK_BOX(next_box), o->next); gtk_box_append(GTK_BOX(footer), o->repair); gtk_box_append(GTK_BOX(footer), o->back); GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); gtk_widget_set_hexpand(spacer, TRUE); gtk_box_append(GTK_BOX(footer), spacer); gtk_box_append(GTK_BOX(footer), next_box); gtk_box_append(GTK_BOX(o->modal), footer); gtk_box_append(GTK_BOX(o->content), o->modal); o->stage = gtk_label_new(""); gtk_widget_add_css_class(o->stage, "stage"); gtk_widget_set_halign(o->stage, GTK_ALIGN_CENTER); gtk_widget_set_margin_top(o->stage, 4); gtk_box_append(GTK_BOX(o->content), o->stage); gtk_box_append(GTK_BOX(o->root), o->content);
    o->final_splash = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18); gtk_widget_add_css_class(o->final_splash, "final-splash"); gtk_widget_set_hexpand(o->final_splash, TRUE); gtk_widget_set_vexpand(o->final_splash, TRUE); gtk_widget_set_size_request(o->final_splash, 300, 160); gtk_widget_set_halign(o->final_splash, GTK_ALIGN_CENTER); gtk_widget_set_valign(o->final_splash, GTK_ALIGN_CENTER); gtk_box_append(GTK_BOX(o->final_splash), scaled_picture(asset(BRAND_LOGO_PATH, LOCAL_LOGO_PATH), 96)); gtk_box_append(GTK_BOX(o->final_splash), label_for(o, "install_title", "final-title")); o->final_progress = gtk_progress_bar_new(); gtk_widget_add_css_class(o->final_progress, "splash-progress"); gtk_widget_set_size_request(o->final_progress, 220, 6); gtk_box_append(GTK_BOX(o->final_splash), o->final_progress); gtk_widget_set_visible(o->final_splash, FALSE); gtk_overlay_add_overlay(GTK_OVERLAY(o->overlay), o->final_splash);
    g_signal_connect(o->next, "clicked", G_CALLBACK(next_clicked), o); g_signal_connect(o->back, "clicked", G_CALLBACK(back_clicked), o); g_signal_connect(o->language_dropdown, "notify::selected", G_CALLBACK(language_changed), o); g_signal_connect(o->theme_light, "toggled", G_CALLBACK(theme_changed), o); g_signal_connect(o->theme_dark, "toggled", G_CALLBACK(theme_changed), o); g_signal_connect(o->full_notch_toggle, "toggled", G_CALLBACK(notch_changed), o); g_signal_connect(o->dynamic_notch_toggle, "toggled", G_CALLBACK(notch_changed), o); g_signal_connect(o->license_accept, "toggled", G_CALLBACK(refresh_next_cb), o); g_signal_connect(o->username, "changed", G_CALLBACK(refresh_next_cb), o); g_signal_connect(o->password, "changed", G_CALLBACK(refresh_next_cb), o); gtk_window_set_child(GTK_WINDOW(o->window), o->overlay); gtk_window_present(GTK_WINDOW(o->window)); o->splash_timer = g_timeout_add(4500, finish_splash, o); o->marquee_timer = g_timeout_add(34, marquee_step, o); o->lock_timer = g_timeout_add(600, lock_pulse, o); update_accent_css(o); const char *forced_language = g_getenv("DANENONE_TEST_LANGUAGE"); guint forced_index = language_index(o, forced_language); if (forced_index != GTK_INVALID_LIST_POSITION) gtk_drop_down_set_selected(GTK_DROP_DOWN(o->language_dropdown), forced_index); const char *forced_page = g_getenv("DANENONE_OOBE_PAGE"); int initial_page = forced_page ? CLAMP((int)g_ascii_strtoll(forced_page, NULL, 10), 0, PAGE_COUNT - 1) : 0; set_page(o, initial_page);
}

int main(int argc, char **argv) { setlocale(LC_ALL, ""); GtkApplication *app = gtk_application_new("com.influent.danenone.firstboot.gtk4", G_APPLICATION_DEFAULT_FLAGS); g_signal_connect(app, "activate", G_CALLBACK(activate), NULL); int status = g_application_run(G_APPLICATION(app), argc, argv); g_object_unref(app); return status; }
