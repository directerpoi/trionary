#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

#define MAX_PARAMS 8

typedef enum {
    NODE_ARITH,    /* arithmetic expression */
    NODE_ASSIGN,   /* variable assignment   */
    NODE_PIPELINE, /* lst | whn | trn | sum */
    NODE_EMT,      /* emit / output         */
    NODE_FN_DEF,   /* function definition   */
    NODE_USE,      /* use <module> directive */
    NODE_INPT      /* standalone inpt statement */
} NodeType;

typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_POW,
    OP_GT, OP_LT, OP_GTE, OP_LTE, OP_EQ, OP_NEQ
} OpType;

/* Expression node shared by arithmetic statements and trn expressions. */
typedef enum {
    EXPR_NUMBER,
    EXPR_VARIABLE,
    EXPR_BINARY,
    EXPR_CALL,       /* function call: fn_name(args...) */
    EXPR_COALESCE    /* left ?? right — if left is undefined, use right */
} ExprType;

typedef struct Expr {
    ExprType      type;
    double        num_val;
    char          var_name[64];  /* variable name or function name (EXPR_CALL) */
    OpType        op;
    struct Expr  *left;
    struct Expr  *right;
    struct Expr  *args[MAX_PARAMS]; /* EXPR_CALL arguments */
    int           arg_count;
    int           line; /* source line where this expression token was seen */
} Expr;

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
    int line; /* source line of the expression root */
} ArithNode;

typedef enum {
    ASSIGN_NUMBER,   /* a = 42       */
    ASSIGN_VARIABLE, /* a = arg0     */
    ASSIGN_INPUT     /* a = inpt     */
} AssignRHSType;

typedef struct {
    NodeType      type;
    char          name[64];
    double        value;         /* used when rhs_type == ASSIGN_NUMBER   */
    char          rhs_name[64]; /* used when rhs_type == ASSIGN_VARIABLE */
    AssignRHSType rhs_type;
    int           line;
} AssignNode;

typedef struct {
    char op_lexeme[4];
    OpType op;
    double value;
    int line;            /* source line where this condition was parsed */
} Condition;

typedef struct {
    char   op_lexeme[4];
    OpType op;
    Expr  *expr;         /* full right-hand arithmetic expression */
    int    line;         /* source line where this transform was parsed */
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
    int line; /* source line of the lst keyword */
} PipelineNode;

/* Function definition node: fn name p1 p2 ... \n expr \n end */
typedef struct {
    NodeType type;
    char     name[64];
    char     params[MAX_PARAMS][64];
    int      param_count;
    Expr    *body;           /* single-expression body */
    int      line;
} FnDefNode;

/* use <module> directive node */
typedef struct {
    NodeType type;
    char     module_name[64];
    int      line;
} UseStmtNode;

/* standalone inpt statement: inpt IDENT [STRING] */
typedef struct {
    NodeType type;
    char     var_name[64];  /* variable to store the input value */
    char     prompt[64];    /* optional prompt string */
    int      has_prompt;    /* 1 if a prompt string was provided */
    int      line;
} InptNode;

typedef struct {
    NodeType type;
    union {
        ArithNode* arith;
        AssignNode* assign;
        PipelineNode* pipeline;
        FnDefNode* fn_def;
        UseStmtNode* use_stmt;
        InptNode* inpt;
    } node;
    enum { STMT_EMTPY, STMT_ARITH, STMT_ASSIGN, STMT_PIPELINE, STMT_FN_DEF, STMT_USE, STMT_INPT } stmt_type;
} ASTNode;

ASTNode* parse(Token* tokens, int token_count);
void free_ast(ASTNode* ast);
void free_expr(Expr* expr);

#endif