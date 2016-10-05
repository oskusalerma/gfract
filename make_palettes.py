#!/usr/bin/env python

import glob
import re
import sys

# base filenames of palette files, e.g. "blues"
palettes = set()

# output file
out = open("palettes.cpp", "w")

def die(msg):
    print "Failed: %s" % msg
    sys.exit(1)

# what variable name should we use for the given base filename
def varName(basename):
    return "_" + basename

def addPalette(filename):
    base = filename[9:-4]
    palettes.add(base)

    out.write('static uint8_t %s[] = {\n' % varName(base))
    
    f = open(filename)

    # match three numbers from the beginning of the line, separated by
    # whitespace, ignoring everything else
    pat = re.compile(r"^\s*(\d+)\s+(\d+)\s+(\d+)")

    for line in f:
        m = pat.match(line)

        r = int(m.group(1))
        g = int(m.group(2))
        b = int(m.group(3))

        out.write("%d,%d,%d,\n" % (r, g, b))

    out.write('};\n\n')

    f.close()

def main():
    out.write('#include "palette_internal.h"\n\n')

    for fn in glob.glob("palettes/*.map"):
        addPalette(fn)

    for name in sorted(palettes):
        vn = varName(name)
        
        out.write(
            'static palette_builtin_loader dummy%s(' \
            '"%s", sizeof(%s) / 3, %s);\n' % (
            vn, name, vn, vn))

    out.close()

main()
