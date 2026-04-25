#define _POSIX_C_SOURCE 200809L
#include "exec.h"
#include "output.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>

static Value builtin_print(Value *args, int n) {
    for (int i = 0; i < n; i++) {
        emit_value(args[i]);
        if (i < n - 1) printf(" ");
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

static Value builtin_fex(Value *args, int n) {
    Value res; res.type = VAL_BOOL; res.as.boolean = 0; res.is_immutable = 0;
    if (n < 1 || args[0].type != VAL_STRING) return res;
    struct stat buffer;
    if (stat(args[0].as.string, &buffer) == 0) res.as.boolean = 1;
    return res;
}

static Value builtin_fls(Value *args, int n) {
    Value res; res.type = VAL_ARRAY; res.is_immutable = 0;
    res.as.list.elements = NULL; res.as.list.length = 0;
    const char *path = (n > 0 && args[0].type == VAL_STRING) ? args[0].as.string : ".";
    DIR *d = opendir(path);
    if (!d) return res;
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;
        res.as.list.elements = realloc(res.as.list.elements, (res.as.list.length + 1) * sizeof(Value));
        Value v; v.type = VAL_STRING; v.as.string = strdup(dir->d_name); v.is_immutable = 0;
        res.as.list.elements[res.as.list.length++] = v;
    }
    closedir(d);
    return res;
}

void register_io_module(FuncTable *ft) {
    static const struct {
        const char *name;
        int         nparams;
        BuiltinFn   fn;
    } io_funcs[] = {
        { "print",    -1, builtin_print     },
        { "read_line", 0, builtin_read_line },
        { "fex",       1, builtin_fex       },
        { "fls",       1, builtin_fls       },
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
