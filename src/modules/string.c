#define _POSIX_C_SOURCE 200809L
#include "exec.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

extern Value clone_value(Value v);
extern void free_value(Value v);
extern char* value_to_string(Value v);

static Value builtin_cat(Value *args, int n) {
    char *res_str = malloc(1);
    res_str[0] = '\0';
    size_t total_len = 0;
    for (int i = 0; i < n; i++) {
        char *s = value_to_string(args[i]);
        size_t slen = strlen(s);
        res_str = realloc(res_str, total_len + slen + 1);
        strcat(res_str, s);
        total_len += slen;
        free(s);
    }
    Value res; res.type = VAL_STRING; res.as.string = res_str; res.is_immutable = 0;
    return res;
}

static Value builtin_len(Value *args, int n) {
    Value res; res.type = VAL_INT; res.is_immutable = 0;
    if (n < 1 || args[0].type != VAL_STRING) { res.as.integer = 0; return res; }
    res.as.integer = strlen(args[0].as.string);
    return res;
}

static Value builtin_sub(Value *args, int n) {
    if (n < 1 || args[0].type != VAL_STRING) { Value v; v.type = VAL_STRING; v.as.string = strdup(""); v.is_immutable = 0; return v; }
    int start = (n > 1) ? (int)args[1].as.integer : 0;
    int len = (n > 2) ? (int)args[2].as.integer : (int)strlen(args[0].as.string);
    int s_len = strlen(args[0].as.string);
    if (start < 0) start = 0;
    if (start > s_len) start = s_len;
    if (start + len > s_len) len = s_len - start;
    if (len < 0) len = 0;
    char *sub = malloc(len + 1);
    strncpy(sub, args[0].as.string + start, len);
    sub[len] = '\0';
    Value res; res.type = VAL_STRING; res.as.string = sub; res.is_immutable = 0;
    return res;
}

static Value builtin_upr(Value *args, int n) {
    if (n < 1 || args[0].type != VAL_STRING) return clone_value(args[0]);
    char *s = strdup(args[0].as.string);
    for (int i = 0; s[i]; i++) s[i] = toupper(s[i]);
    Value res; res.type = VAL_STRING; res.as.string = s; res.is_immutable = 0;
    return res;
}

static Value builtin_lwr(Value *args, int n) {
    if (n < 1 || args[0].type != VAL_STRING) return clone_value(args[0]);
    char *s = strdup(args[0].as.string);
    for (int i = 0; s[i]; i++) s[i] = tolower(s[i]);
    Value res; res.type = VAL_STRING; res.as.string = s; res.is_immutable = 0;
    return res;
}

static Value builtin_trm(Value *args, int n) {
    if (n < 1 || args[0].type != VAL_STRING) return clone_value(args[0]);
    char *s = args[0].as.string;
    while (isspace(*s)) s++;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace(*end)) end--;
    size_t len = (strlen(s) == 0) ? 0 : (end - s + 1);
    char *res_s = malloc(len + 1);
    strncpy(res_s, s, len);
    res_s[len] = '\0';
    Value res; res.type = VAL_STRING; res.as.string = res_s; res.is_immutable = 0;
    return res;
}

static Value builtin_spl(Value *args, int n) {
    Value res; res.type = VAL_ARRAY; res.is_immutable = 0;
    res.as.list.elements = NULL; res.as.list.length = 0;
    if (n < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) return res;
    char *s = strdup(args[0].as.string);
    char *delim = args[1].as.string;
    char *token = strtok(s, delim);
    while (token) {
        res.as.list.elements = realloc(res.as.list.elements, (res.as.list.length + 1) * sizeof(Value));
        Value v; v.type = VAL_STRING; v.as.string = strdup(token); v.is_immutable = 0;
        res.as.list.elements[res.as.list.length++] = v;
        token = strtok(NULL, delim);
    }
    free(s);
    return res;
}

static Value builtin_has(Value *args, int n) {
    Value res; res.type = VAL_BOOL; res.as.boolean = 0; res.is_immutable = 0;
    if (n < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) return res;
    if (strstr(args[0].as.string, args[1].as.string)) res.as.boolean = 1;
    return res;
}

static Value builtin_rep(Value *args, int n) {
    if (n < 3 || args[0].type != VAL_STRING || args[1].type != VAL_STRING || args[2].type != VAL_STRING) return clone_value(args[0]);
    char *src = args[0].as.string;
    char *from = args[1].as.string;
    char *to = args[2].as.string;
    size_t from_len = strlen(from);
    size_t to_len = strlen(to);
    
    if (from_len == 0) return clone_value(args[0]);

    size_t count = 0;
    char *tmp = src;
    while ((tmp = strstr(tmp, from))) { count++; tmp += from_len; }

    char *res_s = malloc(strlen(src) + count * (to_len - from_len) + 1);
    char *p = res_s;
    while (*src) {
        if (strstr(src, from) == src) {
            strcpy(p, to);
            p += to_len;
            src += from_len;
        } else {
            *p++ = *src++;
        }
    }
    *p = '\0';
    Value res; res.type = VAL_STRING; res.as.string = res_s; res.is_immutable = 0;
    return res;
}

static Value builtin_fmt(Value *args, int n) {
    if (n < 1 || args[0].type != VAL_STRING) { Value v; v.type = VAL_STRING; v.as.string = strdup(""); v.is_immutable = 0; return v; }
    char *fmt = args[0].as.string;
    char *res_s = malloc(1); res_s[0] = '\0';
    size_t res_len = 0;
    int arg_idx = 1;
    while (*fmt) {
        if (*fmt == '{' && *(fmt+1) == '}' && arg_idx < n) {
            char *s = value_to_string(args[arg_idx++]);
            res_len += strlen(s);
            res_s = realloc(res_s, res_len + 1);
            strcat(res_s, s);
            free(s);
            fmt += 2;
        } else {
            res_len++;
            res_s = realloc(res_s, res_len + 1);
            res_s[res_len-1] = *fmt++;
            res_s[res_len] = '\0';
        }
    }
    Value res; res.type = VAL_STRING; res.as.string = res_s; res.is_immutable = 0;
    return res;
}

static Value builtin_num(Value *args, int n) {
    if (n < 1 || args[0].type != VAL_STRING) { Value v; v.type = VAL_INT; v.as.integer = 0; v.is_immutable = 0; return v; }
    double d = atof(args[0].as.string);
    Value v;
    if (floor(d) == d) {
        v.type = VAL_INT; v.as.integer = (long long)d;
    } else {
        v.type = VAL_FLOAT; v.as.float_val = d;
    }
    v.is_immutable = 0;
    return v;
}

static Value builtin_tostr(Value *args, int n) {
    if (n < 1) { Value v; v.type = VAL_STRING; v.as.string = strdup(""); v.is_immutable = 0; return v; }
    Value res; res.type = VAL_STRING; res.as.string = value_to_string(args[0]); res.is_immutable = 0;
    return res;
}

void register_string_module(FuncTable *ft) {
    static const struct {
        const char *name;
        int         nparams;
        BuiltinFn   fn;
    } string_funcs[] = {
        { "cat",   -1, builtin_cat },
        { "len",    1, builtin_len },
        { "sub",    3, builtin_sub },
        { "upr",    1, builtin_upr },
        { "lwr",    1, builtin_lwr },
        { "trm",    1, builtin_trm },
        { "spl",    2, builtin_spl },
        { "has",    2, builtin_has },
        { "rep",    3, builtin_rep },
        { "fmt",   -1, builtin_fmt },
        { "num",    1, builtin_num },
        { "tostr",  1, builtin_tostr },
    };
    int nmods = (int)(sizeof(string_funcs) / sizeof(string_funcs[0]));
    for (int i = 0; i < nmods; i++) {
        if (ft->count >= MAX_FUNCS) break;
        FuncDef *fd = &ft->funcs[ft->count++];
        strncpy(fd->name, string_funcs[i].name, 63);
        fd->name[63] = '\0';
        fd->param_count = string_funcs[i].nparams;
        fd->body = NULL;
        fd->line = 0;
        fd->is_builtin = 1;
        fd->builtin_fn = string_funcs[i].fn;
    }
}
