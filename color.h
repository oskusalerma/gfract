#ifndef GFRACT_COLOR_H
#define GFRACT_COLOR_H

typedef struct
{
    double x, y;
    double re,im,re2,im2;
    unsigned int iter;
} point_info;

typedef enum {
    ITER,
    REAL,
    REAL2,
    IMAG,
    IMAG2,
    XPOS,
    YPOS,

    NUMBER_VALUE,
    PALETTE_COUNT,
    
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,

    SQRT,
    /** MIN, MAX, FLOOR, CEIL, DUP etc */
    SIN,
    COS,
    TAN
} op_type;

typedef struct {
    op_type type;
    double value;
} color_op;

double calculate_color(color_op* cops, int cops_nr, point_info* pi);

#endif
