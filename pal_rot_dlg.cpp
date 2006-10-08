#include <gdk/gdkkeysyms.h>
#include "pal_rot_dlg.h"
#include "main.h"

static void pal_rot_destroy(GtkWidget* widget,
                            palette_rotation_dialog* dl);
static void cycle_forward(GtkWidget* widget,
                          palette_rotation_dialog* dl);
static void cycle_backward(GtkWidget* widget,
                           palette_rotation_dialog* dl);
static void step_forward(GtkWidget* widget,
                         palette_rotation_dialog* dl);
static void step_backward(GtkWidget* widget,
                          palette_rotation_dialog* dl);
static void cycle(palette_rotation_dialog* dl, bool forward);
static void cycle_stop(palette_rotation_dialog* dl);
static void stop(GtkWidget* widget, palette_rotation_dialog* dl);
static gint key_event(GtkWidget* widget, GdkEventKey* event,
                      palette_rotation_dialog* dl);

gint key_event(GtkWidget* widget, GdkEventKey* event,
               palette_rotation_dialog* dl)
{
    switch (event->keyval) {
    case GDK_plus:
    case GDK_KP_Add:
        step_forward(NULL, NULL);
        break;
    case GDK_minus:
    case GDK_KP_Subtract:
        step_backward(NULL, NULL);
        break;
    case GDK_f:
    case GDK_F:
        cycle(dl, true);
        break;
    case GDK_b:
    case GDK_B:
        cycle(dl, false);
        break;
    case GDK_s:
    case GDK_S:
        cycle_stop(dl);
        break;
    }

    return TRUE;
}

void cycle_forward(GtkWidget* widget,
                   palette_rotation_dialog* dl)
{
    cycle(dl, true);
}

void cycle_backward(GtkWidget* widget,
                    palette_rotation_dialog* dl)
{
    cycle(dl, false);

}

void cycle(palette_rotation_dialog* dl, bool forward)
{
    cycle_stop(dl);

    gtk_widget_set_sensitive(dl->stop, TRUE);
    gtk_widget_set_sensitive(forward ? dl->cycle_b : dl->cycle_f,
                             TRUE);
    gtk_widget_set_sensitive(forward ? dl->cycle_f : dl->cycle_b,
                             FALSE);
    dl->idle_id = g_idle_add((GtkFunction)do_palette_rotation,
                               (gpointer)forward);
}

void cycle_stop(palette_rotation_dialog* dl)
{
    if (dl->idle_id != -1)
        g_source_remove(dl->idle_id);
    gtk_widget_set_sensitive(dl->stop, FALSE);
    gtk_widget_set_sensitive(dl->cycle_f, TRUE);
    gtk_widget_set_sensitive(dl->cycle_b, TRUE);
}

void stop(GtkWidget* widget, palette_rotation_dialog* dl)
{
    cycle_stop(dl);
}

void step_forward(GtkWidget* widget, palette_rotation_dialog* dl)
{
    do_palette_rotation(TRUE);
}

void step_backward(GtkWidget* widget, palette_rotation_dialog* dl)
{
    do_palette_rotation(FALSE);
}

void pal_rot_destroy(GtkWidget* widget,
                     palette_rotation_dialog* dl)
{
    cycle_stop(dl);
    delete dl;
}

void palette_rotation_dlg_new(palette_rotation_dialog** ptr,
                              image_info* img)
{
    GtkWidget* hbox;
    palette_rotation_dialog* dl;

    dl = new palette_rotation_dialog;
    dl->img = img;
    dl->idle_id = -1;
    *ptr = dl;

    dl->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(GTK_OBJECT(dl->window), "destroy",
                       GTK_SIGNAL_FUNC(pal_rot_destroy),
                       dl);
    g_signal_connect(GTK_OBJECT(dl->window), "destroy",
                       GTK_SIGNAL_FUNC(gtk_widget_destroyed),
                       ptr);
    gtk_window_set_title(GTK_WINDOW(dl->window), "Palette cycling");
    gtk_window_set_resizable(GTK_WINDOW(dl->window), FALSE);
    g_signal_connect(GTK_OBJECT(dl->window), "key_press_event",
                       GTK_SIGNAL_FUNC(key_event), dl);

    gtk_container_set_border_width(GTK_CONTAINER(dl->window), 10);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(dl->window), hbox);


    dl->step_b = gtk_button_new_with_label(" < ");
    g_signal_connect(GTK_OBJECT(dl->step_b), "clicked",
                       GTK_SIGNAL_FUNC(step_backward),
                       dl);
    gtk_box_pack_start(GTK_BOX(hbox), dl->step_b, FALSE, FALSE, 0);

    dl->cycle_b = gtk_button_new_with_label(" << ");
    g_signal_connect(GTK_OBJECT(dl->cycle_b), "clicked",
                       GTK_SIGNAL_FUNC(cycle_backward),
                       dl);
    gtk_box_pack_start(GTK_BOX(hbox), dl->cycle_b, FALSE, FALSE, 0);


    dl->stop = gtk_button_new_with_label(" Stop ");
    g_signal_connect(GTK_OBJECT(dl->stop), "clicked",
                       GTK_SIGNAL_FUNC(stop),
                       dl);
    gtk_box_pack_start(GTK_BOX(hbox), dl->stop, FALSE, FALSE, 4);
    gtk_widget_set_sensitive(dl->stop, FALSE);

    dl->cycle_f = gtk_button_new_with_label(" >> ");
    g_signal_connect(GTK_OBJECT(dl->cycle_f), "clicked",
                       GTK_SIGNAL_FUNC(cycle_forward),
                       dl);
    gtk_box_pack_start(GTK_BOX(hbox), dl->cycle_f, FALSE, FALSE, 0);

    dl->step_f = gtk_button_new_with_label(" > ");
    g_signal_connect(GTK_OBJECT(dl->step_f), "clicked",
                       GTK_SIGNAL_FUNC(step_forward),
                       dl);
    gtk_box_pack_start(GTK_BOX(hbox), dl->step_f, FALSE, FALSE, 0);

    gtk_widget_show_all(dl->window);
}
