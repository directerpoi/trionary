# v0.3.1 — Assignment Extension (IDENT / inpt RHS): Completion Notes

## What Was Done

Upgraded Trionary to **v0.3.1** by extending the assignment statement to accept three forms on the right-hand side:

| Form | Example | Meaning |
|------|---------|---------|
| `IDENT = NUMBER` | `a = 42` | Assign a numeric literal (existing behaviour, unchanged) |
| `IDENT = IDENT` | `a = arg0` | Assign the current value of another variable (e.g. a CLI argument) |
| `IDENT = inpt` | `a = inpt` | Read a numeric value from stdin via `scanf` |

---

## Syntax

```
a = arg0        # copy CLI argument into local variable
b = arg1
a + b -> emt    # → sum of the two CLI args

a = inpt        # prompt: reads one double from stdin
b = inpt
a + b -> emt    # → sum of the two interactively entered numbers
```

---

## Changes Made

### Modified Files

| File | Change |
|------|--------|
| `include/lexer.h` | Added `TOK_INPT` to `TokenType` |
| `src/lexer.c` | Added `inpt` to `is_keyword()` and `keyword_type()` |
| `include/parser.h` | Added `AssignRHSType` enum (`ASSIGN_NUMBER`, `ASSIGN_VARIABLE`, `ASSIGN_INPUT`); added `rhs_type` and `rhs_name` fields to `AssignNode` |
| `src/parser.c` | Updated `parse_assign()` to accept `TOK_NUMBER` (→ `ASSIGN_NUMBER`), `TOK_IDENT` (→ `ASSIGN_VARIABLE`), or `TOK_INPT` (→ `ASSIGN_INPUT`) after `=` |
| `src/exec.c` | Updated `exec_assign()` to resolve the three cases: use literal, look up symbol table, or call `scanf` |
| `src/main.c` | Extended statement-splitting condition to match `IDENT = IDENT` and `IDENT = inpt` in addition to the existing `IDENT = NUMBER` |
| `tests/run_tests.sh` | Extended test runner to support an optional `test_*.stdin` sidecar file for tests that require stdin input |

### New Files

| File | Purpose |
|------|---------|
| `tests/test_input_assign.tri` | Verifies `a = arg0` / `b = arg1` style assignment; run with `./tri run tests/test_input_assign.tri 10 20` → `30 / 10 / 20` |
| `tests/test_input_assign.args` | CLI args `10 20` for the test runner |
| `tests/test_input_assign.expected` | Expected output `30 / 10 / 20` |
| `tests/test_inpt.tri` | Verifies interactive `inpt` keyword; reads two values from stdin and emits their sum |
| `tests/test_inpt.stdin` | Stdin fixture: `10\n20` |
| `tests/test_inpt.expected` | Expected output `30` |

---

## Design Decisions

- **Minimal AST change.** `AssignNode` gains two fields (`rhs_type` + `rhs_name[64]`). The existing `value` field is kept for `ASSIGN_NUMBER`; no union was introduced to avoid changing the struct size dramatically. Memory layout remains simple.
- **`inpt` is a keyword, not a variable.** Treating `inpt` as a reserved keyword (rather than a special variable) ensures it cannot be accidentally shadowed and gives a clean, unambiguous parse path. The lexer emits `TOK_INPT`; the parser matches it in `parse_assign()` and sets `rhs_type = ASSIGN_INPUT`.
- **`scanf` for interactive input.** Input reading is intentionally numeric-only (reads a `double`). No buffering changes were made; the program blocks on stdin until the user supplies a value followed by a newline. On `scanf` failure (e.g. EOF or non-numeric input), `error_at()` is called with a descriptive message.
- **No expression-position `inpt`.** Per plan.md constraints, `inpt` is only valid as the RHS of an assignment, not inside arithmetic expressions. This keeps the change minimal and avoids ambiguity in `parse_primary()`.
- **No behaviour change for existing programs.** All 16 pre-existing tests continue to produce byte-for-byte identical output.
- **Test runner stdin support.** The runner now redirects from a `test_*.stdin` file when present (using `/dev/null` as the default), making it possible to test `inpt`-using programs without interactive intervention.

---

## Verification

```
$ make clean && make
$ ./tri run tests/test_input_assign.tri 10 20   # → 30 / 10 / 20
$ echo -e "10\n20" | ./tri run tests/test_inpt.tri  # → 30
$ make test
# Results: 18 passed, 0 failed
```

All 18 tests pass on a clean build. No pre-existing test output changed.

---

## Files Touched (summary)

```
include/lexer.h                  ← TOK_INPT added
src/lexer.c                      ← inpt keyword
include/parser.h                 ← AssignRHSType enum; rhs_type/rhs_name in AssignNode
src/parser.c                     ← parse_assign() handles NUMBER | IDENT | INPT
src/exec.c                       ← exec_assign() dispatches on rhs_type
src/main.c                       ← statement splitter accepts IDENT = IDENT | INPT
tests/run_tests.sh               ← .stdin sidecar support added
tests/test_input_assign.tri      ← new
tests/test_input_assign.args     ← new
tests/test_input_assign.expected ← new
tests/test_inpt.tri              ← new
tests/test_inpt.stdin            ← new
tests/test_inpt.expected         ← new
```

---

# Task 5 — Module System (Built-in Modules Only): Completion Notes

## What Was Done

Task 5 from `plan.md` has been implemented in full. The goal was to provide a `use <module>` directive that loads a curated set of built-in helpers compiled directly into the interpreter — no external dependencies, no file-system loading.

---

## Syntax

```
use math
use io
```

- The `use` keyword followed by a module name on the **same line** loads all functions from that module into the function table.
- After `use math`, all math functions (`floor`, `ceil`, `abs`, `sqrt`, `pow`) are callable anywhere a function call expression is valid.
- After `use io`, `print` and `read_line` are available.
- Loading the same module twice is idempotent (duplicate entries are silently skipped).

---

## Modules

### `math`

| Function | Params | Description |
|----------|--------|-------------|
| `floor`  | 1      | Round down to nearest integer |
| `ceil`   | 1      | Round up to nearest integer |
| `abs`    | 1      | Absolute value |
| `sqrt`   | 1      | Square root |
| `pow`    | 2      | `pow x y` → x raised to the power y |

### `io`

| Function    | Params | Description |
|-------------|--------|-------------|
| `print`     | 1      | Emits the argument to stdout and returns it |
| `read_line` | 1      | Reads a double from stdin; returns the argument as default on EOF/error |

> **Note on `print`:** When used as a standalone arithmetic statement (e.g. `print 42`), the value is emitted once by `print` and once by the implicit emit in `STMT_ARITH`, producing two output lines. Use `print` inside pipeline transforms or pass its result through a pipeline to avoid duplication.
>
> **Note on `read_line`:** The current expression parser requires at least one argument to identify a token sequence as a function call. `read_line` therefore takes a mandatory default value (`read_line 0`) that is returned when stdin is empty or at EOF.

---

## Changes Made

### Modified Files

| File | Change |
|------|--------|
| `include/lexer.h` | Added `TOK_USE` to `TokenType` |
| `src/lexer.c` | Added `use` to `is_keyword()` and `keyword_type()`; extended identifier scanning to support underscores and alphanumeric continuation characters (enabling `read_line`) |
| `include/parser.h` | Added `NODE_USE` to `NodeType`; added `UseStmtNode` struct; added `STMT_USE` to `ASTNode.stmt_type`; added `use_stmt` to the node union |
| `src/parser.c` | Added `parse_use_stmt()`; wired `TOK_USE` into `parse()`; added `STMT_USE` branch to `free_ast()` |
| `include/exec.h` | Added `BuiltinFn` typedef; added `is_builtin` and `builtin_fn` fields to `FuncDef`; declared `register_math_module()` and `register_io_module()` |
| `src/exec.c` | Updated `eval_expr()` EXPR_CALL branch to dispatch to `builtin_fn` when `is_builtin == 1`; updated `free_functable()` to skip freeing `body` of builtins; initialised `is_builtin = 0` / `builtin_fn = NULL` in `exec_fn_def()`; added `exec_use_stmt()`; added `STMT_USE` case in `execute()` |
| `src/main.c` | Added `TOK_USE` case to the statement-splitting loop (consumes `use` + module-name identifier) |
| `Makefile` | Added `src/modules/math.c` and `src/modules/io.c` to `SRCS` |

### New Files

| File | Purpose |
|------|---------|
| `src/modules/math.c` | Implements `floor`, `ceil`, `abs`, `sqrt`, `pow` and `register_math_module()` |
| `src/modules/io.c` | Implements `print`, `read_line` and `register_io_module()` |
| `tests/test_modules.tri` | Verifies `use math` and all five math built-ins |

---

## Design Decisions

- **Static module registry.** All built-in modules are compiled into the binary. No file-system I/O is required or performed during module loading. The `register_*_module()` functions simply iterate a compile-time array and append entries to the `FuncTable`.
- **`BuiltinFn` function pointer.** A single `double (*)(double*, int)` signature handles all math and io built-ins uniformly. The `is_builtin` flag in `FuncDef` selects between the C-function path and the body-expression path in `eval_expr`.
- **Idempotent registration.** Calling `use math` twice does not double-register functions; each registration loop checks for an existing entry by name and skips it.
- **Identifier underscore support.** The lexer was extended to allow underscores (and digits after the first character) in identifiers. This enables `read_line` as a valid token. Existing programs are unaffected as no previous identifiers contained underscores.
- **`read_line` default parameter.** Because the parser identifies function calls by an IDENT followed by ≥1 NUMBER/IDENT token(s), zero-argument calls are not distinguishable from variable references without coupling the parser to the function table. `read_line` therefore takes one argument used as the default value on EOF. This is a pragmatic workaround within the current single-pass architecture; a future parser-improvement task could lift this restriction.
- **`print` as an expression.** `print x` emits `x` to stdout as a side effect and returns `x`, making it usable inside pipeline transforms (e.g. `trn * print 2`). When used as a standalone arithmetic statement, the implicit STMT_ARITH emit produces a second line of output — this is expected and documented.
- **No behaviour change for v0.2 / v0.3 programs.** All existing tests produce byte-for-byte identical output.

---

## Verification

```
$ make clean && make
$ ./tri run tests/test_arith.tri              # → 15 / 14
$ ./tri run tests/test_vars.tri               # → 15 / 70
$ ./tri run tests/test_pipeline.tri           # → 120
$ ./tri run tests/test_all.tri                # all expected values
$ ./tri run tests/test_trn_expr.tri           # → 12 16 20 / 21 22 23 / 120
$ ./tri run tests/test_fn.tri                 # → 7 / 42 / 10 / 10 11 12
$ ./tri run tests/test_modules.tri            # → 3 / 4 / 5 / 3 / 8 / 14
# Error cases — exit code 1 with correct message:
$ ./tri run tests/test_invalid.tri            # Error: Unexpected character '"' at line 1
$ ./tri run tests/test_trn_undef.tri          # Error: Undefined variable 'undef' at line 2
```

All checks pass. No pre-existing test output changed.

---

## Files Touched (summary)

```
include/lexer.h            ← TOK_USE added
src/lexer.c                ← use keyword; underscore/alphanumeric identifiers
include/parser.h           ← NODE_USE; UseStmtNode; STMT_USE; use_stmt union member
src/parser.c               ← parse_use_stmt(); TOK_USE in parse(); STMT_USE in free_ast()
include/exec.h             ← BuiltinFn; is_builtin/builtin_fn in FuncDef; register_*_module() decls
src/exec.c                 ← builtin dispatch in eval_expr; free_functable fix; exec_use_stmt(); STMT_USE in execute()
src/main.c                 ← TOK_USE case in statement-splitting loop
Makefile                   ← src/modules/math.c and src/modules/io.c added
src/modules/math.c         ← new
src/modules/io.c           ← new
tests/test_modules.tri     ← new
```

---



## What Was Done

Task 4 from `plan.md` has been implemented in full. The goal was to introduce named, pure functions with positional parameters and a single-expression body, callable inside arithmetic expressions and pipeline stages.

---

## Syntax

```
fn add x y
  x + y
end
```

- `fn` opens the definition; the function name and zero or more parameter names follow on the **same line**.
- A **newline** separates the parameter list from the body expression.
- The **body** is a single arithmetic expression (may reference parameters and any previously-defined variables).
- `end` closes the definition.
- Functions are called as `name arg1 arg2 …` wherever a primary expression is expected.

---

## Changes Made

### Modified Files

| File | Change |
|------|--------|
| `include/lexer.h` | Added `TOK_FN`, `TOK_END`, and `TOK_NEWLINE` to `TokenType` |
| `src/lexer.c` | Added `fn` / `end` to `is_keyword()` and `keyword_type()`; moved `\n` handling out of `skip_whitespace_and_comments` so that `TOK_NEWLINE` tokens are emitted (used as the param/body separator inside `fn` definitions) |
| `include/parser.h` | Added `MAX_PARAMS 8`; added `EXPR_CALL` to `ExprType`; added `args[MAX_PARAMS]` / `arg_count` fields to `Expr`; added `NODE_FN_DEF` to `NodeType`; added `FnDefNode` struct; added `STMT_FN_DEF` to `ASTNode` stmt_type and `fn_def` to the node union; declared `free_expr()` as public |
| `src/parser.c` | Made `free_expr()` non-static and updated it to recurse into `EXPR_CALL` args; extended `parse_primary()` to detect and parse function calls (`EXPR_CALL`) when an `IDENT` is followed by one or more `NUMBER`/`IDENT` arguments; added `parse_fn_def()`; updated `parse()` to handle `TOK_FN`; updated `free_ast()` to handle `STMT_FN_DEF` |
| `include/exec.h` | Added `MAX_FUNCS 64`; added `FuncDef` and `FuncTable` structs; added `create_functable()` / `free_functable()` declarations; updated `execute()` signature to include `FuncTable*` |
| `src/exec.c` | Added `clone_expr()` helper; added `create_functable()` and `free_functable()`; updated `eval_expr()` to take `FuncTable*` and handle `EXPR_CALL` (push scope, bind params, eval body, pop scope); updated `apply_transform()`, `exec_pipeline()`, and `execute()` to thread `FuncTable*` through; added `exec_fn_def()` to register a function (cloning its body expression) |
| `src/main.c` | Created / destroyed `FuncTable`; added `fn … end` block scanning to the statement-splitting loop; added leading-newline skipping; strip `TOK_NEWLINE` from non-`fn` statement slices before passing to `parse()` so existing parsers are unaffected; passed `ft` to `execute()` |

### New Files

| File | Purpose |
|------|---------|
| `tests/test_fn.tri` | Verifies function declarations and calls: two-parameter add, two-parameter mul, single-parameter inc, and a function call inside a pipeline transform |

---

## Design Decisions

- **`TOK_NEWLINE` as separator.** The language reuses the lexer's line-break token to cleanly separate the parameter list (same line as `fn`) from the body expression (next line). All other statement parsers receive a NEWLINE-stripped token slice, preserving backward compatibility.
- **Function call detection in `parse_primary()`.** A plain `IDENT` (not `TOK_VAR_REF`) followed immediately by one or more `NUMBER`/`IDENT` tokens is treated as a function call. This is unambiguous in the existing grammar because no existing construct places adjacent IDENT/NUMBER tokens without an intervening operator.
- **Separate function table.** `FuncTable` is independent of `SymTable`; functions and variables are in distinct namespaces. The table lives for the lifetime of the program and is created/freed in `main.c`.
- **Cloned body expression.** When a function is registered, its body `Expr*` is deep-cloned into the `FuncTable`. The original AST node is freed as normal, so the function table owns its copy independently.
- **Scoped execution.** At call time, all argument expressions are evaluated in the *caller's* scope first; then a new scope is pushed, parameters are bound, the body is evaluated, and the scope is popped. This ensures arguments reference the correct outer variables.
- **No behaviour change for v0.2 programs.** All v0.2 tests continue to produce byte-for-byte identical output.

---

## Verification

```
$ make clean && make
$ ./tri run tests/test_arith.tri              # → 15 / 14
$ ./tri run tests/test_vars.tri               # → 15 / 70
$ ./tri run tests/test_pipeline.tri           # → 120
$ ./tri run tests/test_all.tri                # all expected values
$ ./tri run tests/test_trn_expr.tri           # → 12 16 20 / 21 22 23 / 120
$ ./tri run tests/test_fn.tri                 # → 7 / 42 / 10 / 10 11 12
# Error cases — exit code 1 with correct message:
$ ./tri run tests/test_invalid.tri            # Error: Unexpected character '"' at line 1
$ ./tri run tests/test_trn_undef.tri          # Error: Undefined variable 'undef' at line 2
```

All checks pass. No pre-existing test output changed.

---

## Files Touched (summary)

```
include/lexer.h        ← TOK_FN, TOK_END, TOK_NEWLINE added
src/lexer.c            ← fn/end keywords; TOK_NEWLINE emission
include/parser.h       ← MAX_PARAMS; EXPR_CALL; FnDefNode; STMT_FN_DEF; free_expr()
src/parser.c           ← free_expr() public; EXPR_CALL in parse_primary(); parse_fn_def(); fn in parse(); STMT_FN_DEF in free_ast()
include/exec.h         ← FuncDef; FuncTable; create/free_functable(); execute() updated
src/exec.c             ← clone_expr(); FuncTable ops; EXPR_CALL in eval_expr(); exec_fn_def(); ft threaded through
src/main.c             ← FuncTable lifecycle; fn…end scan; NEWLINE stripping; ft passed to execute()
tests/test_fn.tri      ← new
```

---

# Task 3 — Expression Improvements in `trn`: Completion Notes

## What Was Done

Task 3 from `plan.md` has been implemented in full. The goal was to allow full arithmetic expressions on the right-hand side of the `trn` pipeline stage, going beyond the old `op NUMBER` / `op VAR_REF` restriction.

---

## Convention

The **pipeline element is the implicit left operand** of the leading operator. The right-hand side of the operator may be any arithmetic expression built from numeric literals and previously-assigned variables.

Examples:
- `trn * 10`          → `element * 10`
- `trn * x`           → `element * x`
- `trn * x + 1`       → `element * (x + 1)` *(right side evaluated first as a full expression)*
- `trn + a * 2`       → `element + (a * 2)`

There is no special `_` placeholder; the element is always implicitly the left operand of the leading operator and the right-hand expression is evaluated independently.

---

## Changes Made

### Modified Files

| File | Change |
|------|--------|
| `include/parser.h` | Moved `ExprType` enum and `Expr` struct definition here (was private in `parser.c` and `exec.c`); replaced `Transform.value / is_var_ref / var_name` fields with a single `Expr* expr` pointer |
| `src/parser.c` | Removed local `ExprType`/`Expr` definitions (now from `parser.h`); updated `parse_primary()` to accept `TOK_VAR_REF` as a variable identifier; replaced the `if (TOK_VAR_REF) … else if (TOK_NUMBER)` trn parsing block with a single `parse_expr()` call; updated `free_ast()` to call `free_expr()` on each transform's expression |
| `src/exec.c` | Removed local `ExprType`/`Expr` definitions (now from `parser.h`); replaced the `is_var_ref` branch in `apply_transform()` with a single `eval_expr(trn->expr, sym)` call |

### New Files

| File | Purpose |
|------|---------|
| `tests/test_trn_expr.tri` | Verifies expression RHS in `trn`: `trn * x + 1`, `trn + a * 2`, and backward-compatible `trn * 10` |

---

## Design Decisions

- **Backward compatibility preserved.** The old `trn * 10` and `trn * a` (compact no-space) syntax continues to work unchanged; they are now parsed as trivial single-node expressions (`EXPR_NUMBER` and `EXPR_VARIABLE` respectively).
- **`Expr` moved to `parser.h`.** This was the minimal change needed to share the type between `parser.c`, `exec.c`, and the new `Transform.expr` field. The `ArithNode*`/`Expr*` cast used by the arithmetic-emit statement is unchanged.
- **`parse_primary()` now accepts `TOK_VAR_REF`.** This token type is emitted by the lexer when an alphabetic identifier immediately follows an operator (e.g. `*x`). Making the expression parser understand it ensures compact and spaced forms are interchangeable.
- **No `_` placeholder.** The leading operator already encodes the application of the pipeline element; a `_` binding would be redundant and would require a symbol-table side-effect during pipeline execution.

---

## Verification

```
$ make clean && make
$ ./tri run tests/test_arith.tri              # → 15 / 14
$ ./tri run tests/test_vars.tri               # → 15 / 70
$ ./tri run tests/test_pipeline.tri           # → 120
$ ./tri run tests/test_all.tri                # all expected values
$ ./tri run tests/test_trn_var.tri            # → 120
$ ./tri run tests/test_multi_pipeline.tri     # → 11 12 13 / 8 10 12
$ ./tri run tests/test_mixed_arith_pipeline.tri
$ ./tri run tests/test_trn_expr.tri           # → 12 16 20 / 21 22 23 / 120
# Error cases — exit code 1 with correct message:
$ ./tri run tests/test_invalid.tri
$ ./tri run tests/test_malformed_trn.tri
$ ./tri run tests/test_trn_undef.tri
```

All checks pass. No pre-existing test output changed.

---

## Files Touched (summary)

```
include/parser.h          ← added ExprType/Expr; Transform.expr replaces old fields
src/parser.c              ← removed local Expr; TOK_VAR_REF in parse_primary; parse_expr() for trn
src/exec.c                ← removed local Expr; apply_transform uses eval_expr
tests/test_trn_expr.tri   ← new
```

---

# Task 2 — Symbol Table Hardening: Completion Notes

## What Was Done

Task 2 from `plan.md` has been implemented in full. The goal was to replace the flat 256-slot variable array with a hash-map-based scope stack, and expose a `scope_push()` / `scope_pop()` API to support upcoming function scopes.

### Modified Files

| File | Change |
|------|--------|
| `include/exec.h` | Removed old `Symbol` / flat-array `SymTable`; added `ScopeEntry`, `Scope` (open-addressing hash table with parent pointer), new `SymTable` (wrapper holding `Scope *current`); added `scope_push()` and `scope_pop()` declarations |
| `src/exec.c` | Replaced linear-scan symtable with djb2 hash + linear-probing open-addressing hash table; `sym_set`, `sym_exists`, `sym_get` now walk the scope chain; added `scope_push`, `scope_pop`, and internal helpers `scope_hash`, `scope_lookup`, `scope_reserve` |

---

## Design Decisions

- **Open addressing, power-of-2 capacity.** `SCOPE_CAPACITY = 128` slots per scope level, djb2 hash folded via `& (SCOPE_CAPACITY - 1)`, linear probing for collisions.  Deletion is never needed (no variable removal in the language), so no tombstones are required.
- **Scope chain (`parent` pointer).** Each `Scope` carries a `parent` pointer so `sym_exists` and `sym_get` walk from the innermost scope outward; `sym_set` always writes into the current (innermost) scope, giving isolated function-local variables when `scope_push` is used.
- **Global scope is never popped.** `scope_pop` is a no-op when the current scope has no parent, preventing accidental destruction of the global scope.
- **No behaviour change for v0.2 programs.** All programs run with a single scope (the global one); the hash-map lookup is semantically identical to the old linear scan for any program that never calls `scope_push`.
- **Full memory cleanup.** `free_symtable` walks the scope chain and frees every level; `scope_pop` frees the popped level immediately.
- **Error on full table.** If all 128 slots in a scope level are occupied, `error_at()` is called with a descriptive message rather than silently dropping the variable (improvement over the old silent drop at `count >= 256`).

---

## Verification

```
$ make clean && make
$ ./tri run tests/test_arith.tri              # → 15 / 14
$ ./tri run tests/test_vars.tri               # → 15 / 70
$ ./tri run tests/test_pipeline.tri           # → 120
$ ./tri run tests/test_all.tri                # all expected values
$ ./tri run tests/test_trn_var.tri            # → 120
$ ./tri run tests/test_multi_pipeline.tri     # → 11 12 13 / 8 10 12
$ ./tri run tests/test_mixed_arith_pipeline.tri
# Error cases — exit code 1 with correct message:
$ ./tri run tests/test_invalid.tri            # Error: Unexpected character '"' at line 1
$ ./tri run tests/test_malformed_trn.tri      # Error: Expected number or variable …
$ ./tri run tests/test_trn_undef.tri          # Error: Undefined variable 'undef' at line 2
```

All checks pass. No callers outside `exec.c` required any modification.

---

## Files Touched (summary)

```
include/exec.h   ← replaced Symbol/flat SymTable with ScopeEntry/Scope/SymTable; added scope_push/scope_pop
src/exec.c       ← replaced linear-scan implementation with hash-map + scope-chain implementation
```

---

# Task 1 — Error System Upgrade: Completion Notes

## What Was Done

Task 1 from `plan.md` has been implemented in full. The goal was to improve runtime and parse-time errors with line numbers, meaningful messages, and fail-fast behaviour.

---

## Changes Made

### New Files

| File | Purpose |
|------|---------|
| `include/error.h` | Declares the `error_at(int line, const char *fmt, ...)` helper |
| `src/error.c` | Implements `error_at`: prints `Error: <message> at line <N>` to stderr then calls `exit(1)` |

### Modified Files

| File | Change |
|------|--------|
| `Makefile` | Added `src/error.c` to the `SRCS` list so it is compiled and linked |
| `include/parser.h` | Added `int line` field to `ArithNode`, `AssignNode`, and `PipelineNode` |
| `src/lexer.c` | Replaced bare `fprintf(stderr, ...)` for unknown characters with `error_at()` |
| `src/parser.c` | Removed the local `error()` helper and `error_occurred` flag; replaced every `error(msg, line)` call with `error_at(line, msg)`; propagated line numbers into `AssignNode` and `PipelineNode`; updated `parse_pipeline()` signature to accept `lst_line` |
| `src/exec.c` | Replaced every `fprintf(stderr, ...)` and bare `exit(1)` with `error_at()` calls that carry the correct source line number |

---

## Design Decisions

- **Single exit point for errors.** All error output now goes through `error_at()`, ensuring the message format `Error: <message> at line <N>` is consistent everywhere.
- **Fail-fast.** `error_at()` calls `exit(1)` immediately, so the interpreter never continues executing after the first error. The now-redundant `error_occurred` flag and the `if (error_occurred) return NULL` block in `parse()` were removed.
- **No behaviour change for valid programs.** Every existing test (`test_arith`, `test_vars`, `test_pipeline`, `test_all`, `test_trn_var`, `test_multi_pipeline`, `test_mixed_arith_pipeline`) produces byte-for-byte identical stdout. Error test cases (`test_invalid`, `test_malformed_trn`, `test_trn_undef`) still exit with code 1 and print the same error format.
- **`sym_get` fallback.** The `sym_get` path that was only reachable through a defensive code path now calls `error_at(0, ...)` (line 0 indicates "unknown") instead of a silent `fprintf`. In practice this path remains unreachable because all callers first call `sym_exists`.

---

## Verification

```
$ make clean && make
$ ./tri run tests/test_arith.tri    # → 15 / 14
$ ./tri run tests/test_vars.tri     # → 15 / 70
$ ./tri run tests/test_pipeline.tri # → 120
$ ./tri run tests/test_all.tri      # all expected values
$ ./tri run tests/test_trn_var.tri  # → 120
$ ./tri run tests/test_multi_pipeline.tri
$ ./tri run tests/test_mixed_arith_pipeline.tri
# Error cases — exit code 1 with correct message:
$ ./tri run tests/test_invalid.tri  # Error: Unexpected character '"' at line 1
$ ./tri run tests/test_malformed_trn.tri
$ ./tri run tests/test_trn_undef.tri
```

All checks pass.

---

## Files Touched (summary)

```
include/error.h         ← new
src/error.c             ← new
include/parser.h        ← added line fields to AST nodes
src/lexer.c             ← error_at() replaces fprintf
src/parser.c            ← error_at() replaces local error(); line propagation
src/exec.c              ← error_at() replaces fprintf + exit(1)
Makefile                ← src/error.c added to SRCS
```

---

# Task 6 — CLI Input Improvements: Completion Notes

## What Was Done

Task 6 from `plan.md` has been implemented in full. The goal was to make `arg0`, `arg1`, … more ergonomic and add basic type coercion for CLI arguments passed to the interpreter.

---

## Syntax

```bash
tri run <file.tri> [arg0 arg1 ...]
```

Inside `<file.tri>`:

```tri
argc -> emt          # count of script arguments
arg0 -> emt          # first argument (auto-coerced to double)
arg1 + 5 -> emt      # arithmetic with CLI argument
arg2 ?? 99 -> emt    # fallback if arg2 was not provided
```

---

## Features

### 1 — Auto-coercion of CLI arguments to numbers

Arguments are registered in the global symbol table as `double` values via `atof()`. They participate in any arithmetic expression without any additional cast or conversion.

### 2 — `argc` built-in variable

`argc` is always defined in the global scope and holds the count of script arguments (i.e. `argv` entries after `<file.tri>`). When no extra arguments are provided, `argc` equals `0`.

### 3 — `??` default-value operator

`left ?? right` evaluates to the value of `left` if `left` is a defined variable, and to `right` otherwise. This is the primary way to handle optional CLI arguments:

```tri
arg0 ?? 1 -> emt    # arg0 if provided, else 1
arg1 ?? 0 -> emt    # arg1 if provided, else 0
```

The operator is right-associative and has lower precedence than all arithmetic operators.

---

## Changes Made

### Modified Files

| File | Change |
|------|--------|
| `include/lexer.h` | Added `TOK_COALESCE` to `TokenType` |
| `src/lexer.c` | Added `??` two-character token scanning, emitting `TOK_COALESCE` |
| `include/parser.h` | Added `EXPR_COALESCE` to `ExprType` |
| `src/parser.c` | Added `parse_coalesce()` (right-associative, wraps `parse_expr()`); updated `parse()`, `parse_fn_def()`, and `parse_pipeline()` trn branch to call `parse_coalesce()` as the top-level expression entry point; added `EXPR_COALESCE` case to `free_expr()` |
| `src/exec.c` | Added `EXPR_COALESCE` case to `clone_expr()` and `eval_expr()` — if the left operand is an undefined variable, the right fallback is returned without raising an error |
| `src/main.c` | Relaxed `argc != 3` to `argc < 3` to allow extra script arguments; registers `argc` (count) and `arg0`…`argN` (values via `atof()`) in the global symbol table before executing any statements |
| `README.md` | Updated Usage section; added CLI Input and `??` subsections to Language Reference; added v0.3.0 changelog entry |

### New Files

| File | Purpose |
|------|---------|
| `tests/test_cli_args.tri` | Verifies `argc`, `arg0`, `arg1`, arithmetic with args, and `??` fallback |

---

## Design Decisions

- **Symbol-table registration.** `arg0`…`argN` are only registered for the arguments actually supplied. If only 2 extra args are given, `arg2` is absent from the symbol table, so `arg2 ?? 99` returns `99` — exactly the desired behaviour.
- **`argc` always present.** Unlike `arg0`…`argN`, `argc` is unconditionally registered (value `0` when no extras are given). This makes `argc` safe to use without `??`.
- **`??` is expression-level, not statement-level.** The operator is parsed as part of the expression grammar, so it works anywhere an expression is valid: standalone emit, arithmetic sub-expressions, `trn` bodies, and function bodies.
- **No behaviour change for v0.2 / v0.3 programs.** All existing tests produce byte-for-byte identical output. The only reserved names added are `argc` and `arg0`…`argN` (when extra CLI args are supplied).

---

## Verification

```
$ make clean && make
$ ./tri run tests/test_arith.tri              # → 15 / 14
$ ./tri run tests/test_vars.tri               # → 15 / 70
$ ./tri run tests/test_pipeline.tri           # → 120
$ ./tri run tests/test_all.tri                # all expected values
$ ./tri run tests/test_trn_expr.tri           # → 12 16 20 / 21 22 23 / 120
$ ./tri run tests/test_fn.tri                 # → 7 / 42 / 10 / 10 11 12
$ ./tri run tests/test_modules.tri            # → 3 / 4 / 5 / 3 / 8 / 14
$ ./tri run tests/test_cli_args.tri 7 3       # → 2 / 7 / 3 / 10 / 14 / 99 / 7
# Error cases — exit code 1 with correct message:
$ ./tri run tests/test_invalid.tri            # Error: Unexpected character '"' at line 1
$ ./tri run tests/test_trn_undef.tri          # Error: Undefined variable 'undef' at line 2
```

All checks pass. No pre-existing test output changed.

---

## Files Touched (summary)

```
include/lexer.h            ← TOK_COALESCE added
src/lexer.c                ← ?? token lexing
include/parser.h           ← EXPR_COALESCE added
src/parser.c               ← parse_coalesce(); EXPR_COALESCE in free_expr(); top-level calls updated
src/exec.c                 ← EXPR_COALESCE in clone_expr() and eval_expr()
src/main.c                 ← argc check relaxed; arg0..argN and argc registered
README.md                  ← Usage, Language Reference, and Changelog updated
tests/test_cli_args.tri    ← new
```

---

# Task 7 — Regression Test Suite & CI Script: Completion Notes

## What Was Done

Task 7 from `plan.md` has been implemented in full. The goal was to create an automated regression test runner that verifies all v0.2 programs and v0.3 features produce the expected output, and to wire it into the build system via `make test`.

---

## How the Test Runner Works

```
make test
```

The `make test` target builds `tri` (if needed) then invokes `bash tests/run_tests.sh`.

`run_tests.sh` iterates over every `tests/test_*.tri` file and:
1. Skips the file if no matching `tests/test_*.expected` file exists.
2. Reads an optional `tests/test_*.args` file (one line, space-separated) and passes those tokens as extra CLI arguments to `./tri run`.
3. Runs `./tri run <file> [extra-args]`, capturing **combined stdout + stderr** into a temp file.
4. Diffs the temp file against the `.expected` file; prints `PASS` or `FAIL` accordingly.
5. Reports a summary (`N passed, M failed`) and exits non-zero if any test failed.

---

## Changes Made

### Modified Files

| File | Change |
|------|--------|
| `Makefile` | Added `test` target: `bash tests/run_tests.sh` (depends on `$(TARGET)`) |

### New Files

| File | Purpose |
|------|---------|
| `tests/run_tests.sh` | Test runner script (executable); iterates `test_*.tri`, diffs against `.expected`, reports PASS/FAIL |
| `tests/test_all.expected` | Expected output for `test_all.tri` |
| `tests/test_arith.expected` | Expected output for `test_arith.tri` |
| `tests/test_cli_args.args` | Extra CLI args (`7 3`) to pass when running `test_cli_args.tri` |
| `tests/test_cli_args.expected` | Expected output for `test_cli_args.tri` (run with `7 3`) |
| `tests/test_error.expected` | Expected output for `test_error.tri` |
| `tests/test_fn.expected` | Expected output for `test_fn.tri` |
| `tests/test_invalid.expected` | Expected output (error message) for `test_invalid.tri` |
| `tests/test_malformed.expected` | Expected output for `test_malformed.tri` |
| `tests/test_malformed_trn.expected` | Expected output (error message) for `test_malformed_trn.tri` |
| `tests/test_mixed_arith_pipeline.expected` | Expected output for `test_mixed_arith_pipeline.tri` |
| `tests/test_modules.expected` | Expected output for `test_modules.tri` |
| `tests/test_multi_pipeline.expected` | Expected output for `test_multi_pipeline.tri` |
| `tests/test_pipeline.expected` | Expected output for `test_pipeline.tri` |
| `tests/test_trn_expr.expected` | Expected output for `test_trn_expr.tri` |
| `tests/test_trn_undef.expected` | Expected output (error message) for `test_trn_undef.tri` |
| `tests/test_trn_var.expected` | Expected output for `test_trn_var.tri` |
| `tests/test_vars.expected` | Expected output for `test_vars.tri` |

---

## Design Decisions

- **Combined stdout + stderr capture.** Error-case tests emit their diagnostic to stderr; merging both streams into the `.expected` file means both the error message and any partial output are verified in one diff.
- **`.args` sidecar file.** Tests that require extra CLI arguments (e.g. `test_cli_args.tri`) carry a `test_*.args` file with a single space-separated line. The runner reads it with `read -ra` so each token becomes a separate argument, preserving correct quoting semantics.
- **Skip instead of fail.** Tests without a `.expected` file are skipped (not failed). This makes it safe to add new `.tri` files before their expected output is known.
- **No behaviour change.** No source files were modified; the test infrastructure is purely additive.

---

## Verification

```
$ make clean && make
$ make test
PASS: test_all.tri
PASS: test_arith.tri
PASS: test_cli_args.tri
PASS: test_error.tri
PASS: test_fn.tri
PASS: test_invalid.tri
PASS: test_malformed.tri
PASS: test_malformed_trn.tri
PASS: test_mixed_arith_pipeline.tri
PASS: test_modules.tri
PASS: test_multi_pipeline.tri
PASS: test_pipeline.tri
PASS: test_trn_expr.tri
PASS: test_trn_undef.tri
PASS: test_trn_var.tri
PASS: test_vars.tri

Results: 16 passed, 0 failed
```

All 16 tests pass on a clean build. No pre-existing test output changed.

---

## Files Touched (summary)

```
Makefile                              ← test target added
tests/run_tests.sh                    ← new (executable)
tests/test_all.expected               ← new
tests/test_arith.expected             ← new
tests/test_cli_args.args              ← new
tests/test_cli_args.expected          ← new
tests/test_error.expected             ← new
tests/test_fn.expected                ← new
tests/test_invalid.expected           ← new
tests/test_malformed.expected         ← new
tests/test_malformed_trn.expected     ← new
tests/test_mixed_arith_pipeline.expected ← new
tests/test_modules.expected           ← new
tests/test_multi_pipeline.expected    ← new
tests/test_pipeline.expected          ← new
tests/test_trn_expr.expected          ← new
tests/test_trn_undef.expected         ← new
tests/test_trn_var.expected           ← new
tests/test_vars.expected              ← new
```

---

# Task 8 — Documentation & Release Prep: Completion Notes

## What Was Done

Task 8 from `plan.md` has been implemented in full. The goal was to update all documentation to reflect v0.3.0 and prepare the release.

---

## Changes Made

### Modified Files

| File | Change |
|------|--------|
| `README.md` | Updated Features section (5 → 8 keywords); added `fn`/`end`/`use` to Keywords table and keyword subsections; updated `trn` description to mention full arithmetic expression RHS; added Functions and Modules sections (with syntax, parameter tables, usage notes); added CLI Input default-value documentation; added Named functions, Modules, and CLI arguments examples; updated Project Structure to include all new files and directories; updated v0.3.0 Changelog entry to cover all seven sub-tasks |
| `IMPLEMENTATION_SUMMARY.md` | Rewrote to reflect v0.3.0: updated project structure (all new files), keyword count (8), AST node type table (both statement and expression nodes), language feature examples, implementation detail sections (lexer, parser, exec, error system), test results table, build commands, and design principles |
| `src/main.c` | Bumped version string from `v0` to `v0.3.0` in `print_usage()` |

### New Files

| File | Purpose |
|------|---------|
| `CHANGELOG.md` | Full changelog with sections for v0.1.0, v0.2.0, and v0.3.0 |

---

## Design Decisions

- **Single source of truth for the changelog.** `CHANGELOG.md` is the canonical version history. The `README.md` Changelog section is a concise summary of the same information, kept in sync.
- **IMPLEMENTATION_SUMMARY.md rewritten, not appended.** The original document described only v0 (5 keywords, 3 statement types). A full rewrite was cleaner and more accurate than layering addenda onto a stale description.
- **Version string in `main.c`.** The only version reference in source code is the usage message in `print_usage()`. It now reads `v0.3.0` to match the release.
- **No behaviour change.** No logic was altered. All 16 automated tests continue to pass (`make test`: 16 passed, 0 failed).

---

## Verification

```
$ make clean && make
$ make test
PASS: test_all.tri
PASS: test_arith.tri
PASS: test_cli_args.tri
PASS: test_error.tri
PASS: test_fn.tri
PASS: test_invalid.tri
PASS: test_malformed.tri
PASS: test_malformed_trn.tri
PASS: test_mixed_arith_pipeline.tri
PASS: test_modules.tri
PASS: test_multi_pipeline.tri
PASS: test_pipeline.tri
PASS: test_trn_expr.tri
PASS: test_trn_undef.tri
PASS: test_trn_var.tri
PASS: test_vars.tri

Results: 16 passed, 0 failed
```

All 16 tests pass. No pre-existing test output changed.

---

## Files Touched (summary)

```
README.md                  ← features, keywords, functions section, modules section, project structure, examples, changelog
IMPLEMENTATION_SUMMARY.md  ← full rewrite for v0.3.0
src/main.c                 ← version string v0 → v0.3.0
CHANGELOG.md               ← new
```

---

# v0.3.2 — Step 1 & Step 2: `tri version` and `tri help` Commands: Completion Notes

## What Was Done

Implemented **Step 1** (U3 — `tri version`) and **Step 2** (U2 — `tri help`) from `plan.md`.

---

## Step 1 — `tri version` (U3)

Added a `version` subcommand that prints `Trionary v0.3.2` to stdout and exits with code 0.

```
$ ./tri version
Trionary v0.3.2
```

The version string is stored in a `TRI_VERSION` macro at the top of `src/main.c` so it is easy to update in one place. The version label was bumped from `v0.3.1` to `v0.3.2`.

---

## Step 2 — `tri help` (U2)

Added a `help` subcommand and support for the `--help` / `-h` flags. Any of the following invocations print the usage summary and exit with code 0:

```
$ ./tri help
$ ./tri --help
$ ./tri -h
$ ./tri          # no arguments also shows help
```

The help output lists all available subcommands and a syntax reminder:

```
Usage: ./tri <command> [arguments]

Commands:
  run <file.tri> [arg0 arg1 ...]   Execute a Trionary source file
  help                             Show this help message
  version                          Print the interpreter version

Examples:
  ./tri run script.tri
  ./tri run script.tri 10 20
```

The old `print_usage()` (which wrote to `stderr` and was only shown on error) has been replaced by `print_help()`, which writes to `stdout` for `help`/`--help`/`-h` invocations. Error paths still write their own messages to `stderr`.

---

## Changes Made

### Modified Files

| File | Change |
|------|--------|
| `src/main.c` | Removed `print_usage()`; added `TRI_VERSION` macro (`"Trionary v0.3.2"`); added `print_help()` function; added `version` and `help` (+ `--help` / `-h`) subcommand branches before the existing `run` logic; updated the missing-file-argument error for `run` to be more specific |

---

## Design Decisions

- **`TRI_VERSION` macro.** Centralises the version string so every future bump is a single-line change.
- **`help` exits 0.** Following Unix convention: `--help` and `help` always exit with code 0.
- **No-argument invocation shows help.** When `tri` is run with no arguments it now shows the help message (exit 0) instead of returning an error. This is friendlier for new users.
- **`print_help` writes to stdout.** Error messages still go to `stderr`; help goes to `stdout`. This matches standard CLI conventions and allows `tri help | less`.
- **No changes outside `src/main.c`.** All other source files, headers, and the Makefile are untouched.

---

## Verification

```
$ make clean && make
$ ./tri version              # → Trionary v0.3.2
$ ./tri help                 # → usage summary (exit 0)
$ ./tri --help               # → usage summary (exit 0)
$ ./tri -h                   # → usage summary (exit 0)
$ ./tri                      # → usage summary (exit 0)
$ make test
# Results: 19 passed, 0 failed
```

All 19 tests pass. No pre-existing test output changed.

---

## Files Touched (summary)

```
src/main.c   ← TRI_VERSION macro; print_help(); version + help subcommands; no-arg → help
```
