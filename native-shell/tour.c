#include <cairo.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <string.h>
#include "cube_guide.h"

#define STEP_COUNT 6

typedef struct {
    const char *title;
    const char *body;
    const char *target;
    const char *accent;
} TourStep;

typedef struct {
    GtkApplication *app;
    GtkWidget *window;
    GtkWidget *title;
    GtkWidget *body;
    GtkWidget *counter;
    GtkWidget *target;
    GtkWidget *back;
    GtkWidget *next;
    GtkWidget *skip;
    GtkWidget *cube;
    DanenoneCube *guide_cube;
    guint step;
    gboolean replay;
} TourState;

static const TourStep STEPS[STEP_COUNT] = {
    {"Hola, soy el cubito Danenone", "Te acompañaré durante tu primer recorrido. Puedes avanzar, volver atrás u omitir el tour cuando quieras.", "Bienvenida", "#8fb8ff"},
    {"Este es tu escritorio", "Aquí encontrarás tus aplicaciones y tu fondo del arroyo. El escritorio se mantiene compacto para que siempre sepas dónde estás.", "Escritorio", "#88d7c1"},
    {"La barra inferior", "La barra centrada reúne las aplicaciones principales y se adapta al tamaño de la pantalla, como un punto de acceso familiar.", "Barra de tareas", "#a7a0ff"},
    {"El notch y el Centro de control", "El notch reserva el área superior y el Centro de control concentra conectividad, audio, brillo y notificaciones.", "Notch + Centro de control", "#f3bd79"},
    {"Archivos y tus aplicaciones", "Desde Archivos puedes navegar por tus rutas reales. El tour no inventa datos y nunca modifica tus documentos.", "Archivos", "#8fc8ff"},
    {"Ya puedes comenzar", "El cubito se quedará disponible desde el menú de ayuda. Disfruta Influent Danenone y vuelve al tour cuando lo necesites.", "Listo para comenzar", "#9ee6b4"}
};

static gchar *tour_directory(void) {
    const char *state = g_get_user_state_dir();
    return g_build_filename(state, "influent-danenone", NULL);
}

static gchar *tour_marker(const char *name) {
    gchar *directory = tour_directory();
    gchar *path = g_build_filename(directory, name, NULL);
    g_free(directory);
    return path;
}

static gboolean marker_exists(const char *name) {
    gchar *path = tour_marker(name);
    gboolean exists = g_file_test(path, G_FILE_TEST_EXISTS);
    g_free(path);
    return exists;
}

static void write_marker(const char *name) {
    gchar *directory = tour_directory();
    gchar *path = tour_marker(name);
    if (g_mkdir_with_parents(directory, 0700) == 0) {
        g_file_set_contents(path, "done\n", -1, NULL);
    }
    g_free(path);
    g_free(directory);
}

static void update_cube(TourState *state) {
    static const DanenoneCubeState states[STEP_COUNT] = {
        DANENONE_CUBE_TALKING,
        DANENONE_CUBE_POINTING,
        DANENONE_CUBE_POINTING,
        DANENONE_CUBE_WHISPER,
        DANENONE_CUBE_THINKING,
        DANENONE_CUBE_CELEBRATING
    };
    static const char *messages[STEP_COUNT] = {
        "Hola. Mira a tu alrededor; te mostraré lo esencial.",
        "Pienso que este espacio será tu punto de partida.",
        "Señalo la barra para que encuentres tus herramientas.",
        "Te susurro un detalle: el notch mantiene segura el área superior.",
        "Estoy pensando en cómo ayudarte a encontrar Archivos.",
        "Lo hicimos. Ya puedes disfrutar Danenone."
    };
    danenone_cube_set_state(state->guide_cube, states[state->step]);
    danenone_cube_set_message(state->guide_cube, messages[state->step]);
}

static void render(TourState *state) {
    const TourStep *step = &STEPS[state->step];
    gchar *counter = g_strdup_printf("%u / %u", state->step + 1, STEP_COUNT);
    gtk_label_set_text(GTK_LABEL(state->counter), counter);
    gtk_label_set_text(GTK_LABEL(state->title), step->title);
    gtk_label_set_text(GTK_LABEL(state->body), step->body);
    gtk_label_set_text(GTK_LABEL(state->target), step->target);
    gtk_widget_set_name(state->target, "tour-target");
    gtk_widget_set_name(state->next, state->step == STEP_COUNT - 1 ? "tour-finish" : "tour-next");
    gtk_button_set_label(GTK_BUTTON(state->next), state->step == STEP_COUNT - 1 ? "Disfrutar Danenone" : "Continuar");
    gtk_widget_set_sensitive(state->back, state->step > 0);
    update_cube(state);
    g_free(counter);
}

static void finish_tour(TourState *state) {
    write_marker("tour-complete");
    gtk_widget_destroy(state->window);
}

static void next_clicked(GtkButton *button, gpointer data) {
    (void)button;
    TourState *state = data;
    if (state->step >= STEP_COUNT - 1) {
        finish_tour(state);
        return;
    }
    state->step++;
    render(state);
}

static void back_clicked(GtkButton *button, gpointer data) {
    (void)button;
    TourState *state = data;
    if (state->step > 0) {
        state->step--;
        render(state);
    }
}

static void skip_clicked(GtkButton *button, gpointer data) {
    (void)button;
    TourState *state = data;
    write_marker("tour-skipped");
    gtk_widget_destroy(state->window);
}

static void window_destroy(GtkWidget *window, gpointer data) {
    (void)window;
    TourState *state = data;
    danenone_cube_free(state->guide_cube);
    g_free(state);
}

static void activate(GtkApplication *app, gpointer data) {
    TourState *state = g_new0(TourState, 1);
    state->app = app;
    state->replay = GPOINTER_TO_INT(data) != 0;
    state->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(state->window), "Primer arranque · Influent Danenone");
    gtk_window_set_default_size(GTK_WINDOW(state->window), 820, 560);
    gtk_window_set_position(GTK_WINDOW(state->window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(state->window), FALSE);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *notch = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(notch, "tour-notch");
    gtk_widget_set_size_request(notch, 160, 28);
    gtk_box_pack_start(GTK_BOX(root), notch, FALSE, FALSE, 0);

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 28);
    gtk_widget_set_margin_start(content, 34);
    gtk_widget_set_margin_end(content, 34);
    gtk_widget_set_margin_top(content, 24);
    gtk_widget_set_margin_bottom(content, 24);
    gtk_box_pack_start(GTK_BOX(root), content, TRUE, TRUE, 0);

    GtkWidget *cube_frame = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_name(cube_frame, "cube-card");
    gtk_widget_set_size_request(cube_frame, 280, -1);
    state->guide_cube = danenone_cube_new();
    state->cube = danenone_cube_widget(state->guide_cube);
    gtk_box_pack_start(GTK_BOX(cube_frame), state->cube, FALSE, FALSE, 10);
    danenone_cube_start(state->guide_cube);
    gtk_box_pack_start(GTK_BOX(content), cube_frame, FALSE, FALSE, 0);

    GtkWidget *panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_name(panel, "tour-panel");
    gtk_widget_set_hexpand(panel, TRUE);
    state->counter = gtk_label_new(NULL);
    state->title = gtk_label_new(NULL);
    state->body = gtk_label_new(NULL);
    state->target = gtk_label_new(NULL);
    gtk_widget_set_name(state->counter, "tour-counter");
    gtk_widget_set_name(state->title, "tour-title");
    gtk_widget_set_name(state->body, "tour-body");
    gtk_widget_set_name(state->target, "tour-target");
    gtk_label_set_xalign(GTK_LABEL(state->counter), 0.0);
    gtk_label_set_xalign(GTK_LABEL(state->title), 0.0);
    gtk_label_set_xalign(GTK_LABEL(state->body), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(state->body), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(state->body), 52);
    gtk_box_pack_start(GTK_BOX(panel), state->counter, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(panel), state->title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(panel), state->body, TRUE, TRUE, 0);
    GtkWidget *pointer = gtk_label_new("Señalando ahora");
    gtk_widget_set_name(pointer, "tour-pointer-label");
    gtk_box_pack_start(GTK_BOX(panel), pointer, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(panel), state->target, FALSE, FALSE, 0);

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    state->skip = gtk_button_new_with_label("Omitir");
    state->back = gtk_button_new_with_label("Atrás");
    state->next = gtk_button_new_with_label("Continuar");
    gtk_widget_set_name(state->skip, "tour-skip");
    gtk_widget_set_name(state->back, "tour-back");
    gtk_widget_set_name(state->next, "tour-next");
    gtk_box_pack_start(GTK_BOX(actions), state->skip, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(actions), state->next, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(actions), state->back, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(panel), actions, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), panel, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(state->window), root);

    g_signal_connect(state->next, "clicked", G_CALLBACK(next_clicked), state);
    g_signal_connect(state->back, "clicked", G_CALLBACK(back_clicked), state);
    g_signal_connect(state->skip, "clicked", G_CALLBACK(skip_clicked), state);
    g_signal_connect(state->window, "destroy", G_CALLBACK(window_destroy), state);

    GtkCssProvider *css = gtk_css_provider_new();
    const char *css_data =
        "window { background: #08111f; color: #f3f7ff; }"
        "#tour-notch { background: #02050a; border-radius: 0 0 16px 16px; }"
        "#cube-card, #tour-panel { background: rgba(29, 52, 83, 0.86); border: 1px solid rgba(180, 217, 255, 0.28); border-radius: 24px; padding: 22px; }"
        "#cube-guide { color: #bed2ec; font-size: 18px; font-weight: 600; }"
        "#tour-title { color: #f7fbff; font-size: 30px; font-weight: 700; }"
        "#tour-body { color: #c9d9ec; font-size: 17px; }"
        "#tour-counter, #tour-pointer-label { color: #82b5ff; font-size: 14px; font-weight: 700; }"
        "#tour-target { color: #f1c98e; font-size: 20px; font-weight: 700; padding: 14px; background: rgba(94, 119, 173, 0.34); border-radius: 14px; }"
        "button { color: #f5f8ff; background: rgba(89, 132, 204, 0.75); border: 1px solid rgba(185, 218, 255, 0.32); border-radius: 12px; padding: 10px 18px; }"
        "#tour-skip, #tour-back { background: rgba(53, 73, 105, 0.66); }"
        "#tour-finish { background: rgba(101, 176, 142, 0.9); }";
    gtk_css_provider_load_from_data(css, css_data, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);
    render(state);
    gtk_widget_show_all(state->window);
}

int main(int argc, char **argv) {
    gboolean replay = FALSE;
    for (int i = 1; i < argc; i++) {
        if (g_strcmp0(argv[i], "--replay") == 0 || g_strcmp0(argv[i], "--reset") == 0) {
            replay = TRUE;
        }
    }
    if (!replay && (marker_exists("tour-complete") || marker_exists("tour-skipped"))) {
        return 0;
    }
    GtkApplication *app = gtk_application_new("com.influent.danenone.tour", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), GINT_TO_POINTER(replay));

    /* GTK must receive only its own arguments; consume Danenone flags here. */
    char **gtk_argv = g_new0(char *, (gsize)argc + 1);
    int gtk_argc = 0;
    for (int i = 0; i < argc; i++) {
        if (g_strcmp0(argv[i], "--replay") == 0 || g_strcmp0(argv[i], "--reset") == 0) {
            continue;
        }
        gtk_argv[gtk_argc++] = argv[i];
    }
    int status = g_application_run(G_APPLICATION(app), gtk_argc, gtk_argv);
    g_free(gtk_argv);
    g_object_unref(app);
    return status;
}
