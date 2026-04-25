#include "exec.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

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
static Value builtin_sin  (Value *args, int n) { (void)n; return from_double(sin(to_double(args[0]))); }
static Value builtin_cos  (Value *args, int n) { (void)n; return from_double(cos(to_double(args[0]))); }
static Value builtin_tan  (Value *args, int n) { (void)n; return from_double(tan(to_double(args[0]))); }
static Value builtin_log  (Value *args, int n) { (void)n; return from_double(log(to_double(args[0]))); }
static Value builtin_log10(Value *args, int n) { (void)n; return from_double(log10(to_double(args[0]))); }
static Value builtin_exp  (Value *args, int n) { (void)n; return from_double(exp(to_double(args[0]))); }
static Value builtin_mod  (Value *args, int n) { (void)n; return from_double(fmod(to_double(args[0]), to_double(args[1]))); }
static Value builtin_round(Value *args, int n) { (void)n; return from_double(round(to_double(args[0]))); }
static Value builtin_min  (Value *args, int n) {
    double m = to_double(args[0]);
    for (int i = 1; i < n; i++) {
        double v = to_double(args[i]);
        if (v < m) m = v;
    }
    return from_double(m);
}
static Value builtin_max  (Value *args, int n) {
    double m = to_double(args[0]);
    for (int i = 1; i < n; i++) {
        double v = to_double(args[i]);
        if (v > m) m = v;
    }
    return from_double(m);
}
static Value builtin_clmp (Value *args, int n) {
    (void)n;
    double v = to_double(args[0]);
    double lo = to_double(args[1]);
    double hi = to_double(args[2]);
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    return from_double(v);
}
static Value builtin_rnd  (Value *args, int n) {
    (void)n; (void)args;
    return from_double((double)rand() / (double)RAND_MAX);
}
static Value builtin_rndi (Value *args, int n) {
    (void)n;
    long long lo = (long long)to_double(args[0]);
    long long hi = (long long)to_double(args[1]);
    if (hi < lo) return from_double((double)lo);
    return from_double((double)(lo + rand() % (hi - lo + 1)));
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
        { "sin",   1, builtin_sin   },
        { "cos",   1, builtin_cos   },
        { "tan",   1, builtin_tan   },
        { "log",   1, builtin_log   },
        { "log10", 1, builtin_log10 },
        { "exp",   1, builtin_exp   },
        { "mod",   2, builtin_mod   },
        { "round", 1, builtin_round },
        { "min",  -1, builtin_min   },
        { "max",  -1, builtin_max   },
        { "clmp",  3, builtin_clmp  },
        { "rnd",   0, builtin_rnd   },
        { "rndi",  2, builtin_rndi  },
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
