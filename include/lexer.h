#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOK_LST, TOK_WHN, TOK_TRN, TOK_SUM, TOK_EMT,
    TOK_PIPE,               /* |  */
    TOK_ARROW,              /* -> */
    TOK_LBRACK, TOK_RBRACK,
    TOK_NUMBER,             /* integer or float literal */
    TOK_IDENT,              /* variable name */
    TOK_ASSIGN,             /* = */
    TOK_OP,                 /* + - * / > < >= <= == != ^ */
    TOK_EOF,
    TOK_ERROR,
    TOK_VAR_REF             /* identifier immediately following an operator, e.g. *a +b */
} TokenType;

typedef struct {
    TokenType  type;
    char       lexeme[64];
    int        line;
} Token;

Token* tokenise(const char* src, int* count);

#endif