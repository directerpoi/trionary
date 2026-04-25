#include "exec.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* Helper to clone a value */
extern Value clone_value(Value v);
extern void free_value(Value v);
extern int values_equal(Value a, Value b);
extern double val_to_double(Value v);

static Value builtin_srt(Value *args, int n) {
    if (n < 1 || args[0].type != VAL_ARRAY) return args[0];
    Value res = clone_value(args[0]);
    int len = res.as.list.length;
    for (int i = 0; i < len - 1; i++) {
        for (int j = 0; j < len - i - 1; j++) {
            if (val_to_double(res.as.list.elements[j]) > val_to_double(res.as.list.elements[j+1])) {
                Value tmp = res.as.list.elements[j];
                res.as.list.elements[j] = res.as.list.elements[j+1];
                res.as.list.elements[j+1] = tmp;
            }
        }
    }
    return res;
}

static Value builtin_srtd(Value *args, int n) {
    if (n < 1 || args[0].type != VAL_ARRAY) return args[0];
    Value res = clone_value(args[0]);
    int len = res.as.list.length;
    for (int i = 0; i < len - 1; i++) {
        for (int j = 0; j < len - i - 1; j++) {
            if (val_to_double(res.as.list.elements[j]) < val_to_double(res.as.list.elements[j+1])) {
                Value tmp = res.as.list.elements[j];
                res.as.list.elements[j] = res.as.list.elements[j+1];
                res.as.list.elements[j+1] = tmp;
            }
        }
    }
    return res;
}

static Value builtin_rev(Value *args, int n) {
    if (n < 1 || args[0].type != VAL_ARRAY) return args[0];
    Value res = clone_value(args[0]);
    int len = res.as.list.length;
    for (int i = 0; i < len / 2; i++) {
        Value tmp = res.as.list.elements[i];
        res.as.list.elements[i] = res.as.list.elements[len - i - 1];
        res.as.list.elements[len - i - 1] = tmp;
    }
    return res;
}

static Value builtin_cnt(Value *args, int n) {
    Value res; res.type = VAL_INT; res.is_immutable = 0;
    if (n < 1) { res.as.integer = 0; return res; }
    if (args[0].type == VAL_ARRAY || args[0].type == VAL_SET || args[0].type == VAL_TUPLE) {
        res.as.integer = args[0].as.list.length;
    } else if (args[0].type == VAL_MAP) {
        res.as.integer = args[0].as.map.length;
    } else if (args[0].type == VAL_STRING) {
        res.as.integer = strlen(args[0].as.string);
    } else {
        res.as.integer = 0;
    }
    return res;
}

static Value builtin_avg(Value *args, int n) {
    Value res; res.type = VAL_FLOAT; res.as.float_val = 0.0; res.is_immutable = 0;
    if (n < 1 || args[0].type != VAL_ARRAY || args[0].as.list.length == 0) return res;
    double sum = 0;
    for (int i = 0; i < args[0].as.list.length; i++) {
        sum += val_to_double(args[0].as.list.elements[i]);
    }
    res.as.float_val = sum / args[0].as.list.length;
    return res;
}

static Value builtin_unq(Value *args, int n) {
    if (n < 1 || args[0].type != VAL_ARRAY) return args[0];
    Value res; res.type = VAL_ARRAY; res.is_immutable = 0;
    res.as.list.elements = NULL; res.as.list.length = 0;
    for (int i = 0; i < args[0].as.list.length; i++) {
        int found = 0;
        for (int j = 0; j < res.as.list.length; j++) {
            if (values_equal(args[0].as.list.elements[i], res.as.list.elements[j])) {
                found = 1; break;
            }
        }
        if (!found) {
            res.as.list.elements = realloc(res.as.list.elements, (res.as.list.length + 1) * sizeof(Value));
            res.as.list.elements[res.as.list.length++] = clone_value(args[0].as.list.elements[i]);
        }
    }
    return res;
}

static Value builtin_zip(Value *args, int n) {
    Value res; res.type = VAL_ARRAY; res.is_immutable = 0;
    res.as.list.elements = NULL; res.as.list.length = 0;
    if (n < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_ARRAY) return res;
    int len = args[0].as.list.length < args[1].as.list.length ? args[0].as.list.length : args[1].as.list.length;
    res.as.list.elements = malloc(len * sizeof(Value));
    res.as.list.length = len;
    for (int i = 0; i < len; i++) {
        Value pair; pair.type = VAL_PAIR; pair.is_immutable = 0;
        pair.as.pair.key = malloc(sizeof(Value)); *pair.as.pair.key = clone_value(args[0].as.list.elements[i]);
        pair.as.pair.value = malloc(sizeof(Value)); *pair.as.pair.value = clone_value(args[1].as.list.elements[i]);
        res.as.list.elements[i] = pair;
    }
    return res;
}

static Value builtin_fnd(Value *args, int n) {
    if (n < 2 || args[0].type != VAL_ARRAY) { Value v; v.type = VAL_NIL; v.is_immutable = 0; return v; }
    for (int i = 0; i < args[0].as.list.length; i++) {
        if (values_equal(args[0].as.list.elements[i], args[1])) return clone_value(args[0].as.list.elements[i]);
    }
    Value v; v.type = VAL_NIL; v.is_immutable = 0; return v;
}

static Value builtin_idx(Value *args, int n) {
    Value res; res.type = VAL_INT; res.as.integer = -1; res.is_immutable = 0;
    if (n < 2 || args[0].type != VAL_ARRAY) return res;
    for (int i = 0; i < args[0].as.list.length; i++) {
        if (values_equal(args[0].as.list.elements[i], args[1])) { res.as.integer = i; break; }
    }
    return res;
}

static Value builtin_slc(Value *args, int n) {
    if (n < 1 || args[0].type != VAL_ARRAY) return args[0];
    int start = (n > 1) ? (int)val_to_double(args[1]) : 0;
    int end = (n > 2) ? (int)val_to_double(args[2]) : args[0].as.list.length;
    if (start < 0) start = 0;
    if (end > args[0].as.list.length) end = args[0].as.list.length;
    if (start > end) start = end;
    Value res; res.type = VAL_ARRAY; res.is_immutable = 0;
    res.as.list.length = end - start;
    res.as.list.elements = malloc(res.as.list.length * sizeof(Value));
    for (int i = 0; i < res.as.list.length; i++) {
        res.as.list.elements[i] = clone_value(args[0].as.list.elements[start + i]);
    }
    return res;
}

static Value builtin_flat(Value *args, int n) {
    if (n < 1 || args[0].type != VAL_ARRAY) return args[0];
    Value res; res.type = VAL_ARRAY; res.is_immutable = 0;
    res.as.list.elements = NULL; res.as.list.length = 0;
    /* Flatten one level as per spec: "Flatten a nested list one level" */
    for (int i = 0; i < args[0].as.list.length; i++) {
        if (args[0].as.list.elements[i].type == VAL_ARRAY) {
            for (int j = 0; j < args[0].as.list.elements[i].as.list.length; j++) {
                res.as.list.elements = realloc(res.as.list.elements, (res.as.list.length + 1) * sizeof(Value));
                res.as.list.elements[res.as.list.length++] = clone_value(args[0].as.list.elements[i].as.list.elements[j]);
            }
        } else {
            res.as.list.elements = realloc(res.as.list.elements, (res.as.list.length + 1) * sizeof(Value));
            res.as.list.elements[res.as.list.length++] = clone_value(args[0].as.list.elements[i]);
        }
    }
    return res;
}

static Value builtin_push(Value *args, int n) {
    if (n < 2 || args[0].type != VAL_ARRAY) return args[0];
    /* Note: Modifying in place or returning a new one? Trionary lists are generally values.
       But push/pop usually imply mutation if possible, however the value system suggests cloning.
       Wait, let's look at how other things work. Pipelines usually return new lists. */
    Value res = clone_value(args[0]);
    res.as.list.elements = realloc(res.as.list.elements, (res.as.list.length + 1) * sizeof(Value));
    res.as.list.elements[res.as.list.length++] = clone_value(args[1]);
    return res;
}

static Value builtin_pop(Value *args, int n) {
    if (n < 1 || args[0].type != VAL_ARRAY || args[0].as.list.length == 0) {
        Value v; v.type = VAL_NIL; v.is_immutable = 0; return v;
    }
    /* pop usually returns the popped element. But it also modifies the list.
       In a functional-ish language, this is tricky. The spec says "Remove and return last element".
       If it's used in a pipeline, it might be expected to return the element. */
    return clone_value(args[0].as.list.elements[args[0].as.list.length - 1]);
}

void register_list_module(FuncTable *ft) {
    static const struct {
        const char *name;
        int         nparams;
        BuiltinFn   fn;
    } list_funcs[] = {
        { "srt",  1, builtin_srt },
        { "srtd", 1, builtin_srtd },
        { "rev",  1, builtin_rev },
        { "cnt",  1, builtin_cnt },
        { "avg",  1, builtin_avg },
        { "unq",  1, builtin_unq },
        { "zip",  2, builtin_zip },
        { "fnd",  2, builtin_fnd },
        { "idx",  2, builtin_idx },
        { "slc",  3, builtin_slc },
        { "flat", 1, builtin_flat },
        { "push", 2, builtin_push },
        { "pop",  1, builtin_pop },
    };
    int nmods = (int)(sizeof(list_funcs) / sizeof(list_funcs[0]));
    for (int i = 0; i < nmods; i++) {
        if (ft->count >= MAX_FUNCS) break;
        FuncDef *fd = &ft->funcs[ft->count++];
        strncpy(fd->name, list_funcs[i].name, 63);
        fd->name[63] = '\0';
        fd->param_count = list_funcs[i].nparams;
        fd->body = NULL;
        fd->line = 0;
        fd->is_builtin = 1;
        fd->builtin_fn = list_funcs[i].fn;
    }
}
