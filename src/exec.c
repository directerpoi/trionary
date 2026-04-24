#include "exec.h"
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
} Expr;

SymTable* create_symtable() {
    SymTable* t = malloc(sizeof(SymTable));
    t->count = 0;
    return t;
}

void free_symtable(SymTable* t) {
    free(t);
}

void sym_set(SymTable* t, const char* name, double val) {
    for (int i = 0; i < t->count; i++) {
        if (strcmp(t->entries[i].name, name) == 0) {
            t->entries[i].value = val;
            return;
        }
    }
    
    if (t->count < 256) {
        strncpy(t->entries[t->count].name, name, 63);
        t->entries[t->count].name[63] = '\0';
        t->entries[t->count].value = val;
        t->count++;
    }
}

int sym_exists(SymTable* t, const char* name) {
    for (int i = 0; i < t->count; i++) {
        if (strcmp(t->entries[i].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

double sym_get(SymTable* t, const char* name) {
    for (int i = 0; i < t->count; i++) {
        if (strcmp(t->entries[i].name, name) == 0) {
            return t->entries[i].value;
        }
    }
    fprintf(stderr, "Error: Undefined variable '%s'\n", name);
    return 0.0;
}

static double eval_expr(Expr* expr, SymTable* sym) {
    if (!expr) return 0.0;
    
    switch (expr->type) {
        case EXPR_NUMBER:
            return expr->num_val;
            
        case EXPR_VARIABLE:
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

static double apply_transform(double val, Transform* trn) {
    switch (trn->op) {
        case OP_ADD: return val + trn->value;
        case OP_SUB: return val - trn->value;
        case OP_MUL: return val * trn->value;
        case OP_DIV: return val / trn->value;
        case OP_POW: {
            double result = 1.0;
            int n = (int)trn->value;
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

static void exec_pipeline(PipelineNode* node) {
    double acc = 0.0;
    int do_sum = node->has_sum;

    for (int i = 0; i < node->list_len; i++) {
        double val = node->list[i];

        if (node->has_filter) {
            double passes = apply_condition(val, node->filter);
            if (passes == 0.0) continue;
        }

        if (node->has_transform) {
            val = apply_transform(val, node->transform);
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
            exec_pipeline(ast->node.pipeline);
            break;
            
        case STMT_EMTPY:
            break;
    }
}