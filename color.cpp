#include <stdio.h>
#include <math.h>
#include "externs.h"
#include "color.h"

#define MAX_STACK 64

#define OP0(code) \
stack[sp++] = code; \
break

#define OP1(op) \
stack[sp - 1] = op(stack[sp - 1]); \
break

#define OP2(op) \
stack[sp - 2] = stack[sp - 2] op stack[sp - 1]; \
sp--; \
break

double calculate_color(color_ops* ops, point_info* pi)
{
    int sp = 0;
    double stack[MAX_STACK];

    color_op* cop = ops->ops;
    int i;
    
    for (i = 0; i < ops->nr; i++)
    {
        switch (cop->type)
        {
        case ITER:
            OP0((double)pi->iter);
        case REAL:
            OP0(pi->re);
        case REAL2:
            OP0(pi->re2);
        case IMAG:
            OP0(pi->im);
        case IMAG2:
            OP0(pi->im2);
        case XPOS:
            OP0(pi->x);
        case YPOS:
            OP0(pi->y);

        case NUMBER_VALUE:
            OP0(cop->value);
        case PALETTE_COUNT:
            OP0((double)palette_size);
            
        case PLUS:
            OP2(+);
        case MINUS:
            OP2(-);
        case MULTIPLY:
            OP2(*);
        case DIVIDE:
            OP2(/);

        case SQRT:
            OP1(sqrt);
            
        default:
            printf("unknown op type %d (%d)\n", cop->type, i);
            break;
        }
        
        cop++;
    }
    
    if (sp != 1)
      printf("invalid number of values left on stack: %d\n", sp);

    return stack[0];
}
