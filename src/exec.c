#include "exec.h"
#include "error.h"
#include "output.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* djb2 hash, folded into [0, SCOPE_CAPACITY). */
static unsigned int scope_hash(const char* name) {
    unsigned int h = 5381;
    while (*name) h = ((h << 5) + h) + (unsigned char)*name++;
    return h & (SCOPE_CAPACITY - 1);
}

/* Find the occupied ScopeEntry for 'name' in a single scope level.
   Returns NULL when the name is absent. */
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

/* Find an empty slot or the existing slot for 'name' to insert / update.
   Returns NULL only when every slot is occupied (table full). */
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
    if (!t->current->parent) return; /* never pop the global scope */
    Scope* old = t->current;
    t->current = old->parent;
    free(old);
}

void sym_set(SymTable* t, const char* name, double val) {
    ScopeEntry* e = scope_reserve(t->current, name);
    if (!e) {
        error_at(0, "Symbol table full: cannot store variable '%s'", name);
    }
    strncpy(e->name, name, 63);
    e->name[63] = '\0';
    e->value    = val;
    e->occupied = 1;
}

int sym_exists(SymTable* t, const char* name) {
    for (Scope* s = t->current; s; s = s->parent) {
        if (scope_lookup(s, name)) return 1;
    }
    return 0;
}

double sym_get(SymTable* t, const char* name) {
    for (Scope* s = t->current; s; s = s->parent) {
        ScopeEntry* e = scope_lookup(s, name);
        if (e) return e->value;
    }
    error_at(0, "Undefined variable '%s'", name);
    return 0.0; /* unreachable */
}

/* ---- Function table ---- */

/* Deep-clone an Expr tree (used when storing a function body). */
static Expr* clone_expr(const Expr* src) {
    if (!src) return NULL;
    Expr* dst = malloc(sizeof(Expr));
    *dst = *src; /* copies all scalar fields shallowly */
    if (src->type == EXPR_BINARY) {
        dst->left  = clone_expr(src->left);
        dst->right = clone_expr(src->right);
    } else if (src->type == EXPR_CALL) {
        for (int i = 0; i < src->arg_count; i++)
            dst->args[i] = clone_expr(src->args[i]);
    } else if (src->type == EXPR_COALESCE) {
        dst->left  = clone_expr(src->left);
        dst->right = clone_expr(src->right);
    }
    return dst;
}

FuncTable* create_functable(void) {
    FuncTable* ft = calloc(1, sizeof(FuncTable));
    return ft;
}

void free_functable(FuncTable* ft) {
    for (int i = 0; i < ft->count; i++) {
        if (!ft->funcs[i].is_builtin)
            free_expr(ft->funcs[i].body);
    }
    free(ft);
}

/* ---- Expression evaluator ---- */

static double eval_expr(Expr* expr, SymTable* sym, FuncTable* ft);

static double eval_expr(Expr* expr, SymTable* sym, FuncTable* ft) {
    if (!expr) return 0.0;
    
    switch (expr->type) {
        case EXPR_NUMBER:
            return expr->num_val;
            
        case EXPR_VARIABLE:
            if (!sym_exists(sym, expr->var_name)) {
                error_at(expr->line, "Undefined variable '%s'", expr->var_name);
            }
            return sym_get(sym, expr->var_name);
            
        case EXPR_BINARY: {
            double left = eval_expr(expr->left, sym, ft);
            double right = eval_expr(expr->right, sym, ft);
            
            switch (expr->op) {
                case OP_ADD: return left + right;
                case OP_SUB: return left - right;
                case OP_MUL: return left * right;
                case OP_DIV: return left / right;
                case OP_POW: {
                    double result = 1.0;
                    int n = (int)right;
                    if (n >= 0) {
                        for (int i = 0; i < n; i++) result *= left;
                    } else {
                        for (int i = 0; i < -n; i++) result /= left;
                    }
                    return result;
                }
                default: return left;
            }
        }

        case EXPR_CALL: {
            /* Look up the function definition */
            FuncDef* fd = NULL;
            for (int i = 0; i < ft->count; i++) {
                if (strcmp(ft->funcs[i].name, expr->var_name) == 0) {
                    fd = &ft->funcs[i];
                    break;
                }
            }
            if (!fd) {
                error_at(expr->line, "Undefined function '%s'", expr->var_name);
            }
            if (expr->arg_count != fd->param_count) {
                error_at(expr->line,
                         "Function '%s' expects %d argument(s) but got %d",
                         expr->var_name, fd->param_count, expr->arg_count);
            }
            /* Evaluate all arguments in the caller's scope before pushing */
            double arg_vals[MAX_PARAMS];
            for (int i = 0; i < expr->arg_count; i++)
                arg_vals[i] = eval_expr(expr->args[i], sym, ft);

            if (fd->is_builtin) {
                return fd->builtin_fn(arg_vals, expr->arg_count);
            }

            /* Push function scope, bind parameters, evaluate body, pop */
            scope_push(sym);
            for (int i = 0; i < fd->param_count; i++)
                sym_set(sym, fd->params[i], arg_vals[i]);
            double result = eval_expr(fd->body, sym, ft);
            scope_pop(sym);
            return result;
        }

        case EXPR_COALESCE: {
            /* If the left side is a variable that does not exist, return the
               right-hand fallback value without raising an error. */
            if (expr->left && expr->left->type == EXPR_VARIABLE &&
                !sym_exists(sym, expr->left->var_name)) {
                return eval_expr(expr->right, sym, ft);
            }
            return eval_expr(expr->left, sym, ft);
        }
    }
    
    return 0.0;
}

static double apply_condition(double val, Condition* cond) {
    switch (cond->op) {
        case OP_GT:  return val > cond->value ? 1.0 : 0.0;
        case OP_LT:  return val < cond->value ? 1.0 : 0.0;
        case OP_GTE: return val >= cond->value ? 1.0 : 0.0;
        case OP_LTE: return val <= cond->value ? 1.0 : 0.0;
        case OP_EQ:  return val == cond->value ? 1.0 : 0.0;
        case OP_NEQ: return val != cond->value ? 1.0 : 0.0;
        default: return 0.0;
    }
}

static double apply_transform(double val, Transform* trn, SymTable* sym, FuncTable* ft) {
    double operand = eval_expr(trn->expr, sym, ft);
    switch (trn->op) {
        case OP_ADD: return val + operand;
        case OP_SUB: return val - operand;
        case OP_MUL: return val * operand;
        case OP_DIV: return val / operand;
        case OP_POW: {
            double result = 1.0;
            int n = (int)operand;
            if (n >= 0) {
                for (int i = 0; i < n; i++) result *= val;
            } else {
                for (int i = 0; i < -n; i++) result /= val;
            }
            return result;
        }
        default: return val;
    }
}

static void exec_pipeline(PipelineNode* node, SymTable* sym, FuncTable* ft) {
    double acc = 0.0;
    int do_sum = node->has_sum;

    for (int i = 0; i < node->list_len; i++) {
        double val = node->list[i];

        if (node->has_filter) {
            double passes = apply_condition(val, node->filter);
            if (passes == 0.0) continue;
        }

        if (node->has_transform) {
            for (int t = 0; t < node->transform_count; t++) {
                val = apply_transform(val, node->transforms[t], sym, ft);
            }
        }

        if (do_sum) {
            acc += val;
        } else {
            emit_value(val);
        }
    }
    
    if (do_sum) {
        emit_value(acc);
    }
}

static void exec_assign(AssignNode* node, SymTable* sym) {
    double val;
    switch (node->rhs_type) {
        case ASSIGN_NUMBER:
            val = node->value;
            break;
        case ASSIGN_VARIABLE:
            if (!sym_exists(sym, node->rhs_name)) {
                error_at(node->line, "Undefined variable '%s'", node->rhs_name);
            }
            val = sym_get(sym, node->rhs_name);
            break;
        case ASSIGN_INPUT: {
            if (scanf("%lf", &val) != 1) {
                error_at(node->line, "Failed to read numeric input for '%s'", node->name);
            }
            break;
        }
        default:
            val = 0.0;
            break;
    }
    sym_set(sym, node->name, val);
}

static void exec_inpt(InptNode* node, SymTable* sym) {
    if (node->has_prompt) {
        printf("%s", node->prompt);
        fflush(stdout);
    }
    double val;
    if (scanf("%lf", &val) != 1) {
        error_at(node->line, "Failed to read numeric input for '%s'", node->var_name);
    }
    sym_set(sym, node->var_name, val);
}

static void exec_fn_def(FnDefNode* node, FuncTable* ft) {
    if (ft->count >= MAX_FUNCS) {
        error_at(node->line, "Function table full: cannot define '%s'", node->name);
    }
    FuncDef* fd = &ft->funcs[ft->count++];
    strncpy(fd->name, node->name, 63);
    fd->name[63] = '\0';
    fd->param_count = node->param_count;
    for (int i = 0; i < node->param_count; i++) {
        strncpy(fd->params[i], node->params[i], 63);
        fd->params[i][63] = '\0';
    }
    fd->body       = clone_expr(node->body);
    fd->line       = node->line;
    fd->is_builtin = 0;
    fd->builtin_fn = NULL;
}

static void exec_use_stmt(UseStmtNode* node, FuncTable* ft) {
    if (strcmp(node->module_name, "math") == 0) {
        register_math_module(ft);
    } else if (strcmp(node->module_name, "io") == 0) {
        register_io_module(ft);
    } else {
        error_at(node->line, "Unknown module '%s'", node->module_name);
    }
}

void execute(ASTNode* ast, SymTable* sym, FuncTable* ft) {
    /* sym is shared across all statements in the file so that variables
       assigned before or between pipelines remain visible in subsequent
       pipelines (symbol table is never reset between pipelines). */
    if (!ast) return;
    
    switch (ast->stmt_type) {
        case STMT_ASSIGN:
            exec_assign(ast->node.assign, sym);
            break;
            
        case STMT_ARITH: {
            double result = eval_expr((Expr*)ast->node.arith, sym, ft);
            emit_value(result);
            break;
        }
            
        case STMT_PIPELINE:
            exec_pipeline(ast->node.pipeline, sym, ft);
            break;

        case STMT_FN_DEF:
            exec_fn_def(ast->node.fn_def, ft);
            break;

        case STMT_USE:
            exec_use_stmt(ast->node.use_stmt, ft);
            break;

        case STMT_INPT:
            exec_inpt(ast->node.inpt, sym);
            break;
            
        case STMT_EMTPY:
            break;
    }
}