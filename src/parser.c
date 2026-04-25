#include "parser.h"
#include "error.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int current = 0;
static Token* tokens;
static int token_count;

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
    expr->col  = 0;
    return expr;
}

static Expr* parse_primary() {
    Expr* expr;

    if (match(TOK_NUMBER)) {
        expr = create_expr(EXPR_NUMBER);
        expr->num_val = atof(tokens[current - 1].lexeme);
        expr->line = tokens[current - 1].line;
        expr->col  = tokens[current - 1].col;
    } else if (peek().type == TOK_IDENT || peek().type == TOK_VAR_REF) {
        TokenType matched_type = peek().type;
        advance();
        int name_line = tokens[current - 1].line;
        int name_col  = tokens[current - 1].col;
        char name[64];
        strncpy(name, tokens[current - 1].lexeme, 63);
        name[63] = '\0';

        /* Function call: plain IDENT (not VAR_REF) followed by one or more
           NUMBER/IDENT arguments.  VAR_REF tokens are always variable refs
           because they follow an operator character without whitespace. */
        if (matched_type == TOK_IDENT &&
            current < token_count &&
            (peek().type == TOK_NUMBER || peek().type == TOK_IDENT)) {
            expr = create_expr(EXPR_CALL);
            strncpy(expr->var_name, name, 63);
            expr->var_name[63] = '\0';
            expr->arg_count = 0;
            expr->line = name_line;
            expr->col  = name_col;

            while (expr->arg_count < MAX_PARAMS &&
                   current < token_count &&
                   (peek().type == TOK_NUMBER || peek().type == TOK_IDENT)) {
                advance();
                Expr* arg;
                if (tokens[current - 1].type == TOK_NUMBER) {
                    arg = create_expr(EXPR_NUMBER);
                    arg->num_val = atof(tokens[current - 1].lexeme);
                } else {
                    arg = create_expr(EXPR_VARIABLE);
                    strncpy(arg->var_name, tokens[current - 1].lexeme, 63);
                    arg->var_name[63] = '\0';
                }
                arg->line = tokens[current - 1].line;
                arg->col  = tokens[current - 1].col;
                expr->args[expr->arg_count++] = arg;
            }
        } else {
            expr = create_expr(EXPR_VARIABLE);
            strncpy(expr->var_name, name, 63);
            expr->var_name[63] = '\0';
            expr->line = name_line;
            expr->col  = name_col;
        }
    } else {
        set_error_col(peek().col);
        error_at(peek().line, "Expected number or identifier");
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

/* Lowest-precedence expression: handles the ?? (coalesce) operator.
   left ?? right — evaluates to left if left is a defined variable,
   otherwise evaluates to right.  Right-associative. */
static Expr* parse_coalesce() {
    Expr* left = parse_expr();

    if (current < token_count && peek().type == TOK_COALESCE) {
        int coalesce_line = peek().line;
        advance(); /* consume ?? */
        Expr* right = parse_coalesce(); /* right-associative */
        Expr* expr = create_expr(EXPR_COALESCE);
        expr->left  = left;
        expr->right = right;
        expr->line  = coalesce_line;
        return expr;
    }

    return left;
}

void free_expr(Expr* expr) {
    if (!expr) return;
    if (expr->type == EXPR_BINARY) {
        free_expr(expr->left);
        free_expr(expr->right);
    } else if (expr->type == EXPR_CALL) {
        for (int i = 0; i < expr->arg_count; i++)
            free_expr(expr->args[i]);
    } else if (expr->type == EXPR_COALESCE) {
        free_expr(expr->left);
        free_expr(expr->right);
    }
    free(expr);
}

static AssignNode* parse_assign() {
    AssignNode* node = malloc(sizeof(AssignNode));
    node->type = NODE_ASSIGN;
    node->line = peek().line;

    if (!match(TOK_IDENT)) {
        error_at(peek().line, "Expected identifier in assignment");
        return node;
    }
    strncpy(node->name, tokens[current - 1].lexeme, 63);
    node->name[63] = '\0';
    node->line = tokens[current - 1].line;

    if (!match(TOK_ASSIGN)) {
        error_at(peek().line, "Expected '=' in assignment");
        return node;
    }

    if (match(TOK_NUMBER)) {
        node->rhs_type = ASSIGN_NUMBER;
        node->value = atof(tokens[current - 1].lexeme);
    } else if (match(TOK_IDENT)) {
        node->rhs_type = ASSIGN_VARIABLE;
        strncpy(node->rhs_name, tokens[current - 1].lexeme, 63);
        node->rhs_name[63] = '\0';
    } else if (match(TOK_INPT)) {
        node->rhs_type = ASSIGN_INPUT;
    } else {
        error_at(peek().line, "Expected number, identifier, or 'inpt' in assignment");
    }

    return node;
}

static PipelineNode* parse_pipeline(int lst_line) {
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
    node->line = lst_line;
    node->emt_label[0] = '\0';
    node->has_emt_label = 0;
    node->emt_sep[0] = '\0';
    node->has_emt_sep = 0;

    if (!match(TOK_LBRACK)) {
        error_at(peek().line, "Expected '[' after 'lst'");
        return node;
    }

    double* values = malloc(256 * sizeof(double));
    int val_count = 0;

    if (match(TOK_NUMBER)) {
        values[val_count++] = atof(tokens[current - 1].lexeme);

        while (match(TOK_OP)) {
            if (strcmp(tokens[current - 1].lexeme, ",") == 0) {
                if (!match(TOK_NUMBER)) {
                    error_at(peek().line, "Expected number after ','");
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
        error_at(peek().line, "Expected ']' after list");
        free(values);
        return node;
    }

    node->list = values;
    node->list_len = val_count;

    while (match(TOK_PIPE)) {
        if (match(TOK_WHN)) {
            int cond_line = tokens[current - 1].line;
            if (!match(TOK_OP)) {
                error_at(peek().line, "Expected operator after 'whn'");
                return node;
            }

            Condition* cond = malloc(sizeof(Condition));
            strncpy(cond->op_lexeme, tokens[current - 1].lexeme, 3);
            cond->op_lexeme[3] = '\0';
            cond->op = get_op_type(cond->op_lexeme);
            cond->line = cond_line;

            if (!match(TOK_NUMBER)) {
                error_at(peek().line, "Expected number after condition operator");
                free(cond);
                return node;
            }
            cond->value = atof(tokens[current - 1].lexeme);

            node->filter = cond;
            node->has_filter = 1;
        } else if (match(TOK_TRN)) {
            int trn_line = tokens[current - 1].line;
            if (!match(TOK_OP)) {
                error_at(peek().line, "Expected operator after 'trn'");
                return node;
            }

            Transform* trn = malloc(sizeof(Transform));
            strncpy(trn->op_lexeme, tokens[current - 1].lexeme, 3);
            trn->op_lexeme[3] = '\0';
            trn->op = get_op_type(trn->op_lexeme);
            trn->line = trn_line;
            trn->expr = parse_coalesce();

            if (!trn->expr) {
                error_at(peek().line, "Expected expression after transform operator");
                free(trn);
                return node;
            }

            Transform** tmp = realloc(node->transforms,
                                       (node->transform_count + 1) * sizeof(Transform*));
            if (!tmp) {
                int err_line = trn->line;
                free(trn);
                error_at(err_line, "Out of memory allocating transform");
                return node;
            }
            node->transforms = tmp;
            node->transforms[node->transform_count++] = trn;
            node->has_transform = 1;
        } else if (match(TOK_SUM)) {
            node->has_sum = 1;
        } else {
            error_at(peek().line, "Expected 'whn', 'trn', or 'sum' after '|'");
            return node;
        }
    }

    return node;
}

static FnDefNode* parse_fn_def() {
    FnDefNode* node = malloc(sizeof(FnDefNode));
    node->type = NODE_FN_DEF;
    node->param_count = 0;
    node->body = NULL;

    if (!match(TOK_FN)) {
        error_at(peek().line, "Expected 'fn'");
        return node;
    }
    node->line = tokens[current - 1].line;

    if (!match(TOK_IDENT)) {
        error_at(peek().line, "Expected function name after 'fn'");
        return node;
    }
    strncpy(node->name, tokens[current - 1].lexeme, 63);
    node->name[63] = '\0';

    /* Collect parameter names — all IDENT tokens on the same line as 'fn'. */
    while (peek().type == TOK_IDENT && node->param_count < MAX_PARAMS) {
        advance();
        strncpy(node->params[node->param_count], tokens[current - 1].lexeme, 63);
        node->params[node->param_count][63] = '\0';
        node->param_count++;
    }

    /* Expect the newline that separates parameters from the body. */
    if (!match(TOK_NEWLINE)) {
        error_at(peek().line, "Expected newline after parameter list in 'fn'");
        return node;
    }
    /* Skip any additional blank lines before the body. */
    while (match(TOK_NEWLINE)) {}

    /* Body is a single arithmetic expression. */
    node->body = parse_coalesce();

    /* Skip trailing newlines before 'end'. */
    while (match(TOK_NEWLINE)) {}

    if (!match(TOK_END)) {
        error_at(peek().line, "Expected 'end' after function body");
    }

    return node;
}

static UseStmtNode* parse_use_stmt() {
    UseStmtNode* node = malloc(sizeof(UseStmtNode));
    node->type = NODE_USE;
    node->line = peek().line;

    if (!match(TOK_USE)) {
        error_at(peek().line, "Expected 'use'");
        return node;
    }
    node->line = tokens[current - 1].line;

    if (!match(TOK_IDENT)) {
        error_at(peek().line, "Expected module name after 'use'");
        return node;
    }
    strncpy(node->module_name, tokens[current - 1].lexeme, 63);
    node->module_name[63] = '\0';

    return node;
}

static InptNode* parse_inpt_stmt() {
    InptNode* node = malloc(sizeof(InptNode));
    node->type = NODE_INPT;
    node->has_prompt = 0;
    node->prompt[0] = '\0';
    node->var_name[0] = '\0';

    if (!match(TOK_INPT)) {
        error_at(peek().line, "Expected 'inpt'");
        return node;
    }
    node->line = tokens[current - 1].line;

    if (!match(TOK_IDENT)) {
        error_at(peek().line, "Expected identifier after 'inpt'");
        return node;
    }
    strncpy(node->var_name, tokens[current - 1].lexeme, 63);
    node->var_name[63] = '\0';

    if (peek().type == TOK_STRING) {
        advance();
        strncpy(node->prompt, tokens[current - 1].lexeme, 63);
        node->prompt[63] = '\0';
        node->has_prompt = 1;
    }

    return node;
}

ASTNode* parse(Token* tok, int count) {
    /* Reset all parser state so that consecutive calls (one per pipeline or
       statement) do not inherit position or token-stream pointer from the
       previous invocation.  This is what allows a single source file to
       contain multiple lst…emt pipelines without interference. */
    tokens = tok;
    token_count = count;
    current = 0;

    ASTNode* ast = malloc(sizeof(ASTNode));
    ast->type = NODE_EMT;
    ast->stmt_type = STMT_EMTPY;
    ast->emt_label[0] = '\0';
    ast->has_emt_label = 0;

    if (token_count == 0 || tokens[0].type == TOK_EOF) {
        return ast;
    }

    if (tokens[0].type == TOK_IDENT && token_count > 1 && tokens[1].type == TOK_ASSIGN) {
        ast->stmt_type = STMT_ASSIGN;
        ast->node.assign = parse_assign();
    } else if (tokens[0].type == TOK_NUMBER || tokens[0].type == TOK_IDENT) {
        /* Check for keyword misspellings at statement position. */
        if (tokens[0].type == TOK_IDENT) {
            const char* suggestion = suggest_keyword(tokens[0].lexeme);
            if (suggestion) {
                char hint_msg[128];
                snprintf(hint_msg, sizeof(hint_msg), "Did you mean '%s'?", suggestion);
                set_error_hint(hint_msg);
                set_error_col(tokens[0].col);
                error_at(tokens[0].line, "Unknown keyword '%s'", tokens[0].lexeme);
            }
        }
        ast->stmt_type = STMT_ARITH;
        ast->node.arith = (ArithNode*)parse_coalesce();

        if (match(TOK_ARROW)) {
            if (!match(TOK_EMT)) {
                error_at(peek().line, "Expected 'emt' after '->'");
            }
        }
        /* Optional label after -> emt (U1) */
        if (peek().type == TOK_STRING) {
            strncpy(ast->emt_label, peek().lexeme, 63);
            ast->emt_label[63] = '\0';
            ast->has_emt_label = 1;
            advance();
        }
    } else if (tokens[0].type == TOK_LST) {
        int lst_line = tokens[current].line;
        advance();
        ast->stmt_type = STMT_PIPELINE;
        ast->node.pipeline = parse_pipeline(lst_line);

        if (!match(TOK_EMT)) {
            if (match(TOK_ARROW)) {
                if (!match(TOK_EMT)) {
                    error_at(peek().line, "Expected 'emt'");
                }
            }
        }
        /* Optional label after emt (U1) */
        if (peek().type == TOK_STRING) {
            strncpy(ast->node.pipeline->emt_label, peek().lexeme, 63);
            ast->node.pipeline->emt_label[63] = '\0';
            ast->node.pipeline->has_emt_label = 1;
            advance();
        }
        /* Optional sep "string" after emt [label] (U10) */
        if (peek().type == TOK_IDENT && strcmp(peek().lexeme, "sep") == 0) {
            advance(); /* consume "sep" */
            if (peek().type == TOK_STRING) {
                strncpy(ast->node.pipeline->emt_sep, peek().lexeme, 63);
                ast->node.pipeline->emt_sep[63] = '\0';
                ast->node.pipeline->has_emt_sep = 1;
                advance();
            } else {
                error_at(peek().line, "Expected string after 'sep'");
            }
        }
    } else if (tokens[0].type == TOK_EMT) {
        /* Standalone emt ["label"] expr  (U1) */
        advance(); /* consume emt */
        if (peek().type == TOK_STRING) {
            strncpy(ast->emt_label, peek().lexeme, 63);
            ast->emt_label[63] = '\0';
            ast->has_emt_label = 1;
            advance();
        }
        ast->stmt_type = STMT_ARITH;
        ast->node.arith = (ArithNode*)parse_coalesce();
    } else if (tokens[0].type == TOK_FN) {
        ast->stmt_type = STMT_FN_DEF;
        ast->node.fn_def = parse_fn_def();
    } else if (tokens[0].type == TOK_USE) {
        ast->stmt_type = STMT_USE;
        ast->node.use_stmt = parse_use_stmt();
    } else if (tokens[0].type == TOK_INPT) {
        ast->stmt_type = STMT_INPT;
        ast->node.inpt = parse_inpt_stmt();
    } else {
        error_at(tokens[0].line, "Expected lst, identifier, or number");
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
            for (int i = 0; i < ast->node.pipeline->transform_count; i++) {
                free_expr(ast->node.pipeline->transforms[i]->expr);
                free(ast->node.pipeline->transforms[i]);
            }
            free(ast->node.pipeline->transforms);
        }
        free(ast->node.pipeline);
    } else if (ast->stmt_type == STMT_FN_DEF) {
        /* Body ownership is transferred to FuncTable (cloned there);
           free the original body and the node itself. */
        free_expr(ast->node.fn_def->body);
        free(ast->node.fn_def);
    } else if (ast->stmt_type == STMT_USE) {
        free(ast->node.use_stmt);
    } else if (ast->stmt_type == STMT_INPT) {
        free(ast->node.inpt);
    }
    
    free(ast);
}
