#include "exec.h"
#include "output.h"
#include <stdio.h>
#include <string.h>

/* print(x): emits x to stdout and returns x */
static double builtin_print(double *args, int n) {
    (void)n;
    emit_value(args[0]);
    return args[0];
}

/* read_line(default): reads a double from stdin; returns default on EOF/error */
static double builtin_read_line(double *args, int n) {
    (void)n;
    double val;
    if (scanf("%lf", &val) == 1) return val;
    return args[0];
}

void register_io_module(FuncTable *ft) {
    static const struct {
        const char *name;
        int         nparams;
        BuiltinFn   fn;
    } io_funcs[] = {
        { "print",     1, builtin_print     },
        { "read_line", 1, builtin_read_line },
    };
    int nmods = (int)(sizeof(io_funcs) / sizeof(io_funcs[0]));
    for (int i = 0; i < nmods; i++) {
        /* Skip if already registered (idempotent re-use). */
        int already = 0;
        for (int j = 0; j < ft->count; j++) {
            if (strcmp(ft->funcs[j].name, io_funcs[i].name) == 0) {
                already = 1;
                break;
            }
        }
        if (already) continue;
        if (ft->count >= MAX_FUNCS) break;
        FuncDef *fd       = &ft->funcs[ft->count++];
        strncpy(fd->name, io_funcs[i].name, 63);
        fd->name[63]      = '\0';
        fd->param_count   = io_funcs[i].nparams;
        fd->body          = NULL;
        fd->line          = 0;
        fd->is_builtin    = 1;
        fd->builtin_fn    = io_funcs[i].fn;
    }
}
