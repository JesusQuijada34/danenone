#ifndef DANENONE_CUBE_GUIDE_H
#define DANENONE_CUBE_GUIDE_H

#include <gtk/gtk.h>

typedef enum {
    DANENONE_CUBE_IDLE,
    DANENONE_CUBE_TALKING,
    DANENONE_CUBE_WHISPER,
    DANENONE_CUBE_THINKING,
    DANENONE_CUBE_POINTING,
    DANENONE_CUBE_CELEBRATING
} DanenoneCubeState;

typedef struct _DanenoneCube DanenoneCube;

DanenoneCube *danenone_cube_new(void);
GtkWidget *danenone_cube_widget(DanenoneCube *cube);
void danenone_cube_set_state(DanenoneCube *cube, DanenoneCubeState state);
void danenone_cube_set_message(DanenoneCube *cube, const char *message);
void danenone_cube_start(DanenoneCube *cube);
void danenone_cube_stop(DanenoneCube *cube);
void danenone_cube_free(DanenoneCube *cube);

#endif
