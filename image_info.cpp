#include "image_info.h"
#include "misc.h"

std::string fractal_info::to_str() const
{
    std::string s;

    s += "Type: ";
    switch (type) {
    case MANDELBROT:
        s += "Mandelbrot";
        break;
    case JULIA:
        s += "Julia";
        break;
    };

    s += "\n";

    s += strf(
        "XMin: %s\n"
        "XMax: %s\n"
        "YMax: %s\n",
        dbl2str(xmin).c_str(),
        dbl2str(xmax).c_str(),
        dbl2str(ymax).c_str());

    // FIXME: add Julia-specific info here

    return s;
}

bool fractal_info::operator==(const fractal_info& rhs) const
{
    if ((type != rhs.type) ||
        (xmin != rhs.xmin) ||
        (xmax != rhs.xmax) ||
        (ymax != rhs.ymax)) {
        return false;
    }

    // FIXME: test Julia-specific stuff here (if type is JULIA)

    return true;
}

bool fractal_info::operator!=(const fractal_info& rhs) const
{
    return !(*this == rhs);
}

