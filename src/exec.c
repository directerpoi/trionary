#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "exec.h"
#include "error.h"
#include "output.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>

static void free_value(Value v);
Value clone_value(Value v);
static struct Scope* clone_scope(struct Scope* s);
static void free_scope(struct Scope* s);

double val_to_double(Value v) {
    if (v.type == VAL_INT) return (double)v.as.integer;
    if (v.type == VAL_FLOAT) return v.as.float_val;
    if (v.type == VAL_BOOL) return (double)v.as.boolean;
    return 0.0;
}

static int is_argn_variable(const char* name) {
    if (name[0] != 'a' || name[1] != 'r' || name[2] != 'g' || name[3] == '\0')
        return 0;
    for (int i = 3; name[i] != '\0'; i++)
        if (name[i] < '0' || name[i] > '9') return 0;
    return 1;
}

static void warn_argn_undefined(SymTable* sym, const char* name) {
    fflush(stdout);
    int argc_val = sym_exists(sym, "argc") ? (int)val_to_double(sym_get(sym, "argc")) : 0;
    fprintf(stderr,
            "Warning: '%s' is not defined (argc=%d); returning 0."
            " Consider using '%s ?? default'.\n",
            name, argc_val, name);
}

static unsigned int scope_hash(const char* name) {
    unsigned int h = 5381;
    while (*name) h = ((h << 5) + h) + (unsigned char)*name++;
    return h & (SCOPE_CAPACITY - 1);
}

static ScopeEntry* scope_lookup(Scope* scope, const char* name) {
    unsigned int idx = scope_hash(name);
    for (unsigned int i = 0; i < SCOPE_CAPACITY; i++) {
        unsigned int slot = (idx + i) & (SCOPE_CAPACITY - 1);
        ScopeEntry* e = &scope->slots[slot];
        if (!e->occupied) return NULL;
        if (strcmp(e->name, name) == 0) return e;
    }
    return NULL;
}

static ScopeEntry* scope_reserve(Scope* scope, const char* name) {
    unsigned int idx = scope_hash(name);
    for (unsigned int i = 0; i < SCOPE_CAPACITY; i++) {
        unsigned int slot = (idx + i) & (SCOPE_CAPACITY - 1);
        ScopeEntry* e = &scope->slots[slot];
        if (!e->occupied || strcmp(e->name, name) == 0) return e;
    }
    return NULL;
}

static Scope* clone_scope(Scope* s) {
    if (!s) return NULL;
    Scope* copy = calloc(1, sizeof(Scope));
    copy->parent = clone_scope(s->parent);
    for (int i = 0; i < SCOPE_CAPACITY; i++) {
        if (s->slots[i].occupied) {
            strncpy(copy->slots[i].name, s->slots[i].name, 63);
            copy->slots[i].value = clone_value(s->slots[i].value);
            copy->slots[i].occupied = 1;
        }
    }
    return copy;
}

static void free_scope(Scope* s) {
    if (!s) return;
    free_scope(s->parent);
    for (int i = 0; i < SCOPE_CAPACITY; i++) {
        if (s->slots[i].occupied) {
            free_value(s->slots[i].value);
        }
    }
    free(s);
}

SymTable* create_symtable(void) {
    SymTable* t = malloc(sizeof(SymTable));
    t->current = calloc(1, sizeof(Scope));
    t->current->parent = NULL;
    return t;
}

void scope_push(SymTable* t) {
    Scope* s = calloc(1, sizeof(Scope));
    s->parent = t->current;
    t->current = s;
}

static void free_value(Value v) {
    switch (v.type) {
        case VAL_STRING:
        case VAL_ERROR:
            free(v.as.string);
            break;
        case VAL_ARRAY:
        case VAL_SET:
        case VAL_TUPLE:
            for (int i = 0; i < v.as.list.length; i++) {
                free_value(v.as.list.elements[i]);
            }
            free(v.as.list.elements);
            break;
        case VAL_MAP:
            for (int i = 0; i < v.as.map.length; i++) {
                free_value(v.as.map.keys[i]);
                free_value(v.as.map.values[i]);
            }
            free(v.as.map.keys);
            free(v.as.map.values);
            break;
        case VAL_PAIR:
            if (v.as.pair.key) { free_value(*v.as.pair.key); free(v.as.pair.key); }
            if (v.as.pair.value) { free_value(*v.as.pair.value); free(v.as.pair.value); }
            break;
        case VAL_FN:
            free_expr(v.as.fn.body);
            free_scope(v.as.fn.closure);
            break;
        default:
            break;
    }
}

Value clone_value(Value v) {
    Value res = v;
    switch (v.type) {
        case VAL_STRING:
        case VAL_ERROR:
            res.as.string = strdup(v.as.string);
            break;
        case VAL_ARRAY:
        case VAL_SET:
        case VAL_TUPLE:
            res.as.list.elements = malloc(v.as.list.length * sizeof(Value));
            for (int i = 0; i < v.as.list.length; i++) {
                res.as.list.elements[i] = clone_value(v.as.list.elements[i]);
            }
            break;
        case VAL_MAP:
            res.as.map.keys = malloc(v.as.map.length * sizeof(Value));
            res.as.map.values = malloc(v.as.map.length * sizeof(Value));
            for (int i = 0; i < v.as.map.length; i++) {
                res.as.map.keys[i] = clone_value(v.as.map.keys[i]);
                res.as.map.values[i] = clone_value(v.as.map.values[i]);
            }
            break;
        case VAL_PAIR:
            res.as.pair.key = malloc(sizeof(Value));
            res.as.pair.value = malloc(sizeof(Value));
            *res.as.pair.key = clone_value(*v.as.pair.key);
            *res.as.pair.value = clone_value(*v.as.pair.value);
            break;
        case VAL_FN:
            res.as.fn.body = clone_expr(v.as.fn.body);
            res.as.fn.closure = clone_scope(v.as.fn.closure);
            break;
        default:
            break;
    }
    return res;
}

char* value_to_string(Value v) {
    char buf[128];
    switch (v.type) {
        case VAL_NIL: return strdup("nil");
        case VAL_BOOL: return strdup(v.as.boolean ? "true" : "fls");
        case VAL_INT: sprintf(buf, "%lld", v.as.integer); return strdup(buf);
        case VAL_FLOAT: sprintf(buf, "%.6g", v.as.float_val); return strdup(buf);
        case VAL_STRING: return strdup(v.as.string);
        case VAL_ARRAY: return strdup("[array]");
        case VAL_MAP: return strdup("{map}");
        case VAL_SET: return strdup("{set}");
        case VAL_TUPLE: return strdup("(tuple)");
        case VAL_PAIR: return strdup("pair");
        default: return strdup("unknown");
    }
}

void free_symtable(SymTable* t) {
    Scope* s = t->current;
    while (s) {
        Scope* parent = s->parent;
        for (int i = 0; i < SCOPE_CAPACITY; i++) {
            if (s->slots[i].occupied) {
                free_value(s->slots[i].value);
            }
        }
        free(s);
        s = parent;
    }
    free(t);
}

void scope_pop(SymTable* t) {
    if (!t->current->parent) return;
    Scope* old = t->current;
    t->current = old->parent;
    for (int i = 0; i < SCOPE_CAPACITY; i++) {
        if (old->slots[i].occupied) {
            free_value(old->slots[i].value);
        }
    }
    free(old);
}

void sym_set(SymTable* t, const char* name, Value val) {
    ScopeEntry* e = NULL;
    /* Check all scopes for an existing binding to check immutability. */
    for (Scope* s = t->current; s; s = s->parent) {
        e = scope_lookup(s, name);
        if (e) {
            if (e->value.is_immutable) {
                error_at(0, "Cannot reassign to immutable variable '%s'", name);
            }
            free_value(e->value);
            e->value = clone_value(val);
            return;
        }
    }
    /* New binding in current scope. */
    e = scope_reserve(t->current, name);
    if (!e) error_at(0, "Symbol table full");
    strncpy(e->name, name, 63);
    e->name[63] = '\0';
    e->value = clone_value(val);
    e->occupied = 1;
}

int sym_exists(SymTable* t, const char* name) {
    for (Scope* s = t->current; s; s = s->parent) {
        if (scope_lookup(s, name)) return 1;
    }
    return 0;
}

Value sym_get(SymTable* t, const char* name) {
    for (Scope* s = t->current; s; s = s->parent) {
        ScopeEntry* e = scope_lookup(s, name);
        if (e) return e->value;
    }
    Value v; v.type = VAL_NIL; v.is_immutable = 0;
    return v;
}

FuncTable* create_functable(void) {
    return calloc(1, sizeof(FuncTable));
}

void free_functable(FuncTable* ft) {
    for (int i = 0; i < ft->count; i++) {
        if (!ft->funcs[i].is_builtin) free_expr(ft->funcs[i].body);
    }
    free(ft);
}

static Value eval_expr(Expr* expr, SymTable* sym, FuncTable* ft);

static int is_truthy(Value v) {
    switch (v.type) {
        case VAL_NIL: return 0;
        case VAL_BOOL: return v.as.boolean;
        case VAL_INT: return v.as.integer != 0;
        case VAL_FLOAT: return v.as.float_val != 0.0;
        case VAL_STRING: return v.as.string[0] != '\0';
        case VAL_ARRAY:
        case VAL_SET:
        case VAL_TUPLE: return v.as.list.length > 0;
        case VAL_MAP: return v.as.map.length > 0;
        default: return 1;
    }
}

int values_equal(Value a, Value b) {
    if (a.type != b.type) {
        if ((a.type == VAL_INT || a.type == VAL_FLOAT) && (b.type == VAL_INT || b.type == VAL_FLOAT))
            return val_to_double(a) == val_to_double(b);
        return 0;
    }
    switch (a.type) {
        case VAL_NIL: return 1;
        case VAL_BOOL: return a.as.boolean == b.as.boolean;
        case VAL_INT: return a.as.integer == b.as.integer;
        case VAL_FLOAT: return a.as.float_val == b.as.float_val;
        case VAL_STRING: return strcmp(a.as.string, b.as.string) == 0;
        case VAL_ARRAY:
        case VAL_SET:
        case VAL_TUPLE:
            if (a.as.list.length != b.as.list.length) return 0;
            for (int i = 0; i < a.as.list.length; i++)
                if (!values_equal(a.as.list.elements[i], b.as.list.elements[i])) return 0;
            return 1;
        default: return 0;
    }
}

static Value eval_expr(Expr* expr, SymTable* sym, FuncTable* ft) {
    Value res; res.type = VAL_NIL; res.is_immutable = 0;
    if (!expr) return res;

    switch (expr->type) {
        case EXPR_NUMBER:
            if (floor(expr->num_val) == expr->num_val) {
                res.type = VAL_INT; res.as.integer = (long long)expr->num_val;
            } else {
                res.type = VAL_FLOAT; res.as.float_val = expr->num_val;
            }
            return res;
        case EXPR_BOOL:
            res.type = VAL_BOOL; res.as.boolean = (int)expr->num_val;
            return res;
        case EXPR_STRING:
            res.type = VAL_STRING; res.as.string = strdup(expr->string_val);
            return res;
        case EXPR_NIL:
            res.type = VAL_NIL;
            return res;
        case EXPR_VARIABLE:
            if (!sym_exists(sym, expr->var_name)) {
                if (is_argn_variable(expr->var_name)) {
                    warn_argn_undefined(sym, expr->var_name);
                    res.type = VAL_INT; res.as.integer = 0;
                } else {
                    if (expr->col > 0) set_error_col(expr->col);
                    error_at(expr->line, "Undefined variable '%s'", expr->var_name);
                }
                return res;
            }
            return clone_value(sym_get(sym, expr->var_name));
        case EXPR_LIST:
        case EXPR_TUPLE:
            res.type = (expr->type == EXPR_LIST) ? VAL_ARRAY : VAL_TUPLE;
            res.as.list.length = expr->arg_count;
            res.as.list.elements = malloc(expr->arg_count * sizeof(Value));
            for (int i = 0; i < expr->arg_count; i++)
                res.as.list.elements[i] = eval_expr(expr->args[i], sym, ft);
            return res;
        case EXPR_SET:
            res.type = VAL_SET;
            res.as.list.length = 0;
            res.as.list.elements = malloc(expr->arg_count * sizeof(Value));
            for (int i = 0; i < expr->arg_count; i++) {
                Value v = eval_expr(expr->args[i], sym, ft);
                int found = 0;
                for (int j = 0; j < res.as.list.length; j++) {
                    if (values_equal(v, res.as.list.elements[j])) {
                        found = 1; break;
                    }
                }
                if (!found) res.as.list.elements[res.as.list.length++] = v;
                else free_value(v);
            }
            return res;
        case EXPR_MAP:
            res.type = VAL_MAP;
            res.as.map.length = expr->arg_count;
            res.as.map.keys = malloc(expr->arg_count * sizeof(Value));
            res.as.map.values = malloc(expr->arg_count * sizeof(Value));
            for (int i = 0; i < expr->arg_count; i++) {
                Value pair = eval_expr(expr->args[i], sym, ft);
                if (pair.type == VAL_PAIR) {
                    res.as.map.keys[i] = clone_value(*pair.as.pair.key);
                    res.as.map.values[i] = clone_value(*pair.as.pair.value);
                } else {
                    res.as.map.keys[i] = pair;
                    res.as.map.values[i].type = VAL_NIL;
                }
                free_value(pair);
            }
            return res;
        case EXPR_PAIR:
            res.type = VAL_PAIR;
            res.as.pair.key = malloc(sizeof(Value));
            res.as.pair.value = malloc(sizeof(Value));
            *res.as.pair.key = eval_expr(expr->left, sym, ft);
            *res.as.pair.value = eval_expr(expr->right, sym, ft);
            return res;
        case EXPR_LMB:
            res.type = VAL_FN;
            res.as.fn.param_count = expr->arg_count;
            for (int i = 0; i < expr->arg_count; i++) {
                strncpy(res.as.fn.params[i], expr->args[i]->var_name, 63);
            }
            res.as.fn.body = clone_expr(expr->left);
            res.as.fn.closure = clone_scope(sym->current);
            return res;
        case EXPR_ERR: {
            Value msg = eval_expr(expr->left, sym, ft);
            res.type = VAL_ERROR;
            res.as.string = value_to_string(msg);
            free_value(msg);
            return res;
        }
        case EXPR_TIM: {
            struct timespec start, end;
            clock_gettime(CLOCK_MONOTONIC, &start);
            res = eval_expr(expr->left, sym, ft);
            clock_gettime(CLOCK_MONOTONIC, &end);
            double elapsed = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1e9;
            fprintf(stderr, "Time: %.6fs\n", elapsed);
            return res;
        }
        case EXPR_IO: {
            if (expr->op == (OpType)TOK_ASK) {
                if (expr->arg_count > 0) {
                    Value prompt = eval_expr(expr->args[0], sym, ft);
                    char* s = value_to_string(prompt);
                    printf("%s", s); fflush(stdout);
                    free(s); free_value(prompt);
                }
                char buf[1024];
                if (fgets(buf, sizeof(buf), stdin)) {
                    size_t len = strlen(buf);
                    while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) buf[--len] = '\0';
                    res.type = VAL_STRING; res.as.string = strdup(buf);
                }
            } else if (expr->op == (OpType)TOK_FRD) {
                if (expr->arg_count > 0) {
                    Value path = eval_expr(expr->args[0], sym, ft);
                    if (path.type == VAL_STRING) {
                        FILE* f = fopen(path.as.string, "r");
                        if (f) {
                            fseek(f, 0, SEEK_END);
                            long len = ftell(f);
                            fseek(f, 0, SEEK_SET);
                            char* content = malloc(len + 1);
                            if (content) {
                                size_t read_bytes = fread(content, 1, len, f);
                                content[read_bytes] = '\0';
                                res.type = VAL_STRING; res.as.string = content;
                            }
                            fclose(f);
                        }
                    }
                    free_value(path);
                }
            } else if (expr->op == (OpType)TOK_CSV) {
                if (expr->arg_count > 0) {
                    Value path = eval_expr(expr->args[0], sym, ft);
                    if (path.type == VAL_STRING) {
                        FILE* f = fopen(path.as.string, "r");
                        if (f) {
                            res.type = VAL_ARRAY; res.as.list.elements = NULL; res.as.list.length = 0;
                            char line[4096];
                            while (fgets(line, sizeof(line), f)) {
                                size_t len = strlen(line);
                                while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
                                Value row; row.type = VAL_ARRAY; row.as.list.elements = NULL; row.as.list.length = 0;
                                char* lptr = line;
                                char* tok;
                                while ((tok = strsep(&lptr, ",")) != NULL) {
                                    row.as.list.elements = realloc(row.as.list.elements, (row.as.list.length + 1) * sizeof(Value));
                                    Value v; v.type = VAL_STRING; v.as.string = strdup(tok); v.is_immutable = 0;
                                    row.as.list.elements[row.as.list.length++] = v;
                                }
                                res.as.list.elements = realloc(res.as.list.elements, (res.as.list.length + 1) * sizeof(Value));
                                res.as.list.elements[res.as.list.length++] = row;
                            }
                            fclose(f);
                        }
                    }
                    free_value(path);
                }
            } else if (expr->op == (OpType)TOK_JRD) {
                if (expr->arg_count > 0) {
                    Value path = eval_expr(expr->args[0], sym, ft);
                    if (path.type == VAL_STRING) {
                        FILE* f = fopen(path.as.string, "r");
                        if (f) {
                            fseek(f, 0, SEEK_END);
                            long len = ftell(f);
                            fseek(f, 0, SEEK_SET);
                            char* content = malloc(len + 1);
                            if (content) {
                                size_t read_bytes = fread(content, 1, len, f);
                                content[read_bytes] = '\0';
                                res.type = VAL_STRING; res.as.string = content;
                            }
                            fclose(f);
                        }
                    }
                    free_value(path);
                }
            }
            return res;
        }
        case EXPR_BINARY: {
            if (expr->op == OP_NOT) {
                Value v = eval_expr(expr->left, sym, ft);
                res.type = VAL_BOOL; res.as.boolean = !is_truthy(v);
                free_value(v);
                return res;
            }
            Value left = eval_expr(expr->left, sym, ft);
            if (expr->op == OP_AND) {
                if (is_truthy(left)) {
                    free_value(left);
                    return eval_expr(expr->right, sym, ft);
                }
                return left;
            }
            if (expr->op == OP_OR) {
                if (is_truthy(left)) return left;
                free_value(left);
                return eval_expr(expr->right, sym, ft);
            }
            Value right = eval_expr(expr->right, sym, ft);
            if (expr->op == OP_IN) {
                res.type = VAL_BOOL; res.as.boolean = 0;
                if (right.type == VAL_ARRAY || right.type == VAL_SET || right.type == VAL_TUPLE) {
                    for (int i = 0; i < right.as.list.length; i++) {
                        if (values_equal(left, right.as.list.elements[i])) {
                            res.as.boolean = 1; break;
                        }
                    }
                } else if (right.type == VAL_MAP) {
                    for (int i = 0; i < right.as.map.length; i++) {
                        if (values_equal(left, right.as.map.keys[i])) {
                            res.as.boolean = 1; break;
                        }
                    }
                }
                free_value(left); free_value(right);
                return res;
            }
            if (expr->op == OP_EQ || expr->op == OP_NEQ) {
                res.type = VAL_BOOL;
                int eq = values_equal(left, right);
                res.as.boolean = (expr->op == OP_EQ) ? eq : !eq;
                free_value(left); free_value(right);
                return res;
            }
            if (left.type == VAL_STRING && right.type == VAL_STRING && expr->op == OP_ADD) {
                res.type = VAL_STRING;
                res.as.string = malloc(strlen(left.as.string) + strlen(right.as.string) + 1);
                strcpy(res.as.string, left.as.string);
                strcat(res.as.string, right.as.string);
                free_value(left); free_value(right);
                return res;
            }
            if ((left.type == VAL_INT || left.type == VAL_FLOAT) && (right.type == VAL_INT || right.type == VAL_FLOAT)) {
                double l = val_to_double(left);
                double r = val_to_double(right);
                double rnum = 0;
                int is_float = (left.type == VAL_FLOAT || right.type == VAL_FLOAT || expr->op == OP_DIV || expr->op == OP_POW);
                switch (expr->op) {
                    case OP_ADD: rnum = l + r; break;
                    case OP_SUB: rnum = l - r; break;
                    case OP_MUL: rnum = l * r; break;
                    case OP_DIV: rnum = l / r; break;
                    case OP_POW: rnum = pow(l, r); break;
                    case OP_GT:  res.type = VAL_BOOL; res.as.boolean = l > r; free_value(left); free_value(right); return res;
                    case OP_LT:  res.type = VAL_BOOL; res.as.boolean = l < r; free_value(left); free_value(right); return res;
                    case OP_GTE: res.type = VAL_BOOL; res.as.boolean = l >= r; free_value(left); free_value(right); return res;
                    case OP_LTE: res.type = VAL_BOOL; res.as.boolean = l <= r; free_value(left); free_value(right); return res;
                    default: break;
                }
                if (is_float) { res.type = VAL_FLOAT; res.as.float_val = rnum; }
                else { res.type = VAL_INT; res.as.integer = (long long)rnum; }
                free_value(left); free_value(right);
                return res;
            }
            free_value(left); free_value(right);
            return res;
        }
        case EXPR_CALL: {
            FuncDef* fd = NULL;
            Value fval = sym_get(sym, expr->var_name);
            if (fval.type == VAL_FN) {
                Value args[MAX_PARAMS];
                for (int i = 0; i < expr->arg_count; i++) args[i] = eval_expr(expr->args[i], sym, ft);
                Scope* old_current = sym->current;
                sym->current = clone_scope(fval.as.fn.closure);
                scope_push(sym);
                for (int i = 0; i < fval.as.fn.param_count && i < expr->arg_count; i++) 
                    sym_set(sym, fval.as.fn.params[i], args[i]);
                Value val = eval_expr(fval.as.fn.body, sym, ft);
                scope_pop(sym);
                free_scope(sym->current);
                sym->current = old_current;
                for (int i = 0; i < expr->arg_count; i++) free_value(args[i]);
                return val;
            }
            for (int i = 0; i < ft->count; i++) {
                if (strcmp(ft->funcs[i].name, expr->var_name) == 0) {
                    fd = &ft->funcs[i]; break;
                }
            }
            if (!fd) {
                if (expr->col > 0) set_error_col(expr->col);
                error_at(expr->line, "Undefined variable '%s'", expr->var_name);
            }
            Value args[MAX_PARAMS];
            for (int i = 0; i < expr->arg_count; i++) args[i] = eval_expr(expr->args[i], sym, ft);
            Value val;
            if (fd->is_builtin) val = fd->builtin_fn(args, expr->arg_count);
            else {
                scope_push(sym);
                for (int i = 0; i < fd->param_count; i++) sym_set(sym, fd->params[i], args[i]);
                val = eval_expr(fd->body, sym, ft);
                scope_pop(sym);
            }
            for (int i = 0; i < expr->arg_count; i++) free_value(args[i]);
            return val;
        }
        case EXPR_COALESCE:
        case EXPR_DFLT: {
            if (expr->left->type == EXPR_VARIABLE && !sym_exists(sym, expr->left->var_name))
                return eval_expr(expr->right, sym, ft);
            Value v = eval_expr(expr->left, sym, ft);
            if (v.type == VAL_NIL || (expr->type == EXPR_DFLT && v.type == VAL_ERROR)) {
                free_value(v);
                return eval_expr(expr->right, sym, ft);
            }
            return v;
        }
    }
    return res;
}

static Value builtin_ok(Value *args, int n) {
    Value res; res.type = VAL_BOOL; res.is_immutable = 0;
    if (n < 1) res.as.boolean = 0;
    else res.as.boolean = (args[0].type != VAL_ERROR);
    return res;
}

static Value builtin_typ(Value *args, int n) {
    Value res; res.type = VAL_STRING; res.is_immutable = 0;
    if (n < 1) { res.as.string = strdup("nil"); return res; }
    switch (args[0].type) {
        case VAL_NIL: res.as.string = strdup("nil"); break;
        case VAL_BOOL: res.as.string = strdup("bool"); break;
        case VAL_INT: res.as.string = strdup("int"); break;
        case VAL_FLOAT: res.as.string = strdup("flt"); break;
        case VAL_STRING: res.as.string = strdup("str"); break;
        case VAL_ARRAY: res.as.string = strdup("arr"); break;
        case VAL_MAP: res.as.string = strdup("map"); break;
        case VAL_SET: res.as.string = strdup("set"); break;
        case VAL_TUPLE: res.as.string = strdup("tpl"); break;
        case VAL_PAIR: res.as.string = strdup("pair"); break;
        case VAL_FN: res.as.string = strdup("fn"); break;
        case VAL_ERROR: res.as.string = strdup("err"); break;
        default: res.as.string = strdup("unknown"); break;
    }
    return res;
}

void register_core_builtins(FuncTable *ft) {
    if (ft->count >= MAX_FUNCS) return;
    FuncDef *fd = &ft->funcs[ft->count++];
    strncpy(fd->name, "ok", 63); fd->param_count = 1; fd->is_builtin = 1; fd->builtin_fn = builtin_ok;
    if (ft->count >= MAX_FUNCS) return;
    fd = &ft->funcs[ft->count++];
    strncpy(fd->name, "typ", 63); fd->param_count = 1; fd->is_builtin = 1; fd->builtin_fn = builtin_typ;
}

static int apply_condition(Value val, Condition* cond, SymTable* sym) {
    Value cmp;
    if (cond->is_variable) {
        cmp = sym_get(sym, cond->var_name);
    } else {
        cmp.type = VAL_FLOAT; cmp.as.float_val = cond->value;
    }
    int eq = values_equal(val, cmp);
    switch (cond->op) {
        case OP_EQ:  return eq;
        case OP_NEQ: return !eq;
        case OP_GT:  return val_to_double(val) > val_to_double(cmp);
        case OP_LT:  return val_to_double(val) < val_to_double(cmp);
        case OP_GTE: return val_to_double(val) >= val_to_double(cmp);
        case OP_LTE: return val_to_double(val) <= val_to_double(cmp);
        default: return 0;
    }
}

static Value apply_transform(Value val, Transform* trn, SymTable* sym, FuncTable* ft) {
    Value operand = eval_expr(trn->expr, sym, ft);
    Value res = val;
    if ((val.type == VAL_INT || val.type == VAL_FLOAT) && (operand.type == VAL_INT || operand.type == VAL_FLOAT)) {
        double l = val_to_double(val);
        double r = val_to_double(operand);
        double rnum = 0;
        int is_float = (val.type == VAL_FLOAT || operand.type == VAL_FLOAT || trn->op == OP_DIV || trn->op == OP_POW);
        switch (trn->op) {
            case OP_ADD: rnum = l + r; break;
            case OP_SUB: rnum = l - r; break;
            case OP_MUL: rnum = l * r; break;
            case OP_DIV: rnum = l / r; break;
            case OP_POW: rnum = pow(l, r); break;
            default: rnum = l; break;
        }
        if (is_float) { res.type = VAL_FLOAT; res.as.float_val = rnum; }
        else { res.type = VAL_INT; res.as.integer = (long long)rnum; }
    }
    free_value(operand);
    return res;
}

static void exec_pipeline(PipelineNode* node, SymTable* sym, FuncTable* ft) {
    Value list_v = eval_expr(node->list_expr, sym, ft);
    if (list_v.type != VAL_ARRAY && list_v.type != VAL_SET && list_v.type != VAL_TUPLE) {
        free_value(list_v); return;
    }
    
    Value acc; acc.type = VAL_INT; acc.as.integer = 0;
    int do_sum = node->has_sum;
    int has_label = node->has_emt_label;
    int has_sep   = node->has_emt_sep;
    int first = 1;

    for (int i = 0; i < list_v.as.list.length; i++) {
        Value val = clone_value(list_v.as.list.elements[i]);
        if (node->has_filter && !apply_condition(val, node->filter, sym)) {
            free_value(val); continue;
        }
        if (node->has_transform) {
            for (int t = 0; t < node->transform_count; t++) {
                Value next = apply_transform(val, node->transforms[t], sym, ft);
                free_value(val);
                val = next;
            }
        }
        if (do_sum) {
            double l = val_to_double(acc);
            double r = val_to_double(val);
            if (acc.type == VAL_FLOAT || val.type == VAL_FLOAT) {
                acc.type = VAL_FLOAT; acc.as.float_val = l + r;
            } else {
                acc.type = VAL_INT; acc.as.integer = (long long)(l + r);
            }
        } else {
            if (has_sep) {
                if (!first) printf("%s", node->emt_sep);
                if (first && has_label) printf("%s ", node->emt_label);
                emit_value_no_newline(val);
                first = 0;
            } else {
                if (has_label) emit_labeled_value(node->emt_label, val);
                else emit_value(val);
            }
        }
        free_value(val);
    }
    if (do_sum) {
        if (has_label) emit_labeled_value(node->emt_label, acc);
        else emit_value(acc);
    } else if (has_sep && !first) {
        printf("\n");
    }
    free_value(list_v);
}

static Value read_input(int line) {
    char buf[256];
    if (!fgets(buf, sizeof(buf), stdin)) {
        error_at(line, "Expected input but got EOF");
    }
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
        buf[--len] = '\0';
    
    char *endptr;
    errno = 0;
    double val = strtod(buf, &endptr);
    while (*endptr == ' ' || *endptr == '\t') endptr++;
    if (endptr == buf || *endptr != '\0') {
        error_at(line, "Expected a number, got '%s'", buf);
    }
    Value res;
    if (floor(val) == val) {
        res.type = VAL_INT; res.as.integer = (long long)val;
    } else {
        res.type = VAL_FLOAT; res.as.float_val = val;
    }
    res.is_immutable = 0;
    return res;
}

static ExecResult exec_block(BlockNode* block, SymTable* sym, FuncTable* ft) {
    ExecResult res; res.status = STATUS_NORMAL; res.value.type = VAL_NIL; res.value.is_immutable = 0;
    for (int i = 0; i < block->count; i++) {
        res = execute(block->nodes[i], sym, ft);
        if (res.status != STATUS_NORMAL) return res;
    }
    return res;
}

ExecResult execute(ASTNode* ast, SymTable* sym, FuncTable* ft) {
    ExecResult res; res.status = STATUS_NORMAL; res.value.type = VAL_NIL; res.value.is_immutable = 0;
    if (!ast) return res;
    switch (ast->stmt_type) {
        case STMT_BLOCK: return exec_block(ast->node.block, sym, ft);
        case STMT_EXT: {
            int code = 0;
            if (ast->node.generic->expr) {
                Value v = eval_expr(ast->node.generic->expr, sym, ft);
                code = (int)val_to_double(v);
                free_value(v);
            }
            exit(code);
        }
        case STMT_STP: exit(1);
        case STMT_THR: {
            Value v = eval_expr(ast->node.generic->expr, sym, ft);
            res.status = STATUS_ERROR;
            res.value = v;
            return res;
        }
        case STMT_TRY: {
            res = exec_block(&ast->node.try_node->try_block, sym, ft);
            if (res.status == STATUS_ERROR) {
                if (ast->node.try_node->has_catch) {
                    Value err_val = res.value;
                    res.status = STATUS_NORMAL;
                    res.value.type = VAL_NIL;
                    scope_push(sym);
                    if (ast->node.try_node->err_var[0]) {
                        sym_set(sym, ast->node.try_node->err_var, err_val);
                    }
                    free_value(err_val);
                    ExecResult catch_res = exec_block(&ast->node.try_node->catch_block, sym, ft);
                    scope_pop(sym);
                    return catch_res;
                }
            }
            return res;
        }
        case STMT_ASRT: {
            Value v = eval_expr(ast->node.generic->expr, sym, ft);
            if (!is_truthy(v)) {
                error_at(ast->node.generic->line, "Assertion failed");
            }
            free_value(v);
            break;
        }
        case STMT_DBG: {
            Value v = eval_expr(ast->node.generic->expr, sym, ft);
            char* s = value_to_string(v);
            fprintf(stderr, "DEBUG: %s\n", s);
            free(s); free_value(v);
            break;
        }
        case STMT_LOG: {
            Value v = eval_expr(ast->node.generic->expr, sym, ft);
            char* s = value_to_string(v);
            fprintf(stderr, "LOG: %s\n", s);
            free(s); free_value(v);
            break;
        }
        case STMT_TRC: {
            Value v = eval_expr(ast->node.generic->expr, sym, ft);
            char* s = value_to_string(v);
            fprintf(stderr, "TRACE: %s\n", s);
            free(s); free_value(v);
            break;
        }
        case STMT_DOC: break;
        case STMT_PKG: break;
        case STMT_EXP: break;
        case STMT_IMP: {
            if (strcmp(ast->node.imp_node->module_name, "math") == 0) register_math_module(ft);
            else if (strcmp(ast->node.imp_node->module_name, "io") == 0) register_io_module(ft);
            break;
        }
        case STMT_TST: {
            Value v = eval_expr(ast->node.tst_node->expr, sym, ft);
            if (!is_truthy(v)) {
                fprintf(stderr, "TEST FAILED: %s (line %d)\n", ast->node.tst_node->label, ast->node.tst_node->line);
            } else {
                fprintf(stderr, "TEST PASSED: %s\n", ast->node.tst_node->label);
            }
            free_value(v);
            break;
        }
        case STMT_CHK: {
            Value v = eval_expr(ast->node.chk_node->expr, sym, ft);
            const char* expected = ast->node.chk_node->type_name;
            int ok = 0;
            if (strcmp(expected, "int") == 0) ok = (v.type == VAL_INT);
            else if (strcmp(expected, "flt") == 0) ok = (v.type == VAL_FLOAT);
            else if (strcmp(expected, "str") == 0) ok = (v.type == VAL_STRING);
            else if (strcmp(expected, "bool") == 0) ok = (v.type == VAL_BOOL);
            else if (strcmp(expected, "arr") == 0) ok = (v.type == VAL_ARRAY);
            if (!ok) error_at(ast->node.chk_node->line, "Type check failed");
            free_value(v);
            break;
        }
        case STMT_ASSIGN: {
            Value v;
            if (ast->node.assign->rhs_type == ASSIGN_INPUT) {
                v = read_input(ast->node.assign->line);
            } else {
                v = eval_expr(ast->node.assign->expr, sym, ft);
            }
            sym_set(sym, ast->node.assign->name, v);
            free_value(v);
            break;
        }
        case STMT_LET: {
            Value v = eval_expr(ast->node.let_node->value, sym, ft);
            v.is_immutable = 1;
            sym_set(sym, ast->node.let_node->name, v);
            free_value(v);
            break;
        }
        case STMT_DECL: {
            Value v = eval_expr(ast->node.decl_node->value, sym, ft);
            // Optionally check if type(v) matches decl_node->type_name
            sym_set(sym, ast->node.decl_node->var_name, v);
            free_value(v);
            break;
        }
        case STMT_ARITH: {
            Value v = eval_expr((Expr*)ast->node.arith, sym, ft);
            if (ast->has_emt_label) {
                emit_labeled_value(ast->emt_label, v);
            } else {
                emit_value(v);
            }
            free_value(v);
            break;
        }
        case STMT_PIPELINE: exec_pipeline(ast->node.pipeline, sym, ft); break;
        case STMT_FN_DEF: {
            FuncDef* fd = &ft->funcs[ft->count++];
            strncpy(fd->name, ast->node.fn_def->name, 63);
            fd->name[63] = '\0';
            fd->param_count = ast->node.fn_def->param_count;
            for (int i = 0; i < fd->param_count; i++) {
                strncpy(fd->params[i], ast->node.fn_def->params[i], 63);
                fd->params[i][63] = '\0';
            }
            fd->body = clone_expr(ast->node.fn_def->body);
            fd->is_builtin = 0;
            break;
        }
        case STMT_USE:
            if (strcmp(ast->node.use_stmt->module_name, "math") == 0) register_math_module(ft);
            else if (strcmp(ast->node.use_stmt->module_name, "io") == 0) register_io_module(ft);
            else if (strcmp(ast->node.use_stmt->module_name, "list") == 0) register_list_module(ft);
            else if (strcmp(ast->node.use_stmt->module_name, "string") == 0) register_string_module(ft);
            break;
        case STMT_IO: {
            IONode* io = ast->node.io_node;
            if (io->io_type == TOK_SAY || io->io_type == TOK_PRT) {
                for (int i = 0; i < io->arg_count; i++) {
                    Value v = eval_expr(io->args[i], sym, ft);
                    emit_value_no_newline(v);
                    if (i < io->arg_count - 1) printf(" ");
                    free_value(v);
                }
                if (io->io_type == TOK_SAY) printf("\n");
                fflush(stdout);
            } else if (io->io_type == TOK_FWR || io->io_type == TOK_FAP) {
                if (io->arg_count >= 2) {
                    Value path = eval_expr(io->args[0], sym, ft);
                    Value data = eval_expr(io->args[1], sym, ft);
                    if (path.type == VAL_STRING) {
                        FILE* f = fopen(path.as.string, io->io_type == TOK_FWR ? "w" : "a");
                        if (f) {
                            char* s = value_to_string(data);
                            fprintf(f, "%s", s);
                            free(s);
                            fclose(f);
                        }
                    }
                    free_value(path); free_value(data);
                }
            }
            break;
        }
        case STMT_INPT: {
            if (ast->node.inpt->has_prompt) { printf("%s", ast->node.inpt->prompt); fflush(stdout); }
            Value v = read_input(ast->node.inpt->line);
            sym_set(sym, ast->node.inpt->var_name, v);
            free_value(v);
            break;
        }
        case STMT_IF: {
            Value cond = eval_expr(ast->node.if_node->condition, sym, ft);
            if (is_truthy(cond)) {
                res = exec_block(&ast->node.if_node->then_block, sym, ft);
            } else {
                int done = 0;
                for (int i = 0; i < ast->node.if_node->elif_count; i++) {
                    Value elif_cond = eval_expr(ast->node.if_node->elif_branches[i].condition, sym, ft);
                    if (is_truthy(elif_cond)) {
                        res = exec_block(&ast->node.if_node->elif_branches[i].body, sym, ft);
                        done = 1; free_value(elif_cond); break;
                    }
                    free_value(elif_cond);
                }
                if (!done && ast->node.if_node->has_else) {
                    res = exec_block(&ast->node.if_node->else_block, sym, ft);
                }
            }
            free_value(cond);
            break;
        }
        case STMT_FOR: {
            Value start_v = eval_expr(ast->node.for_node->start, sym, ft);
            Value end_v = eval_expr(ast->node.for_node->end, sym, ft);
            double start = val_to_double(start_v);
            double end = val_to_double(end_v);
            for (double i = start; i <= end; i++) {
                Value v;
                if (floor(i) == i) { v.type = VAL_INT; v.as.integer = (long long)i; }
                else { v.type = VAL_FLOAT; v.as.float_val = i; }
                v.is_immutable = 0;
                sym_set(sym, ast->node.for_node->var_name, v);
                res = exec_block(&ast->node.for_node->body, sym, ft);
                if (res.status == STATUS_BREAK) { res.status = STATUS_NORMAL; break; }
                if (res.status == STATUS_RETURN) break;
                if (res.status == STATUS_CONTINUE) res.status = STATUS_NORMAL;
            }
            free_value(start_v); free_value(end_v);
            break;
        }
        case STMT_WHL: {
            while (1) {
                Value cond = eval_expr(ast->node.whl_node->condition, sym, ft);
                if (is_truthy(cond)) {
                    free_value(cond);
                    res = exec_block(&ast->node.whl_node->body, sym, ft);
                    if (res.status == STATUS_BREAK) { res.status = STATUS_NORMAL; break; }
                    if (res.status == STATUS_RETURN) break;
                    if (res.status == STATUS_CONTINUE) res.status = STATUS_NORMAL;
                } else { free_value(cond); break; }
            }
            break;
        }
        case STMT_EACH: {
            Value list_v = sym_get(sym, ast->node.each_node->list_name);
            if (list_v.type == VAL_ARRAY || list_v.type == VAL_SET || list_v.type == VAL_TUPLE) {
                for (int i = 0; i < list_v.as.list.length; i++) {
                    Value item = clone_value(list_v.as.list.elements[i]);
                    sym_set(sym, ast->node.each_node->item_name, item);
                    res = exec_block(&ast->node.each_node->body, sym, ft);
                    free_value(item);
                    if (res.status == STATUS_BREAK) { res.status = STATUS_NORMAL; break; }
                    if (res.status == STATUS_RETURN) break;
                    if (res.status == STATUS_CONTINUE) res.status = STATUS_NORMAL;
                }
            }
            break;
        }
        case STMT_RPT: {
            Value count_v = eval_expr(ast->node.rpt_node->count, sym, ft);
            long long count = (count_v.type == VAL_INT) ? count_v.as.integer : (long long)val_to_double(count_v);
            for (long long i = 0; i < count; i++) {
                res = exec_block(&ast->node.rpt_node->body, sym, ft);
                if (res.status == STATUS_BREAK) { res.status = STATUS_NORMAL; break; }
                if (res.status == STATUS_RETURN) break;
                if (res.status == STATUS_CONTINUE) res.status = STATUS_NORMAL;
            }
            free_value(count_v);
            break;
        }
        case STMT_BRK: res.status = STATUS_BREAK; break;
        case STMT_NXT: res.status = STATUS_CONTINUE; break;
        case STMT_RET:
            res.status = STATUS_RETURN;
            res.value = eval_expr(ast->node.ret_node->value, sym, ft);
            break;
        default: break;
    }
    return res;
}
