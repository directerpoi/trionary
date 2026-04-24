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
