# Trionary v0.1.0 — Step 1 Audit (Regression Baseline)

> This document is the deliverable for **Step 1** of the `plan.md` v0.2.0 implementation plan.
> It records the exact grammar, token set, symbol-table layout, and pipeline execution model
> of the v0.1.0 codebase. No future v0.2.0 change may produce output that deviates from the
> baseline test results captured here.

---

## 1. Source File Inventory

| File | Role |
|------|------|
| `src/main.c` | CLI entry point, statement-splitting loop, top-level dispatch |
| `src/reader.c` | Reads a `.tri` file into a malloc'd string buffer |
| `src/lexer.c` | Single-pass character scanner that emits a flat `Token[]` array |
| `src/parser.c` | Pattern-matching parser; builds one `ASTNode` per statement |
| `src/exec.c` | AST walker, symbol table, pipeline runner, arithmetic evaluator |
| `src/output.c` | `emit_value()` — formats and prints a `double` to stdout |
| `include/lexer.h` | `TokenType` enum + `Token` struct declarations |
| `include/parser.h` | All AST node types, `OpType` enum, `parse()` / `free_ast()` |
| `include/exec.h` | `Symbol`, `SymTable`, `execute()` declarations |
| `include/reader.h` | `read_file()` declaration |
| `include/output.h` | `emit_value()` declaration |

---

## 2. Token Set

All token types are defined in `include/lexer.h` (`TokenType` enum):

| Token | Lexeme(s) | Notes |
|-------|-----------|-------|
| `TOK_LST` | `lst` | Pipeline start keyword |
| `TOK_WHN` | `whn` | Filter keyword |
| `TOK_TRN` | `trn` | Transform keyword |
| `TOK_SUM` | `sum` | Aggregate keyword |
| `TOK_EMT` | `emt` | Emit / output keyword |
| `TOK_PIPE` | `\|` | Pipeline stage separator |
| `TOK_ARROW` | `->` | Inline emit separator |
| `TOK_LBRACK` | `[` | Opens list literal |
| `TOK_RBRACK` | `]` | Closes list literal |
| `TOK_NUMBER` | integer or float literal | e.g. `42`, `3.14`, `-7` |
| `TOK_IDENT` | alphabetic identifier | variable name, e.g. `price` |
| `TOK_ASSIGN` | `=` | Variable assignment operator |
| `TOK_OP` | `+` `-` `*` `/` `^` `>` `<` `>=` `<=` `==` `!=` `,` | Arithmetic, comparison, and list separator |
| `TOK_EOF` | (empty) | Sentinel at end of token stream |
| `TOK_ERROR` | (empty) | Emitted on unrecognised input character |

**Lexer behaviour notes:**
- Single-pass, O(n) character scan (`src/lexer.c`).
- `line` counter starts at 1, incremented on every `\n`.
- Comments begin with `#` and extend to end of line; consumed silently.
- Identifiers consist of alphabetic characters only (`isalpha`); digits and underscores are **not** part of an identifier.
- A leading `-` is absorbed into a `TOK_NUMBER` only when followed immediately by a digit.
- `->` is recognised before the single-character `-` branch.
- Unknown characters emit `TOK_ERROR` and halt the token stream immediately (error to `stderr`).
- Maximum token stream size: 4096 tokens (`MAX_TOKENS`).
- Maximum lexeme length: 63 characters + null terminator.

---

## 3. Grammar (v0.1.0)

A Trionary source file is a sequence of **statements**. The statement boundary is determined by `main.c`'s scanning loop, not the parser.

```
program        ::= statement* EOF

statement      ::= assign_stmt
                 | arith_stmt
                 | pipeline_stmt

assign_stmt    ::= IDENT '=' NUMBER

arith_stmt     ::= expr '->' 'emt'

expr           ::= term ( ('+' | '-') term )*
term           ::= factor ( ('*' | '/') factor )*
factor         ::= NUMBER | IDENT

pipeline_stmt  ::= 'lst' '[' number_list ']' stage* emit_tail

number_list    ::= NUMBER ( ',' NUMBER )*
               |   (empty)

stage          ::= '|' filter_stage
               |   '|' transform_stage
               |   '|' sum_stage

filter_stage   ::= 'whn' OP NUMBER
transform_stage::= 'trn' OP NUMBER
sum_stage      ::= 'sum'

emit_tail      ::= '->' 'emt'
               |   'emt'               /* standalone on next line */

OP             ::= '>' | '<' | '>=' | '<=' | '=' | '!='
                 | '+' | '-' | '*' | '/' | '^'
```

**Constraints / observations:**
- Only one `whn` stage per pipeline is supported (the parser stores a single `Condition*`).
- Only one `trn` stage per pipeline is supported (the parser stores a single `Transform*`).
- The `sum_stage` sets a flag; there is no operand.
- `assign_stmt` accepts only a literal `NUMBER` on the right-hand side, not an expression.
- `arith_stmt` requires `-> emt` to produce output; a bare expression without `-> emt` is treated as a malformed statement and emits `0` (no error is raised for `5 + 10` with no emit).
- `emt` may appear standalone on the line after a pipeline that ends with `sum` (the `sum -> emt` inline form is also accepted).

---

## 4. AST Node Types

Defined in `include/parser.h`:

### `ASTNode` (wrapper)

```c
typedef struct {
    NodeType type;
    union { ArithNode* arith; AssignNode* assign; PipelineNode* pipeline; } node;
    enum { STMT_EMTPY, STMT_ARITH, STMT_ASSIGN, STMT_PIPELINE } stmt_type;
} ASTNode;
```

`stmt_type` is the authoritative discriminator used by `execute()`.

### `AssignNode`

```c
typedef struct { NodeType type; char name[64]; double value; } AssignNode;
```

Stores variable name and the literal numeric value.

### `ArithNode` (also used as internal `Expr` in `parser.c`)

The public header declares `ArithNode` but the parser internally builds a recursive `Expr` tree cast to `ArithNode*`. The executor casts it back to its own identical `Expr` struct to walk it.

### `PipelineNode`

```c
typedef struct {
    NodeType type;
    double*    list;         /* heap-allocated array of input values */
    int        list_len;
    Condition* filter;       /* NULL when has_filter == 0 */
    Transform* transform;    /* NULL when has_transform == 0 */
    int has_filter;
    int has_transform;
    int has_sum;
} PipelineNode;
```

### `Condition` / `Transform`

```c
typedef struct { char op_lexeme[4]; OpType op; double value; } Condition;
typedef struct { char op_lexeme[4]; OpType op; double value; } Transform;
```

Both hold a parsed operator and a literal `double` operand.

---

## 5. Symbol Table Layout

Defined in `include/exec.h`, implemented in `src/exec.c`:

```c
typedef struct { char name[64]; double value; } Symbol;
typedef struct { Symbol entries[256]; int count; } SymTable;
```

- **Maximum capacity:** 256 variables (static array, no dynamic growth).
- **Lookup:** Linear scan by name (`strcmp`), O(n).
- **Set:** Updates in-place if name exists; appends otherwise.
- **Get:** Returns `0.0` and prints an error to `stderr` for undefined names.
- **Scope:** Single flat global scope; no block or function scoping.
- **Lifetime:** Created once in `main()`, shared across all statements in the file, freed at exit.
- Variable values are `double` internally; only integer literals are accepted by `parse_assign()`.

---

## 6. Pipeline Execution Model

Implemented in `exec_pipeline()` (`src/exec.c`):

```
for each value in list:
    1. Apply filter (whn):  if condition fails → skip element
    2. Apply transform (trn): val = op(val, operand)
    3. If has_sum:  acc += val
       Else:       emit_value(val)
After loop:
    If has_sum: emit_value(acc)
```

- **Order:** filter → transform → aggregate/emit, always in that fixed sequence.
- **Memory:** O(1) working state (`acc`, `val`); the input list is the only allocation.
- **No side effects on symbol table:** pipelines do not read or write variables.
- **Output:** `emit_value()` in `src/output.c` — integers printed with `%.0f`, floats with `%.6g`.

---

## 7. Main Loop (Statement Splitting)

`src/main.c` performs a manual token-scan to find statement boundaries **before** calling `parse()`:

| Statement pattern detected | Tokens consumed |
|---------------------------|----------------|
| `IDENT = NUMBER` | 3 tokens |
| `lst [ … ] \| … emt` | From `lst` up to and including `emt` or `-> emt` |
| `NUMBER \| IDENT … -> emt` | From token to `emt` inclusive |
| Anything else | 1 token (skip) |

The slice `tokens[start..start+stmt_len]` is passed to `parse()` as a mini token stream.

---

## 8. Test Files and Regression Baseline Outputs

All outputs captured with the compiled `tri` binary (v0.1.0, built with `make`).

### `tests/test_arith.tri`
```tri
5 + 10 -> emt
3 * 4 + 2 -> emt
```
**stdout:**
```
15
14
```
**stderr:** *(empty)*  **exit:** 0

---

### `tests/test_vars.tri`
```tri
a = 10
a + 5 -> emt
b = 7
a * b -> emt
```
**stdout:**
```
15
70
```
**stderr:** *(empty)*  **exit:** 0

---

### `tests/test_pipeline.tri`
```tri
lst [1,2,3,4,5] | whn >2 | trn *10 | sum
emt
```
**stdout:**
```
120
```
**stderr:** *(empty)*  **exit:** 0

---

### `tests/test_all.tri`
```tri
# Comments are supported - they start with #

# Arithmetic with operator precedence
3 * 4 + 2 -> emt
10 - 2 * 3 -> emt

# Variables
x = 5
y = 10
x * y -> emt

# Pipeline with filter
lst [1,2,3,4,5,6,7,8,9,10] | whn >5 -> emt

# Pipeline with transform
lst [2,3,4] | trn *10 -> emt

# Pipeline with sum
lst [1,2,3,4,5] | sum -> emt

# Combined pipeline
lst [1,2,3,4,5] | whn >2 | trn *10 -> emt
```
**stdout:**
```
14
4
50
6
7
8
9
10
20
30
40
15
30
40
50
```
**stderr:** *(empty)*  **exit:** 0

---

### `tests/demo.tri`
**stdout:**
```
15
14
80
6
7
8
9
10
12
14
16
18
20
15
120
```
**stderr:** *(empty)*  **exit:** 0

---

### `tests/test_error.tri`
```tri
5 + 10
x =
```
**stdout:**
```
15
```
**stderr:** *(empty)*  **exit:** 0  
*(Note: `5 + 10` without `-> emt` still emits 15 because main.c's arith scan consumes to `TOK_ARROW` which is absent, leaving the expression parsed as a statement. `x =` is parsed as an assign with missing number — `parse_assign()` sets `error_occurred` but the node's value defaults to 0, and since the AST is freed on error… actually the file produces exit 0 and stdout 15 as captured.)*

---

### `tests/test_invalid.tri`
```tri
print "hello"
```
**stdout:**
```
0
```
**stderr:**
```
Error: Unexpected character '"' at line 1
Error: Undefined variable 'print' at line 1
```
**exit:** 0

---

### `tests/test_malformed.tri`
```tri
5 + 10
```
**stdout:**
```
15
```
**stderr:** *(empty)*  **exit:** 0

---

## 9. Identified Constraints for v0.2.0

The following behaviours must be **preserved unchanged** by every v0.2.0 step:

1. All baseline stdout/stderr/exit values above are byte-for-byte fixed.
2. The five keyword spellings (`lst`, `whn`, `trn`, `sum`, `emt`) are immutable.
3. No existing `TokenType` value is removed or renumbered.
4. `SymTable` max capacity (256) and linear-scan semantics remain.
5. `emit_value()` output format (integer vs `%.6g` float) is unchanged.
6. The single-pass pipeline order (filter → transform → aggregate) is unchanged.
7. `assign_stmt` still accepts only `IDENT = NUMBER` (expression on RHS is a Step 3+ concern).
8. Error messages continue going to `stderr`; normal output goes to `stdout`.

---

## 15. What Was Done (Step 6 Summary)

**Goal:** Support multiple pipelines per file — the interpreter must reset pipeline-internal state after each `emt` and immediately accept a new `lst` (or end-of-file), while keeping the symbol table alive so variables set before or between pipelines remain visible in subsequent ones.

### Architecture review

The v0.1.0 / v0.2.0 architecture already satisfies the requirement through `main.c`'s statement-splitting loop:

1. **`main.c`** — a `while` loop scans the flat token stream, isolates each statement (assignment, arithmetic emit, or a complete `lst … emt` pipeline) into a sub-slice, then calls `parse()` and `execute()` for that slice. After `execute()` returns the loop immediately advances to the next statement, so any number of `lst … emt` pipelines in the same file are processed in order.
2. **`parser.c` — `parse()`** — resets all static parser state (`current`, `tokens`, `token_count`, `error_occurred`) at the very start of every call. Each pipeline therefore begins with a fresh `PipelineNode`; no filter, transform, or sum flag from a previous pipeline can bleed through.
3. **`exec.c` — `execute()`** — receives the same `SymTable* sym` pointer on every invocation. Variables assigned by an earlier statement (including variables assigned between pipelines) are visible to all subsequent pipelines. The symbol table is never cleared between pipelines.

No code path assumed "exactly one pipeline per file". The multi-pipeline capability was inherent in the per-statement dispatch model from the outset.

### Changes made

To make the above intent explicit and discoverable, the following documentation comments were added. No logic was altered.

| File | Change |
|------|--------|
| `src/main.c` | Added a block comment above the statement-dispatch loop explaining that (a) the loop supports an arbitrary number of pipelines, (b) the symbol table is shared across all iterations, and (c) each pipeline's internal state is owned by its `PipelineNode` and freed after execution, so every pipeline starts clean. |
| `src/parser.c` | Added a comment at the top of `parse()` explaining why all static state is reset on every call and how that enables consecutive `lst … emt` pipelines to coexist without interference. |
| `src/exec.c` | Added a comment at the top of `execute()` noting that `sym` is shared across all statements and is never reset, which is what gives symbol-table persistence across pipelines. |

### Multi-pipeline behaviour verified

The following cases were exercised and confirmed correct after the change:

| Input pattern | Result |
|---------------|--------|
| Two `lst…emt` pipelines with `-> emt` | Both emit correctly; second pipeline unaffected by first |
| First pipeline ends with standalone `emt`; second follows | Both emit correctly |
| Variable assigned before pipelines used inside both | Correct value seen in both pipelines |
| Variable assigned between two pipelines | Second pipeline sees the updated value |
| Mixed: assignment → pipeline → arithmetic emit → pipeline | All four statements execute in order, sharing the same symbol table |

### Backward-compatibility verification

- All eight existing `.tri` test files (`test_arith.tri`, `test_vars.tri`, `test_pipeline.tri`, `test_all.tri`, `demo.tri`, `test_error.tri`, `test_invalid.tri`, `test_malformed.tri`) were rebuilt and re-run after the change.
- Every file produced **byte-for-byte identical** stdout, stderr, and exit code to the v0.1.0 baseline recorded in Section 8.
- No token type, AST node type, symbol-table logic, or execution path was removed or altered.

---

## 10. What Was Done (Step 1 Summary)

- Read and annotated every source file (`main.c`, `reader.c`, `lexer.c`, `parser.c`, `exec.c`, `output.c`) and every header (`lexer.h`, `parser.h`, `exec.h`, `reader.h`, `output.h`).
- Compiled the project with `make` (no changes, confirming a clean build).
- Executed `tri run` against all eight `.tri` test files and recorded stdout, stderr, and exit codes.
- Documented the complete token set (16 token types), the v0.1.0 grammar in EBNF, all AST node structures, the symbol-table layout, and the pipeline execution model.
- Noted all behavioural constraints that subsequent steps must not violate.
- No source files were modified; this is a read-only audit step.

---

## 11. What Was Done (Step 2 Summary)

**Goal:** Extend the lexer to recognise variable-reference tokens inside pipeline transform positions (e.g. `*a`, `+b`), emitting a dedicated `TOK_VAR_REF` token so the parser (Step 3) can distinguish a variable operand from a literal number without ambiguity.

### Files modified

| File | Change |
|------|--------|
| `include/lexer.h` | Added `TOK_VAR_REF` to the `TokenType` enum, appended **after** `TOK_ERROR` so no existing enum value is removed, renamed, or renumbered. |
| `src/lexer.c` | Merged the two `continue` branches of the operator scanner into one unified block. After emitting a `TOK_OP` token, the scanner now checks whether the very next character (with no intervening whitespace) is alphabetic. If so, it reads the full identifier and emits it as `TOK_VAR_REF` instead of `TOK_IDENT`. |

### New token

| Token | Lexeme | Trigger |
|-------|--------|---------|
| `TOK_VAR_REF` | identifier name (e.g. `scale`) | An operator character is followed **immediately** (no whitespace) by one or more alphabetic characters, e.g. `*scale`, `+b`, `>=limit`. |

### Lexing examples

| Input fragment | Token stream produced |
|----------------|-----------------------|
| `trn *scale` | `TOK_TRN` `TOK_OP("*")` `TOK_VAR_REF("scale")` |
| `whn >threshold` | `TOK_WHN` `TOK_OP(">")` `TOK_VAR_REF("threshold")` |
| `trn *10` | `TOK_TRN` `TOK_OP("*")` `TOK_NUMBER("10")` — **unchanged** |
| `a * b` (spaces) | `TOK_IDENT("a")` `TOK_OP("*")` `TOK_IDENT("b")` — **unchanged** |

### Backward-compatibility verification

- All eight existing `.tri` test files were re-run after the change.
- Every file produced **byte-for-byte identical** stdout, stderr, and exit code to the v0.1.0 baseline recorded in this document (Section 8).
- No existing token type was removed, renamed, or renumbered.
- The existing `TOK_IDENT` path (alphabetic chars after whitespace) is completely untouched.
- The `TOK_VAR_REF` path only fires when an operator is followed **without whitespace** by an alphabetic character — a pattern absent from all v0.1.0 test inputs.

---

## 12. What Was Done (Step 3 Summary)

**Goal:** Extend the parser's `trn` rule to accept a variable reference (a `TOK_VAR_REF` token produced by Step 2's lexer change) in addition to a literal number operand. The variable name is stored in the AST at parse time so that value look-up is deferred to execution time (Step 4).

### Files modified

| File | Change |
|------|--------|
| `include/parser.h` | Extended the `Transform` struct with two new fields: `int is_var_ref` (flag, 0 for literal / 1 for variable) and `char var_name[64]` (the variable name when `is_var_ref == 1`). No existing field was removed or renamed. |
| `src/parser.c` | In `parse_pipeline()`, replaced the single `TOK_NUMBER` branch after the `trn` operator with a three-way branch: match `TOK_VAR_REF` → set `is_var_ref = 1` and copy the name; match `TOK_NUMBER` → set `value` as before; otherwise → emit error `"Expected number or variable after transform operator"`. New fields are always zero-initialised before the branch so literal-number paths are unaffected. |

### Grammar change (additive)

```
transform_stage ::= 'trn' OP NUMBER        /* unchanged */
                  | 'trn' OP VAR_REF       /* new: variable-reference operand */
```

`whn`, `lst`, `sum`, `emt`, and all arithmetic/assignment paths are **syntactically unchanged**.

### Backward-compatibility verification

- All eight existing `.tri` test files (`test_arith.tri`, `test_vars.tri`, `test_pipeline.tri`, `test_all.tri`, `demo.tri`, `test_error.tri`, `test_invalid.tri`, `test_malformed.tri`) were rebuilt and re-run after the change.
- Every file produced **byte-for-byte identical** stdout, stderr, and exit code to the v0.1.0 baseline recorded in Section 8.
- No existing token type, AST node type, or executor path was removed or altered.
- The `apply_transform()` function in `exec.c` remains untouched; it still reads `trn->value`, which is initialised to `0.0` for variable-reference nodes (Step 4 will add the symbol-table lookup).

---

## 13. What Was Done (Step 4 Summary)

**Goal:** Update the executor's `trn` handler to resolve variable references at execution time. When the `Transform` node carries `is_var_ref == 1`, the executor looks up the variable in the symbol table and aborts with a clear error if it is undefined. All arithmetic and non-pipeline execution paths remain untouched.

### Files modified

| File | Change |
|------|--------|
| `src/exec.c` | Three targeted edits (described below); no other file was changed. |

### Changes in detail

1. **`apply_transform()` signature extended** — The function now accepts a `SymTable* sym` third parameter. At the top of the function, a local `double operand` is resolved: if `trn->is_var_ref == 1` the function calls `sym_exists()` and, if the variable is absent, prints `"Error: Undefined variable '<name>'"` to `stderr` and calls `exit(1)` to abort cleanly; if the variable exists, `sym_get()` returns its current value. When `is_var_ref == 0` the literal `trn->value` is used, preserving the unchanged v0.1.0 path exactly. All five arithmetic operators (`+`, `-`, `*`, `/`, `^`) now use `operand` instead of `trn->value`.

2. **`exec_pipeline()` signature extended** — Accepts a new `SymTable* sym` parameter and forwards it to every `apply_transform()` call inside the element loop.

3. **`execute()` updated** — The `STMT_PIPELINE` branch passes the existing `sym` pointer through to `exec_pipeline()`.

### Backward-compatibility verification

- All eight existing `.tri` test files (`test_arith.tri`, `test_vars.tri`, `test_pipeline.tri`, `test_all.tri`, `demo.tri`, `test_error.tri`, `test_invalid.tri`, `test_malformed.tri`) were rebuilt and re-run after the change.
- Every file produced **byte-for-byte identical** stdout, stderr, and exit code to the v0.1.0 baseline recorded in Section 8.
- No existing token type, AST node type, or other execution path was removed or altered.
- The `sym_get()` / `sym_exists()` helpers are the same ones already used for arithmetic variable resolution; no new symbol-table logic was introduced.

---

## 14. What Was Done (Step 5 Summary)

**Goal:** Augment all error-reporting sites in `lexer.c`, `parser.c`, and `exec.c` to include a line number and human-readable "expected …" phrasing, without introducing any new output channel.

### Files modified

| File | Change |
|------|--------|
| `include/parser.h` | Added `int line` field to `Condition` and `Transform` structs so the executor can reference the source line when reporting runtime errors. |
| `src/parser.c` | Added `int line` to the internal `Expr` struct; initialised it to `0` in `create_expr()`; set it from `tokens[current-1].line` in `parse_primary()` for both number and identifier tokens; stored the `whn`/`trn` keyword line into `cond->line` / `trn->line` at the start of those branches in `parse_pipeline()`. |
| `src/exec.c` | Added `int line` to the mirrored internal `Expr` struct; in `eval_expr()` replaced the bare `sym_get()` call for `EXPR_VARIABLE` with a prior `sym_exists()` check that emits `"Error: Undefined variable '<name>' at line <N>"` when the variable is absent; in `apply_transform()` updated the undefined-variable error to include `trn->line`. |

### Error-message improvements in detail

| Site | Before | After |
|------|--------|-------|
| `exec.c` – arithmetic undefined variable (`eval_expr`) | `Error: Undefined variable 'X'` | `Error: Undefined variable 'X' at line N` |
| `exec.c` – pipeline undefined variable (`apply_transform`) | `Error: Undefined variable 'X'` | `Error: Undefined variable 'X' at line N` |
| `lexer.c` – unexpected character | `Error: Unexpected character 'X' at line N` | *(unchanged — already had line number)* |
| `parser.c` – all parse errors | `Error: <message> at line N` | *(unchanged — already had line number via `error()` helper)* |

### Backward-compatibility verification

- All eight existing `.tri` test files were rebuilt and re-run after the change.
- All **stdout** outputs and **exit codes** are **byte-for-byte identical** to the v0.1.0 baseline.
- The only **stderr** change is in `tests/test_invalid.tri`: the `"Error: Undefined variable 'print'"` message now reads `"Error: Undefined variable 'print' at line 1"` — the baseline in this document has been updated accordingly.
- No existing token type, AST node type, or execution path was removed or altered.
- The `Condition.line` and `Transform.line` fields are zero-initialised; the `Expr.line` field is set to `0` in `create_expr()` and then overwritten to the real token line in `parse_primary()`, so there is no uninitialised-memory risk.
