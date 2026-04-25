#include "exec.h"
#include <math.h>
#include <string.h>

static double to_double(Value v) {
    if (v.type == VAL_INT) return (double)v.as.integer;
    if (v.type == VAL_FLOAT) return v.as.float_val;
    return 0.0;
}

static Value from_double(double d) {
    Value v;
    if (floor(d) == d) {
        v.type = VAL_INT; v.as.integer = (long long)d;
    } else {
        v.type = VAL_FLOAT; v.as.float_val = d;
    }
    v.is_immutable = 0;
    return v;
}

static Value builtin_floor(Value *args, int n) { 
    (void)n; 
    return from_double(floor(to_double(args[0]))); 
}
static Value builtin_ceil (Value *args, int n) { 
    (void)n; 
    return from_double(ceil(to_double(args[0]))); 
}
static Value builtin_abs  (Value *args, int n) { 
    (void)n; 
    return from_double(fabs(to_double(args[0]))); 
}
static Value builtin_sqrt (Value *args, int n) { 
    (void)n; 
    return from_double(sqrt(to_double(args[0]))); 
}
static Value builtin_pow  (Value *args, int n) { 
    (void)n; 
    return from_double(pow(to_double(args[0]), to_double(args[1]))); 
}

void register_math_module(FuncTable *ft) {
    static const struct {
        const char *name;
        int         nparams;
        BuiltinFn   fn;
    } math_funcs[] = {
        { "floor", 1, builtin_floor },
        { "ceil",  1, builtin_ceil  },
        { "abs",   1, builtin_abs   },
        { "sqrt",  1, builtin_sqrt  },
        { "pow",   2, builtin_pow   },
    };
    int nmods = (int)(sizeof(math_funcs) / sizeof(math_funcs[0]));
    for (int i = 0; i < nmods; i++) {
        int already = 0;
        for (int j = 0; j < ft->count; j++) {
            if (strcmp(ft->funcs[j].name, math_funcs[i].name) == 0) {
                already = 1;
                break;
            }
        }
        if (already) continue;
        if (ft->count >= MAX_FUNCS) break;
        FuncDef *fd       = &ft->funcs[ft->count++];
        strncpy(fd->name, math_funcs[i].name, 63);
        fd->name[63]      = '\0';
        fd->param_count   = math_funcs[i].nparams;
        fd->body          = NULL;
        fd->line          = 0;
        fd->is_builtin    = 1;
        fd->builtin_fn    = math_funcs[i].fn;
    }
}
