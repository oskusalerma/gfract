#ifndef __EXTERNS_H
#define __EXTERNS_H

#include <stdint.h>
#include <vector>

extern std::vector<uint32_t> palette;

// same as palette.size(), but faster
extern unsigned int palette_size;

#endif /* __EXTERNS_H */
