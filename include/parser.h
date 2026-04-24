#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum {
    NODE_ARITH,    /* arithmetic expression */
    NODE_ASSIGN,   /* variable assignment   */
    NODE_PIPELINE, /* lst | whn | trn | sum */
    NODE_EMT       /* emit / output         */
} NodeType;

typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_POW,
    OP_GT, OP_LT, OP_GTE, OP_LTE, OP_EQ, OP_NEQ
} OpType;

typedef struct ArithNode {
    NodeType type;
    OpType op;
    union {
        double num_val;
        char var_name[64];
    } left;
    union {
        double num_val;
        struct ArithNode* node;
    } right;
    int is_right_num;
} ArithNode;

typedef struct {
    NodeType type;
    char name[64];
    double value;
} AssignNode;

typedef struct {
    char op_lexeme[4];
    OpType op;
    double value;
    int line;            /* source line where this condition was parsed */
} Condition;

typedef struct {
    char op_lexeme[4];
    OpType op;
    double value;
    int is_var_ref;      /* 1 if operand is a variable reference */
    char var_name[64];   /* variable name when is_var_ref == 1   */
    int line;            /* source line where this transform was parsed */
} Transform;

typedef struct {
    NodeType type;
    double* list;
    int list_len;
    Condition* filter;
    Transform** transforms;   /* array of transform pointers */
    int transform_count;      /* number of transforms        */
    int has_filter;
    int has_transform;
    int has_sum;
} PipelineNode;

typedef struct {
    NodeType type;
    union {
        ArithNode* arith;
        AssignNode* assign;
        PipelineNode* pipeline;
    } node;
    enum { STMT_EMTPY, STMT_ARITH, STMT_ASSIGN, STMT_PIPELINE } stmt_type;
} ASTNode;

ASTNode* parse(Token* tokens, int token_count);
void free_ast(ASTNode* ast);

#endif