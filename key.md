# Trionary — Complete Keyword & Function Reference

> Every keyword, operator, and built-in function defined for Trionary — one entry per name, no duplicates.
> Status column: ✅ implemented | 🔶 v0.4 | 🔷 v0.5 | 🔲 v1.0

---

## Core Pipeline Keywords (existing v0.3.2)

| Keyword | Kind | Description | Example |
|---------|------|-------------|---------|
| `lst` | keyword | Create a list / start a pipeline | `lst [1, 2, 3]` |
| `whn` | keyword | Filter pipeline values by condition | `lst nums \| whn > 0` |
| `trn` | keyword | Transform / map pipeline values | `lst nums \| trn * 2` |
| `sum` | keyword | Aggregate / sum pipeline values | `lst nums \| sum` |
| `emt` | keyword | Emit (print) output from a pipeline | `lst nums \| emt` |
| `fn` | keyword | Define a named function | `fn add a b` |
| `end` | keyword | Close a function or block | `end` |
| `use` | keyword | Import a module | `use math` |
| `inpt` | keyword | Read a numeric value from the user | `inpt "Enter number: "` |

---

## Control Flow

| Keyword | Kind | Description | Example |
|---------|------|-------------|---------|
| `if` | keyword | Begin a conditional block | `if x > 0` |
| `els` | keyword | Else branch of a conditional | `els` |
| `elif` | keyword | Else-if branch | `elif x == 0` |
| `for` | keyword | Counted / range loop | `for i 1 10` |
| `whl` | keyword | While loop (condition-driven) | `whl x < 100` |
| `each` | keyword | Iterate over every element of a list | `each item arr` |
| `brk` | keyword | Break out of the nearest loop | `brk` |
| `nxt` | keyword | Skip to the next iteration (continue) | `nxt` |
| `ret` | keyword | Return a value from a function | `ret x * 2` |
| `rpt` | keyword | Repeat a block N times | `rpt 5` |
| `ext` | keyword | Exit the program with a status code | `ext 0` |
| `stp` | keyword | Stop the program immediately (exit 1) | `stp` |
| `yld` | keyword | Yield a value from a pipeline stage | `yld x * 2` |

---

## Logical & Membership Operators

| Keyword | Kind | Description | Example |
|---------|------|-------------|---------|
| `not` | operator | Logical NOT | `not x > 0` |
| `and` | operator | Logical AND | `x > 0 and x < 10` |
| `or` | operator | Logical OR | `x < 0 or x > 10` |
| `in` | operator | Membership test (value in list/string) | `x in arr` |

---

## Variable Binding

| Keyword | Kind | Description | Example |
|---------|------|-------------|---------|
| `let` | keyword | Declare an immutable (constant) binding | `let pi = 3.14159` |

---

## Data Types

| Keyword | Kind | Description | Example |
|---------|------|-------------|---------|
| `str` | type | Declare a string variable | `str name = "Alice"` |
| `arr` | type | Declare an ordered list / array | `arr nums = [1, 2, 3]` |
| `bool` | type | Declare a boolean variable | `bool flag = true` |
| `int` | type | Declare an integer-typed variable | `int count = 0` |
| `flt` | type | Declare a float-typed variable | `flt pi = 3.14` |
| `map` | type | Declare a key-value dictionary | `map cfg = {key: 1}` |
| `pair` | type | A single key-value pair | `pair p = key:val` |
| `tpl` | type | Immutable fixed-size tuple | `tpl pt = (1, 2)` |
| `set` | type | Unordered collection of unique values | `set s = {1, 2, 3}` |

---

## Literals

| Keyword | Kind | Description | Example |
|---------|------|-------------|---------|
| `true` | literal | Boolean true value | `bool ok = true` |
| `fls` | literal | Boolean false value | `bool ok = fls` |
| `nil` | literal | Null / no-value sentinel | `x = nil` |

---

## Input / Output

| Keyword | Kind | Description | Example |
|---------|------|-------------|---------|
| `say` | keyword | Print value(s) followed by a newline | `say "Hello, World!"` |
| `prt` | keyword | Print value(s) without a trailing newline | `prt "Loading..."` |
| `ask` | keyword | Prompt the user and read a line of input | `ask "Enter name: "` |
| `frd` | keyword | Read the entire contents of a file | `frd "data.txt"` |
| `fwr` | keyword | Write (overwrite) content to a file | `fwr "out.txt" data` |
| `fap` | keyword | Append content to a file | `fap "log.txt" line` |
| `fex` | function | Check whether a file path exists | `fex "data.txt"` |
| `lsf` | function | List files in a directory | `lsf "."` |
| `csv` | keyword | Parse a CSV file into a list of lists | `csv "data.csv"` |
| `jrd` | keyword | Read and parse a JSON file | `jrd "config.json"` |

---

## Math Functions

| Keyword | Kind | Description | Example |
|---------|------|-------------|---------|
| `floor` | function | Floor (round down) | `floor 3.7` |
| `ceil` | function | Ceiling (round up) | `ceil 3.2` |
| `abs` | function | Absolute value | `abs -5` |
| `sqrt` | function | Square root | `sqrt 9` |
| `pow` | function | Raise base to exponent | `pow 2 8` |
| `mod` | function | Modulo (remainder) | `mod 10 3` |
| `round` | function | Round to nearest integer | `round 3.7` |
| `min` | function | Minimum of two or more values | `min 3 7 2` |
| `max` | function | Maximum of two or more values | `max 3 7 2` |
| `sin` | function | Sine of angle in radians | `sin 1.57` |
| `cos` | function | Cosine of angle in radians | `cos 0` |
| `tan` | function | Tangent of angle in radians | `tan 0.78` |
| `ln` | function | Natural logarithm | `ln 2.71` |
| `log10` | function | Base-10 logarithm | `log10 100` |
| `epow` | function | Euler's number raised to a power | `epow 1` |
| `clmp` | function | Clamp value between low and high | `clmp x 0 100` |
| `rnd` | function | Random float in [0, 1) | `rnd` |
| `rndi` | function | Random integer in range | `rndi 1 6` |

---

## List / Array Functions

| Keyword | Kind | Description | Example |
|---------|------|-------------|---------|
| `srt` | function | Sort a list ascending | `arr \| srt` |
| `srtd` | function | Sort a list descending | `arr \| srtd` |
| `rev` | function | Reverse a list | `arr \| rev` |
| `cnt` | function | Count / length of a list | `cnt arr` |
| `avg` | function | Arithmetic mean of a list | `avg arr` |
| `unq` | function | Remove duplicate values | `arr \| unq` |
| `zip` | function | Zip two lists element-by-element | `zip a b` |
| `fnd` | function | Find first matching element | `fnd arr x` |
| `idx` | function | Index of element in list | `idx arr x` |
| `slc` | function | Slice a sub-list from start to end | `slc arr 1 3` |
| `flat` | function | Flatten a nested list one level | `flat arr` |
| `push` | function | Append element to end of list | `push arr x` |
| `pop` | function | Remove and return last element | `pop arr` |

---

## String Functions

| Keyword | Kind | Description | Example |
|---------|------|-------------|---------|
| `cat` | function | Concatenate strings | `cat "Hello" " " "World"` |
| `len` | function | Length of a string | `len "hello"` |
| `sub` | function | Extract a substring | `sub "hello" 1 3` |
| `upr` | function | Convert string to uppercase | `upr "hello"` |
| `lwr` | function | Convert string to lowercase | `lwr "HELLO"` |
| `trm` | function | Trim leading and trailing whitespace | `trm " hi "` |
| `spl` | function | Split string by delimiter | `spl "a,b,c" ","` |
| `has` | function | Check if string contains a substring | `has "hello" "ell"` |
| `rep` | function | Replace occurrences in a string | `rep "aab" "a" "x"` |
| `fmt` | function | Format a string with values | `fmt "x={}" x` |
| `num` | function | Parse a number from a string | `num "3.14"` |
| `tostr` | function | Convert any value to its string form | `tostr 42` |

---

## Functional Features

| Keyword | Kind | Description | Example |
|---------|------|-------------|---------|
| `lmb` | keyword | Define an anonymous (lambda) function | `lmb x -> x * 2` |
| `mpa` | function | Apply a function to each list element | `lst [1,2,3] \| mpa lmb x -> x*2` |
| `fil` | function | Keep elements satisfying a predicate | `lst [1,2,3] \| fil lmb x -> x>1` |
| `red` | function | Reduce a list to a single value | `lst [1,2,3] \| red lmb a x -> a+x` |
| `cmp` | function | Compose two functions into one | `cmp f g` |
| `apl` | function | Apply a function with an argument list | `apl fn [1, 2]` |
| `part` | function | Partially apply a function | `part add 1` |
| `once` | keyword | Wrap a function so it runs only once | `once fn init` |

---

## Module System

| Keyword | Kind | Description | Example |
|---------|------|-------------|---------|
| `imp` | keyword | Import a module | `imp math` |
| `as` | keyword | Alias an imported module or symbol | `imp math as m` |
| `frm` | keyword | Import a specific symbol from a module | `frm math imp sqrt` |
| `exp` | keyword | Export a symbol from the current file | `exp fn add` |
| `pkg` | keyword | Declare the current file's package name | `pkg utils` |
| `std` | keyword | Reference the standard library root | `use std.str` |
| `nsp` | keyword | Define a named namespace block | `nsp geometry` |
| `bnd` | keyword | Bind an external C function | `bnd fn c_memcpy` |

---

## Error Handling

| Keyword | Kind | Description | Example |
|---------|------|-------------|---------|
| `try` | keyword | Begin a guarded block | `try` |
| `ctch` | keyword | Catch an error from the preceding `try` | `ctch e` |
| `thr` | keyword | Throw a runtime error | `thr "Value must be positive"` |
| `err` | keyword / type | Named error value | `err "Division by zero"` |
| `ok` | function | Return true if a value is not an error | `ok result` |
| `orr` | keyword | Pipeline safe-fallback operator | `x / y orr 0` |
| `asrt` | keyword | Assert a condition; abort on failure | `asrt x > 0` |
| `dflt` | keyword | Provide a default when a value is nil/error | `x dflt 0` |

---

## Developer Experience

| Keyword | Kind | Description | Example |
|---------|------|-------------|---------|
| `dbg` | keyword | Dump a value and its variable name to stderr | `dbg x` |
| `log` | keyword | Emit a labeled message to stderr | `log "step reached"` |
| `typ` | function | Return the type name of a value as a string | `typ x` |
| `tst` | keyword | Declare an inline unit test | `tst "add" add 1 2 == 3` |
| `trc` | keyword | Trace every pipeline stage value to stderr | `trc on` |
| `doc` | keyword | Attach a documentation string to a function | `doc "Computes the mean"` |
| `chk` | keyword | Assert a runtime type check | `chk x str` |
| `bpt` | keyword | Pause execution and enter interactive debug | `bpt` |

---

## Performance / Runtime

| Keyword | Kind | Description | Example |
|---------|------|-------------|---------|
| `tim` | function | Measure wall-clock time of an expression | `tim fn heavy_calc` |
| `lzy` | keyword | Mark an expression for lazy evaluation | `lzy x = expensive_fn` |
| `csh` | keyword | Cache the result of a pure expression | `csh fn fib n` |
| `par` | keyword | Execute pipeline branches in parallel | `par lst [a,b] \| trn fn` |
| `byt` | keyword | Compile a function to bytecode ahead-of-time | `byt fn hot_path` |
| `opt` | keyword | Hint the optimizer for a code block | `opt level 2` |
| `mem` | function | Return current heap memory usage in bytes | `mem` |
| `spwn` | keyword | Spawn a concurrent background task | `spwn fn worker data` |

---

## Conflict Resolution Log

The following names appeared for more than one purpose and were resolved by renaming the lower-priority entry:

| Conflict | Kept as | Renamed to | Reason |
|----------|---------|------------|--------|
| `fls` (false literal) vs `fls` (list files) | `fls` = Boolean false | `lsf` = list files | `fls` is already an implemented keyword |
| `ext` (exit program) vs `ext` (C binding) | `ext` = exit program | `bnd` = bind C function | `ext` is already an implemented keyword |
| `exp` (export) vs `exp` (Euler exponent) | `exp` = export symbol | `epow` = e^x | `exp` is already an implemented keyword |
| `log` (dev log) vs `log` (natural log) | `log` = debug log to stderr | `ln` = natural logarithm | `log` is already an implemented keyword; `ln` is the standard math abbreviation |
| `map` (dict type) vs `map` (functional map) | `map` = dictionary type | `mpa` = map-apply function | `map` is already an implemented keyword for the dict type |
| `prt` (print) vs `prt` (partial apply) | `prt` = print without newline | `part` = partial apply | `prt` is already an implemented keyword |
| `if`/`els`/`elif`/`brk`/`nxt` in A and E | Defined in A_core_language.md | Removed from E_control_flow.md | Duplicates — single definition is sufficient |
