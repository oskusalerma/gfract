#ifndef __FRACTAL_TYPES_H
#define __FRACTAL_TYPES_H

#include <stdint.h>
#include <gtk/gtk.h>
#include "color.h"

struct julia_info
{
    double c_re;
    double c_im;
};

enum fractal_type
{
    MANDELBROT, JULIA
};

struct image_info
{
    /* coordinates */
    double xmin,xmax,ymax;
    
    /* saved mandelbrot coordinates. we need these when switching back
       from julia mode */
    double old_xmin, old_xmax, old_ymax;
    
    /* recursion depth */
    unsigned int depth;

    /* lines done */
    int lines_done;

    /* our idle callback function id */
    int idle_id;

    /* actual data pointers */
    double* raw_data;
    uint32_t* rgb_data;

    /* true if we're a julia preview */
    bool j_pre;
    
    /* it's handy to keep this around */
    GtkWidget* drawing_area;
    
    /* real size. differs from user_size if anti-aliasing is used */
    int real_width;
    int real_height;

    /* user-visible size */
    int user_width;
    int user_height;

    /* width/height ratio */
    double ratio;
    
    /* anti-aliasing factor. if 1, no anti-aliasing */
    int aa_factor;

    /* fractal type */
    fractal_type fr_type;

    /* true if palette interpolation should be used. */
    bool palette_ip;
    
    /* different fractal types' parameters */
    union {
        julia_info julia;
    } u;

    /* coloring information */
    int cops_nr;
    color_op cops[MAX_OPS];
    
};

void fractal_next_line(image_info* img);

#endif
