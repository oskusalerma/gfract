#include <stdio.h>
#include "externs.h"
#include "fractal_types.h"
#include "misc.h"
#include "palette.h"

static void do_aa_pixel(image_info* img, int x, int y);

void set_image_info(image_info* img, int w, int h, int aa_factor)
{
    gboolean same_size;

    if ((img->user_width == w) && (img->user_height == h)
         && (img->rgb_data != NULL))
        same_size = TRUE;
    else
        same_size = FALSE;

    if (!same_size)
        g_free(img->rgb_data);
    g_free(img->raw_data);

    img->user_width = w;
    img->user_height = h;
    img->aa_factor = aa_factor;
    img->ratio = (double)w/h;
    
    if (aa_factor < 1) {
        fprintf(stderr, "aa_factor must be >= 1\n");
        gtk_exit(1);
    }

    img->real_width = w*aa_factor;
    img->real_height = h*aa_factor;

    if (!same_size)
        img->rgb_data = g_malloc(img->user_width*img->user_height*4);
    img->raw_data = g_malloc(img->real_width*img->real_height *
        sizeof(*img->raw_data));

    clear_image(img, TRUE, !same_size);
}

void clear_image(image_info* img, gboolean raw, gboolean rgb)
{
    int i;

    if (raw) {
        for (i=0; i < img->real_width*img->real_height; i++)
            img->raw_data[i] = 0.0;
    }
    if (rgb) {
        for (i=0; i < img->user_width*img->user_height; i++)
            img->rgb_data[i] = palette[0];
    }
}

void do_aa_pixel(image_info* img, int x, int y)
{
    int yoff,xoff;
    int xi,yi;
    guint32 r,g,b,c;

    r = g = b = 0;

    yoff = y*img->aa_factor;
    xoff = x*img->aa_factor;

    for (yi=yoff; yi < yoff+img->aa_factor; yi++) {
        for (xi=xoff; xi < xoff+img->aa_factor; xi++) {
            c = get_pixel(img, xi,yi);
            r += RED(c);
            g += GREEN(c);
            b += BLUE(c);
        }
    }

    xi = img->aa_factor*img->aa_factor;

    r /= xi;
    g /= xi;
    b /= xi;

    img->rgb_data[y*img->user_width + x] = RGB(r,g,b);
}

void do_anti_aliasing(image_info* img, int x0, int y0, int width,
                      int height)
{
    int x,y;
    
    for (y=y0; y < y0+height; y++) {
        for (x=x0; x < x0 + width; x++) {
            do_aa_pixel(img, x, y);
        }
    }
}

void rgb_invert(image_info* img)
{
    int x,y;
    guint32 c,r,g,b;

    for (y=0; y < img->user_height; y++) {
        for (x=0; x < img->user_width; x++) {
            c = img->rgb_data[y*img->user_width + x];
            r = 255-RED(c);
            g = 255-GREEN(c);
            b = 255-BLUE(c);
            c = RGB(r,g,b);
            img->rgb_data[y*img->user_width + x] = c;
        }
    }
}
