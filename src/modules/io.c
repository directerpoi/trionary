#include "exec.h"
#include "output.h"
#include <stdio.h>
#include <string.h>

static Value builtin_print(Value *args, int n) {
    (void)n;
    if (args[0].type == VAL_NUMBER) {
        emit_value(args[0].as.number);
    } else if (args[0].type == VAL_LIST) {
        printf("[");
        for (int i = 0; i < args[0].as.list.length; i++) {
            emit_value_no_newline(args[0].as.list.elements[i]);
            if (i < args[0].as.list.length - 1) printf(", ");
        }
        printf("]\n");
    }
    return args[0];
}

static Value builtin_read_line(Value *args, int n) {
    (void)n;
    char buf[64];
    if (!fgets(buf, sizeof(buf), stdin)) return args[0];
    double val;
    if (sscanf(buf, "%lf", &val) == 1) {
        Value v; v.type = VAL_NUMBER; v.as.number = val; v.is_immutable = 0;
        return v;
    }
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
