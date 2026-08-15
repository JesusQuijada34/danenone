#include <gtk/gtk.h>

static guint page = 0;
static const char *titles[] = {"Bienvenido a Influent Danenone", "Tu escritorio", "Barra y notch", "Listo para comenzar"};
static const char *descriptions[] = {
    "Un escritorio Linux diseñado para sentirse familiar, fluido y directo.",
    "Accede a tus aplicaciones desde un espacio compacto con el fondo del arroyo.",
    "La barra inferior reúne tus herramientas y el notch mantiene visibles los estados del sistema.",
    "Cierra este recorrido y empieza a usar Danenone. Puedes volver a abrirlo desde el menú."
};

static void render(GtkLabel *title, GtkLabel *description, GtkLabel *counter) {
    char number[32];
    gtk_label_set_text(title, titles[page]);
    gtk_label_set_text(description, descriptions[page]);
    g_snprintf(number, sizeof(number), "%u / 4", page + 1);
    gtk_label_set_text(counter, number);
}

static void next_page(GtkButton *button, gpointer data) {
    (void)button;
    GtkWidget **widgets = data;
    if (page < 3) page++;
    render(GTK_LABEL(widgets[0]), GTK_LABEL(widgets[1]), GTK_LABEL(widgets[2]));
}

static void activate(GtkApplication *app, gpointer data) {
    (void)data;
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Tour de Influent Danenone");
    gtk_window_set_default_size(GTK_WINDOW(window), 620, 420);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);
    gtk_widget_set_margin_start(box, 44);
    gtk_widget_set_margin_end(box, 44);
    gtk_widget_set_margin_top(box, 42);
    gtk_widget_set_margin_bottom(box, 36);
    GtkWidget *title = gtk_label_new(NULL);
    GtkWidget *description = gtk_label_new(NULL);
    GtkWidget *counter = gtk_label_new(NULL);
    gtk_widget_set_name(title, "tour-title");
    gtk_widget_set_name(description, "tour-description");
    gtk_widget_set_name(counter, "tour-counter");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_label_set_xalign(GTK_LABEL(description), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(description), TRUE);
    gtk_box_pack_start(GTK_BOX(box), counter, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), description, TRUE, TRUE, 0);
    GtkWidget *next = gtk_button_new_with_label("Continuar");
    gtk_widget_set_name(next, "tour-next");
    gtk_box_pack_end(GTK_BOX(box), next, FALSE, FALSE, 0);
    GtkWidget *widgets[] = {title, description, counter};
    g_signal_connect(next, "clicked", G_CALLBACK(next_page), widgets);
    gtk_container_add(GTK_CONTAINER(window), box);
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css, "window { background: #0a1628; color: #f3f7ff; } #tour-title { font-size: 28px; font-weight: 700; } #tour-description { font-size: 17px; color: #bed2ec; } #tour-counter { color: #7eaeed; } button { background: #4f84c4; color: white; border-radius: 12px; padding: 12px 24px; }", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    render(GTK_LABEL(title), GTK_LABEL(description), GTK_LABEL(counter));
    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.influent.danenone.tour", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
