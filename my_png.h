#ifndef __MY_PNG_H
#define __MY_PNG_H

#include <gtk/gtk.h>

struct image_info;

void do_png_save(image_info* img, GtkWidget* parent);
void my_png_save_img(image_info* img, char* filename);

#endif /* __MY_PNG_H */
