#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOK_LST, TOK_WHN, TOK_TRN, TOK_SUM, TOK_EMT,
    TOK_FN,                 /* fn  */
    TOK_END,                /* end */
    TOK_USE,                /* use */
    TOK_IF, TOK_ELS, TOK_ELIF,
    TOK_FOR, TOK_WHL, TOK_EACH,
    TOK_BRK, TOK_NXT, TOK_RET,
    TOK_NOT, TOK_AND, TOK_OR,
    TOK_IN, TOK_LET, TOK_RPT,
    TOK_STR, TOK_ARR, TOK_BOOL, TOK_TRUE, TOK_FLS, TOK_NIL,
    TOK_MAP, TOK_INT, TOK_FLT, TOK_PAIR, TOK_TPL, TOK_SET,
    TOK_PIPE,               /* |  */
    TOK_ARROW,              /* -> */
    TOK_LBRACK, TOK_RBRACK,
    TOK_LBRACE, TOK_RBRACE,
    TOK_LPAREN, TOK_RPAREN,
    TOK_COLON, TOK_COMMA, TOK_DOT,
    TOK_NUMBER,             /* integer or float literal */
    TOK_IDENT,              /* variable name */
    TOK_ASSIGN,             /* = */
    TOK_OP,                 /* + - * / > < >= <= == != ^ */
    TOK_EOF,
    TOK_ERROR,
    TOK_VAR_REF,            /* identifier immediately following an operator, e.g. *a +b */
    TOK_NEWLINE,            /* newline — used to separate fn params from body */
    TOK_COALESCE,           /* ?? — default/fallback operator */
    TOK_INPT,               /* inpt — interactive numeric input keyword */
    TOK_SAY,                /* say — print with newline */
    TOK_PRT,                /* prt — print without newline */
    TOK_ASK,                /* ask — prompt and read input */
    TOK_EXT,                /* ext — exit program */
    TOK_STP,                /* stp — stop program */
    TOK_LMB,                /* lmb — lambda */
    TOK_IMP,                /* imp — import */
    TOK_AS,                 /* as — alias */
    TOK_FRM,                /* frm — from */
    TOK_EXP,                /* exp — export */
    TOK_PKG,                /* pkg — package */
    TOK_TRY,                /* try */
    TOK_CTCH,               /* ctch — catch */
    TOK_THR,                /* thr — throw */
    TOK_ERR,                /* err — error value/type */
    TOK_ASRT,               /* asrt — assert */
    TOK_DFLT,               /* dflt — default */
    TOK_DBG,                /* dbg — debug */
    TOK_LOG,                /* log */
    TOK_TST,                /* tst — test */
    TOK_TRC,                /* trc — trace */
    TOK_DOC,                /* doc — documentation */
    TOK_CHK,                /* chk — type check */
    TOK_TIM,                /* tim — time */
    TOK_FRD,                /* frd — file read */
    TOK_FWR,                /* fwr — file write */
    TOK_FAP,                /* fap — file append */
    TOK_CSV,                /* csv — csv read */
    TOK_JRD,                /* jrd — json read */
    TOK_STRING              /* "..." — string literal */
} TokenType;

typedef struct {
    TokenType  type;
    char       lexeme[64];
    int        line;
    int        col;  /* 1-based column at start of token */
} Token;

Token* tokenise(const char* src, int* count);

/* suggest_keyword: if 'word' is a common misspelling of a Trionary keyword,
 * return the correct keyword string; otherwise return NULL.
 * Used by the parser to emit actionable "Did you mean?" hints.
 */
const char* suggest_keyword(const char* word);

#endif