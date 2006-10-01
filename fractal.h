#ifndef FRACTAL_H
#define FRACTAL_H

struct color_ops;

/* We only support two different fractals (for now, anyway). */
enum fractal_type
{
    MANDELBROT, JULIA
};

/* Return y coordinate for row Y (0-based) calculated from the other
   parameters. */
double fractal_calc_y(int y, double ymax, double xmin, double xmax,
    int width);

/* Calculate one row of a fractal. Output consists of WIDTH doubles being
   stored in OUTPUT. */
void fractal_do_row(double xmin, double xmax, double y, int width,
    int depth, fractal_type fr_type, double julia_c_re, double julia_c_im,
    const color_ops* color_in, const color_ops* color_out, double* output);

#endif
