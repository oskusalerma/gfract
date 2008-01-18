#ifndef __MAIN_H
#define __MAIN_H

#include <gtk/gtk.h>

class image_info;

gint do_palette_rotation(bool forward);
void main_refresh(void);
void draw_xor_rect(const GdkRectangle& rect);
void start_rendering(image_info* img);
void stop_rendering(image_info* img);
void image_size_changed();
void get_coords(double* x, double* y);
void tool_deactivate();

gint expose_event(GtkWidget* widget, GdkEventExpose* event, image_info* img);

#endif /* __MAIN_H */
