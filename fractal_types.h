#ifndef __FRACTAL_TYPES_H
#define __FRACTAL_TYPES_H

#include <gtk/gtk.h>
#include "color.h"

typedef struct {
    double c_re;
    double c_im;
} julia_info;

typedef enum {
    MANDELBROT, JULIA
} fractal_type;

typedef struct {
    /* coordinates */
    double xmin,xmax,ymax;
    
    /* saved mandelbrot coordinates. we need these when switching back
       from julia mode */
    double old_xmin, old_xmax, old_ymax;
    
    /* recursion depth */
    int depth;

    /* lines done */
    int lines_done;

    /* our idle callback function id */
    int idle_id;

    /* actual data pointers */
    double* raw_data;
    guint32* rgb_data;

    /* TRUE if we're a julia preview */
    gboolean j_pre;
    
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

    /* 1 if palette interpolation should be used. */
    int palette_ip;
    
    /* different fractal types' parameters */
    union {
        julia_info julia;
    } u;

    /* coloring information */
    int cops_nr;
    color_op cops[MAX_OPS];
    
} image_info;

void fractal_next_line(image_info* img);

#endif /* __FRACTAL_TYPES_H */
