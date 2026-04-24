# Trionary v0.3.2 — Upgrade Plan

> Based on: `V0.3.2.md` | Current version: v0.3.1 | Goal: improve usability, DX, and stability without breaking syntax.

---

## 1. Final Feature List (10 Upgrades)

| # | Feature | Priority |
|---|---------|----------|
| U1 | `emt` label prefix — print a string literal before the value | High |
| U2 | `tri help` CLI command | High |
| U3 | `tri version` CLI command | High |
| U4 | Improved file-not-found / unreadable-file error messages | High |
| U5 | `inpt` numeric validation with a clear error on non-numeric input | High |
| U6 | `tri test` built-in test runner subcommand | Medium |
| U7 | Syntax-error hints (suggest the correct keyword on common typos) | Medium |
| U8 | Error context line — show the offending source line under the error message | Medium |
| U9 | Warning when `argN` is accessed beyond `argc` without a `??` fallback | Low |
| U10 | Separator control for multi-value `emt` output (`emt sep ","`) | Low |

---

## 2. Priority Table

| Priority | Upgrades | Rationale |
|----------|----------|-----------|
| **High** | U1, U2, U3, U4, U5 | Immediately visible to every user; low risk; no syntax change |
| **Medium** | U6, U7, U8 | Improves DX; small, self-contained changes |
| **Low** | U9, U10 | Nice-to-have; can be deferred without user impact |

---

## 3. Implementation Roadmap (10 Steps)

### Step 1 — `tri version` command *(U3)*

**Files:** `src/main.c`

Add a `version` subcommand that prints `Trionary v0.3.2` and exits with code 0.
Update the version string constant from `v0.3.1` to `v0.3.2` in the same place.

**Why:** Every CLI tool needs a reliable way to confirm the installed version.
**Risk:** None — purely additive.

---

### Step 2 — `tri help` command *(U2)*

**Files:** `src/main.c`

Add a `help` subcommand (and handle the `--help` / `-h` flags) that prints a concise usage summary:
- Available subcommands (`run`, `test`, `help`, `version`)
- Syntax reminder (`tri run <file.tri> [arg0 arg1 …]`)

**Why:** Discoverability — new users hit the CLI first, not the README.
**Risk:** None.

---

### Step 3 — Improved file-error messages *(U4)*

**Files:** `src/reader.c`, `src/main.c`

Replace bare `perror` / generic `fprintf` calls with specific, actionable messages:
- File not found → `Error: File '<path>' not found.`
- Permission denied → `Error: Cannot read '<path>' — permission denied.`
- Empty file → `Error: File '<path>' is empty.`

Include a `Did you mean 'tri run file.tri'?` hint when the subcommand is missing.

**Why:** The current error is too generic for beginners.
**Risk:** None — same exit code (1), same stderr channel.

---

### Step 4 — Syntax-error hints *(U7)*

**Files:** `src/lexer.c`, `src/parser.c`

Build a small static table of common misspellings / near-matches for the 8 keywords (`lst`, `whn`, `trn`, `sum`, `emt`, `fn`, `end`, `use`). When the lexer encounters an unknown identifier at statement position, check the table and append a hint:

```
Error: Unknown keyword 'list' at line 3
  Hint: Did you mean 'lst'?
```

**Why:** Typos are the most common beginner mistake; a hint removes the guesswork.
**Risk:** Hint logic is purely informational; no execution path changes.

---

### Step 5 — Error context line *(U8)*

**Files:** `src/error.c`, `include/error.h`, `src/reader.c`

Pass the original source buffer through to `error_at()`. When printing an error, also print the offending source line and a `^` caret pointing at the column:

```
Error: Unknown keyword 'list' at line 3
  3 | list [1,2,3]
         ^
```

Store the source buffer pointer in a module-level static so `error_at()` can access it without changing every call-site signature.

**Why:** Pinpointing the exact character dramatically speeds up debugging.
**Risk:** Low — the reader already loads the entire file into a buffer. Column tracking requires a small addition to the `Token` struct.

---

### Step 6 — `inpt` numeric validation *(U5)*

**Files:** `src/exec.c`, `src/modules/io.c` (where `read_line` lives)

After reading a line from stdin, use `strtod` with `endptr` checking to validate that the input is a number. If it is not:
- Print `Error: Expected a number, got '<input>' at line <N>.` to stderr.
- Exit with code 1 (consistent with fail-fast policy).

**Why:** Without validation, a non-numeric `inpt` silently produces `0`, leading to wrong results.
**Risk:** Breaks any script that relied on silent-zero coercion — but that behaviour was undefined, so this is a safe fix.

---

### Step 7 — `emt` label prefix *(U1)*

**Files:** `src/lexer.c`, `src/parser.c`, `src/exec.c`

Extend the `emt` statement to accept an optional string literal prefix:

```tri
emt "Result:" 42 + 8    # Output: Result: 50
x = 99
emt "x =" x             # Output: x = 99
```

Grammar change (additive only):
```
emt_stmt  →  'emt'  ( STRING_LITERAL )?  expr
```

Add `TOK_STRING` to the lexer (only inside `emt` context, not globally). Parser detects the optional `TOK_STRING` after `emt` and stores it in the node. Exec prints the label, then the value.

**Why:** Labelled output is the single most-requested usability improvement.
**Risk:** Medium — introduces the first string token. Strictly limited to `emt` statement; no string variables, no string expressions.

---

### Step 8 — `emt` separator control *(U10)*

**Files:** `src/parser.c`, `src/exec.c`, `src/output.c`

Add an optional `sep` keyword after `emt` to control multi-value output delimiter:

```tri
lst [1,2,3,4,5] | whn >2 -> emt sep ","   # 3,4,5
lst [1,2,3]              -> emt sep " | " # 1 | 2 | 3
```

Default separator remains a newline (backward compatible). `sep` is parsed only inside an `emt` stage, not at the top level.

**Why:** The default newline-per-value output is hard to use with downstream tools.
**Risk:** Low — parser extension only. Does not affect scripts that don't use `sep`.

---

### Step 9 — Undefined `argN` warning *(U9)*

**Files:** `src/exec.c`

When resolving `arg0` … `argN` (any `argN` where N ≥ 0), check whether N < `argc`. If it exceeds `argc` **and** no `??` operator is present in the expression, emit a warning to stderr before returning `0`:

```
Warning: 'arg2' is not defined (argc=1); returning 0. Consider using 'arg2 ?? default'.
```

This is a warning, not an error — execution continues.

**Why:** Silent `0` for missing arguments is a common source of wrong results. The `??` operator already provides the right pattern.
**Risk:** Low — warning only, no behaviour change.

---

### Step 10 — `tri test` subcommand *(U6)*

**Files:** `src/main.c`, `tests/run_tests.sh` (reference implementation)

Implement `tri test [dir]` that:
1. Scans the given directory (default: `./tests`) for `test_*.tri` files.
2. Runs each file with `./tri run`.
3. Compares stdout to the corresponding `test_*.expected` file.
4. Reports `PASS` / `FAIL` per test with a final count.
5. Exits with code 0 on all-pass, code 1 on any failure.

This brings the existing shell-script runner (`tests/run_tests.sh`) into the binary itself, removing the Bash dependency.

**Why:** `tri test` makes the test workflow self-contained and cross-platform.
**Risk:** Low — replicates existing shell logic in C. The shell script is kept as a fallback.

---

## 4. Risk Analysis

| Upgrade | Risk Level | Mitigation |
|---------|-----------|------------|
| U1 — `emt` label | Medium | String token is scoped to `emt` only; no string variables |
| U2 — `tri help` | None | Additive CLI path |
| U3 — `tri version` | None | Additive CLI path |
| U4 — file errors | None | Same exit code; same stderr channel |
| U5 — `inpt` validation | Low | Fails on previously-silent bad input (correct behaviour) |
| U6 — `tri test` | Low | Shell script kept as fallback |
| U7 — syntax hints | None | Informational only |
| U8 — error context | Low | Requires column tracking in Token; small struct change |
| U9 — arg warning | None | Warning only; execution unchanged |
| U10 — `emt sep` | Low | Backward compatible; default unchanged |

### What NOT to include in v0.3.2

- Loops (`for`, `while`, `each`)
- User-defined string variables or a string type
- Module imports from the file system
- Bytecode compilation or a VM
- Recursive function calls
- New pipeline operators beyond `lst`, `whn`, `trn`, `sum`, `emt`
- Conditional branching (`if`, `else`)

---

## 5. Final Recommendation

**GO ✅**

All 10 upgrades are backward-compatible, low-risk, and within the stated design principles (minimal syntax, single-pass, no new complex features). The five High-priority items (U1–U5) can ship as a first pass; the remaining five can follow without any rework of core components.

Suggested delivery order: **Step 1 → 2 → 3 → 4 → 6 → 7 → 5 → 8 → 9 → 10**
(CLI and error improvements first, then language surface changes, then test infrastructure last.)
