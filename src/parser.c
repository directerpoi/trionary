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
    return OP_ADD;
}

static Expr* create_expr(ExprType type) {
    Expr* expr = calloc(1, sizeof(Expr));
    expr->type = type;
    return expr;
}

static Expr* parse_coalesce();

static Expr* parse_primary() {
    Expr* expr;
    if (match(TOK_NUMBER)) {
        expr = create_expr(EXPR_NUMBER);
        expr->num_val = atof(global_tokens[current_pos - 1].lexeme);
        expr->line = global_tokens[current_pos - 1].line;
        expr->col  = global_tokens[current_pos - 1].col;
    } else if (peek().type == TOK_OP && strcmp(peek().lexeme, "(") == 0) {
        advance();
        expr = parse_coalesce();
        if (peek().type == TOK_OP && strcmp(peek().lexeme, ")") == 0) advance();
        else error_at(peek().line, "Expected ')' after expression");
    } else if (match(TOK_LBRACK)) {
        expr = create_expr(EXPR_LIST);
        expr->line = global_tokens[current_pos - 1].line;
        expr->col = global_tokens[current_pos - 1].col;
        if (peek().type != TOK_RBRACK) {
            do {
                if (expr->arg_count < MAX_PARAMS) {
                    expr->args[expr->arg_count++] = parse_coalesce();
                } else error_at(peek().line, "Too many elements in list literal");
            } while (peek().type == TOK_OP && strcmp(peek().lexeme, ",") == 0 && advance().type == TOK_OP);
        }
        if (!match(TOK_RBRACK)) error_at(peek().line, "Expected ']' after list literal");
    } else if (peek().type == TOK_IDENT || peek().type == TOK_VAR_REF) {
        Token t = advance();
        if (t.type == TOK_IDENT && (peek().type == TOK_NUMBER || peek().type == TOK_IDENT || 
            (peek().type == TOK_OP && strcmp(peek().lexeme, "(") == 0) || peek().type == TOK_LBRACK)) {
            expr = create_expr(EXPR_CALL);
            strncpy(expr->var_name, t.lexeme, 63);
            expr->line = t.line; expr->col = t.col;
            while (expr->arg_count < MAX_PARAMS && (peek().type == TOK_NUMBER || peek().type == TOK_IDENT || 
                   (peek().type == TOK_OP && strcmp(peek().lexeme, "(") == 0) || peek().type == TOK_LBRACK)) {
                if (peek().type == TOK_OP && strcmp(peek().lexeme, "(") == 0) {
                    expr->args[expr->arg_count++] = parse_primary();
                } else if (peek().type == TOK_LBRACK) {
                    expr->args[expr->arg_count++] = parse_primary();
                } else {
                    Token arg_t = advance();
                    Expr* arg;
                    if (arg_t.type == TOK_NUMBER) {
                        arg = create_expr(EXPR_NUMBER);
                        arg->num_val = atof(arg_t.lexeme);
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
            strncpy(expr->var_name, t.lexeme, 63);
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

static Expr* parse_coalesce() {
    Expr* left = parse_or();
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
    } else if (match(TOK_INPT)) {
        node->stmt_type = STMT_INPT; InptNode* in = node->node.inpt = calloc(1, sizeof(InptNode));
        in->line = global_tokens[current_pos-1].line;
        if (match(TOK_IDENT)) strncpy(in->var_name, global_tokens[current_pos-1].lexeme, 63);
        if (peek().type == TOK_STRING) { strncpy(in->prompt, advance().lexeme, 63); in->has_prompt = 1; }
    } else if (match(TOK_LST)) {
        int line = global_tokens[current_pos-1].line;
        node->stmt_type = STMT_PIPELINE; PipelineNode* p = node->node.pipeline = calloc(1, sizeof(PipelineNode));
        p->line = line; match(TOK_LBRACK);
        p->list = malloc(256 * sizeof(double));
        if (peek().type == TOK_NUMBER) {
            do { if (match(TOK_NUMBER)) p->list[p->list_len++] = atof(global_tokens[current_pos-1].lexeme);
            } while (peek().type == TOK_OP && strcmp(peek().lexeme, ",") == 0 && advance().type == TOK_OP);
        }
        match(TOK_RBRACK); skip_newlines();
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
            if (peek().type == TOK_STRING) { strncpy(p->emt_label, advance().lexeme, 63); p->has_emt_label = 1; }
            if (peek().type == TOK_IDENT && strcmp(peek().lexeme, "sep") == 0) {
                advance(); if (peek().type == TOK_STRING) { strncpy(p->emt_sep, advance().lexeme, 63); p->has_emt_sep = 1; }
            }
        }
    } else if (match(TOK_EMT)) {
        if (peek().type == TOK_STRING) { strncpy(node->emt_label, advance().lexeme, 63); node->has_emt_label = 1; }
        node->stmt_type = STMT_ARITH; node->node.arith = (ArithNode*)parse_coalesce();
    } else if (peek().type == TOK_NUMBER || peek().type == TOK_IDENT || peek().type == TOK_VAR_REF || peek().type == TOK_LBRACK || peek().type == TOK_NOT) {
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
            if (peek().type == TOK_STRING) { strncpy(node->emt_label, advance().lexeme, 63); node->has_emt_label = 1; }
        }
    } else {
        /* Do not report error here; let main.c handle skipping unknown tokens. */
        return NULL;
    }
    return node;
}

ASTNode* parse(Token* tok, int count) {
    global_tokens = tok; global_token_count = count; current_pos = 0;
    ASTNode* root = calloc(1, sizeof(ASTNode)); root->stmt_type = STMT_BLOCK;
    root->node.block = malloc(sizeof(BlockNode)); *root->node.block = parse_block();
    return root;
}

void free_expr(Expr* expr) {
    if (!expr) return;
    if (expr->type == EXPR_BINARY || expr->type == EXPR_COALESCE) { free_expr(expr->left); free_expr(expr->right); }
    else if (expr->type == EXPR_CALL || expr->type == EXPR_LIST) { for (int i = 0; i < expr->arg_count; i++) free_expr(expr->args[i]); }
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
        case STMT_PIPELINE:
            free(ast->node.pipeline->list); if (ast->node.pipeline->filter) free(ast->node.pipeline->filter);
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
        default: break;
    }
    free(ast);
}
