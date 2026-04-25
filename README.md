# Trionary

> A minimal, readable programming language for pipeline-based data transformations.

Trionary is a scripting language built around a curated vocabulary of keywords. It is intentionally lean: no runtime, no dependencies, no ambiguity. The entire language is a single C11 binary.

---

## Table of Contents

- [Features](#features)
- [Installation](#installation)
  - [Prerequisites](#prerequisites)
  - [Build from Source](#build-from-source)
- [Usage](#usage)
- [Language Reference](#language-reference)
  - [Comments](#comments)
  - [Variables](#variables)
  - [Arithmetic](#arithmetic)
  - [Keywords](#keywords)
  - [Pipelines](#pipelines)
  - [Control Flow](#control-flow)
  - [Data Types](#data-types)
  - [Functions](#functions)
  - [Lambdas](#lambdas)
  - [Modules](#modules)
  - [Error Handling](#error-handling)
  - [CLI Input](#cli-input)
  - [Interactive Input](#interactive-input)
  - [I/O Keywords](#io-keywords)
  - [Module System](#module-system)
  - [Developer Tools](#developer-tools)
  - [Exit and Stop](#exit-and-stop)
- [Examples](#examples)
- [Project Structure](#project-structure)
- [Design Principles](#design-principles)
- [Contributing](#contributing)
- [Changelog](#changelog)
- [License](#license)

---

## Features

- **Curated keyword set** — Core: `lst` `whn` `trn` `sum` `emt` `fn` `end` `use` `inpt` | Control Flow: `if` `elif` `els` `for` `whl` `each` `rpt` `brk` `nxt` `ret` | Types: `str` `arr` `bool` `int` `flt` `map` `pair` `tpl` `set` `let` `true` `fls` `nil` | Modules: `imp` `frm` `as` `exp` `pkg` | Errors: `try` `ctch` `thr` `err` `asrt` `dflt` | I/O: `say` `prt` `frd` `fwr` `fap` `csv` `jrd` | System: `ext` `stp` | Functional: `lmb` | DX: `dbg` `log` `tst` `trc` `doc` `chk` `tim`
- **Pipeline-oriented** — chain filters, transforms, and aggregations
- **Control flow** — `if`/`elif`/`els`, `for`, `whl`, `each`, `rpt` loops with `brk`/`nxt`/`ret`
- **Rich data types** — strings, arrays, maps, sets, tuples, pairs, booleans, nil
- **Named functions** — define reusable pure functions with `fn … end`
- **First-class lambdas** — anonymous functions with `lmb params -> expr`
- **Built-in modules** — `math`, `io`, `list`, `string` loaded with `use` or `imp`
- **Structured error handling** — `try`/`ctch` blocks, `thr`, `asrt`
- **Interactive input** — read numeric values from stdin with `inpt`
- **Rich I/O** — `say`, `prt`, file read/write (`frd`, `fwr`, `fap`), CSV, JSON
- **Labeled output** — prefix any emitted value with a string label (`emt "Result:" expr`)
- **Separator control** — join pipeline output with a custom delimiter (`sep ","`)
- **Developer tools** — inline tests (`tst`), debug output (`dbg`, `log`, `trc`), timing (`tim`), type checks (`chk`)
- **Zero dependencies** — pure C11, no libraries beyond the C standard math library
- **Single binary** — one executable, no installer or runtime required
- **POSIX-compatible** — runs on Linux, macOS, and any POSIX-compliant system
- **Helpful error messages** — line/column diagnostics, context lines, and keyword-typo hints

---

## Installation

### Prerequisites

| Tool | Minimum Version |
|------|----------------|
| GCC (or any C11-compatible compiler) | 4.8+ |
| GNU Make | 3.81+ |

### Build from Source

```bash
git clone https://github.com/api-bridges/trionary.git
cd trionary
make
```

This produces a `tri` binary in the project root.

To remove the compiled binary:

```bash
make clean
```

---

## Usage

```
tri <command> [arguments]
```

| Command | Description |
|---------|-------------|
| `run <file.tri> [arg0 arg1 …]` | Execute a Trionary source file |
| `test [dir]` | Run all `test_*.tri` tests in `dir` (default: `./tests`) |
| `help` | Show usage help |
| `version` | Print the interpreter version |

**Examples:**

```bash
tri run tests/demo.tri
tri run tests/test_cli_args.tri 7 3
tri test
tri test ./mytests
tri version
tri help
```

---

## Language Reference

Trionary source files use the `.tri` extension.

### Comments

```tri
# This is a comment — everything after # is ignored
```

### Variables

Variables are assigned with `=`. Only numeric values (integer or float) are supported. A variable must be assigned before it is used. The right-hand side may be a numeric literal, another variable, or `inpt` (reads from stdin).

```tri
price    = 100
discount = 20
total    = price        # copy another variable
amount   = inpt         # read a number from stdin
```

### Arithmetic

Standard arithmetic expressions with proper operator precedence (PEMDAS). Emit the result with `-> emt`.

```tri
5 + 10 -> emt          # 15
3 * 4 + 2 -> emt       # 14  (* binds tighter than +)
```

Variables can appear in expressions:

```tri
price = 100
discount = 20
price - discount -> emt   # 80
```

### Keywords

#### Pipeline keywords

| Keyword | Role | Description |
|---------|------|-------------|
| `lst` | Source | Opens a pipeline with a literal list of values |
| `whn` | Filter | Keeps only elements that satisfy a condition |
| `trn` | Transform | Applies an arithmetic operation to each element |
| `sum` | Aggregate | Reduces the pipeline to a single sum |
| `emt` | Output | Prints the current value(s) to stdout |

#### Structure keywords

| Keyword | Role | Description |
|---------|------|-------------|
| `fn` | Definition | Opens a named function definition |
| `end` | Structure | Closes a `fn`, `if`, `for`, `whl`, `each`, `rpt`, or `try` block |
| `use` | Module | Loads a built-in module's functions into scope |
| `imp` | Module | Import a module (supports aliasing with `as`) |
| `frm` | Module | Selective import: `frm module imp symbol` |
| `as` | Module | Alias a module: `imp math as m` |
| `exp` | Module | Export a symbol from the current package |
| `pkg` | Module | Declare the current package name |
| `inpt` | Input | Reads a numeric value from stdin |

#### Control flow keywords

| Keyword | Role | Description |
|---------|------|-------------|
| `if` | Branch | Conditional block; closed with `end` |
| `elif` | Branch | Additional condition branch inside an `if` block |
| `els` | Branch | Else branch inside an `if` block |
| `for` | Loop | Numeric range loop: `for var start end` |
| `whl` | Loop | Condition-controlled loop |
| `each` | Loop | Iterate over an array, set, or tuple |
| `rpt` | Loop | Repeat a block N times |
| `brk` | Loop | Break out of the nearest enclosing loop |
| `nxt` | Loop | Skip to the next iteration |
| `ret` | Function | Return a value from a function |
| `not` | Logic | Logical negation |
| `and` | Logic | Logical conjunction |
| `or` | Logic | Logical disjunction |
| `in` | Membership | Check if a value is in a collection |
| `let` | Variable | Declare an immutable variable |
| `ext` | System | Exit the program with an optional status code |
| `stp` | System | Stop the program immediately (exit code 1) |

#### Data-type keywords

| Keyword | Role | Description |
|---------|------|-------------|
| `str` | Type | Declare a string variable |
| `arr` | Type | Declare an ordered array |
| `bool` | Type | Declare a boolean variable |
| `true` | Literal | Boolean true |
| `fls` | Literal | Boolean false |
| `nil` | Literal | Null / no-value sentinel |
| `int` | Type | Declare an integer variable |
| `flt` | Type | Declare a float variable |
| `map` | Type | Declare a key-value map |
| `pair` | Type | A single key-value pair |
| `tpl` | Type | Declare an immutable tuple |
| `set` | Type | Declare a collection of unique values |

#### Error-handling keywords

| Keyword | Role | Description |
|---------|------|-------------|
| `try` | Error | Start a guarded block |
| `ctch` | Error | Catch block for a `try` statement |
| `thr` | Error | Throw a runtime error |
| `err` | Error | Create a named error value |
| `asrt` | Error | Assert a condition; aborts with an error if false |
| `dflt` | Error | Default fallback for nil or error values |

#### I/O keywords

| Keyword | Role | Description |
|---------|------|-------------|
| `say` | Output | Print value(s) followed by a newline |
| `prt` | Output | Print value(s) without a trailing newline |
| `frd` | File | Read entire contents of a file |
| `fwr` | File | Overwrite a file with content |
| `fap` | File | Append content to a file |
| `csv` | File | Parse a CSV file into an array of arrays |
| `jrd` | File | Read a JSON file |

#### Developer-experience keywords

| Keyword | Role | Description |
|---------|------|-------------|
| `lmb` | Functional | Anonymous lambda: `lmb params -> expr` |
| `dbg` | DX | Dump a value to stderr with a `DEBUG:` prefix |
| `log` | DX | Log a value to stderr with a `LOG:` prefix |
| `trc` | DX | Trace a value to stderr with a `TRACE:` prefix |
| `tst` | DX | Inline unit test: fails with a message if condition is false |
| `doc` | DX | Attach a documentation string to the following definition |
| `chk` | DX | Runtime type-check; aborts if the value is the wrong type |
| `tim` | Performance | Measure wall-clock execution time of an expression |

#### `lst` — List source

```tri
lst [1, 2, 3, 4, 5]
```

Begins a pipeline with a comma-separated list of numbers.

#### `whn` — Filter

```tri
whn >5
whn <3
whn >=5
whn <=10
whn =3
whn !=0
```

Keeps elements that satisfy the given comparison. Supported operators: `>`, `<`, `>=`, `<=`, `=`, `!=`.

#### `trn` — Transform

```tri
trn *2
trn +10
trn -1
trn * x + 1
```

Applies an arithmetic operation to every element in the pipeline. The pipeline element is the implicit left operand of the leading operator; the right-hand side may be any full arithmetic expression using literals and variables.

#### `sum` — Aggregate

```tri
sum
```

Collapses the pipeline into a single value equal to the sum of all remaining elements.

#### `emt` — Emit

```tri
emt
-> emt
emt "Result:" 42 + 8
42 + 8 -> emt "Arith:"
lst [1,2,3] -> emt "Item:"
```

Prints the current pipeline value(s) or the result of an expression to stdout. An optional string literal immediately after `emt` is printed as a label prefix before each value.

##### Labeled output

Any `emt` can be followed by a double-quoted string label that is prepended to the output:

```tri
emt "Result:" 10 + 5      # Result: 15
10 + 5 -> emt "Arith:"    # Arith: 15
lst [1,2,3] -> emt "Item:" # Item: 1 \n Item: 2 \n Item: 3
```

### Pipelines

Stages are chained with `|`. A pipeline begins with `lst` and ends with `emt` (either standalone on the next line, or inline with `-> emt`).

```tri
lst [values] | stage | stage | ... -> emt
```

Or, when using `sum` as the final reduction stage:

```tri
lst [values] | stage | stage | sum
emt
```

#### Separator control

By default each value in a pipeline is printed on its own line. The optional `sep "string"` modifier after `emt` joins all output values with the given separator instead:

```tri
lst [1,2,3,4,5] | whn >2 -> emt sep ","        # 3,4,5
lst [1,2,3]              -> emt sep " | "       # 1 | 2 | 3
lst [1,2,3]              -> emt "Values:" sep "," # Values: 1,2,3
```

`sep` may be combined with a label:

```tri
lst [values] -> emt "Label:" sep ","
```

---

### Functions

Named pure functions are defined with `fn … end`. The function name and parameter names appear on the same line as `fn`; the body is a single arithmetic expression on the next line.

```tri
fn add x y
  x + y
end
```

Functions are called by writing the name followed by arguments:

```tri
fn double n
  n * 2
end
double 5 -> emt       # 10
```

Functions may be called inside arithmetic expressions and inside pipeline transform stages:

```tri
fn square n
  n * n
end
lst [1,2,3,4,5] | trn * square 1 -> emt   # 1 4 9 16 25
```

- Up to **8 parameters** per function.
- Function bodies may reference previously-defined variables from the enclosing scope.
- Recursive calls are not supported in v0.3.

---

### Modules

Built-in modules are loaded with `use` or `imp`. All functions from the module become available immediately after the directive.

```tri
use math
use io
use list
use string
```

#### `math` module

| Function | Params | Description |
|----------|--------|-------------|
| `floor`  | 1 | Round down to nearest integer |
| `ceil`   | 1 | Round up to nearest integer |
| `abs`    | 1 | Absolute value |
| `sqrt`   | 1 | Square root |
| `pow`    | 2 | `pow x y` → x raised to the power y |
| `sin`    | 1 | Sine (radians) |
| `cos`    | 1 | Cosine (radians) |
| `tan`    | 1 | Tangent (radians) |
| `log`    | 1 | Natural logarithm |
| `log10`  | 1 | Base-10 logarithm |
| `exp`    | 1 | e raised to the power x |
| `mod`    | 2 | Floating-point remainder |
| `round`  | 1 | Round to nearest integer |
| `min`    | ≥1 | Minimum of arguments |
| `max`    | ≥1 | Maximum of arguments |
| `clmp`   | 3 | `clmp x lo hi` — clamp x between lo and hi |
| `rnd`    | 0 | Random float in [0, 1) |
| `rndi`   | 2 | `rndi lo hi` — random integer in [lo, hi] |

#### `io` module

| Function    | Params | Description |
|-------------|--------|-------------|
| `print`     | ≥1 | Emit argument(s) to stdout and return the value |
| `read_line` | 1 | Read a double from stdin; argument is the default on EOF/error |
| `fex`       | 1 | Return `true` if the path exists, `fls` otherwise |
| `fls`       | 1 | Return an array of filenames in the given directory |

> **Note on `print`:** When used as a standalone arithmetic statement (e.g. `print 42`), the value is emitted once by `print` and once by the implicit emit in `STMT_ARITH`, producing two output lines. Use `print` inside pipeline transforms or assign its result to avoid duplication.

> **Note on `read_line`:** Takes one mandatory default value argument (`read_line 0`) that is returned when stdin is empty or at EOF.

#### `list` module

| Function | Params | Description |
|----------|--------|-------------|
| `srt`    | 1 | Return a sorted copy of the array (ascending) |
| `srtd`   | 1 | Return a sorted copy (descending) |
| `rev`    | 1 | Return a reversed copy of the array |
| `cnt`    | 1 | Return the number of elements |
| `avg`    | 1 | Return the average of all numeric elements |
| `unq`    | 1 | Return an array with duplicates removed |
| `zip`    | 2 | Zip two arrays into an array of pairs |
| `fnd`    | 2 | `fnd arr val` — return the first matching element or nil |
| `idx`    | 2 | `idx arr val` — return the index of the first match or -1 |
| `push`   | 2 | `push arr val` — return a new array with val appended |
| `pop`    | 1 | Return a new array with the last element removed |
| `slc`    | 3 | `slc arr start end` — return a sub-array slice |
| `flat`   | 1 | Flatten one level of nested arrays |

#### `string` module

| Function | Params | Description |
|----------|--------|-------------|
| `cat`    | ≥1 | Concatenate all string arguments |
| `len`    | 1 | Length of a string |
| `sub`    | 3 | `sub s start end` — substring |
| `upr`    | 1 | Convert to uppercase |
| `lwr`    | 1 | Convert to lowercase |
| `trm`    | 1 | Trim leading and trailing whitespace |
| `spl`    | 2 | `spl s delim` — split string by delimiter, returns array |
| `has`    | 2 | `has s substr` — return true if s contains substr |
| `rep`    | 3 | `rep s old new` — replace occurrences |
| `fmt`    | ≥1 | Format string with `{}` placeholders |
| `num`    | 1 | Parse a string to a number |
| `tostr`  | 1 | Convert any value to its string representation |

---

### CLI Input

Arguments passed after the file name are available as `arg0`, `arg1`, … inside the script. They are automatically coerced to numbers. The built-in variable `argc` holds the count of script arguments.

```bash
tri run my_script.tri 10 20
```

Inside `my_script.tri`:

```tri
argc -> emt          # 2  (number of arguments)
arg0 -> emt          # 10 (first argument)
arg1 -> emt          # 20 (second argument)
arg0 + arg1 -> emt   # 30 (arithmetic with auto-coerced values)
```

#### Default values with `??`

The `??` operator returns its left operand if it is a defined variable, and the right operand (fallback) otherwise. This is useful when an argument may or may not be supplied:

```tri
arg0 ?? 1 -> emt     # arg0 if provided, else 1
arg1 ?? 0 -> emt     # arg1 if provided, else 0
```

`??` is right-associative and has lower precedence than all arithmetic operators.

#### Out-of-range argument warning

Accessing an `argN` variable where `N >= argc` (without a `??` fallback) prints a warning to stderr and returns `0`. Execution continues.

```
Warning: 'arg1' is not defined (argc=1); returning 0. Consider using 'arg1 ?? default'.
```

---

### Interactive Input

The `inpt` keyword reads a single numeric value from stdin. Non-numeric input causes a fatal error.

**Assignment form** — assigns the value directly to a variable:

```tri
a = inpt
b = inpt
a + b -> emt
```

**Standalone form** — reads into a named variable with an optional prompt string that is printed to stdout before reading:

```tri
inpt x
inpt y "Enter y: "
x + y -> emt
```

The prompt string (if any) is printed immediately before the read, with no trailing newline, so the user's input appears on the same line.

```bash
$ echo -e "10\n20" | tri run calc.tri
30
```

---

### Control Flow

All blocks are terminated with `end`.

#### `if` / `elif` / `els` — Conditionals

```tri
x = 10
if x > 5
  say "big"
elif x == 5
  say "five"
els
  say "small"
end
```

Boolean operators `not`, `and`, and `or` can combine conditions:

```tri
if x > 0 and x < 100
  say "in range"
end
```

#### `for` — Numeric range loop

```tri
for i 1 5
  say i        # prints 1 2 3 4 5
end
```

#### `whl` — Condition-controlled loop

```tri
x = 0
whl x < 3
  x = x + 1
  say x
end
```

#### `each` — Iterate over a collection

```tri
arr nums = [10, 20, 30]
each n nums
  say n
end
```

#### `rpt` — Repeat N times

```tri
rpt 3
  say "hello"
end
```

#### `brk`, `nxt`, `ret` — Loop and function control

```tri
for i 1 10
  if i == 5
    brk        # exit loop early
  end
  say i
end

fn first_positive nums
  each n nums
    if n > 0
      ret n    # return from function
    end
  end
  ret 0
end
```

---

### Data Types

Typed variable declarations use the type keyword followed by the name and an optional `= value` initialiser. Untyped assignment (`x = value`) also works for any value.

#### Strings

```tri
str name = "Alice"
say name             # Alice
say "Hello " + name  # Hello Alice
```

#### Arrays

```tri
arr nums = [1, 2, 3, 4, 5]
each n nums
  say n
end
```

#### Booleans

```tri
bool ok = true
bool fail = fls
if ok
  say "yes"
end
```

#### Nil

```tri
x = nil
if x == nil
  say "no value"
end
```

#### Maps

```tri
map cfg = {key: "value", count: 42}
```

#### Integers and floats

```tri
int count = 0
flt pi = 3.14159
```

#### Tuples and sets

```tri
tpl pt = (1, 2)
set s  = {1, 2, 3, 2, 1}   # {1, 2, 3}
```

#### Pairs

```tri
pair p = "name" : "Alice"
```

#### `let` — Immutable variables

```tri
let max = 100
let greeting = "Hello"
```

Reassigning an immutable variable is a runtime error.

---

### Lambdas

Anonymous functions are created with `lmb`. The syntax is `lmb params -> expr`. A lambda may be assigned to a variable and called like a regular function.

```tri
let double = lmb x -> x * 2
double 5 -> emt          # 10

let add = lmb x y -> x + y
add 3 4 -> emt           # 7
```

---

### Error Handling

#### `try` / `ctch` — Guarded blocks

```tri
try
  thr "something went wrong"
ctch e
  say e
end
```

#### `thr` — Throw an error

```tri
thr "Invalid input"
```

#### `asrt` — Assert a condition

```tri
asrt x > 0          # aborts with "Assertion failed" if x <= 0
```

#### `dflt` — Default fallback

The `dflt` operator is an alias for `??` that also covers error values:

```tri
result dflt 0        # use 0 if result is nil or an error
```

---

### Module System

#### `use` — Load a module

```tri
use math
use io
use list
use string
```

#### `imp` — Import with optional alias

```tri
imp math
imp math as m
```

#### `frm` — Selective import

```tri
frm math imp sqrt, pow
```

#### `pkg` and `exp` — Package and export declarations

```tri
pkg utils
exp fn add
```

---

### Developer Tools

#### `say` and `prt` — Print to stdout

```tri
say "Hello"          # Hello\n
prt "Prompt: "       # Prompt: (no newline)
```

Multiple arguments are separated by a space:

```tri
say "Result:" 42     # Result: 42
```

#### `dbg`, `log`, `trc` — Diagnostic output to stderr

```tri
dbg x                # DEBUG: 42
log "checkpoint"     # LOG: checkpoint
trc result           # TRACE: 15
```

#### `tst` — Inline unit test

```tri
tst "addition" 1 + 1 == 2     # TEST PASSED: addition
tst "never true" 1 == 0       # TEST FAILED: never true (line N)
```

#### `chk` — Runtime type check

```tri
chk name str         # aborts if name is not a string
chk count int        # aborts if count is not an integer
```

#### `tim` — Execution timing

```tri
tim heavy_fn 1000000
```

#### `doc` — Documentation string

```tri
doc "Returns x squared"
fn square x
  x * x
end
```

---

### Exit and Stop

```tri
ext 0          # exit with status code 0
ext 1          # exit with status code 1
stp            # stop immediately (exit code 1)
```

---

### I/O Keywords

#### `say` / `prt` — Output

```tri
say "Hello, world!"     # print with newline
prt "Enter value: "     # print without newline
```

#### File I/O

```tri
fwr "out.txt" "line one\n"   # overwrite file
fap "out.txt" "line two\n"   # append to file
let contents = frd "out.txt" # read entire file into a string
```

#### `csv` — Parse a CSV file

```tri
let rows = csv "data.csv"
each row rows
  say row
end
```

#### `jrd` — Read a JSON file

```tri
let cfg = jrd "config.json"
```

---

## Examples

### Hello, arithmetic

```tri
10 + 5 -> emt        # 15
3 * 4 + 2 -> emt     # 14
```

### Variables

```tri
a = 10
b = 7
a + 5  -> emt    # 15
a * b  -> emt    # 70
```

### Filter a list

```tri
# Print numbers greater than 5
lst [1,2,3,4,5,6,7,8,9,10] | whn >5 -> emt
# Output: 6  7  8  9  10
```

### Filter and transform

```tri
# Double every number greater than 5
lst [1,2,3,4,5,6,7,8,9,10] | whn >5 | trn *2 -> emt
# Output: 12  14  16  18  20
```

### Sum a list

```tri
lst [1,2,3,4,5] | sum -> emt    # 15
```

### Complex pipeline

```tri
# Keep numbers >2, multiply each by 10, then sum: (3+4+5)*10 = 120
lst [1,2,3,4,5] | whn >2 | trn *10 | sum
emt
# Output: 120
```

### Named functions

```tri
fn add x y
  x + y
end

fn mul x y
  x * y
end

add 3 4 -> emt    # 7
mul 6 7 -> emt    # 42
```

### Functions in pipelines

```tri
fn inc n
  n + 1
end

# Add 1 to every element greater than 2
lst [1,2,3,4,5] | whn >2 | trn + inc 0 -> emt
# Output: 4  5  6
```

### Built-in modules

```tri
use math

sqrt 16 -> emt          # 4
pow 2 10 -> emt         # 1024
abs -7 -> emt           # 7
floor 3.9 -> emt        # 3
ceil 3.1 -> emt         # 4
```

### CLI arguments

```bash
tri run calc.tri 10 20
```

```tri
# calc.tri
argc -> emt          # 2
arg0 + arg1 -> emt   # 30
arg2 ?? 0 -> emt     # 0 (not provided, falls back to 0)
```

### Labeled output

```tri
emt "Result:" 10 + 5       # Result: 15

x = 99
emt "x =" x                # x = 99

lst [1,2,3] -> emt "Item:" # Item: 1
                           # Item: 2
                           # Item: 3

lst [1,2,3,4,5] | sum -> emt "Sum:"   # Sum: 15
```

### Separator control

```tri
lst [1,2,3,4,5] | whn >2 -> emt sep ","       # 3,4,5
lst [1,2,3]              -> emt sep " | "      # 1 | 2 | 3
lst [1,2,3]              -> emt "Values:" sep "," # Values: 1,2,3
```

### Interactive input

```bash
echo -e "10\n20" | tri run sum.tri
```

```tri
# sum.tri — read two numbers and print their sum
a = inpt
b = inpt
a + b -> emt    # 30
```

With a prompt:

```tri
inpt a "Enter a: "
inpt b "Enter b: "
a + b -> emt
```

### Control flow

```tri
x = 7
if x > 10
  say "big"
elif x > 5
  say "medium"
els
  say "small"
end
# Output: medium
```

### Loops

```tri
# for loop
for i 1 3
  say i
end
# Output: 1  2  3

# each loop over an array
arr items = [10, 20, 30]
each v items
  say v
end
# Output: 10  20  30

# rpt loop
rpt 3
  say "hi"
end
# Output: hi  hi  hi
```

### Lambdas

```tri
let square = lmb x -> x * x
square 6 -> emt    # 36
```

### Error handling

```tri
try
  thr "oops"
ctch e
  say e
end
# Output: oops
```

### Strings and list module

```tri
use string
use list

str greeting = "Hello, World!"
say len greeting       # 13
say upr greeting       # HELLO, WORLD!

arr nums = [5, 3, 1, 4, 2]
let sorted = srt nums
each n sorted
  say n                # 1  2  3  4  5
end
```

### Inline tests

```tri
tst "two plus two" 2 + 2 == 4
tst "sqrt" sqrt 9 == 3
```

---

## Project Structure

```
trionary/
├── src/
│   ├── main.c         # CLI entry point, statement execution loop, built-in test runner
│   ├── reader.c       # File reading into memory buffer
│   ├── lexer.c        # Tokenizer / lexical analyser
│   ├── parser.c       # Pattern-based parser with AST generation
│   ├── exec.c         # Execution engine with symbol and function tables
│   ├── output.c       # Output formatting
│   ├── error.c        # Centralised error_at() helper
│   └── modules/
│       ├── math.c     # Built-in math module (floor, ceil, abs, sqrt, pow, sin, cos, …)
│       ├── io.c       # Built-in io module (print, read_line, fex, fls)
│       ├── list.c     # Built-in list module (srt, rev, cnt, avg, push, pop, …)
│       └── string.c   # Built-in string module (cat, len, sub, upr, lwr, spl, …)
├── include/
│   ├── reader.h
│   ├── lexer.h
│   ├── parser.h
│   ├── exec.h
│   ├── output.h
│   └── error.h
├── tests/
│   ├── run_tests.sh          # Shell-based test runner (Bash fallback)
│   ├── demo.tri              # Full feature demonstration
│   ├── test_arith.tri        # Arithmetic tests
│   ├── test_vars.tri         # Variable tests
│   ├── test_pipeline.tri     # Pipeline tests
│   ├── test_all.tri          # Combined test suite
│   ├── test_trn_expr.tri     # Expression RHS in trn
│   ├── test_fn.tri           # Function definition and calls
│   ├── test_modules.tri      # Built-in module usage
│   ├── test_cli_args.tri     # CLI argument variables
│   ├── test_emt_label.tri    # Labeled emt output
│   ├── test_emt_sep.tri      # Separator control
│   ├── test_inpt.tri         # Interactive input (assignment form)
│   ├── test_inpt_prompt.tri  # Interactive input with prompt
│   ├── test_input_assign.tri # Variable-to-variable assignment
│   ├── test_keyword_hint.tri # Keyword typo hint in error messages
│   ├── test_arg_warning.tri  # Out-of-range argN warning
│   ├── test_error.tri        # Error case: missing emit
│   ├── test_invalid.tri      # Error case: invalid character
│   └── test_malformed.tri    # Error case: malformed syntax
├── keywords/
│   ├── README.md             # Keyword & feature roadmap
│   ├── A_core_language.md
│   ├── B_data_types.md
│   ├── C_standard_functions.md
│   ├── D_input_output.md
│   ├── E_control_flow.md
│   ├── F_functional.md
│   ├── G_modules.md
│   ├── H_error_handling.md
│   ├── I_developer_experience.md
│   └── J_performance.md
├── Makefile
├── CHANGELOG.md
├── IMPLEMENTATION_SUMMARY.md
└── dine.md
```

---

## Design Principles

| Principle | Description |
|-----------|-------------|
| **Readable like English** | Keywords are short but self-describing |
| **One concept = one keyword** | No synonyms, no aliases |
| **Strict structure** | Every statement has exactly one valid form |
| **Clarity over flexibility** | Predictable behaviour is preferred over expressive power |
| **Zero dependencies** | The binary links only against the C standard math library |
| **Single executable** | No installer, no runtime, no package manager |

---

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/your-feature`)
3. Make your changes
4. Build and verify: `make && ./tri test`
5. Open a pull request

Please keep changes focused and minimal. New language features should follow the existing design principles.

---

## Changelog

### v0.4.0

#### New Features

| # | Feature | Description |
|---|---------|-------------|
| 1 | **Control flow** | `if`/`elif`/`els`/`end`, `for var start end`, `whl cond … end`, `each item list … end`, `rpt N … end`. Loop control with `brk`, `nxt`, and `ret`. |
| 2 | **Boolean and logic operators** | `not`, `and`, `or`, `in`. Boolean literals `true` and `fls`. |
| 3 | **Rich data types** | `str`, `arr`, `bool`, `int`, `flt`, `map`, `pair`, `tpl`, `set`, `nil`. Typed declarations: `str name = "value"`. |
| 4 | **Immutable variables (`let`)** | `let x = value` creates a constant; reassignment is a runtime error. |
| 5 | **First-class lambdas (`lmb`)** | `lmb params -> expr` creates an anonymous function that can be stored in a variable and called like a regular function. |
| 6 | **Structured error handling** | `try … ctch e … end`, `thr expr`, `asrt condition`, `dflt` fallback operator. |
| 7 | **Module system expansion** | `imp module`, `imp module as alias`, `frm module imp symbol`. `pkg` declares the package name; `exp` exports a symbol. |
| 8 | **`list` built-in module** | `use list` provides `srt`, `srtd`, `rev`, `cnt`, `avg`, `unq`, `zip`, `fnd`, `idx`, `push`, `pop`, `slc`, `flat`. |
| 9 | **`string` built-in module** | `use string` provides `cat`, `len`, `sub`, `upr`, `lwr`, `trm`, `spl`, `has`, `rep`, `fmt`, `num`, `tostr`. |
| 10 | **Expanded `math` module** | Added `sin`, `cos`, `tan`, `log`, `log10`, `exp`, `mod`, `round`, `min`, `max`, `clmp`, `rnd`, `rndi`. |
| 11 | **Expanded `io` module** | Added `fex` (path exists) and `fls` (directory listing). |
| 12 | **I/O keywords** | `say` (print + newline), `prt` (print, no newline), `frd` (file read), `fwr` (file write), `fap` (file append), `csv` (CSV parse), `jrd` (JSON read). |
| 13 | **Exit control** | `ext N` (exit with status code), `stp` (exit with code 1). |
| 14 | **Developer tools** | `dbg expr` (DEBUG to stderr), `log expr` (LOG to stderr), `trc expr` (TRACE to stderr), `tst "label" cond` (inline test), `chk expr type` (runtime type check), `tim expr` (wall-clock timing), `doc "string"` (documentation attachment). |
| 15 | **String concatenation** | The `+` operator concatenates strings. |
| 16 | **Collection literals** | Array `[1, 2, 3]`, map `{k: v}`, set `{1, 2, 3}`, tuple `(1, 2, 3)`. |
| 17 | **Pair operator** | `:` creates a `pair` value: `"name" : "Alice"`. |
| 18 | **`tri version` update** | `tri version` now prints `Trionary v0.4.0`. |

#### Backward Compatibility

All v0.3.x programs run without change. All new keywords are new additions; no existing keyword was removed or renamed.

---

### v0.3.2

#### New Features

| # | Feature | Description |
|---|---------|-------------|
| 1 | **`inpt` keyword (9th keyword)** | Read a numeric value from stdin. Use `varname = inpt` to assign directly or `inpt varname "prompt"` as a standalone statement with an optional prompt string. Non-numeric input is a fatal error. |
| 2 | **Assignment from variable** | The right-hand side of `=` may now be another variable name in addition to a numeric literal or `inpt` (e.g. `a = arg0`). |
| 3 | **Labeled `emt` output** | Any `emt` statement accepts an optional double-quoted string before the value: `emt "Result:" expr`, `expr -> emt "Label:"`, `lst … -> emt "Item:"`. The label is printed before each value. |
| 4 | **Separator control (`sep`)** | Pipeline `emt` accepts `sep "string"` to join all output values with the given delimiter instead of printing one per line: `lst [1,2,3] -> emt sep ","` → `1,2,3`. |
| 5 | **`tri help` command** | `tri help` (also `--help` / `-h`) prints a concise usage summary and exits with code 0. |
| 6 | **`tri version` command** | `tri version` prints `Trionary v0.3.2` and exits with code 0. |
| 7 | **`tri test` built-in runner** | `tri test [dir]` scans the given directory (default: `./tests`) for `test_*.tri` files, runs each one, diffs output against its `.expected` file, and reports PASS/FAIL with a final count. Replaces the Bash dependency for `make test`. |
| 8 | **Keyword-typo hints** | When an unknown identifier appears at statement position and matches a common misspelling (e.g. `list`, `emit`, `input`), the error message includes a "Did you mean?" hint. |
| 9 | **Error context lines** | Error messages now include the offending source line and a `^` caret pointing at the column of the error. |
| 10 | **Out-of-range `argN` warning** | Accessing `arg1` when `argc=1` (without a `??` fallback) now emits a warning to stderr and returns `0` instead of being a hard error. |
| 11 | **`!=` filter operator** | `whn !=value` is now a supported pipeline filter condition. |

#### Error-Message Format

```
Error: <description> at line <N>
  N | <source line>
      ^
  Hint: Did you mean '<keyword>'?
```

#### Backward Compatibility

All v0.2.0 and v0.3.0 programs run without change. The `inpt` keyword is new; existing programs that do not use it are completely unaffected. The default output format (one value per line) is unchanged.

---

### v0.3.0

#### New Features

| # | Feature | Description |
|---|---------|-------------|
| 1 | **Centralised error system** | All errors go through a single `error_at(line, msg, …)` helper in `src/error.c`. Consistent format: `Error: <message> at line <N>`. Fail-fast: the process exits with code 1 on the first error. |
| 2 | **Hash-map symbol table with scope stack** | The flat 256-slot variable array is replaced by a djb2-hashed open-addressing table with a `scope_push()` / `scope_pop()` API, enabling isolated function scopes. |
| 3 | **Expression RHS in `trn`** | The right-hand side of a `trn` operator may now be a full arithmetic expression (e.g. `trn * x + 1`), not just a literal or single variable. |
| 4 | **Named functions (`fn … end`)** | Define pure, reusable functions with positional parameters. Functions may be called in arithmetic expressions and in pipeline transform stages. |
| 5 | **Built-in modules (`use`)** | `use math` loads `floor`, `ceil`, `abs`, `sqrt`, `pow`. `use io` loads `print` and `read_line`. All modules are compiled in — no file-system access. |
| 6 | **CLI argument variables** | `arg0`, `arg1`, … hold script arguments (auto-coerced to `double`). `argc` holds the argument count. The `??` operator provides default values for missing arguments. |
| 7 | **Automated test suite** | `tests/run_tests.sh` runs all `test_*.tri` files and diffs against `.expected` output. `make test` executes the full suite. |

#### Backward Compatibility

All v0.2 programs run without change. The `argc` variable is always defined (value `0` when no extra arguments are supplied) and is reserved. Existing programs that do not reference `argc`, `arg0`…`argN`, `fn`, `end`, or `use` are completely unaffected.

---

### v0.2.0

#### New Features

| # | Feature | Description |
|---|---------|-------------|
| 1 | **Variable references in `trn`** | The `trn` keyword now accepts a variable name as its operand in addition to a literal number (e.g. `trn *scale`). The variable is resolved against the symbol table at execution time, so it always reflects the value most recently assigned before the pipeline runs. |
| 2 | **Line-numbered error messages** | All runtime error messages now include the source line number (e.g. `Error: Undefined variable 'x' at line 3`). This applies to undefined-variable errors in both arithmetic expressions and pipeline transforms. The lexer already included line numbers; this change makes the executor consistent. |
| 3 | **Multiple pipelines per file** | A single `.tri` file may now contain any number of complete `lst … emt` pipelines. Each pipeline's internal state (filter, transform, sum flag) is reset between runs while the symbol table remains shared, so variables assigned before or between pipelines are visible to all subsequent pipelines. |

#### Error-Message Format

Errors are written to `stderr` and follow this format:

```
Error: <description> at line <N>
```

**Examples:**

```
Error: Undefined variable 'scale' at line 5
Error: Expected number or variable after transform operator at line 3
Error: Unexpected character '"' at line 1
```

The program exits with code `1` on fatal errors and `0` on success.

#### Strict Constraints Preserved

The following v0.1.0 behaviours are **unchanged** in v0.2.0:

- All five keyword spellings (`lst`, `whn`, `trn`, `sum`, `emt`) are immutable.
- No existing `TokenType` value was removed or renumbered.
- `emit_value()` output format (integers without decimal point, floats with `%.6g`) is unchanged.
- Pipeline execution order (filter → transform → aggregate/emit) is unchanged.
- `assign_stmt` still accepts only `IDENT = NUMBER` (literal number on the right-hand side).
- Errors continue going to `stderr`; normal output goes to `stdout`.
- All v0.1.0 regression test files produce byte-for-byte identical stdout, stderr, and exit codes.

---

## License

This project is open source. See [LICENSE](LICENSE) for details.
