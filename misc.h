#ifndef __MISC_H
#define __MISC_H

void set_image_info(image_info* img, int w, int h, int aa_factor);
void clear_image(image_info* img, bool raw, bool rgb);
void rgb_invert(image_info* img);
void do_anti_aliasing(image_info* img, int x0, int y0, int width,
                      int height);

#endif /* __MISC_H */
