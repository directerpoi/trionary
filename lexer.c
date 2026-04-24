#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKENS 4096

static int line = 1;

static Token make_token(TokenType type, const char* lexeme) {
    Token tok;
    tok.type = type;
    strncpy(tok.lexeme, lexeme, 63);
    tok.lexeme[63] = '\0';
    tok.line = line;
    return tok;
}

static int is_keyword(const char* word) {
    return strcmp(word, "lst") == 0 ||
           strcmp(word, "whn") == 0 ||
           strcmp(word, "trn") == 0 ||
           strcmp(word, "sum") == 0 ||
           strcmp(word, "emt") == 0;
}

static TokenType keyword_type(const char* word) {
    if (strcmp(word, "lst") == 0) return TOK_LST;
    if (strcmp(word, "whn") == 0) return TOK_WHN;
    if (strcmp(word, "trn") == 0) return TOK_TRN;
    if (strcmp(word, "sum") == 0) return TOK_SUM;
    if (strcmp(word, "emt") == 0) return TOK_EMT;
    return TOK_ERROR;
}

static void skip_whitespace_and_comments(const char* src, int* i) {
    while (src[*i] != '\0') {
        if (src[*i] == ' ' || src[*i] == '\t' || src[*i] == '\r') {
            (*i)++;
        } else if (src[*i] == '\n') {
            line++;
            (*i)++;
        } else if (src[*i] == '#') {
            while (src[*i] != '\n' && src[*i] != '\0') (*i)++;
        } else {
            break;
        }
    }
}

static int is_op_char(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || 
           c == '>' || c == '<' || c == '=' || c == '!' || c == '^';
}

Token* tokenise(const char* src, int* count) {
    Token* tokens = malloc(MAX_TOKENS * sizeof(Token));
    if (!tokens) return NULL;

    int i = 0, n = 0;
    line = 1;

    while (src[i] != '\0') {
        skip_whitespace_and_comments(src, &i);
        if (src[i] == '\0') break;

        // Keywords and identifiers
        if (isalpha(src[i])) {
            char word[64];
            int wi = 0;
            while (isalpha(src[i]) && wi < 63) {
                word[wi++] = src[i++];
            }
            word[wi] = '\0';

            if (is_keyword(word)) {
                tokens[n++] = make_token(keyword_type(word), word);
            } else {
                tokens[n++] = make_token(TOK_IDENT, word);
            }
            continue;
        }

        // Numbers (integer or float)
        if (isdigit(src[i]) || (src[i] == '-' && isdigit(src[i+1]))) {
            char num[64];
            int ni = 0;
            if (src[i] == '-') num[ni++] = src[i++];
            while (isdigit(src[i]) && ni < 63) {
                num[ni++] = src[i++];
            }
            if (src[i] == '.' && isdigit(src[i+1]) && ni < 63) {
                num[ni++] = src[i++];
                while (isdigit(src[i]) && ni < 63) {
                    num[ni++] = src[i++];
                }
            }
            num[ni] = '\0';
            tokens[n++] = make_token(TOK_NUMBER, num);
            continue;
        }

        // Single and double character operators/symbols
        if (src[i] == '|') {
            tokens[n++] = make_token(TOK_PIPE, "|");
            i++;
            continue;
        }

        if (src[i] == '-' && src[i+1] == '>') {
            tokens[n++] = make_token(TOK_ARROW, "->");
            i += 2;
            continue;
        }

        if (src[i] == '[') {
            tokens[n++] = make_token(TOK_LBRACK, "[");
            i++;
            continue;
        }

        if (src[i] == ']') {
            tokens[n++] = make_token(TOK_RBRACK, "]");
            i++;
            continue;
        }

        if (src[i] == '=') {
            tokens[n++] = make_token(TOK_ASSIGN, "=");
            i++;
            continue;
        }

        if (src[i] == ',') {
            tokens[n++] = make_token(TOK_OP, ",");
            i++;
            continue;
        }

        // Comparison operators
        if (is_op_char(src[i])) {
            char op[4];
            op[0] = src[i];
            if ((src[i] == '>' || src[i] == '<' || src[i] == '=' || src[i] == '!') && 
                src[i+1] == '=') {
                op[1] = src[i+1];
                op[2] = '\0';
                tokens[n++] = make_token(TOK_OP, op);
                i += 2;
                continue;
            }
            op[1] = '\0';
            tokens[n++] = make_token(TOK_OP, op);
            i++;
            continue;
        }

        // Unknown character
        fprintf(stderr, "Error: Unexpected character '%c' at line %d\n", src[i], line);
        tokens[n++] = make_token(TOK_ERROR, "");
        *count = n;
        return tokens;
    }

    tokens[n++] = make_token(TOK_EOF, "");
    *count = n;
    return tokens;
}