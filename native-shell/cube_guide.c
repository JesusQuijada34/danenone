#include "cube_guide.h"
#include <cairo.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct _DanenoneCube {
    GtkWidget *root;
    GtkWidget *drawing;
    GtkWidget *bubble;
    DanenoneCubeState state;
    gdouble phase;
    guint timer_id;
} DanenoneCube;

static const char *state_label(DanenoneCubeState state) {
    switch (state) {
        case DANENONE_CUBE_TALKING: return "Danenone dice";
        case DANENONE_CUBE_WHISPER: return "Danenone susurra";
        case DANENONE_CUBE_THINKING: return "Danenone piensa";
        case DANENONE_CUBE_POINTING: return "Danenone señala";
        case DANENONE_CUBE_CELEBRATING: return "Danenone celebra";
        case DANENONE_CUBE_IDLE:
        default: return "Danenone";
    }
}

static gboolean tick_cb(gpointer data) {
    DanenoneCube *cube = data;
    cube->phase += 0.075;
    if (cube->phase > 1000.0) cube->phase = 0.0;
    gtk_widget_queue_draw(cube->drawing);
    return G_SOURCE_CONTINUE;
}

static void draw_face(cairo_t *cr, gdouble bob, DanenoneCubeState state, gdouble phase) {
    cairo_save(cr);
    cairo_translate(cr, 80.0, 28.0 + bob);
    cairo_move_to(cr, 0, 48);
    cairo_line_to(cr, 54, 15);
    cairo_line_to(cr, 108, 48);
    cairo_line_to(cr, 54, 82);
    cairo_close_path(cr);
    cairo_set_source_rgba(cr, 0.48, 0.73, 1.0, 0.82);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.9, 0.97, 1.0, 0.95);
    cairo_set_line_width(cr, 2.0);
    cairo_stroke(cr);

    cairo_move_to(cr, 0, 48);
    cairo_line_to(cr, 0, 110);
    cairo_line_to(cr, 54, 144);
    cairo_line_to(cr, 54, 82);
    cairo_close_path(cr);
    cairo_set_source_rgba(cr, 0.25, 0.5, 0.9, 0.7);
    cairo_fill_preserve(cr);
    cairo_stroke(cr);

    cairo_move_to(cr, 54, 82);
    cairo_line_to(cr, 108, 48);
    cairo_line_to(cr, 108, 110);
    cairo_line_to(cr, 54, 144);
    cairo_close_path(cr);
    cairo_set_source_rgba(cr, 0.52, 0.42, 0.95, 0.64);
    cairo_fill_preserve(cr);
    cairo_stroke(cr);

    const gdouble blink = fmod(fabs(sin(phase * 0.23)), 1.0);
    const gboolean eyes_closed = blink > 0.975;
    cairo_set_source_rgba(cr, 0.04, 0.1, 0.2, 0.92);
    cairo_set_line_width(cr, 4.0);
    if (eyes_closed) {
        cairo_move_to(cr, 34, 54); cairo_line_to(cr, 43, 54);
        cairo_move_to(cr, 65, 54); cairo_line_to(cr, 74, 54);
        cairo_stroke(cr);
    } else {
        cairo_arc(cr, 39, 54, 4, 0, 2 * G_PI);
        cairo_arc(cr, 70, 54, 4, 0, 2 * G_PI);
        cairo_fill(cr);
    }

    cairo_set_source_rgba(cr, 0.96, 0.99, 1.0, 0.92);
    cairo_set_line_width(cr, 2.5);
    if (state == DANENONE_CUBE_TALKING || state == DANENONE_CUBE_CELEBRATING) {
        cairo_arc(cr, 54, 72, 12, 0, G_PI);
        cairo_stroke(cr);
    } else if (state == DANENONE_CUBE_THINKING) {
        cairo_arc(cr, 54, 72, 8, G_PI, 2 * G_PI);
        cairo_stroke(cr);
    } else if (state == DANENONE_CUBE_WHISPER) {
        cairo_move_to(cr, 49, 72); cairo_line_to(cr, 59, 72); cairo_stroke(cr);
    } else {
        cairo_move_to(cr, 46, 72); cairo_curve_to(cr, 52, 77, 58, 77, 64, 72); cairo_stroke(cr);
    }

    if (state == DANENONE_CUBE_POINTING) {
        cairo_set_source_rgba(cr, 0.9, 0.76, 0.42, 0.95);
        cairo_set_line_width(cr, 3.0);
        cairo_move_to(cr, 106, 72); cairo_line_to(cr, 148, 54); cairo_line_to(cr, 138, 52);
        cairo_move_to(cr, 148, 54); cairo_line_to(cr, 141, 64); cairo_stroke(cr);
    }
    cairo_restore(cr);
}

static gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data) {
    (void)widget;
    DanenoneCube *cube = data;
    const gdouble bob = sin(cube->phase) * (cube->state == DANENONE_CUBE_CELEBRATING ? 8.0 : 3.0);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    draw_face(cr, bob, cube->state, cube->phase);
    return FALSE;
}

DanenoneCube *danenone_cube_new(void) {
    DanenoneCube *cube = g_new0(DanenoneCube, 1);
    cube->state = DANENONE_CUBE_IDLE;
    cube->root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    cube->drawing = gtk_drawing_area_new();
    cube->bubble = gtk_label_new(NULL);
    gtk_widget_set_size_request(cube->drawing, 280, 190);
    gtk_widget_set_name(cube->root, "cube-guide-root");
    gtk_widget_set_name(cube->bubble, "cube-guide-bubble");
    gtk_label_set_line_wrap(GTK_LABEL(cube->bubble), TRUE);
    gtk_label_set_justify(GTK_LABEL(cube->bubble), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(cube->root), cube->drawing, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cube->root), cube->bubble, FALSE, FALSE, 6);
    g_signal_connect(cube->drawing, "draw", G_CALLBACK(draw_cb), cube);
    danenone_cube_set_message(cube, "Estoy aquí para acompañarte.");
    return cube;
}

GtkWidget *danenone_cube_widget(DanenoneCube *cube) {
    return cube ? cube->root : NULL;
}

void danenone_cube_set_state(DanenoneCube *cube, DanenoneCubeState state) {
    if (!cube) return;
    cube->state = state;
    gtk_widget_set_tooltip_text(cube->drawing, state_label(state));
    gtk_widget_queue_draw(cube->drawing);
}

void danenone_cube_set_message(DanenoneCube *cube, const char *message) {
    if (!cube) return;
    gtk_label_set_text(GTK_LABEL(cube->bubble), message ? message : "");
    gtk_widget_set_tooltip_text(cube->bubble, message ? message : "");
}

void danenone_cube_start(DanenoneCube *cube) {
    if (!cube || cube->timer_id != 0) return;
    cube->timer_id = g_timeout_add(16, tick_cb, cube);
}

void danenone_cube_stop(DanenoneCube *cube) {
    if (!cube || cube->timer_id == 0) return;
    g_source_remove(cube->timer_id);
    cube->timer_id = 0;
}

void danenone_cube_free(DanenoneCube *cube) {
    if (!cube) return;
    danenone_cube_stop(cube);
    g_free(cube);
}
