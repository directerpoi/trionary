#include "exec.h"
#include <math.h>
#include <string.h>

static double builtin_floor(double *args, int n) { (void)n; return floor(args[0]); }
static double builtin_ceil (double *args, int n) { (void)n; return ceil(args[0]);  }
static double builtin_abs  (double *args, int n) { (void)n; return fabs(args[0]);  }
static double builtin_sqrt (double *args, int n) { (void)n; return sqrt(args[0]);  }
static double builtin_pow  (double *args, int n) { (void)n; return pow(args[0], args[1]); }

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
        /* Skip if already registered (idempotent re-use). */
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
