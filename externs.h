#ifndef __EXTERNS_H
#define __EXTERNS_H

#include <stdint.h>
#include <vector>

typedef std::vector<uint32_t> palette_t;
extern palette_t palette;

// same as palette.size(), but faster
extern unsigned int palette_size;

#endif /* __EXTERNS_H */
