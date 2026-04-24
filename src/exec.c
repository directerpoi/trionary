#include "exec.h"
#include "error.h"
#include "output.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    EXPR_NUMBER,
    EXPR_VARIABLE,
    EXPR_BINARY
} ExprType;

typedef struct Expr {
    ExprType type;
    double num_val;
    char var_name[64];
    OpType op;
    struct Expr* left;
    struct Expr* right;
    int line; /* source line where this expression token was seen */
} Expr;

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

static double eval_expr(Expr* expr, SymTable* sym) {
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
            double left = eval_expr(expr->left, sym);
            double right = eval_expr(expr->right, sym);
            
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

static double apply_transform(double val, Transform* trn, SymTable* sym) {
    double operand;
    if (trn->is_var_ref) {
        if (!sym_exists(sym, trn->var_name)) {
            error_at(trn->line, "Undefined variable '%s'", trn->var_name);
        }
        operand = sym_get(sym, trn->var_name);
    } else {
        operand = trn->value;
    }
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

static void exec_pipeline(PipelineNode* node, SymTable* sym) {
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
                val = apply_transform(val, node->transforms[t], sym);
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
    sym_set(sym, node->name, node->value);
}

void execute(ASTNode* ast, SymTable* sym) {
    /* sym is shared across all statements in the file so that variables
       assigned before or between pipelines remain visible in subsequent
       pipelines (symbol table is never reset between pipelines). */
    if (!ast) return;
    
    switch (ast->stmt_type) {
        case STMT_ASSIGN:
            exec_assign(ast->node.assign, sym);
            break;
            
        case STMT_ARITH: {
            double result = eval_expr((Expr*)ast->node.arith, sym);
            emit_value(result);
            break;
        }
            
        case STMT_PIPELINE:
            exec_pipeline(ast->node.pipeline, sym);
            break;
            
        case STMT_EMTPY:
            break;
    }
}