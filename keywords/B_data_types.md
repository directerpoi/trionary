# B. Data Types

> Trionary is currently numeric-only. These types extend the language's data model.

| # | Name | Type | Description | Example Syntax | Priority |
|---|------|------|-------------|----------------|----------|
| B01 | `str` | keyword / type | Declare a string variable | `str name = "Alice"` | v0.4 |
| B02 | `arr` | keyword / type | Declare an ordered list/array | `arr nums = [1, 2, 3]` | v0.4 |
| B03 | `bool` | keyword / type | Declare a boolean variable | `bool flag = true` | v0.4 |
| B04 | `true` | literal | Boolean true value | `bool ok = true` | v0.4 |
| B05 | `fls` | literal | Boolean false value | `bool ok = fls` | v0.4 |
| B06 | `nil` | literal | Null / no-value sentinel | `x = nil` | v0.5 |
| B07 | `map` | keyword / type | Declare a key-value dictionary | `map cfg = {key: 1}` | v0.5 |
| B08 | `int` | keyword / type | Declare an integer-typed variable | `int count = 0` | v0.5 |
| B09 | `flt` | keyword / type | Declare a float-typed variable | `flt pi = 3.14` | v0.5 |
| B10 | `pair` | keyword / type | A single key-value pair | `pair p = key:val` | v0.5 |
| B11 | `tpl` | keyword / type | Immutable fixed-size tuple | `tpl pt = (1, 2)` | v1.0 |
| B12 | `set` | keyword / type | Unordered collection of unique values | `set s = {1, 2, 3}` | v1.0 |
