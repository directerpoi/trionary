# v0.5.0 Core Language Features — New Data Types & Literal Syntax

## What Was Done

Implemented all keywords and data types from `keywords/B_data_types.md`: `str`, `arr`, `bool`, `true`, `fls`, `nil`, `map`, `int`, `flt`, `pair`, `tpl`, `set`. Significantly expanded the value system to support nested collections and recursive memory management.

---

## New Keywords & Types

| Keyword | Type | Description | Example |
|---------|------|-------------|---------|
| `str` | Type | Declare a string variable | `str name = "Alice"` |
| `arr` | Type | Declare an ordered list/array | `arr nums = [1, 2, 3]` |
| `bool` | Type | Declare a boolean variable | `bool flag = true` |
| `true` | Literal | Boolean true value | `ok = true` |
| `fls` | Literal | Boolean false value | `ok = fls` |
| `nil` | Literal | Null / no-value sentinel | `x = nil` |
| `map` | Type | Declare a key-value dictionary | `map cfg = {key: 1}` |
| `int` | Type | Declare an integer variable | `int count = 0` |
| `flt` | Type | Declare a float variable | `flt pi = 3.14` |
| `pair` | Type | A single key-value pair | `pair p = k:v` |
| `tpl` | Type | Immutable fixed-size tuple | `tpl pt = (1, 2)` |
| `set` | Type | Collection of unique values | `set s = {1, 2, 3}` |

---

## Technical Changes

### Value System Evolution
- **Recursive Value Struct:** Transitioned from a flat numeric-focused `Value` to a recursive tagged union supporting `VAL_STRING`, `VAL_ARRAY`, `VAL_MAP`, `VAL_SET`, `VAL_TUPLE`, and `VAL_PAIR`.
- **Memory Management:** Implemented recursive `free_value` and `clone_value` to handle deep cloning of nested collections.
- **Dynamic Typing with Initialization Hints:** Typed declarations like `str x = "val"` are now supported as statements.

### Parser Expansion
- **New Literals:** Added support for string literals `"..."`, boolean literals `true`/`fls`, and null literal `nil`.
- **Collection Literals:** 
    - `[e1, e2, ...]` for Arrays.
    - `{k1: v1, k2: v2, ...}` for Maps (detected by colon in first element).
    - `{e1, e2, ...}` for Sets.
    - `(e1, e2, ...)` for Tuples (differentiated from parenthesized expressions by the presence of a comma).
- **Pair Operator:** Added `:` as a binary operator to create `Pair` values.

### Execution Engine
- **String Operations:** Added support for string concatenation using the `+` operator.
- **Enhanced Membership:** The `in` operator now supports checking for values in Arrays, Sets, Tuples, and keys in Maps.
- **Mixed Arithmetic:** Seamlessly handles operations between `int` and `flt` types.

### Output System
- **Pretty Printing:** Updated `emit_value` to recursively format all new data types for output.

---

## Grammar Changes

```
primary    → NUMBER | STRING | 'true' | 'fls' | 'nil'
           | IDENT | '(' expr ')' | '(' expr_list ')' (tuple)
           | '[' expr_list? ']' (array)
           | '{' pair_list? '}' (map)
           | '{' expr_list? '}' (set)
           | IDENT primary* (function call)
expr       → pair_expr
pair_expr  → coalesce_expr (':' pair_expr)?
```

---

## Backward Compatibility
- Existing numeric pipelines and arithmetic statements remain fully functional.
- The `inpt` keyword still reads interactive input (now supports string fallback if input is not numeric).
- `print` and `read_line` in the `io` module remain available for compatibility.

---

# v0.6.0 Standard Functions & I/O Expansion

## What Was Done

Implemented a comprehensive suite of standard functions for math, list, and string manipulation, and expanded the language with new I/O keywords for better developer experience and file handling.

## New I/O Keywords

| Keyword | Type | Description | Example |
|---------|------|-------------|---------|
| `say` | Keyword | Print value(s) followed by a newline | `say "Hello"` |
| `prt` | Keyword | Print value(s) without trailing newline | `prt "> "` |
| `ask` | Keyword | Prompt and read string input (expression) | `let name = ask "Name: "` |
| `frd` | Keyword | Read entire contents of a file | `let data = frd "input.txt"` |
| `fwr` | Keyword | Overwrite file with content | `fwr "out.txt" "content"` |
| `fap` | Keyword | Append content to a file | `fap "log.txt" "new line"` |
| `csv` | Keyword | Parse CSV file into a list of lists | `let rows = csv "data.csv"` |
| `jrd` | Keyword | Read a file (JSON placeholder) | `let cfg = jrd "config.json"` |

## Standard Modules

### Math Module (`use math`)
- **Trigonometric:** `sin`, `cos`, `tan`
- **Logarithmic/Exponential:** `log`, `log10`, `exp`
- **Utility:** `mod`, `round`, `min`, `max`, `clmp`, `rnd`, `rndi`

### List Module (`use list`)
- **Sorting/Reversing:** `srt`, `srtd`, `rev`
- **Analysis:** `cnt`, `avg`
- **Manipulation:** `unq`, `zip`, `slc`, `flat`, `push`, `pop`
- **Search:** `fnd`, `idx`

### String Module (`use string`)
- **Basics:** `cat`, `len`, `sub`, `tostr`
- **Case:** `upr`, `lwr`
- **Edit:** `trm`, `spl`, `rep`, `fmt`
- **Search/Parse:** `has`, `num`

### IO Module (`use io`)
- **Existing:** `print`, `read_line`
- **New:** `fex` (check path exists), `fls` (list directory)

## Technical Changes
- **Variadic Built-ins:** Updated function table and executor to support variadic arguments for built-ins like `min`, `max`, `cat`, `fmt`.
- **I/O Node Type:** Added `IONode` to AST and `STMT_IO` / `EXPR_IO` to handle keyword-based I/O.
- **Improved Print:** `print` now accepts multiple arguments and separates them with spaces.

---

# v1.0.0 Control Flow, Modules & Error Handling

## What Was Done

Implemented advanced language features including explicit control flow, first-class functions (lambdas), a robust module system, structured error handling, and performance/DX tools.

## New Keywords

| Keyword | Category | Description | Example |
|---------|----------|-------------|---------|
| `ext` | Control | Exit the program with status code | `ext 0` |
| `stp` | Control | Stop program immediately (exit 1) | `stp` |
| `lmb` | Functional | Define anonymous lambda function | `lmb x -> x * 2` |
| `imp` | Modules | Import a module (alias for `use`) | `imp math` |
| `as` | Modules | Alias a module or symbol | `imp math as m` |
| `frm` | Modules | Selective import | `frm math imp sqrt` |
| `try` | Errors | Start a guarded block | `try` |
| `ctch` | Errors | Catch block for `try` | `ctch e` |
| `thr` | Errors | Throw a runtime error | `thr "Invalid input"` |
| `err` | Errors | Create a named error value | `let e = err "msg"` |
| `asrt` | Errors | Assert condition; abort on failure | `asrt x > 0` |
| `dflt` | DX | Default fallback (handles nil/error) | `x dflt 0` |
| `dbg` | DX | Debug dump to stderr | `dbg x` |
| `log` | DX | Labeled log to stderr | `log "reached"` |
| `tst` | DX | Inline unit test declaration | `tst "add" 1+1==2` |
| `trc` | DX | Trace execution to stderr | `trc on` |
| `doc` | DX | Attach documentation to function | `doc "Adds two"` |
| `pkg` | Modules | Declare package name | `pkg utils` |
| `exp` | Modules | Export a symbol | `exp fn add` |
| `chk` | DX | Runtime type check | `chk x str` |
| `tim` | Performance| Measure execution time | `tim heavy_fn()` |

## New Core Functions

- `ok(val)`: Returns `true` if `val` is not an error.
- `typ(val)`: Returns the type name of `val` as a string.

## Technical Changes

### Control Flow Evolution
- **Recoverable Errors:** Introduced `VAL_ERROR` type and `STATUS_ERROR` control status, allowing `try`/`ctch` to intercept and handle runtime errors.
- **First-Class Functions:** Lambdas (`lmb`) now capture their defining scope (simple closure) and can be passed as values or assigned to variables.

### Module System
- **Namespacing:** Added support for dot-notation in identifiers (e.g., `math.sqrt`) and module aliasing using `as`.
- **Selective Imports:** `frm module imp symbol` syntax for granular control.

### Developer Experience (DX)
- **High-Resolution Timing:** The `tim` keyword uses monotonic clock to provide accurate wall-clock execution time.
- **Enhanced Diagnostics:** `dbg`, `log`, and `trc` provide lightweight ways to inspect program state without cluttering standard output.

