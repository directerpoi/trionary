# Trionary

> A minimal, readable programming language for pipeline-based data transformations.

Trionary is a statically-structured scripting language built around five keywords. It is intentionally small: no runtime, no dependencies, no ambiguity. The entire language is a single C11 binary.

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
- [Examples](#examples)
- [Project Structure](#project-structure)
- [Design Principles](#design-principles)
- [Contributing](#contributing)
- [Changelog](#changelog)
- [License](#license)

---

## Features

- **5 keywords** — `lst`, `whn`, `trn`, `sum`, `emt`
- **Pipeline-oriented** — chain filters, transforms, and aggregations
- **Zero dependencies** — pure C11, no libraries beyond the C standard math library
- **Single binary** — one executable, no installer or runtime required
- **POSIX-compatible** — runs on Linux, macOS, and any POSIX-compliant system
- **Clear error messages** — line-number-aware diagnostics

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
tri run <file.tri> [arg0 arg1 ...]
```

| Argument | Description |
|----------|-------------|
| `run` | Execute a Trionary source file |
| `<file.tri>` | Path to the source file |
| `arg0 arg1 …` | Optional script arguments (accessible inside the script as `arg0`, `arg1`, …) |

**Example:**

```bash
tri run tests/demo.tri
tri run tests/test_cli_args.tri 7 3
```

---

## Language Reference

Trionary source files use the `.tri` extension.

### Comments

```tri
# This is a comment — everything after # is ignored
```

### Variables

Variables are assigned with `=`. Only numeric values (integer or float) are supported. A variable must be assigned before it is used.

```tri
price    = 100
discount = 20
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

| Keyword | Role | Description |
|---------|------|-------------|
| `lst` | Source | Opens a pipeline with a literal list of values |
| `whn` | Filter | Keeps only elements that satisfy a condition |
| `trn` | Transform | Applies an arithmetic operation to each element |
| `sum` | Aggregate | Reduces the pipeline to a single sum |
| `emt` | Output | Prints the current value(s) to stdout |

#### `lst` — List source

```tri
lst [1, 2, 3, 4, 5]
```

Begins a pipeline with a comma-separated list of numbers.

#### `whn` — Filter

```tri
whn >5
whn <3
```

Keeps elements that satisfy the given comparison. Supported operators: `>`, `<`, `>=`, `<=`, `=`.

#### `trn` — Transform

```tri
trn *2
trn +10
trn -1
```

Applies an arithmetic operation to every element in the pipeline.

#### `sum` — Aggregate

```tri
sum
```

Collapses the pipeline into a single value equal to the sum of all remaining elements.

#### `emt` — Emit

```tri
emt
-> emt
```

Prints the current pipeline value(s) or the result of an expression to stdout.

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

---

## Project Structure

```
trionary/
├── src/
│   ├── main.c      # CLI entry point and statement execution loop
│   ├── reader.c    # File reading into memory buffer
│   ├── lexer.c     # Tokenizer / lexical analyser
│   ├── parser.c    # Pattern-based parser with AST generation
│   ├── exec.c      # Execution engine with symbol table
│   └── output.c    # Output formatting
├── include/
│   ├── reader.h
│   ├── lexer.h
│   ├── parser.h
│   ├── exec.h
│   └── output.h
├── tests/
│   ├── demo.tri          # Full feature demonstration
│   ├── test_arith.tri    # Arithmetic tests
│   ├── test_vars.tri     # Variable tests
│   ├── test_pipeline.tri # Pipeline tests
│   ├── test_all.tri      # Combined test suite
│   ├── test_error.tri    # Error case: missing emit
│   ├── test_invalid.tri  # Error case: unknown keyword
│   └── test_malformed.tri# Error case: malformed syntax
├── Makefile
└── IMPLEMENTATION_SUMMARY.md
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
4. Build and verify: `make && ./tri run tests/test_all.tri`
5. Open a pull request

Please keep changes focused and minimal. New language features should follow the existing design principles.

---

## Changelog

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
- `SymTable` maximum capacity (256 variables) and linear-scan semantics are unchanged.
- `emit_value()` output format (integers without decimal point, floats with `%.6g`) is unchanged.
- Pipeline execution order (filter → transform → aggregate/emit) is unchanged.
- `assign_stmt` still accepts only `IDENT = NUMBER` (literal number on the right-hand side).
- Errors continue going to `stderr`; normal output goes to `stdout`.
- All v0.1.0 regression test files produce byte-for-byte identical stdout, stderr, and exit codes.

---

### v0.3.0 (Task 6 — CLI Input Improvements)

#### New Features

| # | Feature | Description |
|---|---------|-------------|
| 1 | **CLI argument variables** | Arguments passed after the file name (`tri run <file> arg0 arg1 …`) are automatically registered as numeric variables `arg0`, `arg1`, … in the global scope. Values are coerced to `double` via `atof()`. |
| 2 | **`argc` built-in variable** | The read-only variable `argc` always holds the count of script arguments (i.e. `argv` entries after `<file>`). When no extra arguments are given, `argc` is `0`. |
| 3 | **`??` default-value operator** | `expr ?? fallback` evaluates to `expr` if its left-hand side is a defined variable, and to `fallback` otherwise. Useful for optional CLI arguments: `arg0 ?? 1`. Operator is right-associative with lower precedence than all arithmetic operators. |

#### Backward Compatibility

All v0.2 programs continue to run without change. The `argc` variable is now always defined (value `0` when no extra arguments are supplied), so it is reserved and should not be used as a user variable name. Existing programs that do not reference `argc` or `arg0`…`argN` are completely unaffected.

---

## License

This project is open source. See [LICENSE](LICENSE) for details.
