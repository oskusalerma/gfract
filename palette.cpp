#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "externs.h"
#include "image_info.h"
#include "palette.h"
#include "palette_internal.h"

namespace {
    typedef std::vector<palette_builtin*> builtinVec;
}

// we have to create this dynamically because there are no guarantees
// about the ordering of the creation of static objects.
static builtinVec* builtins = NULL;

#define BUF_SIZE 256

static std::string _name;

void palette_add_builtin(palette_builtin* bp)
{
    if (builtins == NULL) {
        builtins = new builtinVec();
    }

    builtins->push_back(bp);
}

int palette_get_nr_of_builtins(void)
{
    return builtins->size();
}

const char* palette_get_builtin_name(int n)
{
    return builtins->at(n)->name;
}

void palette_load_builtin(int n)
{
    palette_t new_pal;

    palette_builtin* bp = builtins->at(n);

    for (int i = 0; i < bp->entries; i++)
    {
        uint8_t* c = bp->data + i * 3;

        new_pal.push_back(RGB(c[0], c[1], c[2]));
    }

    g_assert(new_pal.size() != 0);

    palette = new_pal;
    palette_size = palette.size();

    _name = bp->name;
}

bool palette_load(const char* filename)
{
    int r,g,b;
    char buf[BUF_SIZE];
    palette_t new_pal;

    FILE* fp = fopen(filename, "rt");

    if (fp == NULL)
    {
        return false;
    }

    while (fgets(buf, BUF_SIZE, fp) != NULL) {
        if (sscanf(buf, " %d %d %d", &r, &g,&b) != 3)
        {
            break;
        }

        new_pal.push_back(RGB(r, g, b));
    }

    gf_a(fclose(fp) == 0);

    if (new_pal.size() == 0)
    {
        return false;
    }

    palette = new_pal;
    palette_size = palette.size();
    _name = filename;

    return true;
}

const std::string& palette_get_current_name()
{
    return _name;
}

void palette_apply(image_info* img, int x0, int y0, int width, int height)
{
    int x,y;

    for (y=y0; y < y0+height; y++) {
        for (x=x0; x < x0 + width; x++) {
            img->rgb_data[y*width + x] = get_pixel(img, x,y);
        }
    }
}

void palette_invert(void)
{
    int i;
    uint32_t r,g,b;

    for (i=0; i < (int)palette_size; i++) {
        r = 255-RED(palette[i]);
        g = 255-GREEN(palette[i]);
        b = 255-BLUE(palette[i]);
        palette[i] = RGB(r,g,b);
    }
}

void palette_rotate_backward(void)
{
    int i, max;
    uint32_t temp;

    max = palette_size-1;
    temp = palette[0];
    for (i=0; i < max; i++)
        palette[i] = palette[i+1];
    palette[max] = temp;
}

void palette_rotate_forward(void)
{
    int i, max;
    uint32_t temp;

    max = palette_size-1;
    temp = palette[max];
    for (i=max; i > 0; i--)
        palette[i] = palette[i-1];
    palette[0] = temp;
}

uint32_t get_pixel(image_info* img, int x, int y)
{
    if (img->palette_ip)
    {
        double val,cval,diff,rdiff;
        int ind1,ind2;
        uint32_t c1,c2;
        uint8_t r,g,b;

        val = img->raw_data[y*img->real_width + x];

        /** FIXME: optimize this */

        cval = ceil(val);
        diff = cval - val;
        rdiff = 1.0 - diff;

        ind1 = ((uint32_t)floor(val)) % palette_size;
        ind2 = (uint32_t)cval % palette_size;

        c1 = palette[ind1];
        c2 = palette[ind2];

        r = uint8_t(diff * RED(c1) + rdiff * RED(c2));
        g = uint8_t(diff * GREEN(c1) + rdiff * GREEN(c2));
        b = uint8_t(diff * BLUE(c1) + rdiff * BLUE(c2));

        return RGB(r,g,b);
    }
    else
    {
        double c;
        int index;

        c = img->raw_data[y*img->real_width + x];
        index = (uint32_t)c % palette_size;

        return palette[index];
    }
}
