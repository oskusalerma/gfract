#include "fractal.h"
#include "color.h"

double fractal_calc_y(int y, double ymax, double xmin, double xmax,
    int width)
{
    return ymax - ((xmax - xmin)/width) * y;
}

void fractal_do_row(double xmin, double xmax, double y, int width,
    int depth, fractal_type fr_type, double julia_c_re, double julia_c_im,
    const color_ops* color_in, const color_ops* color_out, double* output)
{
    int i, z;
    double x;
    double c_re, c_im;
    double re,im;
    double re2,im2;
    point_info pi;

    pi.y = y;

    /* silence gcc warnings */
    c_re = c_im = 0.0;

    for (i=0; i < width; i++) {
        x = ((double)i / width) * (xmax - xmin) + xmin;

        re = x;
        im = y;
        re2 = re * re;
        im2 = im * im;

        switch (fr_type) {
        case MANDELBROT:
            c_re = x;
            c_im = y;
            break;

        case JULIA:
            c_re = julia_c_re;
            c_im = julia_c_im;
            break;
        }

        for (z = 0; z < depth; z++)
        {
            im = 2.0 * re * im + c_im;
            re = re2 - im2 + c_re;
            re2 = re * re;
            im2 = im * im;

            if ((re2 + im2) > 4.0)
            {
                break;
            }
        }

        const color_ops* ops = (z == depth) ? color_in : color_out;

        pi.x = x;
        pi.re = re;
        pi.re2 = re2;
        pi.im = im;
        pi.im2 = im2;
        pi.iter = z;

        output[i] = calculate_color(ops, &pi);
    }
}
