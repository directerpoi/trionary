#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    EXPR_NUMBER,
    EXPR_VARIABLE,
    EXPR_BINARY
} ExprType;

typedef struct Expr {
    ExprType type;
    
    // For NUMBER
    double num_val;
    
    // For VARIABLE
    char var_name[64];
    
    // For BINARY
    OpType op;
    struct Expr* left;
    struct Expr* right;

    int line; /* source line where this expression token was seen */
} Expr;

static int current = 0;
static Token* tokens;
static int token_count;
static int error_occurred = 0;

static void error(const char* message, int line) {
    fprintf(stderr, "Error: %s at line %d\n", message, line);
    error_occurred = 1;
}

static Token peek() {
    if (current >= token_count) return tokens[token_count - 1];
    return tokens[current];
}

static Token advance() {
    if (current < token_count) current++;
    return tokens[current - 1];
}

static int match(TokenType type) {
    if (peek().type == type) {
        advance();
        return 1;
    }
    return 0;
}

static OpType get_op_type(const char* op) {
    if (strcmp(op, "+") == 0) return OP_ADD;
    if (strcmp(op, "-") == 0) return OP_SUB;
    if (strcmp(op, "*") == 0) return OP_MUL;
    if (strcmp(op, "/") == 0) return OP_DIV;
    if (strcmp(op, "^") == 0) return OP_POW;
    if (strcmp(op, ">") == 0) return OP_GT;
    if (strcmp(op, "<") == 0) return OP_LT;
    if (strcmp(op, ">=") == 0) return OP_GTE;
    if (strcmp(op, "<=") == 0) return OP_LTE;
    if (strcmp(op, "==") == 0) return OP_EQ;
    if (strcmp(op, "!=") == 0) return OP_NEQ;
    return OP_ADD;
}

static Expr* create_expr(ExprType type) {
    Expr* expr = malloc(sizeof(Expr));
    expr->type = type;
    expr->line = 0;
    return expr;
}

static Expr* parse_primary() {
    Expr* expr;
    
    if (match(TOK_NUMBER)) {
        expr = create_expr(EXPR_NUMBER);
        expr->num_val = atof(tokens[current - 1].lexeme);
        expr->line = tokens[current - 1].line;
    } else if (match(TOK_IDENT)) {
        expr = create_expr(EXPR_VARIABLE);
        strncpy(expr->var_name, tokens[current - 1].lexeme, 63);
        expr->var_name[63] = '\0';
        expr->line = tokens[current - 1].line;
    } else {
        error("Expected number or identifier", peek().line);
        expr = create_expr(EXPR_NUMBER);
        expr->num_val = 0;
        expr->line = peek().line;
    }
    
    return expr;
}

static Expr* parse_factor() {
    return parse_primary();
}

static Expr* parse_term() {
    Expr* left = parse_factor();
    
    while (match(TOK_OP)) {
        char op[4];
        strncpy(op, tokens[current - 1].lexeme, 3);
        op[3] = '\0';
        
        if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
            Expr* expr = create_expr(EXPR_BINARY);
            expr->op = get_op_type(op);
            expr->left = left;
            expr->right = parse_factor();
            left = expr;
        } else {
            current--;
            break;
        }
    }
    
    return left;
}

static Expr* parse_expr() {
    Expr* left = parse_term();
    
    while (match(TOK_OP)) {
        char op[4];
        strncpy(op, tokens[current - 1].lexeme, 3);
        op[3] = '\0';
        
        if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) {
            Expr* expr = create_expr(EXPR_BINARY);
            expr->op = get_op_type(op);
            expr->left = left;
            expr->right = parse_term();
            left = expr;
        } else {
            current--;
            break;
        }
    }
    
    return left;
}

static void free_expr(Expr* expr) {
    if (!expr) return;
    if (expr->type == EXPR_BINARY) {
        free_expr(expr->left);
        free_expr(expr->right);
    }
    free(expr);
}

static AssignNode* parse_assign() {
    AssignNode* node = malloc(sizeof(AssignNode));
    node->type = NODE_ASSIGN;
    
    if (!match(TOK_IDENT)) {
        error("Expected identifier in assignment", peek().line);
        return node;
    }
    strncpy(node->name, tokens[current - 1].lexeme, 63);
    node->name[63] = '\0';
    
    if (!match(TOK_ASSIGN)) {
        error("Expected '=' in assignment", peek().line);
        return node;
    }
    
    if (!match(TOK_NUMBER)) {
        error("Expected number in assignment", peek().line);
        return node;
    }
    node->value = atof(tokens[current - 1].lexeme);
    
    return node;
}

static PipelineNode* parse_pipeline() {
    PipelineNode* node = malloc(sizeof(PipelineNode));
    node->type = NODE_PIPELINE;
    node->list = NULL;
    node->list_len = 0;
    node->filter = NULL;
    node->transforms = NULL;
    node->transform_count = 0;
    node->has_filter = 0;
    node->has_transform = 0;
    node->has_sum = 0;
    
    if (!match(TOK_LBRACK)) {
        error("Expected '[' after 'lst'", peek().line);
        return node;
    }
    
    double* values = malloc(256 * sizeof(double));
    int val_count = 0;
    
    if (match(TOK_NUMBER)) {
        values[val_count++] = atof(tokens[current - 1].lexeme);
        
        while (match(TOK_OP)) {
            if (strcmp(tokens[current - 1].lexeme, ",") == 0) {
                if (!match(TOK_NUMBER)) {
                    error("Expected number after ','", peek().line);
                    free(values);
                    return node;
                }
                values[val_count++] = atof(tokens[current - 1].lexeme);
            } else {
                current--;
                break;
            }
        }
    }
    
    if (!match(TOK_RBRACK)) {
        error("Expected ']' after list", peek().line);
        free(values);
        return node;
    }
    
    node->list = values;
    node->list_len = val_count;
    
    while (match(TOK_PIPE)) {
        if (match(TOK_WHN)) {
            int cond_line = tokens[current - 1].line;
            if (!match(TOK_OP)) {
                error("Expected operator after 'whn'", peek().line);
                return node;
            }
            
            Condition* cond = malloc(sizeof(Condition));
            strncpy(cond->op_lexeme, tokens[current - 1].lexeme, 3);
            cond->op_lexeme[3] = '\0';
            cond->op = get_op_type(cond->op_lexeme);
            cond->line = cond_line;
            
            if (!match(TOK_NUMBER)) {
                error("Expected number after condition operator", peek().line);
                free(cond);
                return node;
            }
            cond->value = atof(tokens[current - 1].lexeme);
            
            node->filter = cond;
            node->has_filter = 1;
        } else if (match(TOK_TRN)) {
            int trn_line = tokens[current - 1].line;
            if (!match(TOK_OP)) {
                error("Expected operator after 'trn'", peek().line);
                return node;
            }
            
            Transform* trn = malloc(sizeof(Transform));
            strncpy(trn->op_lexeme, tokens[current - 1].lexeme, 3);
            trn->op_lexeme[3] = '\0';
            trn->op = get_op_type(trn->op_lexeme);
            trn->is_var_ref = 0;
            trn->var_name[0] = '\0';
            trn->value = 0.0;
            trn->line = trn_line;
            
            if (match(TOK_VAR_REF)) {
                trn->is_var_ref = 1;
                strncpy(trn->var_name, tokens[current - 1].lexeme, 63);
                trn->var_name[63] = '\0';
            } else if (match(TOK_NUMBER)) {
                trn->value = atof(tokens[current - 1].lexeme);
            } else {
                error("Expected number or variable after transform operator", peek().line);
                free(trn);
                return node;
            }
            
            node->transforms = realloc(node->transforms,
                                       (node->transform_count + 1) * sizeof(Transform*));
            node->transforms[node->transform_count++] = trn;
            node->has_transform = 1;
        } else if (match(TOK_SUM)) {
            node->has_sum = 1;
        } else {
            error("Expected 'whn', 'trn', or 'sum' after '|'", peek().line);
            return node;
        }
    }
    
    return node;
}

ASTNode* parse(Token* tok, int count) {
    /* Reset all parser state so that consecutive calls (one per pipeline or
       statement) do not inherit position, token-stream pointer, or error
       flag from the previous invocation.  This is what allows a single source
       file to contain multiple lst…emt pipelines without interference. */
    tokens = tok;
    token_count = count;
    current = 0;
    error_occurred = 0;
    
    ASTNode* ast = malloc(sizeof(ASTNode));
    ast->type = NODE_EMT;
    ast->stmt_type = STMT_EMTPY;
    
    if (token_count == 0 || tokens[0].type == TOK_EOF) {
        return ast;
    }
    
    if (tokens[0].type == TOK_IDENT && token_count > 1 && tokens[1].type == TOK_ASSIGN) {
        ast->stmt_type = STMT_ASSIGN;
        ast->node.assign = parse_assign();
    } else if (tokens[0].type == TOK_NUMBER || tokens[0].type == TOK_IDENT) {
        ast->stmt_type = STMT_ARITH;
        ast->node.arith = (ArithNode*)parse_expr();
        
        if (match(TOK_ARROW)) {
            if (!match(TOK_EMT)) {
                error("Expected 'emt' after '->'", peek().line);
            }
        }
    } else if (tokens[0].type == TOK_LST) {
        advance();
        ast->stmt_type = STMT_PIPELINE;
        ast->node.pipeline = parse_pipeline();
        
        if (!match(TOK_EMT)) {
            if (match(TOK_ARROW)) {
                if (!match(TOK_EMT)) {
                    error("Expected 'emt'", peek().line);
                }
            }
        }
    } else {
        error("Expected lst, identifier, or number", tokens[0].line);
    }
    
    if (error_occurred) {
        free_ast(ast);
        return NULL;
    }
    
    return ast;
}

void free_ast(ASTNode* ast) {
    if (!ast) return;
    
    if (ast->stmt_type == STMT_ARITH) {
        free_expr((Expr*)ast->node.arith);
    } else if (ast->stmt_type == STMT_ASSIGN) {
        free(ast->node.assign);
    } else if (ast->stmt_type == STMT_PIPELINE) {
        if (ast->node.pipeline->list) free(ast->node.pipeline->list);
        if (ast->node.pipeline->filter) free(ast->node.pipeline->filter);
        if (ast->node.pipeline->transforms) {
            for (int i = 0; i < ast->node.pipeline->transform_count; i++)
                free(ast->node.pipeline->transforms[i]);
            free(ast->node.pipeline->transforms);
        }
        free(ast->node.pipeline);
    }
    
    free(ast);
}
