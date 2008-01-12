#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <png.h>
#include <gtk/gtk.h>
#include "my_png.h"
#include "externs.h"
#include "image_info.h"
#include "palette.h"

#if PNG_LIBPNG_VER <= 10002
#error Your libpng is too old. Upgrade to at least version 1.0.3
#endif


void my_png_save_img(image_info* img, char* filename)
{
    FILE* fp;
    png_struct* png_ptr;
    png_info* info_ptr;
    bool pal;                /* true if img is palettized */
    int i;
    png_color* png_pal = NULL;
    uint8_t* pal_data = NULL;
    uint8_t** row_p = NULL;

    pal = (img->aa_factor == 1) && !img->palette_ip;

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

    if (!pal) {
        png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);
    } else {
        /* convert data to palette index format */

        int pixels = img->user_width * img->user_height;
        uint8_t* dst;
        double* src;

        pal_data = new uint8_t[pixels];
        dst = pal_data;
        src = img->raw_data;

        for (i=0; i < pixels; i++) {
            *dst = (uint32_t)(*src) % palette_size;

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

void do_png_save(image_info* img, GtkWidget* parent)
{
    GtkWidget* dlg;

    dlg = gtk_file_chooser_dialog_new("Save as PNG", GTK_WINDOW(parent),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg),
        TRUE);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
        my_png_save_img(img, filename);
        g_free(filename);
    }

    gtk_widget_destroy(dlg);
}
