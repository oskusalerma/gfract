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
    OP_ITER,
    OP_REAL,
    OP_REAL2,
    OP_IMAG,
    OP_IMAG2,
    OP_XPOS,
    OP_YPOS,
    OP_PALETTE_COUNT,
    OP_NUMBER,
    
    OP_DUP,
    
    OP_PLUS,
    OP_MINUS,
    OP_MULTIPLY,
    OP_DIVIDE,

    OP_MIN,
    OP_MAX,
    
    OP_SQRT,
    OP_FLOOR,
    OP_CEIL,
    OP_SIN,
    OP_COS,
    OP_TAN
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
