#ifndef COLOR_DLG_H
#define COLOR_DLG_H

#include <gtk/gtk.h>

struct image_info;

struct color_dialog
{
    GtkWidget* dialog;

    GtkWidget* in_label;
    GtkWidget* out_label;

    GtkWidget* ok_button;
    GtkWidget* apply_button;

    image_info* img;
};

void color_dlg_new(color_dialog** ptr, image_info* img);

#endif
