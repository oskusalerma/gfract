#include "externs.h"
#include "fractal_types.h"

/* #define LENNY */

#ifdef LENNY

#define PTF .7 /* periodicity tolerance factor */
#define PI 32   /* periodicity interval (preferably a power of 2) */

/* these two functions must be inline, so the compiler 
   can optimize further and there is no overhead of function calling */

/* caclulate a point of the fractal without periodicity testing */

inline guint32 fractal_point(double x, double y, guint32 depth, double c_re,
double c_im)
{
    guint32 z;
    double f, g;
    double re=x;
    double im=y;
    double re2=x*x;
    double im2=y*y;
    for (z=1; z < depth; z+=2) {
        f=re2-im2+c_re;    
        g=2.0*re*im+c_im;  
        re=((f-g)*(f+g))+c_re;
        im=2.0*f*g+c_im;
        re2=re*re;
        im2=im*im;
        if ( (re2+im2) > 4)
            break;
    }
    if (((f*f) + (g*g))>4) z--;
    if (z >= depth)
        z=UINT_MAX;
    return z;
}

/* calculate point of the fractal with periodicity testing */

guint32 fractal_point_periodic(double x, double y, guint32 depth, double
c_re, double c_im, double PT)
{
    guint32 z;
    double f, g;
    double re=x;
    double im=y;
    double re2=x*x;
    double im2=y*y;
    for (z=0; z < depth; z+=2) {
        if ((z%PI)==0) {
            x=re;
            y=im;
        }
        f=re2-im2+c_re;    
        g=2.0*re*im+c_im;  
        if (((f-x)<PT) && ((f-x)>-PT) && ((g-y)<PT) && ((g-y)>-PT)) return
UINT_MAX;
        re=((f-g)*(f+g))+c_re;
        im=2.0*f*g+c_im;
        if (((re-x)<PT) && ((re-x)>-PT) && ((im-y)>-PT) && ((im-y)<PT))
return UINT_MAX;
        re2=re*re;
        im2=im*im;
        if ((re2 + im2)>4) break;
    }
    if (((f*f) + (g*g))>4) z--;
    if (z >= depth)
        z=UINT_MAX;
    return z;
}

/* calculate next line of the fractal
   */
void fractal_next_line(image_info* img)
{
    int i;
    guint32 z=0;
    double x, y, pt;
    double c_re, c_im;
    double re, im;
    
    y=img->ymax - ((img->xmax - img->xmin)/(double)img->real_width) *
(double)img->lines_done;
    pt=((img->xmax - img->xmin)/(double)img->real_width) * PTF;
    for (i=0; i < img->real_width; i++) {
        x=((double)i/(double)img->real_width) * (img->xmax - img->xmin) +
img->xmin;
        re=x;
        im=y;
        switch (img->fr_type) {
        case MANDELBROT:
            c_re=x;
            c_im=y;
            break;
        case JULIA:
            c_re=img->u.julia.c_re;
            c_im=img->u.julia.c_im;
            break;
        }
/*         if (z == UINT_MAX) { */
/*             z = fractal_point_periodic(re, im, img->depth, c_re, c_im, */
/* pt); */
/*         } else { */
            z = fractal_point(re, im, img->depth, c_re, c_im);
/*         } */
        img->raw_data[img->lines_done*img->real_width + i]=z;
    }

    img->lines_done++;
}





#else

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
    
    y=img->ymax - ((img->xmax - img->xmin)/(double)img->real_width)
        * (double)img->lines_done;

    for (i=0; i < img->real_width; i++) {
        x=((double)i/(double)img->real_width) *
            (img->xmax - img->xmin) + img->xmin;
        re=x;
        im=y;
        re2=re*re;
        im2=im*im;

        switch (img->fr_type) {
        case MANDELBROT:
            c_re=x;
            c_im=y;
            break;
        case JULIA:
            c_re=img->u.julia.c_re;
            c_im=img->u.julia.c_im;
            break;
        }
        
        for (z=0; z < img->depth; z++) {
            im=2.0*re*im+c_im;
            re=re2-im2+c_re;
            re2=re*re;
            im2=im*im;
            if ( (re2+im2) > 4)
                break;
        }
        
        if (z == img->depth)
            z=UINT_MAX;
        
        img->raw_data[img->lines_done*img->real_width + i]=z;
    }

    img->lines_done++;
}

#endif
