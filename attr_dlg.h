#ifndef __ATTR_DLG_H
#define __ATTR_DLG_H

#include "fractal_types.h"

struct image_attr_dialog
{
    GtkWidget* dialog;

    GtkWidget* width;
    GtkWidget* height;
    GtkWidget* aa;
    GtkWidget* text;
    GtkWidget* const_ra;
    GtkWidget* ok_button;
    GtkWidget* apply_button;
    double ratio;
};

void attr_dlg_new(image_attr_dialog** ptr, image_info* img);

#endif
