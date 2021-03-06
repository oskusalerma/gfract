#ifndef __PALETTE_H
#define __PALETTE_H

#include <stdint.h>
#include <string>
#include <gtk/gtk.h>

struct image_info;

#if (G_BYTE_ORDER == G_BIG_ENDIAN)
#define RGB(r,g,b) ((r) << 24 | ((g) << 16) | ((b) << 8))
#define RED(x)     (((x) & 0xff000000) >> 24)
#define GREEN(x)   (((x) & 0x00ff0000) >> 16)
#define BLUE(x)    (((x) & 0x0000ff00) >> 8)
#elif (G_BYTE_ORDER == G_LITTLE_ENDIAN)
#define RGB(r,g,b) ((r) | ((g) << 8) | ((b) << 16))
#define RED(x)     ((x) & 0x000000ff)
#define GREEN(x)   (((x) & 0x0000ff00) >> 8)
#define BLUE(x)    (((x) & 0x00ff0000) >> 16)
#else
#error Your machine has an unsupported byte order. Please send patch :)
#endif

// get number of builtin palettes
int palette_get_nr_of_builtins(void);

// get name of the given builtin palette
const char* palette_get_builtin_name(int n);

// load the specified builtin palette
void palette_load_builtin(int n);

/* get the current palette's name. if it begins with a '/', it's a
   filename, otherwise it's a builtin name. */
const std::string& palette_get_current_name();

bool palette_load(const char* filename);
void palette_apply(image_info* img, int x0, int y0, int width, int height);
void palette_invert(void);
void palette_rotate_backward(void);
void palette_rotate_forward(void);
uint32_t get_pixel(image_info* img, int x, int y);

#endif /* __PALETTE_H */
