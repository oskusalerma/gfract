#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "externs.h"
#include "image_info.h"
#include "palette.h"

#define BUF_SIZE 256

/* keep the current palette's filename around */
static char _filename[BUF_SIZE];

bool palette_load(char* filename)
{
    int r,g,b;
    char buf[BUF_SIZE];
    
    FILE* fp = fopen(filename, "r");
    if (fp == NULL)
        return false;

    palette.clear();
    
    while (fgets(buf, BUF_SIZE, fp) != NULL) {
        if (sscanf(buf, " %d %d %d", &r, &g,&b) != 3)
            break;
        palette.push_back(RGB(r,g,b));
    }

    fclose(fp);
    
    if (palette.size() == 0)
    {
        palette.push_back(RGB(0, 0, 0));
        
        return false;
    }

    palette_size = palette.size();
    strncpy(_filename, filename, BUF_SIZE);
    _filename[BUF_SIZE-1] = '\0';

    return true;
}

char* palette_get_filename(void)
{
    return &_filename[0];
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
