#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "externs.h"
#include "fractal_types.h"
#include "palette.h"

#define BUF_SIZE 256

/* keep the current palette's filename around */
static char _filename[BUF_SIZE];

gboolean palette_load(char* filename)
{
    int i,r,g,b;
    char buf[BUF_SIZE];
    
    FILE* fp = fopen(filename, "r");
    if (fp == NULL)
        return FALSE;

    i = 0;
    while (fgets(buf, BUF_SIZE, fp) != NULL) {
        if (i >= 256)
            break;
        if (sscanf(buf, " %d %d %d", &r, &g,&b) != 3)
            break;
        palette[i] = RGB(r,g,b);
        i++;
    }

    fclose(fp);
    
    if(i == 0)
        return FALSE;

    pal_indexes = i;
    strncpy(_filename, filename, BUF_SIZE);
    _filename[BUF_SIZE-1] = '\0';

    return TRUE;
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
    guint32 r,g,b;
    
    for (i=0; i < pal_indexes; i++) {
        r = 255-RED(palette[i]);
        g = 255-GREEN(palette[i]);
        b = 255-BLUE(palette[i]);
        palette[i] = RGB(r,g,b);
    }
}

void palette_rotate_backward(void)
{
    int i, max;
    guint32 temp;
    
    max = pal_indexes-1;
    temp = palette[0];
    for (i=0; i < max; i++)
        palette[i] = palette[i+1];
    palette[max] = temp;
}

void palette_rotate_forward(void)
{
    int i, max;
    guint32 temp;

    max = pal_indexes-1;
    temp = palette[max];
    for (i=max; i > 0; i--)
        palette[i] = palette[i-1];
    palette[0] = temp;
}

guint32 get_pixel(image_info* img, int x, int y)
{
    if (img->palette_ip)
    {
        double val,cval,diff,rdiff;
        int ind1,ind2;
        guint32 c1,c2;
        uint8_t r,g,b;
    
        val = img->raw_data[y*img->real_width + x];

        // FIXME: optimize this
        
        cval = ceil(val);
        diff = cval - val;
        rdiff = 1.0 - diff;
    
        ind1 = ((guint32)floor(val)) % pal_indexes;
        ind2 = (guint32)cval % pal_indexes;
    
        c1 = palette[ind1];
        c2 = palette[ind2];

        r = diff * RED(c1) + rdiff * RED(c2);
        g = diff * GREEN(c1) + rdiff * GREEN(c2);
        b = diff * BLUE(c1) + rdiff * BLUE(c2);

        return RGB(r,g,b);
    }
    else
    {
        double c;
        int index;
    
        c = img->raw_data[y*img->real_width + x];
        index = (guint32)c % pal_indexes;

        return palette[index];
    }
}
