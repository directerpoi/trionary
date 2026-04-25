#include "parser.h"
#include "error.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int current_pos = 0;
Token* global_tokens;
int global_token_count;

static Token peek() {
    if (current_pos >= global_token_count) return global_tokens[global_token_count - 1];
    return global_tokens[current_pos];
}

static Token advance() {
    if (current_pos < global_token_count) current_pos++;
    return global_tokens[current_pos - 1];
}

static int match(TokenType type) {
    if (peek().type == type) {
        advance();
        return 1;
    }
    return 0;
}

static void skip_newlines() {
    while (peek().type == TOK_NEWLINE) advance();
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
    if (strcmp(op, "and") == 0) return OP_AND;
    if (strcmp(op, "or") == 0) return OP_OR;
    if (strcmp(op, "not") == 0) return OP_NOT;
    if (strcmp(op, "in") == 0) return OP_IN;
    if (strcmp(op, ":") == 0) return OP_COLON;
    return OP_ADD;
}

static Expr* create_expr(ExprType type) {
    Expr* expr = calloc(1, sizeof(Expr));
    expr->type = type;
    return expr;
}

static int can_start_expr(TokenType type) {
    return type == TOK_NUMBER || type == TOK_IDENT || type == TOK_VAR_REF ||
           type == TOK_LPAREN || type == TOK_LBRACK || type == TOK_LBRACE ||
           type == TOK_STRING || type == TOK_TRUE || type == TOK_FLS ||
           type == TOK_NIL || type == TOK_NOT ||
           type == TOK_ASK || type == TOK_FRD || type == TOK_CSV || type == TOK_JRD ||
           type == TOK_LMB || type == TOK_ERR || type == TOK_TIM;
}

static Expr* parse_coalesce();

static Expr* parse_primary() {
    Expr* expr;
    if (match(TOK_NUMBER)) {
        expr = create_expr(EXPR_NUMBER);
        expr->num_val = atof(global_tokens[current_pos - 1].lexeme);
        expr->line = global_tokens[current_pos - 1].line;
        expr->col  = global_tokens[current_pos - 1].col;
    } else if (match(TOK_TRUE)) {
        expr = create_expr(EXPR_BOOL);
        expr->num_val = 1.0;
        expr->line = global_tokens[current_pos - 1].line;
        expr->col  = global_tokens[current_pos - 1].col;
    } else if (match(TOK_FLS)) {
        expr = create_expr(EXPR_BOOL);
        expr->num_val = 0.0;
        expr->line = global_tokens[current_pos - 1].line;
        expr->col  = global_tokens[current_pos - 1].col;
    } else if (match(TOK_NIL)) {
        expr = create_expr(EXPR_NIL);
        expr->line = global_tokens[current_pos - 1].line;
        expr->col  = global_tokens[current_pos - 1].col;
    } else if (match(TOK_STRING)) {
        expr = create_expr(EXPR_STRING);
        strncpy(expr->string_val, global_tokens[current_pos - 1].lexeme, 255);
        expr->line = global_tokens[current_pos - 1].line;
        expr->col  = global_tokens[current_pos - 1].col;
    } else if (match(TOK_LMB)) {
        expr = create_expr(EXPR_LMB);
        expr->line = global_tokens[current_pos - 1].line;
        expr->col  = global_tokens[current_pos - 1].col;
        while (peek().type == TOK_IDENT && expr->arg_count < MAX_PARAMS) {
            Expr* p = create_expr(EXPR_VARIABLE);
            strncpy(p->var_name, advance().lexeme, 63);
            expr->args[expr->arg_count++] = p;
        }
        if (!match(TOK_ARROW)) error_at(expr->line, "Expected '->' after lambda parameters");
        expr->left = parse_coalesce();
    } else if (match(TOK_ERR)) {
        expr = create_expr(EXPR_ERR);
        expr->line = global_tokens[current_pos - 1].line;
        expr->col  = global_tokens[current_pos - 1].col;
        if (can_start_expr(peek().type)) expr->left = parse_coalesce();
    } else if (match(TOK_TIM)) {
        expr = create_expr(EXPR_TIM);
        expr->line = global_tokens[current_pos - 1].line;
        expr->col  = global_tokens[current_pos - 1].col;
        expr->left = parse_coalesce();
    } else if (match(TOK_LPAREN)) {
        int start_line = global_tokens[current_pos-1].line;
        int start_col = global_tokens[current_pos-1].col;
        Expr* first = parse_coalesce();
        if (peek().type == TOK_COMMA) {
            expr = create_expr(EXPR_TUPLE);
            expr->line = start_line; expr->col = start_col;
            expr->args[expr->arg_count++] = first;
            while (match(TOK_COMMA)) {
                if (peek().type == TOK_RPAREN) break;
                expr->args[expr->arg_count++] = parse_coalesce();
            }
            if (!match(TOK_RPAREN)) error_at(peek().line, "Expected ')' after tuple literal");
        } else {
            if (!match(TOK_RPAREN)) error_at(peek().line, "Expected ')' after expression");
            expr = first;
        }
    } else if (match(TOK_LBRACK)) {
        expr = create_expr(EXPR_LIST);
        expr->line = global_tokens[current_pos - 1].line;
        expr->col = global_tokens[current_pos - 1].col;
        if (peek().type != TOK_RBRACK) {
            do {
                if (expr->arg_count < MAX_PARAMS) {
                    expr->args[expr->arg_count++] = parse_coalesce();
                } else error_at(peek().line, "Too many elements in list literal");
            } while (match(TOK_COMMA));
        }
        if (!match(TOK_RBRACK)) error_at(peek().line, "Expected ']' after list literal");
    } else if (match(TOK_LBRACE)) {
        expr = create_expr(EXPR_MAP);
        expr->line = global_tokens[current_pos - 1].line;
        expr->col = global_tokens[current_pos - 1].col;
        if (peek().type != TOK_RBRACE) {
            Expr* first = parse_coalesce();
            if (peek().type == TOK_COLON) {
                advance();
                Expr* val = parse_coalesce();
                Expr* pair = create_expr(EXPR_PAIR);
                pair->left = first; pair->right = val;
                expr->args[expr->arg_count++] = pair;
                while (match(TOK_COMMA)) {
                    if (peek().type == TOK_RBRACE) break;
                    Expr* k = parse_coalesce();
                    if (!match(TOK_COLON)) error_at(peek().line, "Expected ':' in map literal");
                    Expr* v = parse_coalesce();
                    Expr* p = create_expr(EXPR_PAIR);
                    p->left = k; p->right = v;
                    expr->args[expr->arg_count++] = p;
                }
            } else {
                expr->type = EXPR_SET;
                expr->args[expr->arg_count++] = first;
                while (match(TOK_COMMA)) {
                    if (peek().type == TOK_RBRACE) break;
                    expr->args[expr->arg_count++] = parse_coalesce();
                }
            }
        }
        if (!match(TOK_RBRACE)) error_at(peek().line, "Expected '}' after literal");
    } else if (peek().type == TOK_ASK || peek().type == TOK_FRD || peek().type == TOK_CSV || peek().type == TOK_JRD) {
        Token t = advance();
        expr = create_expr(EXPR_IO);
        expr->op = (OpType)t.type;
        expr->line = t.line; expr->col = t.col;
        if (can_start_expr(peek().type)) {
            expr->args[expr->arg_count++] = parse_coalesce();
        }
    } else if (peek().type == TOK_IDENT || peek().type == TOK_VAR_REF) {
        Token t = advance();
        char name[128];
        strncpy(name, t.lexeme, 127);
        if (t.type == TOK_IDENT) {
            while (match(TOK_DOT)) {
                strncat(name, ".", 127);
                if (match(TOK_IDENT)) strncat(name, global_tokens[current_pos-1].lexeme, 127 - strlen(name));
            }
        }
        if (t.type == TOK_IDENT && (peek().type == TOK_NUMBER || peek().type == TOK_IDENT || 
            peek().type == TOK_LPAREN || peek().type == TOK_LBRACK || peek().type == TOK_LBRACE ||
            peek().type == TOK_STRING || peek().type == TOK_TRUE || peek().type == TOK_FLS || peek().type == TOK_NIL)) {
            expr = create_expr(EXPR_CALL);
            strncpy(expr->var_name, name, 63);
            expr->line = t.line; expr->col = t.col;
            while (expr->arg_count < MAX_PARAMS && (peek().type == TOK_NUMBER || peek().type == TOK_IDENT || 
                   peek().type == TOK_LPAREN || peek().type == TOK_LBRACK || peek().type == TOK_LBRACE ||
                   peek().type == TOK_STRING || peek().type == TOK_TRUE || peek().type == TOK_FLS || peek().type == TOK_NIL)) {
                if (peek().type == TOK_LPAREN || peek().type == TOK_LBRACK || peek().type == TOK_LBRACE) {
                    expr->args[expr->arg_count++] = parse_primary();
                } else {
                    Token arg_t = advance();
                    Expr* arg = NULL;
                    if (arg_t.type == TOK_NUMBER) {
                        arg = create_expr(EXPR_NUMBER);
                        arg->num_val = atof(arg_t.lexeme);
                    } else if (arg_t.type == TOK_STRING) {
                        arg = create_expr(EXPR_STRING);
                        strncpy(arg->string_val, arg_t.lexeme, 255);
                    } else if (arg_t.type == TOK_TRUE) {
                        arg = create_expr(EXPR_BOOL); arg->num_val = 1.0;
                    } else if (arg_t.type == TOK_FLS) {
                        arg = create_expr(EXPR_BOOL); arg->num_val = 0.0;
                    } else if (arg_t.type == TOK_NIL) {
                        arg = create_expr(EXPR_NIL);
                    } else {
                        arg = create_expr(EXPR_VARIABLE);
                        strncpy(arg->var_name, arg_t.lexeme, 63);
                    }
                    arg->line = arg_t.line; arg->col = arg_t.col;
                    expr->args[expr->arg_count++] = arg;
                }
            }
        } else {
            expr = create_expr(EXPR_VARIABLE);
            strncpy(expr->var_name, name, 63);
            expr->line = t.line; expr->col = t.col;
        }
    } else {
        set_error_col(peek().col);
        error_at(peek().line, "Expected number or identifier");
        expr = create_expr(EXPR_NUMBER);
    }
    return expr;
}

static Expr* parse_unary() {
    if (match(TOK_NOT)) {
        Expr* expr = create_expr(EXPR_BINARY);
        expr->op = OP_NOT;
        expr->line = global_tokens[current_pos - 1].line;
        expr->left = parse_unary();
        return expr;
    }
    return parse_primary();
}

static Expr* parse_pow() {
    Expr* left = parse_unary();
    while (peek().type == TOK_OP && strcmp(peek().lexeme, "^") == 0) {
        advance();
        Expr* expr = create_expr(EXPR_BINARY);
        expr->op = OP_POW;
        expr->left = left;
        expr->right = parse_unary();
        left = expr;
    }
    return left;
}

static Expr* parse_multiplicative() {
    Expr* left = parse_pow();
    while (peek().type == TOK_OP && (strcmp(peek().lexeme, "*") == 0 || strcmp(peek().lexeme, "/") == 0)) {
        Expr* expr = create_expr(EXPR_BINARY);
        expr->op = get_op_type(advance().lexeme);
        expr->left = left;
        expr->right = parse_pow();
        left = expr;
    }
    return left;
}

static Expr* parse_additive() {
    Expr* left = parse_multiplicative();
    while (peek().type == TOK_OP && (strcmp(peek().lexeme, "+") == 0 || strcmp(peek().lexeme, "-") == 0)) {
        Expr* expr = create_expr(EXPR_BINARY);
        expr->op = get_op_type(advance().lexeme);
        expr->left = left;
        expr->right = parse_multiplicative();
        left = expr;
    }
    return left;
}

static Expr* parse_comparison() {
    Expr* left = parse_additive();
    while (1) {
        if (peek().type == TOK_OP && (strcmp(peek().lexeme, ">") == 0 || strcmp(peek().lexeme, "<") == 0 ||
            strcmp(peek().lexeme, ">=") == 0 || strcmp(peek().lexeme, "<=") == 0 ||
            strcmp(peek().lexeme, "==") == 0 || strcmp(peek().lexeme, "!=") == 0)) {
            Expr* expr = create_expr(EXPR_BINARY);
            expr->op = get_op_type(advance().lexeme);
            expr->left = left;
            expr->right = parse_additive();
            left = expr;
        } else if (match(TOK_IN)) {
            Expr* expr = create_expr(EXPR_BINARY);
            expr->op = OP_IN;
            expr->left = left;
            expr->right = parse_additive();
            left = expr;
        } else break;
    }
    return left;
}

static Expr* parse_and() {
    Expr* left = parse_comparison();
    while (match(TOK_AND)) {
        Expr* expr = create_expr(EXPR_BINARY);
        expr->op = OP_AND;
        expr->left = left;
        expr->right = parse_comparison();
        left = expr;
    }
    return left;
}

static Expr* parse_or() {
    Expr* left = parse_and();
    while (match(TOK_OR)) {
        Expr* expr = create_expr(EXPR_BINARY);
        expr->op = OP_OR;
        expr->left = left;
        expr->right = parse_and();
        left = expr;
    }
    return left;
}

static Expr* parse_pair() {
    Expr* left = parse_or();
    if (peek().type == TOK_COLON) {
        advance();
        Expr* expr = create_expr(EXPR_PAIR);
        expr->left = left;
        expr->right = parse_pair();
        expr->line = left->line;
        expr->col = left->col;
        return expr;
    }
    return left;
}

static Expr* parse_coalesce() {
    Expr* left = parse_pair();
    if (match(TOK_COALESCE)) {
        Expr* expr = create_expr(EXPR_COALESCE);
        expr->left = left;
        expr->right = parse_coalesce();
        return expr;
    }
    return left;
}

ASTNode* parse_statement(void);

static BlockNode parse_block() {
    BlockNode block = {NULL, 0};
    while (peek().type != TOK_EOF && peek().type != TOK_END && peek().type != TOK_ELS && peek().type != TOK_ELIF) {
        if (match(TOK_NEWLINE)) continue;
        ASTNode* stmt = parse_statement();
        if (stmt) {
            block.nodes = realloc(block.nodes, (block.count + 1) * sizeof(ASTNode*));
            block.nodes[block.count++] = stmt;
        }
    }
    return block;
}

ASTNode* parse_statement() {
    ASTNode* node = calloc(1, sizeof(ASTNode));
    if (peek().type == TOK_IDENT && current_pos + 1 < global_token_count && global_tokens[current_pos+1].type == TOK_ASSIGN) {
        if (current_pos + 2 >= global_token_count || global_tokens[current_pos+2].type == TOK_NEWLINE || global_tokens[current_pos+2].type == TOK_EOF) {
            return NULL;
        }
        node->stmt_type = STMT_ASSIGN;
        AssignNode* assign = calloc(1, sizeof(AssignNode));
        assign->line = peek().line;
        strncpy(assign->name, advance().lexeme, 63); advance(); // =
        if (peek().type == TOK_INPT) { advance(); assign->rhs_type = ASSIGN_INPUT; }
        else assign->expr = parse_coalesce();
        node->node.assign = assign;
    } else if (match(TOK_LET)) {
        node->stmt_type = STMT_LET;
        LetNode* let = calloc(1, sizeof(LetNode));
        if (match(TOK_IDENT)) strncpy(let->name, global_tokens[current_pos-1].lexeme, 63);
        if (match(TOK_ASSIGN)) let->value = parse_coalesce();
        node->node.let_node = let;
    } else if (match(TOK_EXT)) {
        node->stmt_type = STMT_EXT; node->node.generic = calloc(1, sizeof(GenericNode));
        node->node.generic->line = global_tokens[current_pos-1].line;
        if (can_start_expr(peek().type)) node->node.generic->expr = parse_coalesce();
    } else if (match(TOK_STP)) {
        node->stmt_type = STMT_STP; node->node.generic = calloc(1, sizeof(GenericNode));
        node->node.generic->line = global_tokens[current_pos-1].line;
    } else if (match(TOK_THR)) {
        node->stmt_type = STMT_THR; node->node.generic = calloc(1, sizeof(GenericNode));
        node->node.generic->line = global_tokens[current_pos-1].line;
        node->node.generic->expr = parse_coalesce();
    } else if (match(TOK_ASRT)) {
        node->stmt_type = STMT_ASRT; node->node.generic = calloc(1, sizeof(GenericNode));
        node->node.generic->line = global_tokens[current_pos-1].line;
        node->node.generic->expr = parse_coalesce();
    } else if (match(TOK_DBG)) {
        node->stmt_type = STMT_DBG; node->node.generic = calloc(1, sizeof(GenericNode));
        node->node.generic->line = global_tokens[current_pos-1].line;
        node->node.generic->expr = parse_coalesce();
    } else if (match(TOK_LOG)) {
        node->stmt_type = STMT_LOG; node->node.generic = calloc(1, sizeof(GenericNode));
        node->node.generic->line = global_tokens[current_pos-1].line;
        node->node.generic->expr = parse_coalesce();
    } else if (match(TOK_TRC)) {
        node->stmt_type = STMT_TRC; node->node.generic = calloc(1, sizeof(GenericNode));
        node->node.generic->line = global_tokens[current_pos-1].line;
        node->node.generic->expr = parse_coalesce();
    } else if (match(TOK_DOC)) {
        node->stmt_type = STMT_DOC; node->node.generic = calloc(1, sizeof(GenericNode));
        node->node.generic->line = global_tokens[current_pos-1].line;
        node->node.generic->expr = parse_coalesce();
    } else if (match(TOK_PKG)) {
        node->stmt_type = STMT_PKG; node->node.name_node = calloc(1, sizeof(NameNode));
        node->node.name_node->line = global_tokens[current_pos-1].line;
        if (match(TOK_IDENT)) strncpy(node->node.name_node->name, global_tokens[current_pos-1].lexeme, 63);
    } else if (match(TOK_EXP)) {
        node->stmt_type = STMT_EXP; node->node.name_node = calloc(1, sizeof(NameNode));
        node->node.name_node->line = global_tokens[current_pos-1].line;
        if (match(TOK_IDENT)) strncpy(node->node.name_node->name, global_tokens[current_pos-1].lexeme, 63);
    } else if (match(TOK_TRY)) {
        node->stmt_type = STMT_TRY; TryNode* t = node->node.try_node = calloc(1, sizeof(TryNode));
        t->line = global_tokens[current_pos-1].line;
        skip_newlines(); t->try_block = parse_block();
        if (match(TOK_CTCH)) {
            t->has_catch = 1;
            if (match(TOK_IDENT)) strncpy(t->err_var, global_tokens[current_pos-1].lexeme, 63);
            skip_newlines(); t->catch_block = parse_block();
        }
        if (!match(TOK_END)) error_at(peek().line, "Expected 'end' after try");
    } else if (peek().type == TOK_IMP || peek().type == TOK_FRM) {
        node->stmt_type = STMT_IMP; ImpNode* i = node->node.imp_node = calloc(1, sizeof(ImpNode));
        i->line = peek().line;
        if (match(TOK_FRM)) {
            i->is_from = 1;
            if (match(TOK_IDENT)) strncpy(i->module_name, global_tokens[current_pos-1].lexeme, 63);
            if (!match(TOK_IMP)) error_at(peek().line, "Expected 'imp' after 'frm module'");
        } else {
            advance(); // consume imp
            if (match(TOK_IDENT)) strncpy(i->module_name, global_tokens[current_pos-1].lexeme, 63);
        }
        if (i->is_from) {
            while (peek().type == TOK_IDENT) {
                strncpy(i->symbols[i->symbol_count++], advance().lexeme, 63);
                if (!match(TOK_COMMA)) break;
            }
        }
        if (match(TOK_AS)) {
            if (match(TOK_IDENT)) strncpy(i->alias, global_tokens[current_pos-1].lexeme, 63);
        }
    } else if (match(TOK_TST)) {
        node->stmt_type = STMT_TST; TstNode* t = node->node.tst_node = calloc(1, sizeof(TstNode));
        t->line = global_tokens[current_pos-1].line;
        if (peek().type == TOK_STRING) strncpy(t->label, advance().lexeme, 255);
        t->expr = parse_coalesce();
    } else if (match(TOK_CHK)) {
        node->stmt_type = STMT_CHK; ChkNode* c = node->node.chk_node = calloc(1, sizeof(ChkNode));
        c->line = global_tokens[current_pos-1].line;
        c->expr = parse_coalesce();
        if (match(TOK_IDENT)) strncpy(c->type_name, global_tokens[current_pos-1].lexeme, 63);
    } else if (peek().type == TOK_STR || peek().type == TOK_ARR || peek().type == TOK_BOOL ||
               peek().type == TOK_MAP || peek().type == TOK_INT || peek().type == TOK_FLT ||
               peek().type == TOK_PAIR || peek().type == TOK_TPL || peek().type == TOK_SET) {
        node->stmt_type = STMT_DECL;
        DeclNode* decl = node->node.decl_node = calloc(1, sizeof(DeclNode));
        decl->line = peek().line;
        strncpy(decl->type_name, advance().lexeme, 63);
        if (match(TOK_IDENT)) strncpy(decl->var_name, global_tokens[current_pos-1].lexeme, 63);
        if (match(TOK_ASSIGN)) decl->value = parse_coalesce();
    } else if (match(TOK_IF)) {
        node->stmt_type = STMT_IF;
        node->node.if_node = calloc(1, sizeof(IfNode));
        node->node.if_node->condition = parse_coalesce();
        skip_newlines(); node->node.if_node->then_block = parse_block();
        while (match(TOK_ELIF)) {
            int i = node->node.if_node->elif_count++;
            node->node.if_node->elif_branches = realloc(node->node.if_node->elif_branches, node->node.if_node->elif_count * sizeof(ElifBranch));
            node->node.if_node->elif_branches[i].condition = parse_coalesce();
            skip_newlines(); node->node.if_node->elif_branches[i].body = parse_block();
        }
        if (match(TOK_ELS)) { skip_newlines(); node->node.if_node->else_block = parse_block(); node->node.if_node->has_else = 1; }
        if (!match(TOK_END)) error_at(peek().line, "Expected 'end' after if");
    } else if (match(TOK_FOR)) {
        node->stmt_type = STMT_FOR; ForNode* f = node->node.for_node = calloc(1, sizeof(ForNode));
        if (match(TOK_IDENT)) strncpy(f->var_name, global_tokens[current_pos-1].lexeme, 63);
        f->start = parse_coalesce(); f->end = parse_coalesce();
        skip_newlines(); f->body = parse_block();
        if (!match(TOK_END)) error_at(peek().line, "Expected 'end' after for");
    } else if (match(TOK_WHL)) {
        node->stmt_type = STMT_WHL; WhlNode* w = node->node.whl_node = calloc(1, sizeof(WhlNode));
        w->condition = parse_coalesce(); skip_newlines(); w->body = parse_block();
        if (!match(TOK_END)) error_at(peek().line, "Expected 'end' after whl");
    } else if (match(TOK_EACH)) {
        node->stmt_type = STMT_EACH; EachNode* e = node->node.each_node = calloc(1, sizeof(EachNode));
        if (match(TOK_IDENT)) strncpy(e->item_name, global_tokens[current_pos-1].lexeme, 63);
        if (match(TOK_IDENT)) strncpy(e->list_name, global_tokens[current_pos-1].lexeme, 63);
        skip_newlines(); e->body = parse_block();
        if (!match(TOK_END)) error_at(peek().line, "Expected 'end' after each");
    } else if (match(TOK_RPT)) {
        node->stmt_type = STMT_RPT; RptNode* r = node->node.rpt_node = calloc(1, sizeof(RptNode));
        r->count = parse_coalesce(); skip_newlines(); r->body = parse_block();
        if (!match(TOK_END)) error_at(peek().line, "Expected 'end' after rpt");
    } else if (match(TOK_BRK)) node->stmt_type = STMT_BRK;
    else if (match(TOK_NXT)) node->stmt_type = STMT_NXT;
    else if (match(TOK_RET)) {
        node->stmt_type = STMT_RET; node->node.ret_node = calloc(1, sizeof(RetNode));
        if (peek().type != TOK_NEWLINE && peek().type != TOK_EOF && peek().type != TOK_END) node->node.ret_node->value = parse_coalesce();
    } else if (match(TOK_FN)) {
        node->stmt_type = STMT_FN_DEF; FnDefNode* f = node->node.fn_def = calloc(1, sizeof(FnDefNode));
        if (match(TOK_IDENT)) strncpy(f->name, global_tokens[current_pos-1].lexeme, 63);
        while (peek().type == TOK_IDENT && f->param_count < MAX_PARAMS) strncpy(f->params[f->param_count++], advance().lexeme, 63);
        skip_newlines(); f->body = parse_coalesce(); skip_newlines();
        if (!match(TOK_END)) error_at(peek().line, "Expected 'end' after fn");
    } else if (match(TOK_USE)) {
        node->stmt_type = STMT_USE; node->node.use_stmt = calloc(1, sizeof(UseStmtNode));
        if (match(TOK_IDENT)) strncpy(node->node.use_stmt->module_name, global_tokens[current_pos-1].lexeme, 63);
    } else if (peek().type == TOK_SAY || peek().type == TOK_PRT || peek().type == TOK_FWR || peek().type == TOK_FAP) {
        node->stmt_type = STMT_IO;
        IONode* io = node->node.io_node = calloc(1, sizeof(IONode));
        io->io_type = advance().type;
        io->line = global_tokens[current_pos-1].line;
        while (can_start_expr(peek().type) && io->arg_count < MAX_PARAMS) {
            io->args[io->arg_count++] = parse_coalesce();
        }
    } else if (match(TOK_INPT)) {
        node->stmt_type = STMT_INPT; InptNode* in = node->node.inpt = calloc(1, sizeof(InptNode));
        in->line = global_tokens[current_pos-1].line;
        if (match(TOK_IDENT)) strncpy(in->var_name, global_tokens[current_pos-1].lexeme, 63);
        if (peek().type == TOK_STRING) { strncpy(in->prompt, advance().lexeme, 63); in->has_prompt = 1; }
    } else if (match(TOK_LST)) {
        int line = global_tokens[current_pos-1].line;
        node->stmt_type = STMT_PIPELINE; PipelineNode* p = node->node.pipeline = calloc(1, sizeof(PipelineNode));
        p->line = line;
        p->list_expr = parse_primary(); // Expects TOK_LBRACK and parses EXPR_LIST
        skip_newlines();
        while (match(TOK_PIPE) || peek().type == TOK_WHN || peek().type == TOK_TRN || peek().type == TOK_SUM) {
            skip_newlines();
            if (match(TOK_WHN)) {
                Condition* c = p->filter = calloc(1, sizeof(Condition)); p->has_filter = 1;
                if (match(TOK_OP)) { strncpy(c->op_lexeme, global_tokens[current_pos-1].lexeme, 3); c->op = get_op_type(c->op_lexeme); }
                if (match(TOK_NUMBER)) c->value = atof(global_tokens[current_pos-1].lexeme);
                else if (peek().type == TOK_IDENT || peek().type == TOK_VAR_REF) { c->is_variable = 1; strncpy(c->var_name, advance().lexeme, 63); }
            } else if (match(TOK_TRN)) {
                Transform* t = calloc(1, sizeof(Transform)); p->has_transform = 1;
                if (match(TOK_OP)) { strncpy(t->op_lexeme, global_tokens[current_pos-1].lexeme, 3); t->op = get_op_type(t->op_lexeme); }
                t->expr = parse_coalesce();
                p->transforms = realloc(p->transforms, (p->transform_count + 1) * sizeof(Transform*));
                p->transforms[p->transform_count++] = t;
            } else if (match(TOK_SUM)) p->has_sum = 1;
            skip_newlines();
        }
        if (match(TOK_EMT) || (match(TOK_ARROW) && match(TOK_EMT))) {
            if (peek().type == TOK_STRING) {
                strncpy(p->emt_label, advance().lexeme, 63); p->has_emt_label = 1;
            }
            if (peek().type == TOK_IDENT && strcmp(peek().lexeme, "sep") == 0) {
                advance(); if (peek().type == TOK_STRING) { strncpy(p->emt_sep, advance().lexeme, 63); p->has_emt_sep = 1; }
            }
        }
    } else if (match(TOK_EMT)) {
        if (peek().type == TOK_STRING && can_start_expr(global_tokens[current_pos+1].type)) {
            strncpy(node->emt_label, advance().lexeme, 63);
            node->has_emt_label = 1;
        }
        node->stmt_type = STMT_ARITH;
        node->node.arith = (ArithNode*)parse_coalesce();
    } else if (peek().type == TOK_NUMBER || peek().type == TOK_IDENT || peek().type == TOK_VAR_REF || 
               peek().type == TOK_LBRACK || peek().type == TOK_NOT || peek().type == TOK_STRING ||
               peek().type == TOK_TRUE || peek().type == TOK_FLS || peek().type == TOK_NIL ||
               peek().type == TOK_LBRACE || peek().type == TOK_LPAREN ||
               peek().type == TOK_LMB || peek().type == TOK_ERR || peek().type == TOK_TIM) {
        if (peek().type == TOK_IDENT) {
            const char* sug = suggest_keyword(peek().lexeme);
            if (sug) {
                char hint[128]; snprintf(hint, 128, "Did you mean '%s'?", sug);
                set_error_hint(hint); set_error_col(peek().col);
                error_at(peek().line, "Unknown keyword '%s'", peek().lexeme);
            }
        }
        node->stmt_type = STMT_ARITH; node->node.arith = (ArithNode*)parse_coalesce();
        if (match(TOK_ARROW) && match(TOK_EMT)) {
            if (peek().type == TOK_STRING) {
                strncpy(node->emt_label, advance().lexeme, 63); node->has_emt_label = 1;
            }
        }
    } else {
        return NULL;
    }
    return node;
}

Expr* clone_expr(Expr* expr) {
    if (!expr) return NULL;
    Expr* copy = malloc(sizeof(Expr));
    memcpy(copy, expr, sizeof(Expr));
    copy->left = clone_expr(expr->left);
    copy->right = clone_expr(expr->right);
    for (int i = 0; i < expr->arg_count; i++) {
        copy->args[i] = clone_expr(expr->args[i]);
    }
    return copy;
}

ASTNode* parse(Token* tok, int count) {
    global_tokens = tok; global_token_count = count; current_pos = 0;
    ASTNode* root = calloc(1, sizeof(ASTNode)); root->stmt_type = STMT_BLOCK;
    root->node.block = malloc(sizeof(BlockNode)); *root->node.block = parse_block();
    return root;
}

void free_expr(Expr* expr) {
    if (!expr) return;
    if (expr->type == EXPR_BINARY || expr->type == EXPR_COALESCE || expr->type == EXPR_PAIR || expr->type == EXPR_DFLT) { 
        free_expr(expr->left); free_expr(expr->right); 
    }
    else if (expr->type == EXPR_CALL || expr->type == EXPR_LIST || expr->type == EXPR_MAP || expr->type == EXPR_SET || expr->type == EXPR_TUPLE || expr->type == EXPR_IO || expr->type == EXPR_LMB) { 
        for (int i = 0; i < expr->arg_count; i++) free_expr(expr->args[i]); 
        if (expr->type == EXPR_LMB) free_expr(expr->left);
    }
    else if (expr->type == EXPR_ERR || expr->type == EXPR_TIM) {
        free_expr(expr->left);
    }
    free(expr);
}

void free_ast(ASTNode* ast) {
    if (!ast) return;
    switch (ast->stmt_type) {
        case STMT_BLOCK:
            for (int i = 0; i < ast->node.block->count; i++) free_ast(ast->node.block->nodes[i]);
            free(ast->node.block->nodes); free(ast->node.block); break;
        case STMT_ARITH: free_expr((Expr*)ast->node.arith); break;
        case STMT_ASSIGN: if (ast->node.assign->expr) free_expr(ast->node.assign->expr); free(ast->node.assign); break;
        case STMT_DECL: if (ast->node.decl_node->value) free_expr(ast->node.decl_node->value); free(ast->node.decl_node); break;
        case STMT_PIPELINE:
            if (ast->node.pipeline->list_expr) free_expr(ast->node.pipeline->list_expr);
            if (ast->node.pipeline->filter) free(ast->node.pipeline->filter);
            for (int i = 0; i < ast->node.pipeline->transform_count; i++) {
                free_expr(ast->node.pipeline->transforms[i]->expr); free(ast->node.pipeline->transforms[i]);
            }
            free(ast->node.pipeline->transforms); free(ast->node.pipeline); break;
        case STMT_FN_DEF: free_expr(ast->node.fn_def->body); free(ast->node.fn_def); break;
        case STMT_USE: free(ast->node.use_stmt); break;
        case STMT_INPT: free(ast->node.inpt); break;
        case STMT_IF:
            free_expr(ast->node.if_node->condition);
            for (int i = 0; i < ast->node.if_node->then_block.count; i++) free_ast(ast->node.if_node->then_block.nodes[i]);
            free(ast->node.if_node->then_block.nodes);
            for (int i = 0; i < ast->node.if_node->elif_count; i++) {
                free_expr(ast->node.if_node->elif_branches[i].condition);
                for (int j = 0; j < ast->node.if_node->elif_branches[i].body.count; j++) free_ast(ast->node.if_node->elif_branches[i].body.nodes[j]);
                free(ast->node.if_node->elif_branches[i].body.nodes);
            }
            free(ast->node.if_node->elif_branches);
            for (int i = 0; i < ast->node.if_node->else_block.count; i++) free_ast(ast->node.if_node->else_block.nodes[i]);
            free(ast->node.if_node->else_block.nodes); free(ast->node.if_node); break;
        case STMT_FOR:
            free_expr(ast->node.for_node->start); free_expr(ast->node.for_node->end);
            for (int i = 0; i < ast->node.for_node->body.count; i++) free_ast(ast->node.for_node->body.nodes[i]);
            free(ast->node.for_node->body.nodes); free(ast->node.for_node); break;
        case STMT_WHL:
            free_expr(ast->node.whl_node->condition);
            for (int i = 0; i < ast->node.whl_node->body.count; i++) free_ast(ast->node.whl_node->body.nodes[i]);
            free(ast->node.whl_node->body.nodes); free(ast->node.whl_node); break;
        case STMT_EACH:
            for (int i = 0; i < ast->node.each_node->body.count; i++) free_ast(ast->node.each_node->body.nodes[i]);
            free(ast->node.each_node->body.nodes); free(ast->node.each_node); break;
        case STMT_RPT:
            free_expr(ast->node.rpt_node->count);
            for (int i = 0; i < ast->node.rpt_node->body.count; i++) free_ast(ast->node.rpt_node->body.nodes[i]);
            free(ast->node.rpt_node->body.nodes); free(ast->node.rpt_node); break;
        case STMT_RET: free_expr(ast->node.ret_node->value); free(ast->node.ret_node); break;
        case STMT_LET: free_expr(ast->node.let_node->value); free(ast->node.let_node); break;
        case STMT_IO:
            for (int i = 0; i < ast->node.io_node->arg_count; i++) free_expr(ast->node.io_node->args[i]);
            free(ast->node.io_node); break;
        case STMT_EXT:
        case STMT_STP:
        case STMT_THR:
        case STMT_ASRT:
        case STMT_DBG:
        case STMT_LOG:
        case STMT_TRC:
        case STMT_DOC:
            if (ast->node.generic->expr) free_expr(ast->node.generic->expr);
            free(ast->node.generic); break;
        case STMT_PKG:
        case STMT_EXP:
            free(ast->node.name_node); break;
        case STMT_TRY:
            for (int i = 0; i < ast->node.try_node->try_block.count; i++) free_ast(ast->node.try_node->try_block.nodes[i]);
            free(ast->node.try_node->try_block.nodes);
            if (ast->node.try_node->has_catch) {
                for (int i = 0; i < ast->node.try_node->catch_block.count; i++) free_ast(ast->node.try_node->catch_block.nodes[i]);
                free(ast->node.try_node->catch_block.nodes);
            }
            free(ast->node.try_node); break;
        case STMT_IMP:
            free(ast->node.imp_node); break;
        case STMT_TST:
            if (ast->node.tst_node->expr) free_expr(ast->node.tst_node->expr);
            free(ast->node.tst_node); break;
        case STMT_CHK:
            if (ast->node.chk_node->expr) free_expr(ast->node.chk_node->expr);
            free(ast->node.chk_node); break;
        default: break;
    }
    free(ast);
}
