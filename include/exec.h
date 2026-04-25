#ifndef EXEC_H
#define EXEC_H

#include "parser.h"
#include <stddef.h>

/* Number of hash slots per scope level — must be a power of 2. */
#define SCOPE_CAPACITY 128

typedef enum {
    VAL_NIL,
    VAL_BOOL,
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
    VAL_ARRAY,
    VAL_MAP,
    VAL_SET,
    VAL_TUPLE,
    VAL_PAIR
} ValueType;

typedef struct Value Value;

struct Value {
    ValueType type;
    union {
        int boolean;
        long long integer;
        double float_val;
        char* string;
        struct {
            Value* elements;
            int    length;
        } list;
        struct {
            Value* keys;
            Value* values;
            int    length;
        } map;
        struct {
            Value* key;
            Value* value;
        } pair;
    } as;
    int is_immutable;
};

typedef struct {
    char   name[64];
    Value  value;
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
void      sym_set(SymTable* t, const char* name, Value val);
Value     sym_get(SymTable* t, const char* name);
int       sym_exists(SymTable* t, const char* name);

/* Push a new isolated scope (used by function calls). */
void scope_push(SymTable* t);
/* Pop the current scope and restore the enclosing one. */
void scope_pop(SymTable* t);

/* Maximum number of user-defined functions. */
#define MAX_FUNCS 64

/* Signature for built-in (C-implemented) functions. */
typedef Value (*BuiltinFn)(Value *args, int arg_count);

/* Stored function definition (body is a cloned Expr* owned by FuncTable). */
typedef struct {
    char      name[64];
    char      params[MAX_PARAMS][64];
    int       param_count;
    Expr     *body;           /* NULL for builtins */
    int       line;
    int       is_builtin;     /* 1 = C function, 0 = user-defined */
    BuiltinFn builtin_fn;     /* non-NULL when is_builtin == 1   */
} FuncDef;

typedef struct {
    FuncDef funcs[MAX_FUNCS];
    int     count;
} FuncTable;

FuncTable* create_functable(void);
void       free_functable(FuncTable* ft);

/* Built-in module registration — called by exec.c when a 'use' statement runs. */
typedef enum {
    STATUS_NORMAL,
    STATUS_BREAK,
    STATUS_CONTINUE,
    STATUS_RETURN
} ControlStatus;

typedef struct {
    ControlStatus status;
    Value         value; /* return value if status is STATUS_RETURN */
} ExecResult;

void register_math_module(FuncTable *ft);
void register_io_module(FuncTable *ft);

ExecResult execute(ASTNode* ast, SymTable* sym, FuncTable* ft);

#endif