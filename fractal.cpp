#include "externs.h"
#include "fractal_types.h"


/* calculate next line of the fractal
   */
void fractal_next_line(image_info* img)
{
    int i;
    uint32_t z;
    double x,y;
    double c_re, c_im;
    double re,im;
    double re2,im2;
    point_info pi;
    
    y = img->ymax - ((img->xmax - img->xmin)/(double)img->real_width)
        * (double)img->lines_done;

    pi.y = y;

    /* silence gcc warnings */
    c_re = c_im = 0.0;
    
    for (i=0; i < img->real_width; i++) {
        x = ((double)i/(double)img->real_width) *
            (img->xmax - img->xmin) + img->xmin;

        pi.x = x;

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
            z = 0;

        pi.re = re;
        pi.re2 = re2;
        pi.im = im;
        pi.im2 = im2;

        pi.iter = z;
        
        img->raw_data[img->lines_done*img->real_width + i] =
            calculate_color(img->cops, img->cops_nr, &pi);
    }

    img->lines_done++;
}
