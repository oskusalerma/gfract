#ifndef GFRACT_COLOR_H
#define GFRACT_COLOR_H

/** maximum number of operations */
#define MAX_OPS 256

struct point_info
{
    double x, y;
    double re,im,re2,im2;
    unsigned int iter;
};

enum op_type
{
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
};

struct color_op
{
    op_type type;
    double value;
};

struct color_ops
{
    int nr;
    color_op ops[MAX_OPS];
};

double calculate_color(color_ops* ops, point_info* pi);

#endif
