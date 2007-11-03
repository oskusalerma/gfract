#ifndef __MISC_H
#define __MISC_H

#include <string>
#include <vector>

struct image_info;

std::string strf(const char* format, ...)
    __attribute__ ((format (printf, 1, 2)));

/* for any number X: "str2dbl(dbl2str(X)) == X"
   should be true. */
std::string dbl2str(double x);
double str2dbl(const std::string& s);

void split(const std::string& str, std::vector<std::string>* vec);

/* Report a failed assertion, and die. */
void gf_report_assert_failure(const char* expr, const char* file, int line);

/* Same as assert, except the expression is always evaluated. */
#define gf_a(EXPR) \
do \
{ \
    if (!(EXPR)) \
    { \
        gf_report_assert_failure(#EXPR, __FILE__, __LINE__); \
    } \
} while (0)

void set_image_info(image_info* img, int w, int h, int aa_factor);
void clear_image(image_info* img, bool raw, bool rgb);
void rgb_invert(image_info* img);
void do_anti_aliasing(image_info* img, int x0, int y0, int width,
                      int height);

#endif
