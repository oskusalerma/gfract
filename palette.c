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
    guint32 c;

    c = img->raw_data[y*img->real_width + x];
    if (c == UINT_MAX)
        return palette[0];

    c = (c%(pal_indexes-1))+1;

    return palette[c];
}
