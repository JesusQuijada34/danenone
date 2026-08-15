#include <gtk/gtk.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <math.h>
#include <string.h>

#define SPLASH_DURATION_SECONDS 6.0

typedef struct {
    GtkWidget *window;
    GtkWidget *phase;
    GtkWidget *progress;
    gint64 started_at;
    guint timer_id;
} SplashState;

static const char *cube_path(void) {
    static char path[512];
    g_snprintf(path, sizeof(path), "/usr/share/influent/danenone-cube-splashboot.png");
    if (g_file_test(path, G_FILE_TEST_EXISTS)) return path;
    g_snprintf(path, sizeof(path), "/usr/share/icons/influent/danenone-cube-splashboot.png");
    if (g_file_test(path, G_FILE_TEST_EXISTS)) return path;
    g_snprintf(path, sizeof(path), "assets/danenone-cube/danenone-cube-splashboot.png");
    return path;
}

static const char *wallpaper_path(void) {
    static char path[512];
    g_snprintf(path, sizeof(path), "/usr/share/backgrounds/influent/danenone-river-wallpaper.jpg");
    if (g_file_test(path, G_FILE_TEST_EXISTS)) return path;
    g_snprintf(path, sizeof(path), "branding-v2/danenone-river-wallpaper.jpg");
    return path;
}

static GtkWidget *wallpaper_image(void) {
    GError *error = NULL;
    GdkPixbuf *source = gdk_pixbuf_new_from_file(wallpaper_path(), &error);
    if (!source) {
        if (error) g_error_free(error);
        return gtk_image_new();
    }
    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor = display ? gdk_display_get_primary_monitor(display) : NULL;
    GdkRectangle geometry = {0, 0, 1280, 800};
    if (monitor) gdk_monitor_get_geometry(monitor, &geometry);
    gint target_width = geometry.width;
    gint target_height = geometry.height;
    gint source_width = gdk_pixbuf_get_width(source);
    gint source_height = gdk_pixbuf_get_height(source);
    gdouble scale = MAX((gdouble)target_width / source_width, (gdouble)target_height / source_height);
    gint scaled_width = MAX(target_width, (gint)(source_width * scale));
    gint scaled_height = MAX(target_height, (gint)(source_height * scale));
    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(source, scaled_width, scaled_height, GDK_INTERP_BILINEAR);
    gint crop_x = MAX(0, (scaled_width - target_width) / 2);
    gint crop_y = MAX(0, (scaled_height - target_height) / 2);
    GdkPixbuf *crop = gdk_pixbuf_new_subpixbuf(scaled, crop_x, crop_y, target_width, target_height);
    GtkWidget *image = gtk_image_new_from_pixbuf(crop);
    gtk_widget_set_halign(image, GTK_ALIGN_FILL);
    gtk_widget_set_valign(image, GTK_ALIGN_FILL);
    g_object_unref(crop);
    g_object_unref(scaled);
    g_object_unref(source);
    return image;
}

static const char *phase_for_progress(double progress) {
    if (progress < 0.20) return "Preparando el entorno";
    if (progress < 0.45) return "Cargando la identidad Danenone";
    if (progress < 0.70) return "Conectando las superficies del escritorio";
    if (progress < 0.90) return "Ajustando el espacio de trabajo";
    return "Listo para comenzar";
}

static gboolean tick(gpointer data) {
    SplashState *state = data;
    double elapsed = (double)(g_get_monotonic_time() - state->started_at) / 1000000.0;
    double progress = elapsed / SPLASH_DURATION_SECONDS;
    if (progress > 1.0) progress = 1.0;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(state->progress), progress);
    gtk_label_set_text(GTK_LABEL(state->phase), phase_for_progress(progress));
    if (progress >= 1.0) {
        state->timer_id = 0;
        gtk_widget_destroy(state->window);
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

static gboolean prevent_close(GtkWidget *widget, GdkEvent *event, gpointer data) {
    (void)widget;
    (void)event;
    (void)data;
    return TRUE;
}

static void activate(GtkApplication *app, gpointer data) {
    (void)data;
    SplashState *state = g_new0(SplashState, 1);
    state->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(state->window), "Influent Danenone");
    gtk_window_set_decorated(GTK_WINDOW(state->window), FALSE);
    gtk_window_fullscreen(GTK_WINDOW(state->window));
    gtk_window_set_keep_above(GTK_WINDOW(state->window), TRUE);
    gtk_window_set_accept_focus(GTK_WINDOW(state->window), FALSE);
    g_signal_connect(state->window, "delete-event", G_CALLBACK(prevent_close), NULL);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);
    gtk_widget_set_name(root, "splash-root");
    gtk_widget_set_halign(root, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(root, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start(root, 40);
    gtk_widget_set_margin_end(root, 40);
    gtk_widget_set_margin_top(root, 40);
    gtk_widget_set_margin_bottom(root, 40);

    GtkWidget *image = gtk_image_new_from_file(cube_path());
    gtk_widget_set_name(image, "splash-cube");
    gtk_widget_set_size_request(image, 300, 300);
    gtk_box_pack_start(GTK_BOX(root), image, FALSE, FALSE, 0);

    GtkWidget *title = gtk_label_new("Influent Danenone");
    gtk_widget_set_name(title, "splash-title");
    gtk_box_pack_start(GTK_BOX(root), title, FALSE, FALSE, 0);

    state->phase = gtk_label_new("Preparando el entorno");
    gtk_widget_set_name(state->phase, "splash-phase");
    gtk_box_pack_start(GTK_BOX(root), state->phase, FALSE, FALSE, 0);

    state->progress = gtk_progress_bar_new();
    gtk_widget_set_name(state->progress, "splash-progress");
    gtk_widget_set_size_request(state->progress, 340, 5);
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(state->progress), FALSE);
    gtk_box_pack_start(GTK_BOX(root), state->progress, FALSE, FALSE, 6);

    GtkWidget *overlay = gtk_overlay_new();
    GtkWidget *background = wallpaper_image();
    gtk_container_add(GTK_CONTAINER(overlay), background);
    GtkWidget *veil = gtk_event_box_new();
    gtk_widget_set_name(veil, "splash-veil");
    gtk_widget_set_halign(veil, GTK_ALIGN_FILL);
    gtk_widget_set_valign(veil, GTK_ALIGN_FILL);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), veil);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), root);
    gtk_container_add(GTK_CONTAINER(state->window), overlay);

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "window { background: transparent; }"
        "#splash-root { background: rgba(7,11,23,0.40); border-radius: 28px; padding: 26px 40px; }"
        "#splash-veil { background: rgba(4,8,18,0.26); }"
        "#splash-cube { margin-bottom: 4px; }"
        "#splash-title { color: #f6f8ff; font-size: 28px; font-weight: 700; letter-spacing: 0.4px; }"
        "#splash-phase { color: rgba(219,230,249,0.72); font-size: 14px; }"
        "#splash-progress trough { min-height: 5px; border-radius: 99px; background: rgba(255,255,255,0.14); }"
        "#splash-progress progress { min-height: 5px; border-radius: 99px; background: #86b7ff; }",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    gtk_widget_show_all(state->window);
    state->started_at = g_get_monotonic_time();
    state->timer_id = g_timeout_add(16, tick, state);
    g_signal_connect_swapped(state->window, "destroy", G_CALLBACK(g_free), state);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.influent.danenone.splashboot", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
