#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "externs.h"
#include "image_info.h"
#include "misc.h"
#include "palette.h"

static void vstrf(std::string* str, const char* format, va_list ap);
static void do_aa_pixel(image_info* img, int x, int y);

std::string strf(const char* format, ...)
{
    va_list ap;
    std::string str;

    va_start(ap, format);
    vstrf(&str, format, ap);
    va_end(ap);

    return str;
}

void vstrf(std::string* str, const char* format, va_list ap)
{
    char buf[4096];
    int ret;

    ret = vsnprintf(buf, sizeof(buf), format, ap);
    if (ret >= (int)sizeof(buf))
    {
        char* buf2 = new char[ret+1];
        vsnprintf(buf2, ret+1, format, ap);
        *str = buf2;
        delete[] buf2;
    }
    else if (ret == -1)
    {
        *str = "vsnprintf failed";
    }
    else
    {
        *str = buf;
    }
}

std::string dbl2str(double x)
{
    // 16 digits should be enough to represent all possible doubles since
    // an IEEE-754 double has a 53-bit mantissa and log10(2**53) = 15.95.
    return strf("%.16f", x);
}

double str2dbl(const std::string& s)
{
    return atof(s.c_str());
}

void gf_report_assert_failure(const char* expr, const char* file, int line)
{
    fprintf(stderr, "Assertion failure in file '%s', line %d\n", file, line);
    fprintf(stderr, " Failing code: %s\n", expr);

    abort();
}

void split(const std::string& str, std::vector<std::string>* vec)
{
    std::string tmp;

    vec->clear();

    const char* data = str.c_str();
    while (*data)
    {
        int ch = *data;

        if (ch == ' ')
        {
            if (tmp.size() > 0)
            {
                vec->push_back(tmp);
                tmp = "";
            }
        }
        else
        {
            tmp += ch;
        }

        data++;
    }

    if (tmp.size() > 0)
    {
        vec->push_back(tmp);
    }
}

void do_aa_pixel(image_info* img, int x, int y)
{
    int yoff,xoff;
    int xi,yi;
    uint32_t r,g,b,c;

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
    uint32_t c,r,g,b;

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
