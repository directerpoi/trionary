#include "lexer.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKENS 4096

static int line = 1;
static int col  = 1;

static Token make_token(TokenType type, const char* lexeme, int token_col) {
    Token tok;
    tok.type = type;
    strncpy(tok.lexeme, lexeme, 63);
    tok.lexeme[63] = '\0';
    tok.line = line;
    tok.col  = token_col;
    return tok;
}

static int is_keyword(const char* word) {
    return strcmp(word, "lst") == 0 ||
           strcmp(word, "whn") == 0 ||
           strcmp(word, "trn") == 0 ||
           strcmp(word, "sum") == 0 ||
           strcmp(word, "emt") == 0 ||
           strcmp(word, "fn")  == 0 ||
           strcmp(word, "end") == 0 ||
           strcmp(word, "use") == 0 ||
           strcmp(word, "inpt") == 0;
}

static TokenType keyword_type(const char* word) {
    if (strcmp(word, "lst") == 0) return TOK_LST;
    if (strcmp(word, "whn") == 0) return TOK_WHN;
    if (strcmp(word, "trn") == 0) return TOK_TRN;
    if (strcmp(word, "sum") == 0) return TOK_SUM;
    if (strcmp(word, "emt") == 0) return TOK_EMT;
    if (strcmp(word, "fn")  == 0) return TOK_FN;
    if (strcmp(word, "end") == 0) return TOK_END;
    if (strcmp(word, "use") == 0) return TOK_USE;
    if (strcmp(word, "inpt") == 0) return TOK_INPT;
    return TOK_ERROR;
}

static void skip_whitespace_and_comments(const char* src, int* i) {
    while (src[*i] != '\0') {
        if (src[*i] == ' ' || src[*i] == '\t' || src[*i] == '\r') {
            (*i)++; col++;
        } else if (src[*i] == '#') {
            /* Skip to end of line (but don't consume the newline) */
            while (src[*i] != '\n' && src[*i] != '\0') { (*i)++; col++; }
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
    col  = 1;

    while (src[i] != '\0') {
        skip_whitespace_and_comments(src, &i);
        if (src[i] == '\0') break;

        // Keywords and identifiers
        if (isalpha(src[i]) || src[i] == '_') {
            int token_col = col;
            char word[64];
            int wi = 0;
            while ((isalpha(src[i]) || isdigit(src[i]) || src[i] == '_') && wi < 63) {
                word[wi++] = src[i++]; col++;
            }
            word[wi] = '\0';

            if (is_keyword(word)) {
                tokens[n++] = make_token(keyword_type(word), word, token_col);
            } else {
                tokens[n++] = make_token(TOK_IDENT, word, token_col);
            }
            continue;
        }

        // Numbers (integer or float)
        if (isdigit(src[i]) || (src[i] == '-' && isdigit(src[i+1]))) {
            int token_col = col;
            char num[64];
            int ni = 0;
            if (src[i] == '-') { num[ni++] = src[i++]; col++; }
            while (isdigit(src[i]) && ni < 63) {
                num[ni++] = src[i++]; col++;
            }
            if (src[i] == '.' && isdigit(src[i+1]) && ni < 63) {
                num[ni++] = src[i++]; col++;
                while (isdigit(src[i]) && ni < 63) {
                    num[ni++] = src[i++]; col++;
                }
            }
            num[ni] = '\0';
            tokens[n++] = make_token(TOK_NUMBER, num, token_col);
            continue;
        }

        // Single and double character operators/symbols
        if (src[i] == '|') {
            tokens[n++] = make_token(TOK_PIPE, "|", col);
            i++; col++;
            continue;
        }

        if (src[i] == '\n') {
            tokens[n++] = make_token(TOK_NEWLINE, "\n", col);
            line++; i++; col = 1;
            continue;
        }

        if (src[i] == '?' && src[i+1] != '\0' && src[i+1] == '?') {
            tokens[n++] = make_token(TOK_COALESCE, "??", col);
            i += 2; col += 2;
            continue;
        }

        if (src[i] == '-' && src[i+1] == '>') {
            tokens[n++] = make_token(TOK_ARROW, "->", col);
            i += 2; col += 2;
            continue;
        }

        if (src[i] == '[') {
            tokens[n++] = make_token(TOK_LBRACK, "[", col);
            i++; col++;
            continue;
        }

        if (src[i] == ']') {
            tokens[n++] = make_token(TOK_RBRACK, "]", col);
            i++; col++;
            continue;
        }

        if (src[i] == '=') {
            tokens[n++] = make_token(TOK_ASSIGN, "=", col);
            i++; col++;
            continue;
        }

        if (src[i] == ',') {
            tokens[n++] = make_token(TOK_OP, ",", col);
            i++; col++;
            continue;
        }

        // Comparison operators
        if (is_op_char(src[i])) {
            int token_col = col;
            char op[4];
            op[0] = src[i];
            if ((src[i] == '>' || src[i] == '<' || src[i] == '=' || src[i] == '!') && 
                src[i+1] == '=') {
                op[1] = src[i+1];
                op[2] = '\0';
                i += 2; col += 2;
            } else {
                op[1] = '\0';
                i++; col++;
            }
            tokens[n++] = make_token(TOK_OP, op, token_col);
            /* If the operator is immediately followed (no whitespace) by an alphabetic
               character or underscore, emit the identifier as TOK_VAR_REF so the parser
               can distinguish a variable reference (e.g. *a) from a numeric literal
               (e.g. *10). */
            if (isalpha(src[i]) || src[i] == '_') {
                int ref_col = col;
                char word[64];
                int wi = 0;
                while ((isalpha(src[i]) || isdigit(src[i]) || src[i] == '_') && wi < 63) {
                    word[wi++] = src[i++]; col++;
                }
                word[wi] = '\0';
                tokens[n++] = make_token(TOK_VAR_REF, word, ref_col);
            }
            continue;
        }

        // String literals (used for inpt prompts)
        if (src[i] == '"') {
            int token_col = col;
            char str[64];
            int si = 0;
            i++; col++; /* skip opening quote */
            while (src[i] != '"' && src[i] != '\0' && src[i] != '\n' && si < 63) {
                str[si++] = src[i++]; col++;
            }
            if (src[i] != '"') {
                error_at(line, "Unterminated string literal");
            } else {
                i++; col++; /* skip closing quote */
            }
            str[si] = '\0';
            tokens[n++] = make_token(TOK_STRING, str, token_col);
            continue;
        }

        // Unknown character
        error_at(line, "Unexpected character '%c'", src[i]);
    }

    tokens[n++] = make_token(TOK_EOF, "", col);
    *count = n;
    return tokens;
}

/* Static table of common keyword misspellings mapped to the correct keyword. */
const char* suggest_keyword(const char* word) {
    static const struct { const char* from; const char* to; } hints[] = {
        { "list",      "lst"  },
        { "when",      "whn"  },
        { "transform", "trn"  },
        { "trans",     "trn"  },
        { "emit",      "emt"  },
        { "function",  "fn"   },
        { "func",      "fn"   },
        { "input",     "inpt" },
        { "inp",       "inpt" },
        { "summary",   "sum"  },
        { "summ",      "sum"  },
    };
    for (size_t i = 0; i < sizeof(hints) / sizeof(hints[0]); i++) {
        if (strcmp(word, hints[i].from) == 0)
            return hints[i].to;
    }
    return NULL;
}