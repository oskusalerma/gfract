#ifndef __PAL_ROT_DLG_H
#define __PAL_ROT_DLG_H

#include <gtk/gtk.h>

struct image_info;

struct palette_rotation_dialog
{
    GtkWidget* window;

    GtkWidget* cycle_f;
    GtkWidget* cycle_b;
    GtkWidget* step_f;
    GtkWidget* step_b;
    GtkWidget* stop;
    
    image_info* img;
    int idle_id;
};

void palette_rotation_dlg_new(palette_rotation_dialog** ptr, image_info* img);

#endif
