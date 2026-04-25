#define _POSIX_C_SOURCE 200809L
#include "exec.h"
#include "output.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static Value builtin_print(Value *args, int n) {
    if (n > 0) {
        emit_value(args[0]);
    }
    Value res; res.type = VAL_NIL; res.is_immutable = 0;
    return res;
}

static Value builtin_read_line(Value *args, int n) {
    (void)n; (void)args;
    char buf[256];
    if (!fgets(buf, sizeof(buf), stdin)) {
        Value res; res.type = VAL_NIL; res.is_immutable = 0;
        return res;
    }
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
        buf[--len] = '\0';
    Value res;
    res.type = VAL_STRING;
    res.as.string = strdup(buf);
    res.is_immutable = 0;
    return res;
}

void register_io_module(FuncTable *ft) {
    static const struct {
        const char *name;
        int         nparams;
        BuiltinFn   fn;
    } io_funcs[] = {
        { "print",     1, builtin_print     },
        { "read_line", 0, builtin_read_line },
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
