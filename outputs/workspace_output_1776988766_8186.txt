# Trionary v0 — Implementation Plan

**Language:** Trionary | **Standard:** C11 | **Executable:** `tri` | **Extension:** `.tri`  
**Design Axiom:** Minimal + Clear + Executable — NOT complex, NOT flexible, NOT feature-rich.

---

## Part 1 — Project Overview & Design Constraints

**Goal:** Build a single, zero-dependency C11 binary (`tri`) that runs `.tri` source files via `tri run file.tri` on any POSIX-compatible system.

**Five non-negotiable principles:**
1. Readable like English, parsed like code — syntax is intuitive on first read.
2. Minimal vocabulary — only the five keywords listed in Part 2 are valid.
3. No synonyms — one concept maps to exactly one keyword, forever.
4. Strict structure — no free-form sentences, no ambiguous grammar.
5. Clarity > flexibility — correctness and legibility over power or expressiveness.

**Hard constraints (what is NOT allowed):**
- Natural language sentences of any kind.
- Synonym variations (e.g. `when`, `filter`, `print`, `output` are all **invalid**).
- Complex grammar or optional clauses.
- HTTP, database, authentication, or any I/O beyond stdout.
- Bytecode compilation or a virtual machine.
- External C libraries of any kind.

---

## Part 2 — Keyword Specification (v0)

Trionary v0 defines **exactly five keywords**. Any unrecognised token is a parse error.

| Keyword | Full Name  | Role            | Description |
|---------|-----------|-----------------|-------------|
| `lst`   | List      | Pipeline Source | Initiates a pipeline with a literal list of values, e.g. `lst [1,2,3]` |
| `whn`   | When      | Filter          | Applies a boolean condition to each element; passes only those that match |
| `trn`   | Transform | Map             | Applies an arithmetic expression to each element, producing a new value |
| `sum`   | Summarise | Aggregation     | Reduces the current pipeline to a single numeric sum |
| `emt`   | Emit      | Output          | Writes the current result to stdout; terminates the pipeline |

**Pipeline feature details:**

| Feature        | Implementation Detail |
|----------------|----------------------|
| `lst [values]` | Produces a stream of numeric values. Comma-separated integers or floats inside `[ ]`. |
| `whn CONDITION`| Condition is a comparison operator + literal: `>N`, `<N`, `>=N`, `<=N`, `==N`, `!=N`. Applied element-wise. |
| `trn EXPR`     | Expression uses the implicit current element. Supported: `*N`, `/N`, `+N`, `-N`, `^N`. Applied element-wise. |
| `sum`          | Reduces all elements to a scalar sum. Must appear before `emt` in an aggregate pipeline. |
| `emt`          | Writes the current value or list to stdout and terminates the current statement. |

---

## Part 3 — Required Language Features

### 3.1 Arithmetic
Direct arithmetic expressions are valid standalone statements. The result flows into `emt` for output.

```tri
# Basic arithmetic
5 + 10 -> emt

# Operator precedence is standard (PEMDAS)
3 * 4 + 2 -> emt
```

### 3.2 Variables
Variables are assigned with `=` and are single-pass (no re-declaration). They may be used in arithmetic expressions and passed directly into `emt`.

```tri
a = 10
a + 5 -> emt

x = 3
y = 7
x * y -> emt
```

### 3.3 Pipeline (Core Identity)
The pipeline is the defining feature of Trionary. It chains `lst`, `whn`, `trn`, and `sum` in a single left-to-right data flow, terminating with `emt`. Each stage processes elements produced by the previous stage.

```tri
lst [1,2,3,4,5] | whn >2 | trn *10 | sum
emt
```

---

## Part 4 — File Structure & Core Data Structures

### 4.1 Source File Layout

```
trionary/
├── main.c          # CLI entry point — parses arguments, calls file reader
├── reader.h/.c     # Reads .tri source file into a char buffer
├── lexer.h/.c      # Tokeniser — emits a flat token array
├── parser.h/.c     # Pattern-based parser — builds AST nodes
├── exec.h/.c       # Execution engine — walks AST and produces output
├── output.h/.c     # Output system — wraps printf for emt
└── Makefile        # Build configuration (gcc, C11, single binary)
```

### 4.2 Core Data Structures (`token.h`)

```c
/* Token types */
typedef enum {
    TOK_LST, TOK_WHN, TOK_TRN, TOK_SUM, TOK_EMT,
    TOK_PIPE,               /* |  */
    TOK_ARROW,              /* -> */
    TOK_LBRACK, TOK_RBRACK,
    TOK_NUMBER,             /* integer or float literal */
    TOK_IDENT,              /* variable name */
    TOK_ASSIGN,             /* = */
    TOK_OP,                 /* + - * / > < >= <= == != */
    TOK_EOF,
    TOK_ERROR
} TokenType;

typedef struct {
    TokenType  type;
    char       lexeme[64];  /* raw text of this token */
    int        line;
} Token;

/* AST node types */
typedef enum {
    NODE_ARITH,    /* arithmetic expression */
    NODE_ASSIGN,   /* variable assignment   */
    NODE_PIPELINE, /* lst | whn | trn | sum */
    NODE_EMT       /* emit / output         */
} NodeType;

/* Symbol table */
typedef struct { char name[64]; double value; } Symbol;
typedef struct { Symbol entries[256]; int count; } SymTable;

void   sym_set(SymTable* t, const char* name, double val);
double sym_get(SymTable* t, const char* name);  /* error if not found */
```

---

## Part 5 — Tokenizer / Lexer (`lexer.c`)

The lexer is a **single-pass, O(n) character scanner**. It recognises only the tokens listed above and rejects everything else with a descriptive error. There is no backtracking.

### 5.1 Token Recognition Rules

| Feature      | Implementation Detail |
|-------------|----------------------|
| Keywords    | Exact ASCII match against `{lst, whn, trn, sum, emt}`. Case-sensitive. No partial matches. |
| Numbers     | Optional leading minus, digits, optional `.` + digits. Stored as `double` in `lexeme`. |
| Identifiers | Lowercase `a–z`, up to 63 characters. Used only for variable names. |
| Operators   | Single or double-character: `+ - * / > < >= <= == != \| ->` |
| Brackets    | `[ ]` for list literals only. |
| Whitespace  | Spaces, tabs, and newlines are consumed as delimiters. Newlines increment the line counter. |
| Comments    | `#` to end-of-line — entire segment is discarded. |
| Errors      | Any unrecognised character emits `TOK_ERROR` and halts tokenisation with the line number. |

### 5.2 Lexer Pseudocode

```c
Token* tokenise(const char* src, int* count) {
    Token* tokens = malloc(MAX_TOKENS * sizeof(Token));
    int i = 0, n = 0;

    while (src[i] != '\0') {
        skip_whitespace_and_comments(&i, src);
        if (is_alpha(src[i]))              { scan_word(src, &i, &tokens[n++]);   continue; }
        if (is_digit(src[i]))              { scan_number(src, &i, &tokens[n++]); continue; }
        if (src[i] == '|')                 { tokens[n++] = make(TOK_PIPE,   "|"); i++;      continue; }
        if (src[i] == '-' && src[i+1]=='>'){ tokens[n++] = make(TOK_ARROW, "->"); i+=2;    continue; }
        if (src[i] == '[')                 { tokens[n++] = make(TOK_LBRACK, "["); i++;      continue; }
        if (src[i] == ']')                 { tokens[n++] = make(TOK_RBRACK, "]"); i++;      continue; }
        if (is_op(src[i]))                 { scan_op(src, &i, &tokens[n++]);      continue; }
        error("Unexpected character '%c' at line %d", src[i], line);
    }
    tokens[n++] = make(TOK_EOF, "");
    *count = n;
    return tokens;
}
```

---

## Part 6 — Parser (`parser.c`)

The parser uses **fixed-pattern matching** — not a full recursive-descent grammar. Each statement must match exactly one of three recognised patterns. Any deviation is a hard parse error with no error recovery.

### 6.1 Grammar (EBNF)

```ebnf
program    ::= statement* EOF
statement  ::= assign | arith_emt | pipeline

assign     ::= IDENT '=' NUMBER
arith_emt  ::= expr '->' 'emt'
pipeline   ::= 'lst' list stage* ('emt' | '->' 'emt')

list       ::= '[' NUMBER (',' NUMBER)* ']'
stage      ::= '|' filter | '|' transform | '|' aggregate
filter     ::= 'whn' condition
transform  ::= 'trn' op_expr
aggregate  ::= 'sum'

condition  ::= ('>' | '<' | '>=' | '<=' | '==' | '!=') NUMBER
op_expr    ::= ('*' | '/' | '+' | '-') NUMBER
expr       ::= term (('+' | '-') term)*
term       ::= factor (('*' | '/') factor)*
factor     ::= NUMBER | IDENT
```

### 6.2 Pattern Matching Logic

| First Token             | Rule Applied  | Action |
|------------------------|---------------|--------|
| `IDENT` followed by `=`| `assign`      | Consume `IDENT`, `=`, `NUMBER`. Store in symbol table. |
| `NUMBER` or `IDENT` then op | `arith_emt` | Parse full expression until `->`, then expect `emt`. |
| `lst` token            | `pipeline`    | Parse list literal, then consume zero or more stage clauses. |
| Anything else          | **Hard error**| `Unexpected token {lexeme} at line {N}. Expected lst, identifier, or number.` |

---

## Part 7 — Execution Engine (`exec.c`)

The execution engine **directly walks the AST** produced by the parser. There is no bytecode, no virtual machine, and no intermediate compilation step. Each node type has a dedicated handler function.

### 7.1 Execution Model — Single-Pass Pipeline

- The pipeline is **never materialised into an intermediate array**.
- Each element produced by `lst` flows immediately through `whn` → `trn` → `sum` in one loop.
- Memory usage is **O(1)** with respect to list size for aggregate pipelines.
- For non-aggregate pipelines (no `sum`), output is streamed element-by-element.

### 7.2 Execution Pseudocode

```c
void exec_pipeline(PipelineNode* node) {
    double acc   = 0.0;
    bool do_sum  = node->has_sum;

    for (int i = 0; i < node->list_len; i++) {
        double val = node->list[i];

        /* Stage 1: filter (whn) */
        if (node->has_filter && !apply_condition(val, node->filter)) continue;

        /* Stage 2: transform (trn) */
        if (node->has_transform) val = apply_transform(val, node->transform);

        /* Stage 3: aggregate (sum) or immediate emit */
        if (do_sum) { acc += val; }
        else        { emit_value(val); }
    }
    if (do_sum) emit_value(acc);
}

void exec_assign(AssignNode* node, SymTable* sym) {
    sym_set(sym, node->name, node->value);
}

double exec_arith(ArithNode* node, SymTable* sym) {
    /* Recursive evaluation with operator precedence */
    return eval_expr(node->expr, sym);
}
```

### 7.3 Symbol Table

- Flat array of name-value pairs — max **256 variables** for v0.
- Lookup is a linear scan (acceptable for v0 where programs are small).

---

## Part 8 — Output System (`output.c`)

The `emt` keyword triggers the output system. All output goes to **stdout** followed by a newline.

- **Integers:** printed without a decimal point.
- **Floats:** up to 6 significant figures with trailing zeros stripped.

```c
void emit_value(double v) {
    if (v == (long long)v && v >= LLONG_MIN && v <= LLONG_MAX)
        printf("%lld\n", (long long)v);
    else
        printf("%.6g\n", v);
}
```

---

## Part 9 — Build & Run Instructions

### 9.1 Compile (manual)

```bash
gcc -std=c11 -Wall -Wextra -O2 \
    main.c reader.c lexer.c parser.c exec.c output.c \
    -o tri
```

### 9.2 Run

```bash
tri run file.tri
```

### 9.3 Makefile

```makefile
CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2
SRCS    = main.c reader.c lexer.c parser.c exec.c output.c
TARGET  = tri

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)
```

---

## Part 10 — Example Programs, Validation & Implementation Order

### 10.1 Example `.tri` Files

**test_arith.tri** — expected output: `15`, `14`
```tri
5 + 10 -> emt
3 * 4 + 2 -> emt
```

**test_vars.tri** — expected output: `15`, `70`
```tri
a = 10
a + 5 -> emt
b = 7
a * b -> emt
```

**test_pipeline.tri** — expected output: `120`
```tri
lst [1,2,3,4,5] | whn >2 | trn *10 | sum
emt
```
*(Filter passes 3, 4, 5 → Transform yields 30, 40, 50 → Sum = 120)*

### 10.2 Validation Criteria

| Test | Expected | Notes |
|------|----------|-------|
| `tri run test_arith.tri` | `15` then `14` | No extra whitespace. |
| `tri run test_vars.tri`  | `15` then `70` | Variables resolve correctly. |
| `tri run test_pipeline.tri` | `120` | Full pipeline chain verified. |
| Invalid keyword (e.g. `print`) | `Error: Unknown keyword 'print' at line N.` — exits code 1 | |
| Malformed syntax (missing `emt`) | `Error: Expected 'emt' at line N.` — exits code 1 | |

The implementation is considered **complete** only when all three test files produce the expected output consistently across platforms.

### 10.3 Implementation Order

Follow this exact sequence. Each phase **must compile and pass its own micro-test** before moving to the next.

| Step | File       | Responsibility |
|------|-----------|----------------|
| 1    | `main.c`   | CLI — argument parsing; enforce `tri run <file>` signature; open file or print usage. |
| 2    | `reader.c` | File reader — read entire `.tri` file into a heap-allocated null-terminated `char` buffer. |
| 3    | `lexer.c`  | Tokenizer — produce flat token array from buffer; test with a trivial `.tri` file. |
| 4    | `parser.c` | Parser — pattern-match token array into AST nodes; test each pattern independently. |
| 5    | `exec.c`   | Execution engine — walk AST, evaluate variables and arithmetic; wire up pipeline stages. |
| 6    | `output.c` | Output system — implement `emit_value` with integer/float formatting; hook into `emt` nodes. |
