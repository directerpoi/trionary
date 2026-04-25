#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

#define MAX_PARAMS 128

typedef enum {
    NODE_BLOCK,
    NODE_ARITH,    /* arithmetic expression */
    NODE_ASSIGN,   /* variable assignment   */
    NODE_PIPELINE, /* lst | whn | trn | sum */
    NODE_EMT,      /* emit / output         */
    NODE_FN_DEF,   /* function definition   */
    NODE_USE,      /* use <module> directive */
    NODE_INPT,     /* standalone inpt statement */
    NODE_IF,
    NODE_FOR,
    NODE_WHL,
    NODE_EACH,
    NODE_RPT,
    NODE_BRK,
    NODE_NXT,
    NODE_RET,
    NODE_LET,
    NODE_DECL,
    NODE_IO,
    NODE_EXT,
    NODE_STP,
    NODE_IMP,
    NODE_EXP,
    NODE_PKG,
    NODE_TRY,
    NODE_THR,
    NODE_ASRT,
    NODE_DBG,
    NODE_LOG,
    NODE_TST,
    NODE_TRC,
    NODE_DOC,
    NODE_CHK
} NodeType;

typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_POW,
    OP_GT, OP_LT, OP_GTE, OP_LTE, OP_EQ, OP_NEQ,
    OP_AND, OP_OR, OP_NOT, OP_IN, OP_COLON
} OpType;

/* Expression node shared by arithmetic statements and trn expressions. */
typedef enum {
    EXPR_NUMBER,
    EXPR_VARIABLE,
    EXPR_BINARY,
    EXPR_CALL,       /* function call: fn_name(args...) */
    EXPR_COALESCE,   /* left ?? right — if left is undefined, use right */
    EXPR_LIST,       /* [e1, e2, ...] list literal */
    EXPR_BOOL,
    EXPR_STRING,
    EXPR_NIL,
    EXPR_MAP,
    EXPR_SET,
    EXPR_TUPLE,
    EXPR_PAIR,
    EXPR_IO,
    EXPR_LMB,
    EXPR_ERR,
    EXPR_TIM,
    EXPR_DFLT
} ExprType;

typedef struct Expr {
    ExprType      type;
    double        num_val;
    char          var_name[64];  /* variable name or function name (EXPR_CALL) */
    char          string_val[256];
    OpType        op;
    struct Expr  *left;
    struct Expr  *right;
    struct Expr  *args[MAX_PARAMS]; /* EXPR_CALL arguments, EXPR_LIST, etc. */
    int           arg_count;
    int           line; /* source line where this expression token was seen */
    int           col;  /* 1-based column of the primary token (0 if unknown) */
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
    Expr*         expr;
    AssignRHSType rhs_type;
    int           line;
} AssignNode;

typedef struct {
    char op_lexeme[4];
    OpType op;
    double value;
    char var_name[64];   /* used when is_variable == 1 */
    int  is_variable;    /* 0 = numeric literal, 1 = variable reference */
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
    Expr*  list_expr;
    Condition* filter;
    Transform** transforms;   /* array of transform pointers */
    int transform_count;      /* number of transforms        */
    int has_filter;
    int has_transform;
    int has_sum;
    int line; /* source line of the lst keyword */
    /* Optional emt label prefix (U1) */
    char emt_label[64];
    int  has_emt_label;
    /* Optional emt separator control (U10) */
    char emt_sep[64];
    int  has_emt_sep;
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
    char  type_name[64];
    char  var_name[64];
    Expr* value;
    int   line;
} DeclNode;

typedef struct {
    NodeType type;
    TokenType io_type;
    Expr*     args[MAX_PARAMS];
    int       arg_count;
    int       line;
} IONode;

typedef struct {
    NodeType type;
    Expr*    expr;
    int      line;
} GenericNode;

typedef struct {
    NodeType type;
    char     name[64];
    int      line;
} NameNode;

typedef struct ASTNode ASTNode;

typedef struct {
    ASTNode** nodes;
    int       count;
} BlockNode;

typedef struct {
    NodeType  type;
    BlockNode try_block;
    char      err_var[64];
    BlockNode catch_block;
    int       has_catch;
    int       line;
} TryNode;

typedef struct {
    NodeType type;
    char     module_name[64];
    char     alias[64];
    char     symbols[MAX_PARAMS][64];
    int      symbol_count;
    int      is_from;
    int      line;
} ImpNode;

typedef struct {
    NodeType type;
    char     label[256];
    Expr*    expr;
    int      line;
} TstNode;

typedef struct {
    NodeType type;
    Expr*    expr;
    char     type_name[64];
    int      line;
} ChkNode;

typedef struct {
    Expr*     condition;
    BlockNode body;
} ElifBranch;

typedef struct {
    Expr*       condition;
    BlockNode   then_block;
    ElifBranch* elif_branches;
    int         elif_count;
    BlockNode   else_block;
    int         has_else;
} IfNode;

typedef struct {
    char      var_name[64];
    Expr*     start;
    Expr*     end;
    BlockNode body;
} ForNode;

typedef struct {
    Expr*     condition;
    BlockNode body;
} WhlNode;

typedef struct {
    char      item_name[64];
    char      list_name[64];
    BlockNode body;
} EachNode;

typedef struct {
    Expr*     count;
    BlockNode body;
} RptNode;

typedef struct {
    Expr* value;
} RetNode;

typedef struct {
    char  name[64];
    Expr* value;
} LetNode;

typedef struct ASTNode {
    NodeType type;
    union {
        BlockNode*    block;
        ArithNode*    arith;
        AssignNode*   assign;
        PipelineNode* pipeline;
        FnDefNode*    fn_def;
        UseStmtNode*  use_stmt;
        InptNode*     inpt;
        IfNode*       if_node;
        ForNode*      for_node;
        WhlNode*      whl_node;
        EachNode*     each_node;
        RptNode*      rpt_node;
        RetNode*      ret_node;
        LetNode*      let_node;
        DeclNode*     decl_node;
        IONode*       io_node;
        GenericNode*  generic;
        NameNode*     name_node;
        TryNode*      try_node;
        ImpNode*      imp_node;
        TstNode*      tst_node;
        ChkNode*      chk_node;
    } node;
    enum { 
        STMT_EMPTY, STMT_ARITH, STMT_ASSIGN, STMT_PIPELINE, STMT_FN_DEF, 
        STMT_USE, STMT_INPT, STMT_IF, STMT_FOR, STMT_WHL, STMT_EACH, 
        STMT_RPT, STMT_BRK, STMT_NXT, STMT_RET, STMT_LET, STMT_BLOCK,
        STMT_DECL, STMT_IO, STMT_EXT, STMT_STP, STMT_IMP, STMT_EXP,
        STMT_PKG, STMT_TRY, STMT_THR, STMT_ASRT, STMT_DBG, STMT_LOG,
        STMT_TST, STMT_TRC, STMT_DOC, STMT_CHK
    } stmt_type;
    /* Optional emt label prefix — set for 'emt "label" expr' and 'expr -> emt "label"' (U1) */
    char emt_label[64];
    int  has_emt_label;
} ASTNode;

ASTNode* parse(Token* tokens, int token_count);
ASTNode* parse_statement(void);
void free_ast(ASTNode* ast);
void free_expr(Expr* expr);
Expr* clone_expr(Expr* expr);

#endif