#include "externs.h"
#include "fractal_types.h"


/* calculate next line of the fractal
   */
void fractal_next_line(image_info* img)
{
    int i;
    guint32 z;
    double x,y;
    double c_re, c_im;
    double re,im;
    double re2,im2;
    
    y = img->ymax - ((img->xmax - img->xmin)/(double)img->real_width)
        * (double)img->lines_done;

    for (i=0; i < img->real_width; i++) {
        x = ((double)i/(double)img->real_width) *
            (img->xmax - img->xmin) + img->xmin;
        re = x;
        im = y;
        re2 = re*re;
        im2 = im*im;

        switch (img->fr_type) {
        case MANDELBROT:
            c_re = x;
            c_im = y;
            break;
        case JULIA:
            c_re = img->u.julia.c_re;
            c_im = img->u.julia.c_im;
            break;
        }
        
        for (z=0; z < img->depth; z++) {
            im = 2.0*re*im+c_im;
            re = re2-im2+c_re;
            re2 = re*re;
            im2 = im*im;
            if ( (re2+im2) > 4)
                break;
        }
        
        if (z == img->depth)
            z = UINT_MAX;
        
        img->raw_data[img->lines_done*img->real_width + i] = z;
    }

    img->lines_done++;
}
