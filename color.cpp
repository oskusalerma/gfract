#include <math.h>
#include <stdio.h>
#include <algorithm>
#include "externs.h"
#include "misc.h"
#include "color.h"

#define NELEMS(arr) (sizeof(arr) / sizeof(arr[0]))

#define MAX_STACK 64

#define OP0_1(code) \
stack[sp++] = code; \
break

#define OP1_0(op) \
stack[sp - 1] = op(stack[sp - 1]); \
break

#define OP1_1(op) \
stack[sp++] = op(stack[sp - 1]); \
break

#define OP2_m1(op) \
stack[sp - 2] = stack[sp - 2] op stack[sp - 1]; \
sp--; \
break

#define OP2_pm1(op) \
stack[sp - 2] = op(stack[sp - 2], stack[sp - 1]);   \
sp--; \
break

struct op_info_t
{
    std::string name;
    op_type type;

    // how many operands it needs
    int req;

    // by how much it modifies the stack size
    int delta;
};

op_info_t op_info[] = {
    {"iter", OP_ITER, 0, 1},
    {"real", OP_REAL, 0, 1},
    {"real2", OP_REAL2, 0, 1},
    {"imag", OP_IMAG, 0, 1},
    {"imag2", OP_IMAG2, 0, 1},
    {"xpos", OP_XPOS, 0, 1},
    {"ypos", OP_YPOS, 0, 1},
    {"palette_count", OP_PALETTE_COUNT, 0, 1},
    {"", OP_NUMBER, 0, 1},
    
    {"dup", OP_DUP, 1, 1},
    
    {"+", OP_PLUS, 2, -1},
    {"-", OP_MINUS, 2, -1},
    {"*", OP_MULTIPLY, 2, -1},
    {"/", OP_DIVIDE, 2, -1},
    
    {"min", OP_MIN, 2, -1},
    {"max", OP_MAX, 2, -1},
    {"pow", OP_POW, 2, -1},
    {"hypot", OP_HYPOT, 2, -1},
     
    {"sqrt", OP_SQRT, 1, 0},
    {"abs", OP_ABS, 1, 0},
    {"ln", OP_LN, 1, 0},
    {"floor", OP_FLOOR, 1, 0},
    {"ceil", OP_CEIL, 1, 0},
    {"sin", OP_SIN, 1, 0},
    {"cos", OP_COS, 1, 0},
    {"tan", OP_TAN, 1, 0}
};

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
        case OP_ITER:
            OP0_1((double)pi->iter);
            
        case OP_REAL:
            OP0_1(pi->re);
            
        case OP_REAL2:
            OP0_1(pi->re2);
            
        case OP_IMAG:
            OP0_1(pi->im);
            
        case OP_IMAG2:
            OP0_1(pi->im2);
            
        case OP_XPOS:
            OP0_1(pi->x);
            
        case OP_YPOS:
            OP0_1(pi->y);
            
        case OP_NUMBER:
            OP0_1(cop->value);
            
        case OP_PALETTE_COUNT:
            OP0_1((double)palette_size);

            
        case OP_DUP:
            OP1_1(double);

            
        case OP_PLUS:
            OP2_m1(+);
        case OP_MINUS:
            OP2_m1(-);
        case OP_MULTIPLY:
            OP2_m1(*);
        case OP_DIVIDE:
            OP2_m1(/);

            
        case OP_MIN:
            OP2_pm1(std::min);

        case OP_MAX:
            OP2_pm1(std::max);

        case OP_POW:
            OP2_pm1(pow);

        case OP_HYPOT:
            OP2_pm1(hypot);

        
        case OP_SQRT:
            OP1_0(sqrt);
            
        case OP_ABS:
            OP1_0(fabs);
            
        case OP_LN:
            OP1_0(log);
            
        case OP_FLOOR:
            OP1_0(floor);
            
        case OP_CEIL:
            OP1_0(ceil);
            
        case OP_SIN:
            OP1_0(sin);
            
        case OP_COS:
            OP1_0(cos);
            
        case OP_TAN:
            OP1_0(tan);
            
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

std::string ops2str(color_ops* ops)
{
    std::string res;
    
    for (int i = 0; i < ops->nr; i++)
    {
        color_op* cop = &ops->ops[i];

        bool found = false;
        for (int j = 0; j < (int)NELEMS(op_info); j++)
        {
            op_info_t* oi = &op_info[j];
            
            if (cop->type == oi->type)
            {
                if (cop->type == OP_NUMBER)
                {
                    res += strf("%f", cop->value);
                }
                else
                {
                    res += oi->name;
                }

                found = true;
                break;
            }
        }

        if (!found)
        {
            res += strf("UNKNOWN_OPERATOR(%d)", cop->type);
        }

        res += " ";
    }

    return res;
}

std::string str2ops(const std::string& str, color_ops* ops)
{
    std::vector<std::string> vec;

    split(str, &vec);

    if (vec.size() > MAX_OPS)
    {
        return strf("Too many operations (%d, max is %d)", vec.size(),
            MAX_OPS);
    }

    int stack = 0;
    
    for (int i = 0; i < (int)vec.size(); i++)
    {
        std::string item = vec[i];
        std::string err = strf("Error at operation %d ('%s'):\n", i + 1,
            item.c_str());
        
        bool found = false;
        for (int j = 0; j < (int)NELEMS(op_info); j++)
        {
            op_info_t* oi = &op_info[j];
            
            if (item == oi->name)
            {
                if (stack < oi->req)
                {
                    return err + strf("Need %d values, but stack only has %d",
                        oi->req, stack);
                }

                ops->ops[i].type = oi->type;
                stack += oi->delta;
                
                found = true;
                break;
            }
        }

        if (!found)
        {
            const char* s = item.c_str();
            char* end;
            double tmp = strtod(s, &end);

            if ((end == s) || (end != (s + strlen(s))))
            {
                return err + "Unknown operation";
            }
            
            ops->ops[i].type = OP_NUMBER;
            ops->ops[i].value = tmp;
            stack++;
        }
    }

    if (stack != 1)
    {
        return strf("Algorithm leaves %d values on stack", stack);
    }
    
    ops->nr = vec.size();

    return "";
}
