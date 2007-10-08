#ifndef PALETTE_INTERNAL_H
#define PALETTE_INTERNAL_H

#include <stdint.h>

// internal stuff about the palette system, not meant for normal users

// a palette compiled into the program
struct palette_builtin
{
    palette_builtin(const char* name, int entries, uint8_t* data)
    {
        this->name = name;
        this->entries = entries;
        this->data = data;
    }

    // name of the palette
    const char* name;

    // number of palette entries to follow
    int entries;

    // raw data. 'entries' times palette values in RGB format.
    uint8_t* data;
};


// register a new builtin palette
void palette_add_builtin(palette_builtin* bp);

// create a static instance of this class to register a builtin palette
// automatically
class palette_builtin_loader {
public:
    palette_builtin_loader(const char* name, int entries, uint8_t* data)
    {
        palette_add_builtin(new palette_builtin(name, entries, data));
    }
};

#endif
