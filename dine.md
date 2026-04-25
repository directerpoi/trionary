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
