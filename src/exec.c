#include "exec.h"
#include "error.h"
#include "output.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

static int is_argn_variable(const char* name) {
    if (name[0] != 'a' || name[1] != 'r' || name[2] != 'g' || name[3] == '\0')
        return 0;
    for (int i = 3; name[i] != '\0'; i++)
        if (name[i] < '0' || name[i] > '9') return 0;
    return 1;
}

static void warn_argn_undefined(SymTable* sym, const char* name) {
    fflush(stdout);
    int argc_val = sym_exists(sym, "argc") ? (int)sym_get(sym, "argc").as.number : 0;
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

SymTable* create_symtable(void) {
    SymTable* t = malloc(sizeof(SymTable));
    t->current = calloc(1, sizeof(Scope));
    t->current->parent = NULL;
    return t;
}

void free_symtable(SymTable* t) {
    Scope* s = t->current;
    while (s) {
        Scope* parent = s->parent;
        for (int i = 0; i < SCOPE_CAPACITY; i++) {
            if (s->slots[i].occupied && s->slots[i].value.type == VAL_LIST) {
                free(s->slots[i].value.as.list.elements);
            }
        }
        free(s);
        s = parent;
    }
    free(t);
}

void scope_push(SymTable* t) {
    Scope* s = calloc(1, sizeof(Scope));
    s->parent = t->current;
    t->current = s;
}

void scope_pop(SymTable* t) {
    if (!t->current->parent) return;
    Scope* old = t->current;
    t->current = old->parent;
    for (int i = 0; i < SCOPE_CAPACITY; i++) {
        if (old->slots[i].occupied && old->slots[i].value.type == VAL_LIST) {
            free(old->slots[i].value.as.list.elements);
        }
    }
    free(old);
}

static Value clone_value(Value v) {
    if (v.type == VAL_LIST) {
        Value res = v;
        res.as.list.elements = malloc(v.as.list.length * sizeof(double));
        memcpy(res.as.list.elements, v.as.list.elements, v.as.list.length * sizeof(double));
        return res;
    }
    return v;
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
            if (e->value.type == VAL_LIST) free(e->value.as.list.elements);
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
    Value v; v.type = VAL_NUMBER; v.as.number = 0; v.is_immutable = 0;
    return v;
}

static Expr* clone_expr(const Expr* src) {
    if (!src) return NULL;
    Expr* dst = malloc(sizeof(Expr));
    *dst = *src;
    if (src->type == EXPR_BINARY || src->type == EXPR_COALESCE) {
        dst->left = clone_expr(src->left);
        dst->right = clone_expr(src->right);
    } else if (src->type == EXPR_CALL || src->type == EXPR_LIST) {
        for (int i = 0; i < src->arg_count; i++)
            dst->args[i] = clone_expr(src->args[i]);
    }
    return dst;
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

static Value eval_expr(Expr* expr, SymTable* sym, FuncTable* ft) {
    Value res; res.type = VAL_NUMBER; res.as.number = 0; res.is_immutable = 0;
    if (!expr) return res;

    switch (expr->type) {
        case EXPR_NUMBER:
            res.as.number = expr->num_val;
            return res;
        case EXPR_VARIABLE:
            if (!sym_exists(sym, expr->var_name)) {
                if (is_argn_variable(expr->var_name)) warn_argn_undefined(sym, expr->var_name);
                else {
                    if (expr->col > 0) set_error_col(expr->col);
                    error_at(expr->line, "Undefined variable '%s'", expr->var_name);
                }
                return res;
            }
            return sym_get(sym, expr->var_name);
        case EXPR_LIST:
            res.type = VAL_LIST;
            res.as.list.length = expr->arg_count;
            res.as.list.elements = malloc(expr->arg_count * sizeof(double));
            for (int i = 0; i < expr->arg_count; i++) {
                Value v = eval_expr(expr->args[i], sym, ft);
                res.as.list.elements[i] = (v.type == VAL_NUMBER) ? v.as.number : 0.0;
            }
            return res;
        case EXPR_BINARY: {
            if (expr->op == OP_NOT) {
                Value v = eval_expr(expr->left, sym, ft);
                res.as.number = (v.type == VAL_NUMBER && v.as.number == 0) ? 1.0 : 0.0;
                return res;
            }
            Value left = eval_expr(expr->left, sym, ft);
            if (expr->op == OP_AND) {
                if (left.type == VAL_NUMBER && left.as.number != 0) {
                    return eval_expr(expr->right, sym, ft);
                }
                return left;
            }
            if (expr->op == OP_OR) {
                if (left.type == VAL_NUMBER && left.as.number != 0) {
                    return left;
                }
                return eval_expr(expr->right, sym, ft);
            }
            Value right = eval_expr(expr->right, sym, ft);
            if (expr->op == OP_IN) {
                res.as.number = 0.0;
                if (right.type == VAL_LIST && left.type == VAL_NUMBER) {
                    for (int i = 0; i < right.as.list.length; i++) {
                        if (right.as.list.elements[i] == left.as.number) {
                            res.as.number = 1.0; break;
                        }
                    }
                }
                return res;
            }
            if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                double l = left.as.number;
                double r = right.as.number;
                switch (expr->op) {
                    case OP_ADD: res.as.number = l + r; break;
                    case OP_SUB: res.as.number = l - r; break;
                    case OP_MUL: res.as.number = l * r; break;
                    case OP_DIV: res.as.number = l / r; break;
                    case OP_POW: res.as.number = pow(l, r); break;
                    case OP_GT:  res.as.number = l > r; break;
                    case OP_LT:  res.as.number = l < r; break;
                    case OP_GTE: res.as.number = l >= r; break;
                    case OP_LTE: res.as.number = l <= r; break;
                    case OP_EQ:  res.as.number = l == r; break;
                    case OP_NEQ: res.as.number = l != r; break;
                    default: break;
                }
            }
            return res;
        }
        case EXPR_CALL: {
            FuncDef* fd = NULL;
            for (int i = 0; i < ft->count; i++) {
                if (strcmp(ft->funcs[i].name, expr->var_name) == 0) {
                    fd = &ft->funcs[i]; break;
                }
            }
            if (!fd) error_at(expr->line, "Undefined function '%s'", expr->var_name);
            Value args[MAX_PARAMS];
            for (int i = 0; i < expr->arg_count; i++) args[i] = eval_expr(expr->args[i], sym, ft);
            if (fd->is_builtin) return fd->builtin_fn(args, expr->arg_count);
            scope_push(sym);
            for (int i = 0; i < fd->param_count; i++) sym_set(sym, fd->params[i], args[i]);
            Value val = eval_expr(fd->body, sym, ft);
            scope_pop(sym);
            return val;
        }
        case EXPR_COALESCE: {
            if (expr->left->type == EXPR_VARIABLE && !sym_exists(sym, expr->left->var_name))
                return eval_expr(expr->right, sym, ft);
            Value v = eval_expr(expr->left, sym, ft);
            return v;
        }
    }
    return res;
}

static double apply_condition(double val, Condition* cond, SymTable* sym) {
    double cmp;
    if (cond->is_variable) {
        Value v = sym_get(sym, cond->var_name);
        cmp = (v.type == VAL_NUMBER) ? v.as.number : 0.0;
    } else {
        cmp = cond->value;
    }
    switch (cond->op) {
        case OP_GT:  return val > cmp;
        case OP_LT:  return val < cmp;
        case OP_GTE: return val >= cmp;
        case OP_LTE: return val <= cmp;
        case OP_EQ:  return val == cmp;
        case OP_NEQ: return val != cmp;
        default: return 0.0;
    }
}

static double apply_transform(double val, Transform* trn, SymTable* sym, FuncTable* ft) {
    Value op_v = eval_expr(trn->expr, sym, ft);
    double operand = (op_v.type == VAL_NUMBER) ? op_v.as.number : 0.0;
    switch (trn->op) {
        case OP_ADD: return val + operand;
        case OP_SUB: return val - operand;
        case OP_MUL: return val * operand;
        case OP_DIV: return val / operand;
        case OP_POW: return pow(val, operand);
        default: return val;
    }
}

static void exec_pipeline(PipelineNode* node, SymTable* sym, FuncTable* ft) {
    double acc = 0.0;
    int do_sum = node->has_sum;
    int has_label = node->has_emt_label;
    int has_sep   = node->has_emt_sep;
    if (has_sep && !do_sum) {
        int first = 1;
        for (int i = 0; i < node->list_len; i++) {
            double val = node->list[i];
            if (node->has_filter && !apply_condition(val, node->filter, sym)) continue;
            if (node->has_transform) {
                for (int t = 0; t < node->transform_count; t++)
                    val = apply_transform(val, node->transforms[t], sym, ft);
            }
            if (!first) printf("%s", node->emt_sep);
            if (first && has_label) printf("%s ", node->emt_label);
            emit_value_no_newline(val);
            first = 0;
        }
        if (!first) printf("\n");
        return;
    }
    for (int i = 0; i < node->list_len; i++) {
        double val = node->list[i];
        if (node->has_filter && !apply_condition(val, node->filter, sym)) continue;
        if (node->has_transform) {
            for (int t = 0; t < node->transform_count; t++)
                val = apply_transform(val, node->transforms[t], sym, ft);
        }
        if (do_sum) acc += val;
        else if (has_label) emit_labeled_value(node->emt_label, val);
        else emit_value(val);
    }
    if (do_sum) {
        if (has_label) emit_labeled_value(node->emt_label, acc);
        else emit_value(acc);
    }
}

static double read_numeric_input(int line) {
    char buf[256];
    if (!fgets(buf, sizeof(buf), stdin)) {
        error_at(line, "Expected numeric input but got EOF");
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
    return val;
}

static ExecResult exec_block(BlockNode* block, SymTable* sym, FuncTable* ft) {
    ExecResult res; res.status = STATUS_NORMAL; res.value.type = VAL_NUMBER; res.value.as.number = 0; res.value.is_immutable = 0;
    for (int i = 0; i < block->count; i++) {
        res = execute(block->nodes[i], sym, ft);
        if (res.status != STATUS_NORMAL) return res;
    }
    return res;
}

ExecResult execute(ASTNode* ast, SymTable* sym, FuncTable* ft) {
    ExecResult res; res.status = STATUS_NORMAL; res.value.type = VAL_NUMBER; res.value.as.number = 0; res.value.is_immutable = 0;
    if (!ast) return res;
    switch (ast->stmt_type) {
        case STMT_BLOCK: return exec_block(ast->node.block, sym, ft);
        case STMT_ASSIGN: {
            Value v; v.type = VAL_NUMBER; v.as.number = 0; v.is_immutable = 0;
            if (ast->node.assign->rhs_type == ASSIGN_INPUT) {
                v.as.number = read_numeric_input(ast->node.assign->line);
            } else {
                v = eval_expr(ast->node.assign->expr, sym, ft);
            }
            sym_set(sym, ast->node.assign->name, v);
            break;
        }
        case STMT_LET: {
            Value v = eval_expr(ast->node.let_node->value, sym, ft);
            v.is_immutable = 1;
            sym_set(sym, ast->node.let_node->name, v);
            break;
        }
        case STMT_ARITH: {
            Value v = eval_expr((Expr*)ast->node.arith, sym, ft);
            if (ast->has_emt_label) {
                if (v.type == VAL_NUMBER) emit_labeled_value(ast->emt_label, v.as.number);
            } else {
                if (v.type == VAL_NUMBER) emit_value(v.as.number);
                else if (v.type == VAL_LIST) {
                    printf("[");
                    for (int i=0; i<v.as.list.length; i++) {
                        emit_value_no_newline(v.as.list.elements[i]);
                        if (i < v.as.list.length-1) printf(", ");
                    }
                    printf("]\n");
                }
            }
            break;
        }
        case STMT_PIPELINE: exec_pipeline(ast->node.pipeline, sym, ft); break;
        case STMT_FN_DEF: {
            FuncDef* fd = &ft->funcs[ft->count++];
            strncpy(fd->name, ast->node.fn_def->name, 63);
            fd->param_count = ast->node.fn_def->param_count;
            for (int i = 0; i < fd->param_count; i++) strncpy(fd->params[i], ast->node.fn_def->params[i], 63);
            fd->body = clone_expr(ast->node.fn_def->body);
            fd->is_builtin = 0;
            break;
        }
        case STMT_USE:
            if (strcmp(ast->node.use_stmt->module_name, "math") == 0) register_math_module(ft);
            else if (strcmp(ast->node.use_stmt->module_name, "io") == 0) register_io_module(ft);
            break;
        case STMT_INPT: {
            if (ast->node.inpt->has_prompt) { printf("%s", ast->node.inpt->prompt); fflush(stdout); }
            Value v; v.type = VAL_NUMBER; v.as.number = read_numeric_input(ast->node.inpt->line); v.is_immutable = 0;
            sym_set(sym, ast->node.inpt->var_name, v);
            break;
        }
        case STMT_IF: {
            Value cond = eval_expr(ast->node.if_node->condition, sym, ft);
            if (cond.type == VAL_NUMBER && cond.as.number != 0) {
                res = exec_block(&ast->node.if_node->then_block, sym, ft);
            } else {
                int done = 0;
                for (int i = 0; i < ast->node.if_node->elif_count; i++) {
                    Value elif_cond = eval_expr(ast->node.if_node->elif_branches[i].condition, sym, ft);
                    if (elif_cond.type == VAL_NUMBER && elif_cond.as.number != 0) {
                        res = exec_block(&ast->node.if_node->elif_branches[i].body, sym, ft);
                        done = 1; break;
                    }
                }
                if (!done && ast->node.if_node->has_else) {
                    res = exec_block(&ast->node.if_node->else_block, sym, ft);
                }
            }
            break;
        }
        case STMT_FOR: {
            Value start = eval_expr(ast->node.for_node->start, sym, ft);
            Value end = eval_expr(ast->node.for_node->end, sym, ft);
            if (start.type == VAL_NUMBER && end.type == VAL_NUMBER) {
                for (double i = start.as.number; i <= end.as.number; i++) {
                    Value v; v.type = VAL_NUMBER; v.as.number = i; v.is_immutable = 0;
                    sym_set(sym, ast->node.for_node->var_name, v);
                    res = exec_block(&ast->node.for_node->body, sym, ft);
                    if (res.status == STATUS_BREAK) { res.status = STATUS_NORMAL; break; }
                    if (res.status == STATUS_RETURN) break;
                    if (res.status == STATUS_CONTINUE) res.status = STATUS_NORMAL;
                }
            }
            break;
        }
        case STMT_WHL: {
            while (1) {
                Value cond = eval_expr(ast->node.whl_node->condition, sym, ft);
                if (cond.type == VAL_NUMBER && cond.as.number != 0) {
                    res = exec_block(&ast->node.whl_node->body, sym, ft);
                    if (res.status == STATUS_BREAK) { res.status = STATUS_NORMAL; break; }
                    if (res.status == STATUS_RETURN) break;
                    if (res.status == STATUS_CONTINUE) res.status = STATUS_NORMAL;
                } else break;
            }
            break;
        }
        case STMT_EACH: {
            Value list_v = sym_get(sym, ast->node.each_node->list_name);
            if (list_v.type == VAL_LIST) {
                for (int i = 0; i < list_v.as.list.length; i++) {
                    Value item; item.type = VAL_NUMBER; item.as.number = list_v.as.list.elements[i]; item.is_immutable = 0;
                    sym_set(sym, ast->node.each_node->item_name, item);
                    res = exec_block(&ast->node.each_node->body, sym, ft);
                    if (res.status == STATUS_BREAK) { res.status = STATUS_NORMAL; break; }
                    if (res.status == STATUS_RETURN) break;
                    if (res.status == STATUS_CONTINUE) res.status = STATUS_NORMAL;
                }
            }
            break;
        }
        case STMT_RPT: {
            Value count = eval_expr(ast->node.rpt_node->count, sym, ft);
            if (count.type == VAL_NUMBER) {
                for (int i = 0; i < (int)count.as.number; i++) {
                    res = exec_block(&ast->node.rpt_node->body, sym, ft);
                    if (res.status == STATUS_BREAK) { res.status = STATUS_NORMAL; break; }
                    if (res.status == STATUS_RETURN) break;
                    if (res.status == STATUS_CONTINUE) res.status = STATUS_NORMAL;
                }
            }
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
