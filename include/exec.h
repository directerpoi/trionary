#ifndef EXEC_H
#define EXEC_H

#include "parser.h"
#include <stddef.h>

/* Number of hash slots per scope level — must be a power of 2. */
#define SCOPE_CAPACITY 128

typedef struct {
    char   name[64];
    double value;
    int    occupied;
} ScopeEntry;

/* One level of variable scope (a flat open-addressing hash table). */
typedef struct Scope {
    ScopeEntry   slots[SCOPE_CAPACITY];
    struct Scope *parent; /* enclosing scope, NULL for the global scope */
} Scope;

typedef struct {
    Scope *current; /* innermost (active) scope */
} SymTable;

SymTable* create_symtable(void);
void      free_symtable(SymTable* t);
void      sym_set(SymTable* t, const char* name, double val);
double    sym_get(SymTable* t, const char* name);
int       sym_exists(SymTable* t, const char* name);

/* Push a new isolated scope (used by function calls). */
void scope_push(SymTable* t);
/* Pop the current scope and restore the enclosing one. */
void scope_pop(SymTable* t);

/* Maximum number of user-defined functions. */
#define MAX_FUNCS 64

/* Stored function definition (body is a cloned Expr* owned by FuncTable). */
typedef struct {
    char   name[64];
    char   params[MAX_PARAMS][64];
    int    param_count;
    Expr  *body;
    int    line;
} FuncDef;

typedef struct {
    FuncDef funcs[MAX_FUNCS];
    int     count;
} FuncTable;

FuncTable* create_functable(void);
void       free_functable(FuncTable* ft);

void execute(ASTNode* ast, SymTable* sym, FuncTable* ft);

#endif