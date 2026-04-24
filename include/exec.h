#ifndef EXEC_H
#define EXEC_H

#include "parser.h"
#include <stddef.h>

typedef struct {
    char name[64];
    double value;
} Symbol;

typedef struct {
    Symbol entries[256];
    int count;
} SymTable;

SymTable* create_symtable();
void free_symtable(SymTable* t);
void sym_set(SymTable* t, const char* name, double val);
double sym_get(SymTable* t, const char* name);
int sym_exists(SymTable* t, const char* name);

void execute(ASTNode* ast, SymTable* sym);

#endif