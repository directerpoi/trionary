# Trionary v0.2.0 — 10-Step Implementation Plan

> Design Principle: **Stability > Features | Clarity > Flexibility | Execution > Abstraction**

---

## Step 1 — Audit v0.1.0 baseline

Review every source file (`lexer.c`, `parser.c`, `exec.c`, `reader.c`, `output.c`, `main.c`) and all existing `.tri` test files to document the exact current grammar, token set, symbol-table layout, and pipeline execution model. This snapshot becomes the regression baseline that no v0.2.0 change may violate.

---

## Step 2 — Extend the lexer for variable references inside transforms

In `lexer.c` / `lexer.h`, add recognition of variable-reference tokens inside pipeline transform positions (e.g. `*a`, `+b`). The lexer must emit a dedicated `TOKEN_VAR_REF` (or reuse the existing identifier token with a flag) so the parser can distinguish a literal number from a variable reference without ambiguity. No existing token type is removed or renamed.

---

## Step 3 — Update the parser to resolve variable references in `trn`

In `parser.c` / `parser.h`, extend the `trn` rule to accept a variable reference in addition to a literal operand. When a `TOKEN_VAR_REF` is seen, the parser stores the variable name (not the value) in the AST/IR node so that runtime resolution happens at execution time. All other pipeline keywords (`lst`, `whn`, `sum`, `emt`) remain syntactically unchanged.

---

## Step 4 — Update the executor to look up variables at `trn` execution time

In `exec.c` / `exec.h`, modify the `trn` handler to check whether its operand is a variable reference. If so, perform a symbol-table lookup and abort with a clear error if the variable is undefined. Arithmetic and non-pipeline paths in the executor are untouched, preserving v0.1.0 behavior exactly.

---

## Step 5 — Improve error messages across lexer, parser, and executor

Augment all error-reporting sites in `lexer.c`, `parser.c`, and `exec.c` to include:
- **Line number** (track a `line_num` counter through the single-pass reader)
- **Expected token or construct** (e.g. `expected number or variable after trn`)
- Human-readable phrasing consistent with existing message style

No new output channel is introduced; errors still go to `stderr` via `output.c`.

---

## Step 6 — Support multiple pipelines per file

In `parser.c` and `exec.c`, replace the assumption that a file contains exactly one pipeline with a loop that resets pipeline state after each `emt` keyword and immediately starts accepting a new `lst` (or end-of-file). The symbol table is **not** reset between pipelines so variables set before the first pipeline remain visible in subsequent ones, matching natural scripting expectations.

---

## Step 7 — Write new `.tri` test files

Add the following test files to `tests/`:

| File | Purpose |
|------|---------|
| `test_trn_var.tri` | `trn` with a valid variable reference (`*a`) |
| `test_trn_undef.tri` | `trn` with an undefined variable — must produce a clear error |
| `test_multi_pipeline.tri` | Two complete `lst … emt` pipelines in one file |
| `test_malformed_trn.tri` | Malformed `trn` operand — must produce a line-numbered error |
| `test_mixed_arith_pipeline.tri` | Arithmetic variable assignment followed by a pipeline using that variable |

---

## Step 8 — Run the full regression suite

Execute `tri run` against every existing test in `tests/` (`test_all.tri`, `test_arith.tri`, `test_pipeline.tri`, `test_vars.tri`, `test_error.tri`, `test_invalid.tri`, `test_malformed.tri`, `demo.tri`) and confirm that all outputs are byte-for-byte identical to v0.1.0 results. Any deviation is a blocker before proceeding.

---

## Step 9 — Run the new v0.2.0 test suite and validate edge cases

Execute the five new test files from Step 7 and verify:
- Variable in `trn` produces correct transformed output
- Undefined variable in `trn` exits with a non-zero code and a line-numbered error
- Multiple pipelines in one file each emit correct independent results
- Malformed syntax produces a descriptive, line-numbered error and exits cleanly
- Mixed arithmetic + pipeline works end-to-end

---

## Step 10 — Update README and tag v0.2.0

- Add a **v0.2.0 changelog section** to `README.md` listing the three new features and the strict constraints that were preserved
- Document the new error-message format with a brief example
- Tag the commit as `v0.2.0` once all regression and new tests pass
- Final recommendation: **GO** — all three features are additive, backward-compatible, and low-complexity given the single-pass architecture
