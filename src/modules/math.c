#include "exec.h"
#include <math.h>
#include <string.h>

static Value builtin_floor(Value *args, int n) { 
    (void)n; 
    Value v; v.type = VAL_NUMBER; v.as.number = floor(args[0].as.number); v.is_immutable = 0;
    return v; 
}
static Value builtin_ceil (Value *args, int n) { 
    (void)n; 
    Value v; v.type = VAL_NUMBER; v.as.number = ceil(args[0].as.number); v.is_immutable = 0;
    return v; 
}
static Value builtin_abs  (Value *args, int n) { 
    (void)n; 
    Value v; v.type = VAL_NUMBER; v.as.number = fabs(args[0].as.number); v.is_immutable = 0;
    return v; 
}
static Value builtin_sqrt (Value *args, int n) { 
    (void)n; 
    Value v; v.type = VAL_NUMBER; v.as.number = sqrt(args[0].as.number); v.is_immutable = 0;
    return v; 
}
static Value builtin_pow  (Value *args, int n) { 
    (void)n; 
    Value v; v.type = VAL_NUMBER; v.as.number = pow(args[0].as.number, args[1].as.number); v.is_immutable = 0;
    return v; 
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
