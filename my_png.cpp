#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <png.h>
#include <gtk/gtk.h>
#include "externs.h"
#include "palette.h"
#include "my_png.h"

/* should be using autoconf... */
#if PNG_LIBPNG_VER <= 10002
#error Your libpng is too old. Upgrade to at least version 1.0.3
#endif

static GtkWidget* filesel = NULL;

static gboolean file_exists(char* filename);
static void ask_overwrite(image_info* img, char* filename);
static void save_file(image_info* img, char* filename);
static void overwrite_ok_cmd(GtkWidget* w, image_info* img);

void save_file(image_info* img, char* filename)
{
    FILE* fp;
    png_struct* png_ptr;
    png_info* info_ptr;
    gboolean pal;                /* TRUE if img is palettized */
    int i;
    png_color* png_pal = NULL;
    guchar* pal_data = NULL;
    guchar** row_p = NULL;
    
    if ((img->aa_factor == 1) && !img->palette_ip)
        pal = TRUE;
    else
        pal = FALSE;
    
    fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "Can't open file %s: %s\n", filename,
                strerror(errno));
        return;
    }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL,
                                      NULL);
    if (png_ptr == NULL) {
        fprintf(stderr, "Can't create png_ptr structure\n");
        fclose(fp);

        return;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        fprintf(stderr, "Can't create info_ptr structure\n");
        fclose(fp);
        png_destroy_write_struct(&png_ptr, NULL);

        return;
    }

    if (setjmp(png_ptr->jmpbuf)) {
        fprintf(stderr, "Internal error in libpng\n");
        if (png_pal)
            delete[] png_pal;
        if (row_p)
            delete[] row_p;
        if (pal_data)
            delete[] pal_data;
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);

        return;
    }

    png_init_io(png_ptr, fp);

    /* possible compression level setting here */
    /* png_set_compression_level(png_ptr, 1-9); */

    png_set_IHDR(png_ptr, info_ptr, img->user_width, img->user_height,
        8, pal ? PNG_COLOR_TYPE_PALETTE : PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);

    /* write palette */
    if (pal) {
        png_pal = new png_color[palette_size];
        for (i=0; i < (int)palette_size; i++) {
            png_pal[i].red = RED(palette[i]);
            png_pal[i].green = GREEN(palette[i]);
            png_pal[i].blue = BLUE(palette[i]);
        }
        png_set_PLTE(png_ptr, info_ptr, png_pal, palette_size);
    }

    png_write_info(png_ptr, info_ptr);
    if (!pal)
        png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);
    else
    {
        /* convert data to palette index format */
        
        int pixels = img->user_width * img->user_height;
        guchar* dst;
        double* src;

        pal_data = new uint8_t[pixels];
        dst = pal_data;
        src = img->raw_data;

        for (i=0; i < pixels; i++) {
            *dst = (guint32)(*src) % palette_size;

            src++;
            dst++;
        }
    }

    /* initialize row pointers */
    row_p = new uint8_t*[img->user_height];
    for (i=0; i < img->user_height; i++) {
        if (pal)
            row_p[i] = &(pal_data[i * img->user_width]);
        else
            row_p[i] = (uint8_t*)&(img->rgb_data[i * img->user_width]);
    }
    
    /* write image */
    png_write_image(png_ptr, row_p);

    png_write_end(png_ptr, info_ptr);
    if (pal) {
        delete[] png_pal;
        delete[] pal_data;
    }
    delete[] row_p;
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
}

gboolean file_exists(char* filename)
{
    FILE* fp = fopen(filename, "r");

    if (fp == NULL)
        return FALSE;

    fclose(fp);

    return TRUE;
}

void ask_overwrite(image_info* img, char* filename)
{
    char buf[256];
    GtkWidget* dl;
    GtkWidget* vbox;
    GtkWidget* tmp;
    
    snprintf(buf, 256, "%s exists, overwrite?", filename);
    dl = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dl), "File exists!");
    gtk_window_set_policy(GTK_WINDOW(dl), FALSE, FALSE, FALSE);
    gtk_window_set_modal(GTK_WINDOW(dl), TRUE);
    gtk_window_set_position(GTK_WINDOW(dl), GTK_WIN_POS_MOUSE);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dl)->vbox), vbox,
                       TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_widget_show(vbox);

    tmp = gtk_label_new(buf);
    gtk_box_pack_start(GTK_BOX(vbox), tmp, TRUE, TRUE, 0);
    gtk_widget_show(tmp);
    
    tmp = gtk_button_new_with_label("OK");
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dl)->action_area),
                       tmp, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(tmp), "clicked",
                       GTK_SIGNAL_FUNC(overwrite_ok_cmd),
                       img);
    gtk_signal_connect_object(GTK_OBJECT(tmp), "clicked",
                              GTK_SIGNAL_FUNC(gtk_widget_destroy),
                              GTK_OBJECT(dl));
    gtk_widget_show(tmp);
    
    tmp = gtk_button_new_with_label("Cancel");
    gtk_signal_connect_object(GTK_OBJECT(tmp), "clicked",
                              GTK_SIGNAL_FUNC(gtk_widget_destroy),
                              GTK_OBJECT(dl));
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dl)->action_area), tmp,
                       TRUE, TRUE, 0);
    gtk_widget_show(tmp);

    gtk_widget_show(dl);
}

void overwrite_ok_cmd(GtkWidget* w, image_info* img)
{
    save_file(img, (char*)gtk_file_selection_get_filename(
                  GTK_FILE_SELECTION(filesel)));
    gtk_widget_destroy(filesel);
}

static void my_png_ok(GtkWidget* w, image_info* img)
{
    char* filename = (char*)gtk_file_selection_get_filename(
        GTK_FILE_SELECTION(filesel));

    if (file_exists(filename))
        ask_overwrite(img, filename);
    else {
        save_file(img, filename);
        gtk_widget_destroy(filesel);
    }
}

void do_png_save(image_info* img)
{
    if (filesel)
        return;
    
    filesel = gtk_file_selection_new("Save as PNG");
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(filesel));
    gtk_signal_connect(GTK_OBJECT(filesel), "destroy",
                       GTK_SIGNAL_FUNC(gtk_widget_destroyed),
                       &filesel);
    gtk_signal_connect_object(GTK_OBJECT
                              (GTK_FILE_SELECTION(filesel)->cancel_button),
                              "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy),
                              GTK_OBJECT(filesel));
    gtk_signal_connect(GTK_OBJECT
                       (GTK_FILE_SELECTION(filesel)->ok_button),
                       "clicked", GTK_SIGNAL_FUNC(my_png_ok),
                       img);
    gtk_widget_show(filesel);
}
